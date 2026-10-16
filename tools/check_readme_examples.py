#!/usr/bin/env python3
"""README の ```cpp ブロックを抽出してコンパイル可否を判定する検証スクリプト。

使い方: python3 tools/check_readme_examples.py [--fix-list]
出力: 失敗ブロックの開始行番号と理由
"""
import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
README = ROOT / "README.md"
PRELUDE = (
    '#include "frozenchars.hpp"\n'
    "using namespace frozenchars;\n"
    "using namespace frozenchars::literals;\n"
    "namespace fops = frozenchars::ops;\n"
)


def extract_blocks(text: str):
    blocks = []
    lines = text.splitlines()
    skipping = False
    i = 0
    while i < len(lines):
        if "check: skip" in lines[i]:
            # 意図的にコンパイルエラーになる例は次の ```cpp ブロックまで読み飛ばす
            skipping = True
            i += 1
            continue
        m = re.match(r"^```cpp\s*$", lines[i])
        if m and skipping:
            skipping = False
            start = i + 1
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                i += 1
            i += 1
            continue
        if m:
            start = i + 1
            body = []
            i += 1
            while i < len(lines) and not lines[i].startswith("```"):
                body.append(lines[i])
                i += 1
            blocks.append((start, "\n".join(body)))
        i += 1
    return blocks


def main() -> int:
    text = README.read_text(encoding="utf-8")
    blocks = extract_blocks(text)
    failures = []
    for lineno, body in blocks:
        if not body.strip():
            continue
        # main() を持つブロックはそのまま、それ以外はプリアンブルを足す
        src = body if "int main(" in body else PRELUDE + body + "\nint main() {}\n"
        with tempfile.TemporaryDirectory() as tmp:
            cpp = Path(tmp) / "block.cpp"
            cpp.write_text(src, encoding="utf-8")
            proc = subprocess.run(
                ["g++", "-std=c++23", "-fsyntax-only",
                 f"-I{ROOT / 'include'}", str(cpp)],
                capture_output=True, text=True, timeout=300,
            )
        if proc.returncode != 0:
            first = next(
                (ln for ln in proc.stderr.splitlines() if " error:" in ln), "")
            failures.append((lineno, first.strip()[:160]))
    print(f"blocks: {len(blocks)}, failures: {len(failures)}")
    for lineno, msg in failures:
        print(f"  README.md:{lineno}: {msg}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())