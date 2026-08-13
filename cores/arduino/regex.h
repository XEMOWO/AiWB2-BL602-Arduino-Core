/*
 * regex.h - POSIX regular expressions for the Ai-WB2-12F (BL602) Arduino core.
 *
 * The SiFive/newlib toolchain ships a <regex.h> that *declares* regcomp,
 * regexec, regfree and regerror but no implementation exists in libc.a or the
 * SDK (this file shadows it via the core's -I include path, so <regex.h>
 * resolves here). ESP8266 sketches and libraries — in particular
 * ESP8266WebServer's UriRegex.h — use POSIX EREs (regcomp(..., REG_EXTENDED),
 * regexec, regfree) to match request paths with capturing groups, so a small
 * backtracking ERE engine is provided here.
 *
 * Supported syntax (POSIX ERE subset that UriRegex and typical sketches use):
 *   .              any char except newline
 *   [...] / [^...] char class, ranges, escapes (\d \w \s \D \W \S \n \t ...)
 *   (...)          capturing group
 *   |              alternation
 *   ^ $            anchors
 *   * + ?          greedy quantifiers
 *   {m} {m,} {m,n} bounded repeats
 *   \x             escaped literal
 *
 * Not supported: backreferences, look-around, REG_BASIC mode quirks.
 *
 * Flags honored: REG_EXTENDED, REG_ICASE, REG_NOSUB. eflags is accepted and
 * ignored (matches against '\0'-terminated C strings only).
 *
 * regcomp/regexec/regfree/regerror keep the same signatures as the toolchain
 * header so any code written against POSIX regex compiles unchanged.
 */

#ifndef _WB2_REGEX_H_
#define _WB2_REGEX_H_

#include <sys/types.h>   /* off_t */

#ifdef __cplusplus
extern "C" {
#endif

/* regcomp / regexec flags */
#define REG_EXTENDED 0x0001
#define REG_ICASE    0x0002
#define REG_NOSUB    0x0004

/* regerror / regcomp error codes (subset of POSIX) */
#define REG_NOMATCH   1   /* regexec: no match */
#define REG_BADPAT    2   /* invalid regular expression */
#define REG_ECOLLATE  3   /* collating element invalid */
#define REG_ECTYPE    4   /* invalid character class type */
#define REG_EESCAPE   5   /* trailing backslash */
#define REG_ESUBREG   6   /* invalid back reference */
#define REG_EBRACK    7   /* '[' unmatched */
#define REG_EPAREN    8   /* '(' or ')' unmatched */
#define REG_EBRACE    9   /* '{' or '}' unmatched */
#define REG_BADBR    10   /* invalid repetition */
#define REG_ERANGE   11   /* invalid range */
#define REG_ESPACE   12   /* out of memory */
#define REG_BADRPT   13   /* '*' '+' '?' not preceded by valid expression */
#define REG_EMPTY    14   /* empty (sub)expression */
#define REG_ASSERT   15   /* invalid assertion */
#define REG_INVARG   16   /* invalid argument */

typedef off_t regoff_t;

typedef struct {
    regoff_t rm_so;   /* start offset of match (or -1 if unmatched) */
    regoff_t rm_eo;   /* end offset of match (one past last char) */
} regmatch_t;

/* Compiled regex. Kept opaque: callers allocate one on the stack ({}) like
 * UriRegex does and only pass its address to regcomp/regexec/regfree. */
typedef struct regex_t {
    size_t      re_nsub;      /* number of parenthesized subexpressions */
    int         re_flags;     /* REG_* flags passed to regcomp */
    void       *re_guts;      /* private compiled program */
} regex_t;

int  regcomp(regex_t *preg, const char *pattern, int cflags);
int  regexec(const regex_t *preg, const char *string, size_t nmatch,
             regmatch_t pmatch[], int eflags);
size_t regerror(int errcode, const regex_t *preg, char *errbuf,
                size_t errbuf_size);
void regfree(regex_t *preg);

#ifdef __cplusplus
}
#endif

#endif /* _WB2_REGEX_H_ */
