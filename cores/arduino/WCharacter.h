/*
 * WCharacter.h — character utilities, matching the ESP8266 core header of the
 * same name. Third-party sketches and libraries call isAlpha(), toUpperCase()
 * etc. directly.
 *
 * IMPORTANT: these are inline FUNCTIONS, not macros (as in the official
 * ESP8266 core). String already has toUpperCase()/toLowerCase() members; macro
 * definitions of the same name would silently rewrite `str.toUpperCase()` into
 * `toupper()`.
 */
#ifndef Character_h
#define Character_h

#include <ctype.h>

inline boolean isAlphaNumeric(int c) { return isalnum(c) != 0; }
inline boolean isAlpha(int c)        { return isalpha(c) != 0; }
inline boolean isAscii(int c)        { return ((unsigned)c < 128u); }
inline boolean isWhitespace(int c)   { return isspace(c) != 0; }
inline boolean isControl(int c)      { return iscntrl(c) != 0; }
inline boolean isDigit(int c)        { return isdigit(c) != 0; }
inline boolean isGraph(int c)        { return isgraph(c) != 0; }
inline boolean isLowerCase(int c)    { return islower(c) != 0; }
inline boolean isPrintable(int c)    { return isprint(c) != 0; }
inline boolean isPunct(int c)        { return ispunct(c) != 0; }
inline boolean isSpace(int c)        { return isspace(c) != 0; }
inline boolean isUpperCase(int c)    { return isupper(c) != 0; }
inline boolean isHexadecimalDigit(int c) { return isxdigit(c) != 0; }

inline int toAscii(int c)        { return (c & 0x7F); }
inline int toLowerCase(int c)    { return tolower(c); }
inline int toUpperCase(int c)    { return toupper(c); }

#endif /* Character_h */
