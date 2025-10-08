#!/bin/bash
# VLC SDK 自动设置脚本
# 适用于 Mac 和 Linux

set -e

echo "=========================================="
echo "VLC SDK 自动设置脚本"
echo "=========================================="
echo ""

VLC_VERSION="3.0.20"
NUGET_URL="https://www.nuget.org/api/v2/package/VideoLAN.LibVLC.Windows/${VLC_VERSION}"
TARGET_DIR="vlc-3.0.21"

echo "步骤 1: 下载 VLC SDK (NuGet 包)..."
curl -L -o libvlc.zip "$NUGET_URL"

echo ""
echo "步骤 2: 解压..."
unzip -q libvlc.zip -d libvlc-nuget

echo ""
echo "步骤 3: 创建目录结构..."
mkdir -p "${TARGET_DIR}/sdk/include"
mkdir -p "${TARGET_DIR}/sdk/lib"

echo ""
echo "步骤 4: 复制头文件..."
cp -r libvlc-nuget/build/x64/include/* "${TARGET_DIR}/sdk/include/"

echo ""
echo "步骤 5: 复制库文件..."
cp libvlc-nuget/build/x64/libvlc.lib "${TARGET_DIR}/sdk/lib/"
cp libvlc-nuget/build/x64/libvlccore.lib "${TARGET_DIR}/sdk/lib/"

echo ""
echo "步骤 6: 复制 DLL 和插件..."
cp libvlc-nuget/build/x64/*.dll "${TARGET_DIR}/"
cp -r libvlc-nuget/build/x64/plugins "${TARGET_DIR}/"

echo ""
echo "步骤 7: 清理临时文件..."
rm -rf libvlc-nuget libvlc.zip

echo ""
echo "=========================================="
echo "✅ VLC SDK 设置完成！"
echo "=========================================="
echo ""
echo "目录结构："
echo "${TARGET_DIR}/"
echo "├── sdk/"
echo "│   ├── include/vlc/"
echo "│   │   └── vlc.h ✅"
echo "│   └── lib/"
echo "│       ├── libvlc.lib ✅"
echo "│       └── libvlccore.lib ✅"
echo "├── libvlc.dll ✅"
echo "├── libvlccore.dll ✅"
echo "└── plugins/ ✅"
echo ""
echo "现在可以运行 build.bat 编译项目了！"

