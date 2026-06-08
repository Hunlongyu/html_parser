#!/usr/bin/env bash
# 拉取「事实标准大文件」性能语料（WHATWG HTML 规范单页），并按 SHA256SUMS 校验。
#
# 大文件不入 git（见 .gitignore）；仓库只提交本脚本与 SHA256SUMS，保证可复现。
# 用法：bash benchmark/fetch_corpus.sh
set -euo pipefail
cd "$(dirname "$0")/corpus"

# name|url 列表；如需扩充语料，在此追加并更新 SHA256SUMS。
echo "fetching html-spec.html (WHATWG HTML 标准单页, ~15MB) ..."
curl -fSL --compressed -o html-spec.html "https://html.spec.whatwg.org/"

echo "verifying checksums ..."
if command -v sha256sum >/dev/null 2>&1; then
    sha256sum -c SHA256SUMS
else
    shasum -a 256 -c SHA256SUMS
fi
echo "corpus ready in $(pwd)"
