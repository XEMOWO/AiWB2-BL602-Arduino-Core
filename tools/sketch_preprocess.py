#!/usr/bin/env python3
"""Build the preprocessed sketch the way the Arduino IDE does, for the WB2 core.

Usage: sketch_preprocess.py <primary.ino> <tab1> [<tab2> ...]

The Arduino IDE's sketch preprocessing emits, in order:
    #include <Arduino.h>
    <every #include from every tab>
    <hoisted top-level typedef/struct/union/enum definitions>
    <a prototype for every top-level function definition ("ctags" step)>
    <the concatenated tabs>

Hoisting the type definitions is what makes sketches like ServerSentEvents
compile: `void updateSensor(sensorType &sensor);` is emitted as a prototype,
but `sensorType` is typedef'd later in the sketch, so the IDE moves the typedef
ahead of the prototype block. The original definition is then stripped from the
body to avoid a duplicate-definition error. Sketches that don't need the hoist
are unaffected (empty hoist block).

This script also neutralizes Xtensa-only inline asm (see _build_common.sh
xtensa_filter) so the sketch body keeps portable asm but drops the ESP8266
crash-on-demand idioms that cannot compile on RISC-V.

## How #includes move to the top

From each tab's LEADING region (the contiguous preprocessor directives before
the first real code line) we hoist:

  * every UNCONDITIONAL #include line;
  * a whole `#if/#ifdef/#ifndef ... #endif` block IF it is pure-directive
    (only #-directives, comments, and macro-continuation lines inside) — the
    block moves intact, guard and all;
  * the pure-directive PREFIX of a non-pure guarded block — the `#if` line plus
    the leading directive run of its FIRST branch, extracted as its own
    `#if ... directives ... #endif` unit. The body keeps its own `#if` line and
    the remaining code, so both the hoisted unit and the body are balanced
    #if/#endif pairs and the condition is evaluated identically in both places.
    This is what lets NAPTCaptivePortal / RangeExtender-NAPT compile: every tab
    is wrapped in `#if LWIP_FEATURES && !LWIP_IPV6`, and their includes move to
    the top so the generated prototypes see types like WiFiPhyMode_t.

The prefix is only extracted when the directive run ends at a safe boundary:
the first real code line with no nested conditional open (depth 0), or the
block's own #elif/#else/#endif. If the first code line is reached inside a
nested conditional, hoisting would unbalance the #if/#endif pairs, so the whole
block is left in place.

# #define / #undef lines are NEVER hoisted individually — a multi-line macro
# like `#define PTM(w) \\` would leave a stray '#' if its line were pulled
# out of the body. (They ARE hoisted when part of a pure-directive block
# that moves whole, which keeps the whole macro together.) An #include AFTER
# the first code line stays in place — HelloServerBearSSL relies on that:
# its `#define USING_INSECURE_CERTS_AND_KEYS_AND_CAS 1` + `#include
# <ssl-tls...>` both live after the global `server` object, so they stay
# together in the body and the header's #error guard sees the define.
"""

import re
import sys

# ---- Xtensa asm neutralization (ported from _build_common.sh) ---------------
XT = re.compile(r'\b(mov\.n|mov\.a|movi\.n|l32i(?:\.n)?|s32i(?:\.n)?|'
                r'wsr\.|rsr\.|break|extui|retw|call0|mux\b|bnone|beqz|bnez|'
                r'bbsi|bbci|ssai|sext|slli|srli|sub\.)\b')
ASMPAT = re.compile(r'\b(?:__asm__|asm)\s+(?:volatile|__volatile__)?\s*\(')


def neutralize(s):
    out = []
    i, n = 0, len(s)
    while i < n:
        m = ASMPAT.search(s, i)
        if not m:
            out.append(s[i:])
            break
        out.append(s[i:m.start()])
        depth, j = 0, m.end() - 1
        while j < n:
            if s[j] == '(':
                depth += 1
            elif s[j] == ')':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        stmt = s[m.start():j + 1]
        out.append('/* Xtensa asm neutralized on WB2 (RISC-V) */' if XT.search(stmt) else stmt)
        i = j + 1
    return ''.join(out)


def mask_comments(src):
    """Replace comments with spaces, preserving byte positions so spans detected
    on the masked text index directly into the original source.

    String literals must be skipped whole: a `//` inside a string (e.g.
    FSBrowser's `filename.indexOf("//")`) is not a comment, and treating it as
    one would mask the rest of the line — including any '{'/'}' — and corrupt
    brace-depth tracking.
    """
    chars = list(src)
    n = len(chars)
    i = 0
    while i < n:
        c = chars[i]
        if c in ('"', "'"):
            # Skip a string/char literal (honouring backslash escapes). Any
            # '//' or '/*' inside it is not a comment.
            quote = c
            i += 1
            while i < n:
                if chars[i] == '\\':
                    i += 2
                    continue
                if chars[i] == quote:
                    i += 1
                    break
                i += 1
        elif c == '/' and i + 1 < n and chars[i + 1] == '/':
            chars[i] = chars[i + 1] = ' '
            i += 2
            while i < n and chars[i] != '\n':
                chars[i] = ' '
                i += 1
        elif c == '/' and i + 1 < n and chars[i + 1] == '*':
            chars[i] = chars[i + 1] = ' '
            i += 2
            # Mask to spaces but KEEP the newlines: preserving line structure
            # keeps `masked.split('\n')` aligned with the source lines, which
            # the leading-#include detection (analyze) relies on. Newlines are
            # harmless to the char-based brace-depth tracking.
            while i < n and not (chars[i] == '*' and i + 1 < n and chars[i + 1] == '/'):
                if chars[i] != '\n':
                    chars[i] = ' '
                i += 1
            if i + 1 < n:
                chars[i] = chars[i + 1] = ' '
                i += 2
        else:
            i += 1
    return ''.join(chars)


KEYWORDS = ('if', 'for', 'while', 'switch', 'catch', 'else', 'do', 'return', 'new', 'delete')
HOIST_KW = ('typedef', 'struct', 'union', 'enum')   # NOT class (risky to hoist)


def _match_cond_block(start, masked_lines, src_lines):
    """For a #if/#ifdef/#ifndef line, return (end_idx, is_pure_directive).

    end_idx indexes the line of the MATCHING #endif. is_pure_directive is True
    when every line between the #if and #endif is a preprocessor directive, a
    macro-continuation line (previous line ended with '\\'), a comment or a
    blank line — i.e. the block contains no C++ code and can be hoisted whole
    (e.g. WiFiTelnetToSerial's `#if SERIAL_LOOPBACK ... #endif`).
    """
    depth = 0
    prev_cont = False  # previous line ended with '\' → this line is a continuation
    pure = True
    i = start
    n = len(src_lines)
    while i < n:
        mline = masked_lines[i].strip()
        line = src_lines[i]
        cm = re.match(r'\s*#\s*(if|ifdef|ifndef)\b', line)
        em = re.match(r'\s*#\s*(elif|else|endif)\b', line)
        if cm:
            depth += 1
            prev_cont = False
        elif em:
            if em.group(1) == 'endif':
                depth -= 1
                if depth == 0:
                    return i, pure
            prev_cont = False
        elif mline == '':
            prev_cont = False  # blank or comment-only line
        elif mline[0] == '#':
            prev_cont = mline.endswith('\\')
        elif prev_cont:
            prev_cont = mline.endswith('\\')  # continuation of the macro above
        else:
            pure = False  # a real code line inside the block
            prev_cont = False
        i += 1
    return n - 1, False  # unterminated: consume to the end, treat as non-pure


def _match_pure_prefix(start, masked_lines, src_lines):
    """Return the exclusive end index of the pure-directive prefix of a #if
    block's FIRST branch, or `start+1` when the prefix is not safely hoistable.

    The prefix runs from the #if line over consecutive directive lines
    (tracking nested conditionals), and ends at the first of:
      * the block's own #elif/#else/#endif (depth 0) — prefix excludes that line;
      * a real code line — the prefix is hoistable only if no nested
        conditional is open there (depth 0). If code appears inside a nested
        #if, hoisting the directive run would leave an unbalanced #if/#endif,
        so start+1 (no hoist) is returned and the whole block stays in place.
    """
    depth = 0
    i = start + 1
    n = len(src_lines)
    prev_cont = False
    while i < n:
        mline = masked_lines[i].strip()
        if mline == '':
            prev_cont = False
            i += 1
            continue
        line = src_lines[i]
        if re.match(r'\s*#\s*(if|ifdef|ifndef)\b', line):
            depth += 1
            prev_cont = False
            i += 1
            continue
        if re.match(r'\s*#\s*endif\b', line):
            if depth == 0:
                return i  # the block's own #endif
            depth -= 1
            prev_cont = False
            i += 1
            continue
        if re.match(r'\s*#\s*(elif|else)\b', line):
            if depth == 0:
                return i  # the block's own #elif/#else
            prev_cont = False
            i += 1
            continue
        if mline[0] == '#':
            prev_cont = mline.endswith('\\')
            i += 1
            continue
        if prev_cont:
            prev_cont = mline.endswith('\\')  # continuation of the macro above
            i += 1
            continue
        # first real code line
        return i if depth == 0 else start + 1
    return i if depth == 0 else start + 1


def leading_includes(src):
    """Scan the leading region of one tab.

    Returns (includes, drop_lines):
      includes   : list of strings to emit before the prototypes (unconditional
                   #include lines, and guarded #if..#endif units that moved
                   whole or as a pure-directive prefix).
      drop_lines : 0-based line indices of src that moved to `includes`; they
                   are removed from the body. The #if line of a non-pure block
                   is KEPT in the body so its #if/#endif stays balanced.
    """
    masked = mask_comments(src)
    src_lines = src.split('\n')
    masked_lines = masked.split('\n')
    includes = []
    drop_lines = set()
    cond = 0
    in_extern_c = 0
    idx = 0
    while idx < len(src_lines):
        if idx >= len(masked_lines):
            break
        mline = masked_lines[idx].strip()
        if mline == '':
            idx += 1
            continue  # blank or comment-only line
        cm = re.match(r'\s*#\s*(if|ifdef|ifndef)\b', line := src_lines[idx])
        if cm and cond == 0:
            end, pure = _match_cond_block(idx, masked_lines, src_lines)
            if pure:
                # Whole pure-directive block moves intact (guard and all).
                block = ''.join(src_lines[i] + '\n' for i in range(idx, end + 1))
                includes.append(block)
                drop_lines.update(range(idx, end + 1))
            else:
                # Non-pure guarded block: hoist the pure-directive prefix of the
                # FIRST branch as its own guarded unit; the body keeps its own
                # #if line + the remaining code (see module docstring).
                pend = _match_pure_prefix(idx, masked_lines, src_lines)
                if pend > idx + 1:
                    block = (src_lines[idx] + '\n'
                             + ''.join(src_lines[i] + '\n' for i in range(idx + 1, pend))
                             + '#endif\n')
                    includes.append(block)
                    drop_lines.update(range(idx + 1, pend))
            idx = end + 1
            continue
        em = re.match(r'\s*#\s*(elif|else|endif)\b', line := src_lines[idx])
        if em:
            k = em.group(1)
            if k == 'endif':
                cond = max(0, cond - 1)
            idx += 1
            continue
        if mline[0] == '#':
            if re.match(r'\s*#\s*include\b', src_lines[idx]) and cond == 0:
                includes.append(src_lines[idx] + '\n')
                drop_lines.add(idx)
            idx += 1
            continue
        # Real code line: `extern "C" {` and its matching `}` are linkage, not
        # code — keep scanning through them; anything else ends the region.
        if re.match(r'extern\s*"C"\s*\{?', mline):
            if mline.endswith('{'):
                in_extern_c += 1
            idx += 1
            continue
        if mline == '}' and in_extern_c:
            in_extern_c -= 1
            idx += 1
            continue
        break  # first real code line ends the leading region
    return includes, drop_lines


def drop_lines_from(src, drop_lines):
    parts = src.split('\n')
    return '\n'.join(p for i, p in enumerate(parts) if i not in drop_lines)


def find_types_and_protos(body):
    """Return (hoisted_spans, forward_decls, prototypes) for the final body.

    hoisted_spans : list of (start, end) char offsets of top-level TYPEDEF
                    definitions and named ENUM definitions — these are moved
                    above the prototypes and stripped from the body.
    forward_decls : e.g. "struct Name;" for named struct/union definitions. The
                    full definition stays in the body (it may reference sketch
                    macros like SSE_MAX_CHANNELS), but the type name must be
                    declared before prototypes that use it by ref/pointer.
    prototypes    : list of prototype lines (each ends with ';').
    """
    masked = mask_comments(body)
    n = len(masked)
    hoisted = []
    forward_decls = []
    prototypes = []

    depth = 0
    stmt_start = -1
    stmt_kw = None

    i = 0
    while i < n:
        c = masked[i]
        if c == '#':
            # skip a preprocessor line (start of line only)
            while i < n and masked[i] != '\n':
                i += 1
            continue

        if depth == 0:
            if stmt_start < 0:
                if c not in ' \t\r\n':
                    m = re.match(r'(typedef|struct|union|enum)\b', masked[i:])
                    stmt_kw = m.group(1) if m else None
                    stmt_start = i

            if c == '{':
                depth += 1
                if stmt_kw == 'typedef':
                    # typedef ... { body } name;  — hoist the whole thing at ';'.
                    pass
                elif stmt_kw in ('struct', 'union'):
                    # Named definition: emit a forward declaration now; the full
                    # definition stays in the body (it may use sketch macros).
                    # Enums hoist whole instead (see the ';' branch).
                    head = masked[stmt_start:i].strip()
                    name = re.match(r'(struct|union)\s+([A-Za-z_]\w*)', head)
                    if name:
                        fwd = f"{name.group(1)} {name.group(2)};"
                        if fwd not in forward_decls:
                            forward_decls.append(fwd)
                elif stmt_kw == 'enum':
                    # enum / enum class / enum struct { body }; — keep the
                    # statement open through the body so the ';' branch hoists
                    # the whole definition (scoped enums can't be forward-declared).
                    pass
                else:
                    # A function (or other brace-wrapped statement) body. The
                    # statement ends at the '{' — reset the tracker so the next
                    # top-level statement starts fresh, else the next function's
                    # `rest` would swallow this whole body.
                    sig = masked[stmt_start:i].strip()
                    sig = sig.rstrip(';').strip()
                    semi = sig.rfind(';')
                    if semi >= 0:
                        sig = sig[semi + 1:].lstrip()
                    fm = re.match(
                        r'(template\s*<[^>]*>)?\s*(.*?\b)([A-Za-z_]\w*)\s*\(([^;]*)\)\s*$',
                        sig, flags=re.S)
                    if (fm and fm.group(3) not in KEYWORDS and '(' in sig
                            and not re.match(r'\s*(class|struct|enum|namespace|using|typedef)\b', fm.group(2))
                            and not re.search(r'=\s*$', fm.group(2))
                            and fm.group(3) not in ('setup', 'loop', 'main')):
                        tpl, rest, name, params = fm.groups()
                        prototypes.append(f"{tpl or ''} {rest}{name}({params});")
                    stmt_start = -1
                    stmt_kw = None
            elif c == '}':
                if depth > 0:
                    depth -= 1
            elif c == ';' and stmt_start >= 0:
                # Typedefs hoist whole (with or without body). Named enums hoist
                # whole too (opaque-enum-declaration isn't valid in C++11, so a
                # forward decl can't be used). struct/union stay in the body.
                if stmt_kw == 'typedef':
                    hoisted.append((stmt_start, i + 1))
                elif stmt_kw == 'enum':
                    head = masked[stmt_start:i].strip()
                    if re.match(r'enum\s+[A-Za-z_]\w*', head):
                        hoisted.append((stmt_start, i + 1))
                stmt_start = -1
                stmt_kw = None
        else:
            if c == '{':
                depth += 1
            elif c == '}':
                depth -= 1
        i += 1

    return hoisted, forward_decls, prototypes


def strip_spans(src, spans):
    out = []
    prev = 0
    for s, e in sorted(spans):
        out.append(src[prev:s])
        prev = e
    out.append(src[prev:])
    return ''.join(out)


def main():
    files = sys.argv[1:]
    if not files:
        sys.exit('usage: sketch_preprocess.py <primary.ino> [tabs...]')

    all_includes = []
    all_hoisted = []
    all_fwd = []
    all_protos = []
    bodies = []

    for f in files:
        try:
            src = open(f, encoding='utf-8', errors='replace').read()
        except OSError as e:
            sys.stderr.write(f"sketch_preprocess: {e}\n")
            sys.exit(1)
        includes, drop_lines = leading_includes(src)
        body = drop_lines_from(src, drop_lines)
        spans, fwds, protos = find_types_and_protos(body)
        # Capture the hoisted typedef/enum text from the pre-strip body; the
        # spans index into `body` (the dropped, un-stripped text).
        for s, e in spans:
            all_hoisted.append(body[s:e])
        body = strip_spans(body, spans)
        body = neutralize(body)
        all_includes.extend(includes)
        all_fwd.extend(fwds)
        all_protos.extend(protos)
        bodies.append(body)

    out = ['#include <Arduino.h>\n']
    out.extend(all_includes)
    for h in all_hoisted:
        out.append(h)
        if not h.endswith('\n'):
            out.append('\n')
    for d in all_fwd:
        out.append(d + '\n')
    for p in all_protos:
        out.append(p + '\n')
    out.extend(bodies)
    sys.stdout.write(''.join(out))


if __name__ == '__main__':
    main()
