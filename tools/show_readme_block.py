#!/usr/bin/env python3
"""README の ```cpp ブロックを1行 指定して表示する補助スクリプト。

使い方: python3 tools/show_readme_block.py 1086 [1089]
"""
import re
import sys
from pathlib import Path

README = Path(__file__).resolve().parent.parent / "README.md"


def main() -> int:
    lines = README.read_text(encoding="utf-8").splitlines()
    blocks = []
    i = 0
    while i < len(lines):
        if re.match(r"^```cpp\s*$", lines[i]):
            start = i + 1
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                i += 1
            blocks.append((start, lines[start:i]))
        i += 1
    for arg in sys.argv[1:]:
        lineno = int(arg)
        for start, body in blocks:
            if start == lineno:
                print(f"===== README.md:{start} =====")
                for offset, line in enumerate(body):
                    print(f"{start + 1 + offset}: {line}")
                print()
                break
        else:
            print(f"===== README.md:{lineno}: ブロックなし =====")
    return 0


if __name__ == "__main__":
    sys.exit(main())