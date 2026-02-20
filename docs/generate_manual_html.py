"""
Cranberry Synth マニュアル: Markdown → HTML 変換スクリプト
ブラウザで開いて Ctrl+P → "PDFとして保存" で PDF に変換できます。
"""

import markdown
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MD_PATH = os.path.join(SCRIPT_DIR, "manual.md")
HTML_PATH = os.path.join(SCRIPT_DIR, "manual.html")

CSS = """
@page {
    size: A4;
    margin: 20mm 18mm 20mm 18mm;
}

* {
    box-sizing: border-box;
}

body {
    font-family: "Segoe UI", "Meiryo", "Yu Gothic", "Hiragino Sans", sans-serif;
    font-size: 10.5pt;
    line-height: 1.7;
    color: #1a1a1a;
    max-width: 700px;
    margin: 0 auto;
    padding: 30px 20px;
    background: #fff;
}

h1 {
    font-size: 20pt;
    color: #b22234;
    border-bottom: 3px solid #b22234;
    padding-bottom: 8px;
    margin-top: 40px;
    margin-bottom: 16px;
    page-break-after: avoid;
}

h1:first-of-type {
    text-align: center;
    font-size: 26pt;
    border-bottom: none;
    margin-top: 60px;
    margin-bottom: 4px;
}

h2 {
    font-size: 14pt;
    color: #333;
    border-left: 4px solid #b22234;
    padding-left: 10px;
    margin-top: 28px;
    margin-bottom: 10px;
    page-break-after: avoid;
}

h3 {
    font-size: 12pt;
    color: #444;
    margin-top: 20px;
    margin-bottom: 8px;
    page-break-after: avoid;
}

p {
    margin: 8px 0;
}

table {
    border-collapse: collapse;
    width: 100%;
    margin: 12px 0;
    font-size: 9.5pt;
    page-break-inside: avoid;
}

th, td {
    border: 1px solid #ccc;
    padding: 6px 10px;
    text-align: left;
}

th {
    background: #f2f2f2;
    font-weight: 600;
    color: #333;
}

tr:nth-child(even) {
    background: #fafafa;
}

code {
    font-family: "Consolas", "Source Code Pro", monospace;
    background: #f5f5f5;
    padding: 2px 5px;
    border-radius: 3px;
    font-size: 9pt;
}

pre {
    background: #f5f5f5;
    border: 1px solid #ddd;
    border-radius: 5px;
    padding: 14px;
    overflow-x: auto;
    font-size: 8.5pt;
    line-height: 1.5;
    page-break-inside: avoid;
}

pre code {
    background: none;
    padding: 0;
}

blockquote {
    border-left: 4px solid #d4a843;
    background: #fffbe6;
    margin: 14px 0;
    padding: 10px 16px;
    color: #5a4a00;
    border-radius: 0 5px 5px 0;
    page-break-inside: avoid;
}

blockquote p {
    margin: 4px 0;
}

strong {
    color: #333;
}

hr {
    border: none;
    border-top: 1px solid #ddd;
    margin: 30px 0;
}

ul, ol {
    padding-left: 24px;
}

li {
    margin: 4px 0;
}

/* 目次スタイル */
h1 + ol, h1 + ul {
    columns: 2;
    column-gap: 30px;
}

@media print {
    body {
        padding: 0;
        max-width: none;
    }

    h1 {
        page-break-before: always;
    }

    h1:first-of-type {
        page-break-before: avoid;
    }

    pre, blockquote, table {
        page-break-inside: avoid;
    }
}
"""

def main():
    with open(MD_PATH, "r", encoding="utf-8") as f:
        md_text = f.read()

    md = markdown.Markdown(extensions=["tables", "fenced_code"])
    html_body = md.convert(md_text)

    html = f"""<!DOCTYPE html>
<html lang="ja">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Cranberry Synth 操作マニュアル</title>
    <style>
{CSS}
    </style>
</head>
<body>
{html_body}
</body>
</html>
"""

    with open(HTML_PATH, "w", encoding="utf-8") as f:
        f.write(html)

    print(f"生成完了: {HTML_PATH}")
    print("ブラウザで開いて Ctrl+P → 'PDFとして保存' で PDF に変換できます。")

if __name__ == "__main__":
    main()
