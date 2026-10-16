#!/usr/bin/env python3
"""README の目次リンクが GitHub の見出しアンカーと一致するか検証する。

使い方:
  python3 tools/check_readme_anchors.py          # 不一致を報告
  python3 tools/check_readme_anchors.py --fix   # 目次を自動生成で置き換える
"""
import re
import sys
import unicodedata
from pathlib import Path

README = Path(__file__).resolve().parent.parent / "README.md"

# github-slugger が保持する文字（Unicode 文字・数字・ハイフン・アンダースコア）
KEEP = re.compile(r"[^\w\- ]", re.UNICODE)


def slugify(text: str) -> str:
    text = re.sub(r"`([^`]*)`", r"\1", text)  # インラインコードのバッククォートを外す
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)  # リンクは文言だけを残す
    text = KEEP.sub("", text.lower())
    text = text.replace(" ", "-")
    # 連続するハイフンは github-slugger に従って1つにまとめる
    text = re.sub(r"-+", "-", text)
    return text.strip("-")


def headings(lines):
    for i, line in enumerate(lines):
        m = re.match(r"^(#{2,4})\s+(.*)$", line)
        if m:
            yield i + 1, len(m.group(1)), m.group(2).strip()


def main() -> int:
    fix = "--fix" in sys.argv
    lines = README.read_text(encoding="utf-8").splitlines()

    # 見出しアンカーの集合（重複は -1, -2 を付ける）
    anchors: set[str] = set()
    counts: dict[str, int] = {}
    for _, _, title in headings(lines):
        base = slugify(title)
        n = counts.get(base, 0)
        counts[base] = n + 1
        anchors.add(base if n == 0 else f"{base}-{n}")

    # 目次リンクを収集
    toc_start = None
    toc_end = None
    for i, line in enumerate(lines):
        if toc_start is None and re.match(r"^##\s+目次\s*$", line):
            toc_start = i
        elif toc_start is not None and line.startswith("## ") and i > toc_start:
            toc_end = i
            break
    if toc_start is None:
        print("目次が見つかりません")
        return 2

    links = []
    for i in range(toc_start, toc_end or len(lines)):
        m = re.match(r"^(\s*)- \[(.+?)\]\(#([^)]*)\)\s*$", lines[i])
        if m:
            links.append((i + 1, m.group(1), m.group(2), m.group(3)))

    broken = [(ln, text, anchor) for ln, _, text, anchor in links if anchor not in anchors]

    if fix:
        indent_of = {2: "", 3: "  ", 4: "    "}
        # 見出しの順番どおりにアンカーを解決する（同一見出しには -1, -2 を付ける）
        seen_counts: dict[str, int] = {}
        resolved = []
        for _, level, title in headings(lines):
            if level == 2 and title == "目次":
                continue
            base = slugify(title)
            n = seen_counts.get(base, 0)
            seen_counts[base] = n + 1
            anchor = base if n == 0 else f"{base}-{n}"
            resolved.append(f"{indent_of[level]}- [{title}](#{anchor})")
        new_toc = ["## 目次", ""] + resolved
        out = lines[:toc_start] + new_toc + [""] + lines[toc_end:]
        README.write_text("\n".join(out) + "\n", encoding="utf-8")
        print(f"目次を {len(resolved)} 項目で再生成しました")
        return 0

    print(f"目次リンク: {len(links)}, 見出し: {len(anchors)}, 不一致: {len(broken)}")
    for ln, text, anchor in broken:
        print(f"  README.md:{ln}: {text} -> #{anchor}")
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main())