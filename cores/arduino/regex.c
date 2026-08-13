/*
 * regex.c - POSIX ERE engine for the Ai-WB2-12F (BL602) Arduino core.
 *
 * A small backtracking matcher that satisfies the POSIX regcomp/regexec/
 * regfree/regerror API used by ESP8266 sketches (UriRegex.h in particular).
 * The toolchain's <regex.h> only declares these functions; nothing links, so
 * this core implementation makes `regfree undefined reference` go away.
 *
 * Design: the ERE is parsed into a tiny AST (allocated from the heap, freed by
 * regfree), then matched by greedy recursion with capture tracking. This is
 * deliberately compact rather than maximally POSIX-conformant; see regex.h for
 * the supported syntax. Strings are matched left-to-right; for a fixed start
 * position the greedy quantifiers try the longest match first and backtrack.
 *
 * Memory is bounded: at most one compiled AST lives per regex_t (UriRegex keeps
 * one per route). Stack use per match is proportional to pattern nesting depth.
 */

#include "regex.h"

#include <stdlib.h>
#include <string.h>

/* ---- AST ---------------------------------------------------------------- */

enum {
    N_CH,       /* single literal char (lo = char) */
    N_ANY,      /* '.' */
    N_CLASS,    /* [...] : ranges[] pairs, hi = negate flag */
    N_ANCHOR,   /* ^ or $ (lo = anchor char) */
    N_CONCAT,   /* sequence; a = head of sibling chain linked by b */
    N_ALT,      /* a | b */
    N_PAREN,    /* ( child ); ngroup = 1-based capture index */
    N_REP       /* child{lo..hi}; hi < 0 means unbounded */
};

typedef struct N {
    int            type;
    unsigned char  lo, hi;    /* N_CH: char; N_ANCHOR: '^'/'$'; N_REP: min/max; N_CLASS: negate */
    int            icase;     /* case-fold this char (N_CH) */
    struct N      *a, *b;     /* children / concat sibling */
    int            ngroup;    /* N_PAREN */
    unsigned char *ranges;    /* N_CLASS: [lo,hi,...] pairs */
    int            nrange;
} N;

typedef struct Guts {
    N    *root;
    int   ngroup;             /* number of parenthesized groups */
} Guts;

#define MAXGRP 32             /* max capturing groups we can track */
#define MAX(cap) (int)((cap) / 2)

static N *mknode(int type)
{
    N *n = (N *)calloc(1, sizeof(N));
    if (n)
        n->type = type;
    return n;
}

static void freeN(N *n)
{
    if (!n)
        return;
    switch (n->type) {
    case N_CONCAT:
        while (n->a) {
            N *nx = n->a->b;
            freeN(n->a);
            n->a = nx;
        }
        break;
    case N_ALT:
        /* N_ALT never sits inside a concat chain, so b is a genuine child. */
        freeN(n->a);
        freeN(n->b);
        break;
    case N_PAREN:
    case N_REP:
        /* For these, b is the concat sibling link owned by the enclosing
         * N_CONCAT — only the a child is ours. */
        freeN(n->a);
        break;
    case N_CLASS:
        free(n->ranges);
        break;
    default:
        break;
    }
    free(n);
}

/* ---- parser ------------------------------------------------------------- */

typedef struct {
    const char *p;
    int         cflags;
    int         ngroup;
    int         err;
} Parser;

static N *parse_alt(Parser *P);
static N *parse_concat(Parser *P);
static N *parse_rep(Parser *P);
static N *parse_atom(Parser *P);
static N *parse_class(Parser *P);

static N *parse_alt(Parser *P)
{
    N *left = parse_concat(P);
    if (P->err || !left)
        return left;
    if (P->p[0] == '|') {
        P->p++;
        N *right = parse_alt(P);
        if (P->err || !right) {
            freeN(left);
            return right;
        }
        N *alt = mknode(N_ALT);
        if (!alt) {
            P->err = REG_ESPACE;
            freeN(left);
            freeN(right);
            return NULL;
        }
        alt->a = left;
        alt->b = right;
        return alt;
    }
    return left;
}

static N *parse_concat(Parser *P)
{
    N *head = NULL, *tail = NULL;
    while (P->p[0] && P->p[0] != '|' && P->p[0] != ')') {
        N *node = parse_rep(P);
        if (P->err) {
            while (head) {
                N *nx = head->b;
                freeN(head);
                head = nx;
            }
            return NULL;
        }
        if (!head)
            head = node;
        else
            tail->b = node;
        tail = node;
    }
    N *c = mknode(N_CONCAT);
    if (!c) {
        P->err = REG_ESPACE;
        while (head) {
            N *nx = head->b;
            freeN(head);
            head = nx;
        }
        return NULL;
    }
    c->a = head;
    return c;
}

static N *repwrap(Parser *P, N *atom, int lo, int hi)
{
    N *r = mknode(N_REP);
    if (!r) {
        P->err = REG_ESPACE;
        freeN(atom);
        return NULL;
    }
    r->a = atom;
    r->lo = (unsigned char)lo;
    r->hi = (hi < 0) ? 0xff : (unsigned char)hi;   /* 0xff = unbounded */
    return r;
}

static int parse_braces(Parser *P, int *lo, int *hi)
{
    const char *q = P->p + 1;   /* points past '{' */
    int l = 0, h = -1, saw = 0;
    while (*q >= '0' && *q <= '9') {
        l = l * 10 + (*q - '0');
        saw = 1;
        q++;
    }
    if (!saw)
        return REG_BADBR;
    if (*q == '}') {
        h = l;
        q++;
    } else if (*q == ',') {
        q++;
        if (*q == '}') {
            h = -1;
            q++;
        } else {
            int hh = 0, hs = 0;
            while (*q >= '0' && *q <= '9') {
                hh = hh * 10 + (*q - '0');
                hs = 1;
                q++;
            }
            if (!hs || *q != '}')
                return REG_BADBR;
            h = hh;
            q++;
        }
    } else {
        return REG_BADBR;
    }
    if (h >= 0 && l > h)
        return REG_BADBR;
    P->p = q;
    *lo = l;
    *hi = h;
    return 0;
}

static N *parse_rep(Parser *P)
{
    N *atom = parse_atom(P);
    if (!atom || P->err)
        return atom;
    char c = P->p[0];
    if (c == '*') {
        P->p++;
        return repwrap(P, atom, 0, -1);
    }
    if (c == '+') {
        P->p++;
        return repwrap(P, atom, 1, -1);
    }
    if (c == '?') {
        P->p++;
        return repwrap(P, atom, 0, 1);
    }
    if (c == '{') {
        int lo, hi;
        int e = parse_braces(P, &lo, &hi);
        if (e) {
            P->err = e;
            freeN(atom);
            return NULL;
        }
        return repwrap(P, atom, lo, hi);
    }
    return atom;
}

static N *charnode(Parser *P, unsigned char c)
{
    N *n = mknode(N_CH);
    if (!n) {
        P->err = REG_ESPACE;
        return NULL;
    }
    n->lo = c;
    return n;
}

/* Add [lo..hi] to a class; returns 0 or REG_ESPACE. */
static int class_add(unsigned char **ranges, int *nr, int *cap, unsigned char lo, unsigned char hi)
{
    if (*nr + 2 > *cap) {
        int ncap = *cap ? *cap * 2 : 16;
        unsigned char *nrng = (unsigned char *)realloc(*ranges, ncap);
        if (!nrng)
            return REG_ESPACE;
        *ranges = nrng;
        *cap = ncap;
    }
    (*ranges)[(*nr)++] = lo;
    (*ranges)[(*nr)++] = hi;
    return 0;
}

static void class_posix(unsigned char **ranges, int *nr, int *cap, const char *name, size_t len, int *err)
{
    struct {
        const char *name;
        size_t      len;
        unsigned char lo[8], hi[8];
        int         n;
    } tbl[] = {
        { "digit", 5,  { '0', '0' }, { '9', '9' }, 1 },
        { "alpha", 5,  { 'a', 'A' }, { 'z', 'Z' }, 2 },
        { "alnum", 5,  { 'a', 'A', '0' }, { 'z', 'Z', '9' }, 3 },
        { "space", 5,  { ' ', '\t', '\n', '\r', '\v', '\f' }, { ' ', '\t', '\n', '\r', '\v', '\f' }, 6 },
        { "upper", 5,  { 'A' }, { 'Z' }, 1 },
        { "lower", 5,  { 'a' }, { 'z' }, 1 },
        { "xdigit",6,  { '0', 'a', 'A' }, { '9', 'f', 'F' }, 3 },
        { "punct", 5,  { '!', ':', '[', '{' }, { '/', '@', '`', '~' }, 4 },
        { "cntrl", 5,  { 0x00, 0x7f }, { 0x1f, 0x7f }, 2 },
        { "graph", 5,  { 0x21 }, { 0x7e }, 1 },
        { "print", 5,  { 0x20 }, { 0x7e }, 1 },
        { "blank", 5,  { ' ', '\t' }, { ' ', '\t' }, 2 },
    };
    size_t i;
    for (i = 0; i < sizeof(tbl) / sizeof(tbl[0]); i++) {
        if (tbl[i].len == len && memcmp(tbl[i].name, name, len) == 0) {
            int k;
            for (k = 0; k < tbl[i].n; k++) {
                if (class_add(ranges, nr, cap, tbl[i].lo[k], tbl[i].hi[k])) {
                    *err = REG_ESPACE;
                    return;
                }
            }
            return;
        }
    }
    *err = REG_ECTYPE;   /* unknown POSIX class name */
}

static N *parse_class(Parser *P)
{
    unsigned char *ranges = NULL;
    int nr = 0, cap = 0;
    int err = 0;

    P->p++;                              /* consume '[' */
    int neg = 0;
    if (P->p[0] == '^') {
        neg = 1;
        P->p++;
    }

    while (P->p[0] && P->p[0] != ']') {
        if (P->p[0] == '[' && P->p[1] == ':') {
            const char *end = strstr(P->p + 2, ":]");
            if (end) {
                class_posix(&ranges, &nr, &cap, P->p + 2, (size_t)(end - (P->p + 2)), &err);
                if (err) {
                    free(ranges);
                    P->err = err;
                    return NULL;
                }
                P->p = end + 2;
                continue;
            }
        }

        unsigned char lo;
        if (P->p[0] == '\\') {
            P->p++;
            if (!P->p[0]) {
                P->err = REG_EESCAPE;
                free(ranges);
                return NULL;
            }
            switch (P->p[0]) {
            case 'n': lo = '\n'; break;
            case 't': lo = '\t'; break;
            case 'r': lo = '\r'; break;
            case 'f': lo = '\f'; break;
            case 'v': lo = '\v'; break;
            default:  lo = (unsigned char)P->p[0]; break;
            }
            P->p++;
        } else {
            lo = (unsigned char)P->p[0];
            P->p++;
        }

        if (P->p[0] == '-' && P->p[1] && P->p[1] != ']') {
            P->p++;                     /* consume '-' */
            unsigned char hi;
            if (P->p[0] == '\\') {
                P->p++;
                hi = P->p[0] ? (unsigned char)P->p[0] : 0;
                if (!P->p[0]) {
                    P->err = REG_EESCAPE;
                    free(ranges);
                    return NULL;
                }
                P->p++;
            } else {
                hi = (unsigned char)P->p[0];
                P->p++;
            }
            if (lo > hi) {
                P->err = REG_ERANGE;
                free(ranges);
                return NULL;
            }
            if (class_add(&ranges, &nr, &cap, lo, hi)) {
                P->err = REG_ESPACE;
                free(ranges);
                return NULL;
            }
        } else {
            if (class_add(&ranges, &nr, &cap, lo, lo)) {
                P->err = REG_ESPACE;
                free(ranges);
                return NULL;
            }
        }
    }

    if (P->p[0] != ']') {
        P->err = REG_EBRACK;
        free(ranges);
        return NULL;
    }
    P->p++;                             /* consume ']' */

    N *n = mknode(N_CLASS);
    if (!n) {
        free(ranges);
        P->err = REG_ESPACE;
        return NULL;
    }
    n->ranges = ranges;
    n->nrange = nr;
    n->hi = (unsigned char)neg;
    return n;
}

static N *parse_atom(Parser *P)
{
    char c = P->p[0];
    if (!c) {
        P->err = REG_BADPAT;
        return NULL;
    }
    switch (c) {
    case '(': {
        P->p++;
        N *inner = parse_alt(P);
        if (P->err) {
            freeN(inner);
            return NULL;
        }
        if (P->p[0] != ')') {
            P->err = REG_EPAREN;
            freeN(inner);
            return NULL;
        }
        P->p++;
        N *g = mknode(N_PAREN);
        if (!g) {
            freeN(inner);
            P->err = REG_ESPACE;
            return NULL;
        }
        g->a = inner;
        g->ngroup = ++P->ngroup;
        return g;
    }
    case ')':
        P->err = REG_EPAREN;
        return NULL;
    case '[':
        return parse_class(P);
    case '.':
        P->p++;
        return mknode(N_ANY);
    case '^':
    case '$': {
        P->p++;
        N *n = mknode(N_ANCHOR);
        if (n)
            n->lo = (unsigned char)c;
        else
            P->err = REG_ESPACE;
        return n;
    }
    case '\\': {
        P->p++;
        if (!P->p[0]) {
            P->err = REG_EESCAPE;
            return NULL;
        }
        char e = P->p[0];
        P->p++;
        switch (e) {
        case 'n': return charnode(P, '\n');
        case 't': return charnode(P, '\t');
        case 'r': return charnode(P, '\r');
        case 'f': return charnode(P, '\f');
        case 'v': return charnode(P, '\v');
        default:  return charnode(P, (unsigned char)e);
        }
    }
    default:
        P->p++;
        return charnode(P, (unsigned char)c);
    }
}

/* ---- matcher ------------------------------------------------------------ */

typedef struct {
    int so, eo;   /* eo = -1 => group unmatched so far */
} Cap;

static unsigned char foldc(unsigned char c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

static int classhit(const N *n, unsigned char c, int icase)
{
    int i, hit = 0;
    for (i = 0; i + 1 < n->nrange; i += 2) {
        unsigned char lo = n->ranges[i], hi = n->ranges[i + 1];
        if (c >= lo && c <= hi) {
            hit = 1;
            break;
        }
        if (icase) {
            unsigned char cf = foldc(c), lf = foldc(lo), hf = foldc(hi);
            if (cf >= lf && cf <= hf) {
                hit = 1;
                break;
            }
        }
    }
    return n->hi ? !hit : hit;
}

static int rep_extra(const N *n, const char *s, int len, int pos, Cap *cap, int ncap, int icase, int left);

static int matchn(const N *n, const char *s, int len, int pos, Cap *cap, int ncap, int icase)
{
    if (!n)
        return pos;

    switch (n->type) {
    case N_CH: {
        if (pos < len) {
            unsigned char c = (unsigned char)s[pos];
            if (icase) {
                if (foldc(c) != foldc(n->lo))
                    return -1;
            } else if (c != n->lo) {
                return -1;
            }
            return pos + 1;
        }
        return -1;
    }
    case N_ANY:
        if (pos < len && s[pos] != '\n')
            return pos + 1;
        return -1;
    case N_CLASS:
        if (pos < len && classhit(n, (unsigned char)s[pos], icase))
            return pos + 1;
        return -1;
    case N_ANCHOR:
        if (n->lo == '^')
            return pos == 0 ? pos : -1;
        return pos == len ? pos : -1;
    case N_CONCAT: {
        const N *c;
        int p = pos;
        for (c = n->a; c; c = c->b) {
            int r = matchn(c, s, len, p, cap, ncap, icase);
            if (r < 0)
                return -1;
            p = r;
        }
        return p;
    }
    case N_ALT: {
        Cap save[MAXGRP];
        int r;
        memcpy(save, cap, (size_t)ncap * sizeof(Cap));
        r = matchn(n->a, s, len, pos, cap, ncap, icase);
        if (r >= 0)
            return r;
        memcpy(cap, save, (size_t)ncap * sizeof(Cap));
        return matchn(n->b, s, len, pos, cap, ncap, icase);
    }
    case N_PAREN: {
        Cap saved = cap[n->ngroup];
        int r;
        cap[n->ngroup].so = pos;
        cap[n->ngroup].eo = -1;
        r = matchn(n->a, s, len, pos, cap, ncap, icase);
        if (r < 0) {
            cap[n->ngroup] = saved;
            return -1;
        }
        cap[n->ngroup].eo = r;
        return r;
    }
    case N_REP: {
        /* mandatory lo repetitions */
        int i, p = pos;
        for (i = 0; i < n->lo; i++) {
            int r = matchn(n->a, s, len, p, cap, ncap, icase);
            if (r < 0)
                return -1;
            p = r;
        }
        /* greedy 0..(hi-lo) more; 0xff means unbounded */
        return rep_extra(n, s, len, p, cap, ncap, icase,
                         (n->hi == 0xff) ? -1 : (n->hi - n->lo));
    }
    }
    return -1;
}

static int rep_extra(const N *n, const char *s, int len, int pos, Cap *cap, int ncap, int icase, int left)
{
    Cap save[MAXGRP];
    int r;

    if (left == 0)
        return pos;

    memcpy(save, cap, (size_t)ncap * sizeof(Cap));
    r = matchn(n->a, s, len, pos, cap, ncap, icase);
    if (r >= 0 && r != pos) {           /* r != pos guards empty-match infinite loop */
        int r2 = rep_extra(n, s, len, r, cap, ncap, icase,
                           left > 0 ? left - 1 : left);
        if (r2 >= 0)
            return r2;
    }
    memcpy(cap, save, (size_t)ncap * sizeof(Cap));
    return pos;                          /* zero more repetitions */
}

/* ---- public API ---------------------------------------------------------- */

int regcomp(regex_t *preg, const char *pattern, int cflags)
{
    Parser P;
    N *root;

    if (!preg || !pattern)
        return REG_INVARG;

    memset(preg, 0, sizeof(*preg));
    P.p = pattern;
    P.cflags = cflags;
    P.ngroup = 0;
    P.err = 0;

    root = parse_alt(&P);
    if (P.err || !root) {
        freeN(root);
        return P.err ? P.err : REG_BADPAT;
    }

    if (P.ngroup + 1 > MAXGRP) {
        freeN(root);
        return REG_ESPACE;
    }

    Guts *g = (Guts *)calloc(1, sizeof(Guts));
    if (!g) {
        freeN(root);
        return REG_ESPACE;
    }
    g->root = root;
    g->ngroup = P.ngroup;

    preg->re_nsub = (size_t)P.ngroup;
    preg->re_flags = cflags;
    preg->re_guts = g;
    return 0;
}

int regexec(const regex_t *preg, const char *string, size_t nmatch,
            regmatch_t pmatch[], int eflags)
{
    Guts *g;
    const char *s;
    int len, icase, ncap;
    int start;
    Cap cap[MAXGRP];

    (void)eflags;
    if (!preg || !preg->re_guts || !string)
        return REG_INVARG;

    g = preg->re_guts;
    s = string;
    len = (int)strlen(s);
    icase = (preg->re_flags & REG_ICASE) ? 1 : 0;
    ncap = g->ngroup + 1;

    /* leftmost-match scan over every possible start position */
    for (start = 0; start <= len; start++) {
        int i, end;
        for (i = 0; i < MAXGRP; i++) {
            cap[i].so = -1;
            cap[i].eo = -1;
        }
        end = matchn(g->root, s, len, start, cap, ncap, icase);
        if (end >= 0) {
            if (!(preg->re_flags & REG_NOSUB) && nmatch && pmatch) {
                memset(pmatch, 0xff, nmatch * sizeof(regmatch_t));
                pmatch[0].rm_so = (regoff_t)start;
                pmatch[0].rm_eo = (regoff_t)end;
                for (i = 1; i < ncap && (size_t)i < nmatch; i++) {
                    pmatch[i].rm_so = (regoff_t)cap[i].so;
                    pmatch[i].rm_eo = (regoff_t)cap[i].eo;
                }
            }
            return 0;
        }
    }
    return REG_NOMATCH;
}

void regfree(regex_t *preg)
{
    if (!preg)
        return;
    if (preg->re_guts) {
        Guts *g = (Guts *)preg->re_guts;
        freeN(g->root);
        free(g);
        preg->re_guts = NULL;
    }
    preg->re_nsub = 0;
}

size_t regerror(int errcode, const regex_t *preg, char *errbuf, size_t errbuf_size)
{
    const char *msg;
    size_t need;

    (void)preg;
    switch (errcode) {
    case REG_NOMATCH:  msg = "No match"; break;
    case REG_BADPAT:   msg = "Invalid regular expression"; break;
    case REG_ECOLLATE: msg = "Invalid collating element"; break;
    case REG_ECTYPE:   msg = "Invalid character class type"; break;
    case REG_EESCAPE:  msg = "Trailing backslash"; break;
    case REG_ESUBREG:  msg = "Invalid back reference"; break;
    case REG_EBRACK:   msg = "Unmatched '['"; break;
    case REG_EPAREN:   msg = "Unmatched '(' or ')'"; break;
    case REG_EBRACE:   msg = "Unmatched '{' or '}'"; break;
    case REG_BADBR:    msg = "Invalid repetition"; break;
    case REG_ERANGE:   msg = "Invalid range"; break;
    case REG_ESPACE:   msg = "Out of memory"; break;
    case REG_BADRPT:   msg = "Invalid repetition operator"; break;
    case REG_EMPTY:    msg = "Empty expression"; break;
    case REG_ASSERT:   msg = "Invalid assertion"; break;
    case REG_INVARG:   msg = "Invalid argument"; break;
    default:           msg = "Unknown regex error"; break;
    }
    need = strlen(msg) + 1;
    if (errbuf && errbuf_size) {
        size_t c = need <= errbuf_size ? need : errbuf_size;
        memcpy(errbuf, msg, c - 1);
        errbuf[c - 1] = '\0';
    }
    return need;
}
