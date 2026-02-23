#!/usr/bin/env python3
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

# File exclusions (binary dirs / build)
EXCLUDE_DIRS = {'build', '.git', 'third_party/glad', 'third_party/glfw'}

# Separator pattern: at least 3 repeated non-alphanumeric symbols (dash, long dash, equals, underscore, star, hash)
SEP_RE = re.compile(r'[-=─_*#]{3,}')

# Block comment pattern
BLOCK_RE = re.compile(r'/\*.*?\*/', re.S)

# URL protocol pattern to avoid removing 'http://' inline
PROTO_RE = re.compile(r'[a-zA-Z][a-zA-Z0-9+.-]{0,20}:

def is_separator(text: str) -> bool:
    return bool(SEP_RE.search(text))

def process_text(text: str) -> str:
    # First remove block comments unless they look like separators
    def block_repl(m):
        s = m.group(0)
        if is_separator(s):
            return s
        return ''
    text = BLOCK_RE.sub(block_repl, text)

    out_lines = []
    for line in text.splitlines(True):
        stripped = line.lstrip()
        # Preserve full-line
        m = re.match(r"^(\s*)
        if m:
            if is_separator(m.group(2)):
                out_lines.append(line)
                continue
            # drop whole-line comment
            # keep original leading whitespace as blank line
            out_lines.append(m.group(1) + "\n")
            continue

        # For inline '
        # Find occurrences of '
        newline = line
        idx = 0
        while True:
            idx = newline.find('
            if idx == -1:
                break
            # check if ':
            if idx-1 >= 0 and newline[idx-1] == ':' and PROTO_RE.search(newline[max(0, idx-30):idx+3]):
                idx += 2
                continue
            # Check if the remainder looks like a separator (only punctuation)
            rest = newline[idx+2:]
            if is_separator(rest):
                # preserve as separator comment: keep from idx to end
                break
            # Otherwise remove from idx to end
            newline = newline[:idx].rstrip() + "\n"
            break
        out_lines.append(newline)
    return ''.join(out_lines)

def should_process(path: Path) -> bool:
    if any(p in path.parts for p in EXCLUDE_DIRS):
        return False
    if path.suffix in ['.png', '.jpg', '.jpeg', '.exe', '.dll', '.so', '.a']:
        return False
    return True

def main():
    changed = []
    for p in ROOT.rglob('*'):
        if p.is_file() and should_process(p):
            try:
                data = p.read_bytes()
                # skip binary files
                if b'\x00' in data[:1024]:
                    continue
                text = data.decode('utf-8')
            except Exception:
                continue
            new = process_text(text)
            if new != text:
                p.write_text(new, encoding='utf-8')
                changed.append(str(p.relative_to(ROOT)))
    print(f"Processed files: {len(changed)}")
    for c in changed:
        print(c)

if __name__ == '__main__':
    main()
