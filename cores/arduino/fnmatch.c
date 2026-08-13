/*
 * fnmatch.c — POSIX fnmatch() for the Ai-WB2-12F core.
 *
 * The RISC-V newlib ships fnmatch.h (used by ESP8266WebServer UriGlob.h) but
 * links no fnmatch(), so the WB2 core provides the implementation. Covers the
 * standard wildcards `*` and `?`, bracket character classes `[...]`, and the
 * FNM_NOESCAPE / FNM_PATHNAME / FNM_PERIOD / FNM_CASEFOLD flags.
 */
#include <fnmatch.h>
#include <ctype.h>

static int fnmatch_helper(const char *pattern, const char *string, int flags)
{
    while (*pattern) {
        char pc = *pattern;

        if (pc == '*') {
            /* Collapse consecutive stars; a trailing star matches the rest. */
            while (*pattern == '*') {
                pattern++;
            }
            if (*pattern == '\0') {
                return 0;
            }
            for (; *string; string++) {
                if (flags & FNM_PATHNAME && *string == '/') {
                    break;
                }
                if (fnmatch_helper(pattern, string, flags) == 0) {
                    return 0;
                }
            }
            return FNM_NOMATCH;
        }

        if (pc == '?') {
            if (!*string) {
                return FNM_NOMATCH;
            }
            pattern++;
            string++;
            continue;
        }

        if (pc == '[') {
            const char *b = pattern + 1;
            int negate = 0;
            if (*b == '!' || *b == '^') {
                negate = 1;
                b++;
            }
            if (*b == ']') {          /* leading ']' is a literal member */
                b++;
            }
            if (!*string) {
                return FNM_NOMATCH;
            }
            int matched = 0;
            for (; *b != ']' && *b != '\0'; b++) {
                char want = *b;
                if (want == '\\' && !(flags & FNM_NOESCAPE)) {
                    want = *(b + 1);
                    b++;
                }
                char got = *string;
                if (b[1] == '-' && b[2] != '\0' && b[2] != ']') {  /* a-c range */
                    char lo = want, hi = b[2];
                    b += 2;
                    if (flags & FNM_CASEFOLD) {
                        lo = (char)tolower((unsigned char)lo);
                        hi = (char)tolower((unsigned char)hi);
                        got = (char)tolower((unsigned char)got);
                    }
                    if (got >= lo && got <= hi) {
                        matched = 1;
                    }
                } else {
                    if (flags & FNM_CASEFOLD) {
                        want = (char)tolower((unsigned char)want);
                        got = (char)tolower((unsigned char)got);
                    }
                    if (got == want) {
                        matched = 1;
                    }
                }
            }
            if (*b != ']') {
                return FNM_NOMATCH;   /* unterminated class */
            }
            if (matched == negate) {
                return FNM_NOMATCH;
            }
            pattern = b + 1;
            string++;
            continue;
        }

        if (pc == '\\' && !(flags & FNM_NOESCAPE)) {
            pattern++;
            if (*pattern == '\0' || *pattern != *string) {
                return FNM_NOMATCH;
            }
            pattern++;
            string++;
            continue;
        }

        {
            char p = pc;
            char s = *string;
            if (flags & FNM_CASEFOLD) {
                p = (char)tolower((unsigned char)p);
                s = (char)tolower((unsigned char)s);
            }
            if (p != s) {
                return FNM_NOMATCH;
            }
        }
        pattern++;
        string++;
    }
    return *string ? FNM_NOMATCH : 0;
}

int fnmatch(const char *pattern, const char *string, int flags)
{
    /* FNM_PERIOD: a leading '.' in a component must be matched literally,
     * so `*`/`?` cannot consume it. Handle at the component start. */
    if ((flags & FNM_PERIOD) && *string == '.' && *pattern != '.') {
        return FNM_NOMATCH;
    }
    return fnmatch_helper(pattern, string, flags);
}
