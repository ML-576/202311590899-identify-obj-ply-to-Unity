#!/bin/bash
# ============================================================
# build_mac.sh —— macOS 下编译 Unity 插件 ModelReader.bundle
# 需要 Xcode Command Line Tools:  xcode-select --install
# 用法: 在终端中执行  ./build_mac.sh
# ============================================================
set -e
cd "$(dirname "$0")"

OUTDIR="UnityAssets/Plugins/macOS"
mkdir -p "$OUTDIR"

echo "[1/3] CMake 配置 (需要 cmake: brew install cmake) ..."
cmake -B build -DCMAKE_BUILD_TYPE=Release

echo "[2/3] 编译 ..."
cmake --build build -j

echo "[3/3] 复制插件 ..."
cp -f build/ModelReader.bundle "$OUTDIR/" 2>/dev/null || \
cp -f build/*.bundle "$OUTDIR/" 2>/dev/null || \
{ echo "未找到 .bundle 产物"; exit 1; }

echo ""
echo "================== 编译成功 =================="
echo "  插件: $OUTDIR/ModelReader.bundle"
echo "  将 UnityAssets 文件夹整体复制到 Unity 工程的 Assets/ 下即可使用"
echo "=============================================="
