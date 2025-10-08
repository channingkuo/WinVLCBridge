# VLC SDK 获取指南

> ⚠️ **重要**：从 VLC 官网下载的**播放器安装包**不包含开发所需的 SDK！

## 🎯 问题说明

### VLC 播放器 vs VLC SDK

| 下载内容 | 文件名示例 | 包含 SDK？ | 用途 |
|----------|-----------|-----------|------|
| **VLC 播放器** | `vlc-3.0.21-win64.exe`<br>`vlc-3.0.21-win64.zip` | ❌ 无 | 普通用户播放视频 |
| **VLC SDK** | 需要单独获取 | ✅ 有 | 开发者集成到应用 |

### SDK 包含什么

```
vlc-3.0.21/
├── sdk/
│   ├── include/           # ✅ 头文件（vlc.h 等）
│   │   └── vlc/
│   │       ├── vlc.h
│   │       ├── libvlc.h
│   │       ├── libvlc_media.h
│   │       └── ...
│   └── lib/               # ✅ 链接库（.lib 文件）
│       ├── libvlc.lib
│       └── libvlccore.lib
├── libvlc.dll             # ✅ 运行时库
├── libvlccore.dll         # ✅ 运行时库
└── plugins/               # ✅ VLC 插件
```

---

## ✅ 获取 VLC SDK 的方法

### 方法 1：从 NuGet 下载（最简单，推荐）

NuGet 是微软的包管理器，VLC 官方在这里发布 SDK。

#### 🚀 自动化方式（一键设置）

**在 Windows 上：**
```cmd
cd WinVLCBridge
setup_vlc_sdk.bat
```

**在 Mac/Linux 上：**
```bash
cd WinVLCBridge
chmod +x setup_vlc_sdk.sh
./setup_vlc_sdk.sh
```

这会自动：
- 下载 VLC SDK (NuGet 包)
- 解压并组织目录结构
- 创建正确的 sdk 文件夹

#### 手动方式

**步骤 1：下载 NuGet 包**

访问：https://www.nuget.org/packages/VideoLAN.LibVLC.Windows

或直接下载：
```bash
# VLC 3.0.20 (最新稳定版)
curl -L -o libvlc.zip "https://www.nuget.org/api/v2/package/VideoLAN.LibVLC.Windows/3.0.20"
```

**步骤 2：解压**
```bash
unzip libvlc.zip -d libvlc-nuget
```

**步骤 3：提取文件**

NuGet 包的结构：
```
libvlc-nuget/
└── build/
    └── x64/              # 64位版本
        ├── include/      # 头文件
        │   └── vlc/
        │       └── *.h
        ├── libvlc.lib    # 链接库
        ├── libvlccore.lib
        ├── libvlc.dll    # 运行时
        └── plugins/      # 插件
```

**步骤 4：组织到项目结构**
```bash
mkdir -p vlc-3.0.21/sdk/include
mkdir -p vlc-3.0.21/sdk/lib

cp -r libvlc-nuget/build/x64/include/* vlc-3.0.21/sdk/include/
cp libvlc-nuget/build/x64/*.lib vlc-3.0.21/sdk/lib/
cp libvlc-nuget/build/x64/*.dll vlc-3.0.21/
cp -r libvlc-nuget/build/x64/plugins vlc-3.0.21/
```

---

### 方法 2：从 VLC 官方 FTP 下载

官方 FTP 有完整的开发包。

#### 访问地址

```
https://download.videolan.org/pub/videolan/vlc/
```

#### 选择版本

例如 3.0.21：
```
https://download.videolan.org/pub/videolan/vlc/3.0.21/win64/
```

#### 下载文件

**需要下载两个文件：**

1. **主包（包含 DLL 和插件）：**
   ```
   vlc-3.0.21-win64.zip
   ```

2. **开发包（包含头文件）：**
   
   在同一目录下找：
   ```
   vlc-3.0.21-devel-win64.zip     （如果有的话）
   ```
   
   或者从源码包提取头文件。

---

### 方法 3：从源码编译（高级用户）

如果你需要自定义编译或特定版本：

#### 下载源码

```bash
git clone https://git.videolan.org/git/vlc.git
cd vlc
git checkout 3.0.21
```

#### 编译（需要 MSYS2）

这个过程比较复杂，需要：
- MSYS2 环境
- 完整的编译工具链
- 数小时的编译时间

**不推荐**，除非你有特殊需求。

---

### 方法 4：使用 vcpkg（现代 C++ 包管理器）

vcpkg 是微软的 C++ 包管理器。

#### 安装 vcpkg

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
```

#### 安装 VLC

```bash
./vcpkg install vlc:x64-windows
```

#### 集成到项目

```bash
./vcpkg integrate install
```

修改 CMakeLists.txt：
```cmake
find_package(libvlc CONFIG REQUIRED)
target_link_libraries(WinVLCBridge PRIVATE libvlc)
```

---

## 📦 推荐的目录结构

无论使用哪种方法，最终应该得到这样的结构：

```
WinVLCBridge/
└── vlc-3.0.21/
    ├── sdk/
    │   ├── include/
    │   │   └── vlc/
    │   │       ├── vlc.h              ✅ 主头文件
    │   │       ├── libvlc.h           ✅ LibVLC API
    │   │       ├── libvlc_media.h     ✅ 媒体控制
    │   │       ├── libvlc_media_player.h  ✅ 播放器控制
    │   │       └── ...
    │   └── lib/
    │       ├── libvlc.lib             ✅ 链接库
    │       └── libvlccore.lib         ✅ 核心库
    ├── libvlc.dll                     ✅ 运行时 DLL
    ├── libvlccore.dll                 ✅ 核心 DLL
    └── plugins/                       ✅ VLC 插件目录
        ├── access/
        ├── audio_filter/
        ├── audio_output/
        ├── codec/
        ├── control/
        ├── demux/
        └── ...
```

---

## 🔍 验证 SDK 是否正确

### 检查清单

运行以下命令验证：

**在 Windows (PowerShell)：**
```powershell
cd WinVLCBridge

# 检查头文件
Test-Path "vlc-3.0.21/sdk/include/vlc/vlc.h"
# 应该输出: True

# 检查库文件
Test-Path "vlc-3.0.21/sdk/lib/libvlc.lib"
# 应该输出: True

# 检查 DLL
Test-Path "vlc-3.0.21/libvlc.dll"
# 应该输出: True

# 检查插件
Test-Path "vlc-3.0.21/plugins"
# 应该输出: True
```

**在 Mac/Linux (Bash)：**
```bash
cd WinVLCBridge

# 检查所有必需文件
[ -f "vlc-3.0.21/sdk/include/vlc/vlc.h" ] && echo "✅ vlc.h 存在" || echo "❌ vlc.h 缺失"
[ -f "vlc-3.0.21/sdk/lib/libvlc.lib" ] && echo "✅ libvlc.lib 存在" || echo "❌ libvlc.lib 缺失"
[ -f "vlc-3.0.21/libvlc.dll" ] && echo "✅ libvlc.dll 存在" || echo "❌ libvlc.dll 缺失"
[ -d "vlc-3.0.21/plugins" ] && echo "✅ plugins 目录存在" || echo "❌ plugins 目录缺失"
```

### 查看头文件内容

```bash
# 查看 vlc.h 的前几行，确认是正确的头文件
head -n 20 vlc-3.0.21/sdk/include/vlc/vlc.h
```

应该看到类似这样的内容：
```c
/*****************************************************************************
 * vlc.h: global header for libvlc
 *****************************************************************************
 * Copyright (C) 1998-2008 VLC authors and VideoLAN
 * ...
 */

#ifndef VLC_VLC_H
#define VLC_VLC_H 1

#include <vlc/libvlc.h>
...
```

---

## 🚀 快速开始（推荐流程）

### 在 Windows 上

```cmd
cd WinVLCBridge

REM 1. 自动设置 SDK
setup_vlc_sdk.bat

REM 2. 验证
dir vlc-3.0.21\sdk\include\vlc\vlc.h
REM 应该显示文件信息

REM 3. 编译
build.bat release
```

### 在 Mac 上

```bash
cd WinVLCBridge

# 1. 自动设置 SDK（需要 curl 和 unzip）
chmod +x setup_vlc_sdk.sh
./setup_vlc_sdk.sh

# 2. 验证
ls -lh vlc-3.0.21/sdk/include/vlc/vlc.h
# 应该显示文件信息

# 3. 编译（使用 GitHub Actions 或虚拟机）
# 参考 MAC_BUILD_GUIDE.md
```

---

## 💡 各版本下载链接

### VLC 3.0.x（推荐）

| 版本 | NuGet 包 | 官方 FTP |
|------|----------|----------|
| **3.0.20** | [NuGet](https://www.nuget.org/packages/VideoLAN.LibVLC.Windows/3.0.20) | [FTP](https://download.videolan.org/pub/videolan/vlc/3.0.20/) |
| **3.0.21** | [NuGet](https://www.nuget.org/packages/VideoLAN.LibVLC.Windows/3.0.21) | [FTP](https://download.videolan.org/pub/videolan/vlc/3.0.21/) |
| **3.0.18** | [NuGet](https://www.nuget.org/packages/VideoLAN.LibVLC.Windows/3.0.18) | [FTP](https://download.videolan.org/pub/videolan/vlc/3.0.18/) |

### VLC 4.0.x（测试版）

⚠️ **警告**：4.0 版本仍在开发中，API 可能不稳定。

```
https://download.videolan.org/pub/videolan/vlc/4.0/
```

---

## ❓ 常见问题

### Q1: 我下载了 vlc-3.0.21-win64.zip，但没有 sdk 文件夹

**A:** 普通的播放器安装包不包含 SDK。你需要：
- 使用 **方法 1**（NuGet 包）最简单
- 或运行 `setup_vlc_sdk.bat` 自动设置

### Q2: vlc.h 文件在哪里？

**A:** 正确的路径应该是：
```
vlc-3.0.21/sdk/include/vlc/vlc.h
```

如果没有，说明 SDK 设置不正确，重新运行 `setup_vlc_sdk.bat`。

### Q3: 编译时提示找不到 libvlc.lib

**A:** 检查文件是否存在：
```cmd
dir vlc-3.0.21\sdk\lib\libvlc.lib
```

如果不存在，SDK 不完整，需要重新下载。

### Q4: 下载 NuGet 包很慢

**A:** 可以使用国内镜像：

```bash
# 使用阿里云镜像
curl -L -o libvlc.zip "https://mirrors.aliyun.com/nuget/packages/videolan.libvlc.windows/3.0.20"
```

### Q5: 我需要 32 位版本

**A:** 修改下载 URL：

```bash
# NuGet 包中包含 x86 版本
# 解压后使用 build/x86/ 目录而不是 build/x64/
```

在 CMakeLists.txt 中：
```cmake
# 改为 32 位
cmake .. -G "Visual Studio 16 2019" -A Win32
```

---

## 📋 完整的自动化脚本

如果自动脚本不工作，这是完整的手动步骤：

```bash
#!/bin/bash
# 完整的 VLC SDK 设置（Mac/Linux）

cd WinVLCBridge

# 1. 下载
curl -L -o libvlc.zip \
  "https://www.nuget.org/api/v2/package/VideoLAN.LibVLC.Windows/3.0.20"

# 2. 解压
unzip -q libvlc.zip -d libvlc-nuget

# 3. 创建目录
mkdir -p vlc-3.0.21/sdk/{include,lib}

# 4. 复制文件
cp -r libvlc-nuget/build/x64/include/* vlc-3.0.21/sdk/include/
cp libvlc-nuget/build/x64/{libvlc.lib,libvlccore.lib} vlc-3.0.21/sdk/lib/
cp libvlc-nuget/build/x64/*.dll vlc-3.0.21/
cp -r libvlc-nuget/build/x64/plugins vlc-3.0.21/

# 5. 清理
rm -rf libvlc-nuget libvlc.zip

# 6. 验证
echo "验证文件..."
[ -f "vlc-3.0.21/sdk/include/vlc/vlc.h" ] && echo "✅ SDK 设置成功！" || echo "❌ SDK 设置失败！"
```

---

## 🎯 总结

### 推荐方案

1. **最简单** → 运行 `setup_vlc_sdk.bat`（Windows）或 `VLC_SDK_SETUP.md`（Mac）
2. **手动下载** → 从 NuGet 下载并手动组织
3. **使用 vcpkg** → 如果你熟悉现代 C++ 工具链

### 关键点

- ✅ **不要**从 VLC 官网下载普通的播放器安装包
- ✅ **要**从 NuGet 或官方 FTP 下载 SDK
- ✅ **确保**目录结构正确（sdk/include/vlc/vlc.h）
- ✅ **验证**所有文件都存在再开始编译

---

**下一步：** 运行 `setup_vlc_sdk.bat` 自动设置，然后执行 `build.bat release` 编译项目！

