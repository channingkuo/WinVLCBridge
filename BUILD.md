# Windows VLC Bridge 编译指南

本文档描述如何在 Windows 上编译 `WinVLCBridge.dll`。

## 前置要求

### 1. 安装必要工具

- **Visual Studio 2019 或更高版本**（推荐 Community 版本）
  - 安装时确保勾选 "使用 C++ 的桌面开发" 工作负载
  - 包含 CMake 工具和 Windows SDK

- **CMake 3.15+**（如果 VS 没有包含）
  - 下载地址：https://cmake.org/download/

### 2. 准备 VLC SDK

#### 方案 A：使用已有的 VLC 3.0.21（推荐）

项目已经包含了 `vlc-3.0.21` 目录，需要确保它包含以下结构：

```
vlc-3.0.21/
├── libvlc.dll
├── libvlccore.dll
├── plugins/           # VLC 插件目录
└── sdk/
    ├── include/
    │   └── vlc/
    │       ├── vlc.h
    │       └── ...
    └── lib/
        ├── libvlc.lib
        └── libvlccore.lib
```

如果缺少 `sdk` 目录，请从官方下载 VLC SDK。

#### 方案 B：下载 VLC SDK

1. 访问 VLC 官网：https://www.videolan.org/vlc/download-windows.html
2. 下载 VLC SDK（通常是 `vlc-3.x.x-win64.zip` 或 `vlc-3.x.x-win32.zip`）
3. 解压到 `WinVLCBridge/vlc-3.0.21/` 目录

## 编译步骤

### 方法 1：使用 Visual Studio（推荐）

1. **打开 Visual Studio**

2. **选择 "打开本地文件夹"**，选择 `WinVLCBridge` 目录

3. **配置 CMake**
   - Visual Studio 会自动检测 CMakeLists.txt
   - 在顶部工具栏选择配置（Debug 或 Release）
   - 选择目标平台（x64 推荐）

4. **构建项目**
   - 菜单：生成 → 全部生成
   - 或按 `Ctrl+Shift+B`

5. **输出位置**
   - 编译完成后，DLL 文件位于：
   ```
   WinVLCBridge/out/build/x64-Release/bin/WinVLCBridge.dll
   ```

### 方法 2：使用命令行（CMake + MSBuild）

1. **打开 "x64 Native Tools Command Prompt for VS 2019"**

2. **进入项目目录**
   ```cmd
   cd C:\path\to\electron-vlc-demo\WinVLCBridge
   ```

3. **创建构建目录**
   ```cmd
   mkdir build
   cd build
   ```

4. **运行 CMake 配置**
   ```cmd
   cmake .. -G "Visual Studio 16 2019" -A x64
   ```

5. **编译项目**
   ```cmd
   cmake --build . --config Release
   ```

6. **输出位置**
   ```
   WinVLCBridge/build/bin/Release/WinVLCBridge.dll
   ```

### 方法 3：使用 MinGW（备选方案）

如果使用 MinGW-w64：

1. **安装 MinGW-w64**
   - 下载：https://www.mingw-w64.org/downloads/

2. **配置和编译**
   ```bash
   cd WinVLCBridge
   mkdir build && cd build
   cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
   cmake --build .
   ```

## 自定义 VLC 路径

如果 VLC SDK 不在默认位置，可以通过 CMake 参数指定：

```cmd
cmake .. -DVLC_PATH="C:\path\to\vlc-sdk"
```

## 验证编译结果

编译成功后，`bin` 目录应该包含：

```
bin/
├── WinVLCBridge.dll    # 主要的桥接库
├── libvlc.dll          # VLC 核心库（自动复制）
├── libvlccore.dll      # VLC 核心库（自动复制）
└── plugins/            # VLC 插件目录（自动复制）
```

## 在 Electron 中使用

1. **复制 DLL 文件**
   
   将以下文件复制到 Electron 项目目录：
   ```
   electron-vlc-demo/
   └── electron/
       └── native/
           ├── WinVLCBridge.dll
           ├── libvlc.dll
           ├── libvlccore.dll
           └── plugins/
   ```

2. **在 Node.js 中加载**
   ```javascript
   const ffi = require('ffi-napi');
   const path = require('path');
   
   const dllPath = path.join(__dirname, 'native', 'WinVLCBridge.dll');
   
   const WinVLCBridge = ffi.Library(dllPath, {
       'wv_create_player_for_view': ['pointer', ['pointer', 'float', 'float', 'float', 'float']],
       'wv_player_play': ['void', ['pointer', 'string']],
       'wv_player_pause': ['void', ['pointer']],
       'wv_player_resume': ['void', ['pointer']],
       'wv_player_stop': ['void', ['pointer']],
       'wv_player_release': ['void', ['pointer']],
       'wv_player_update_rectangles': ['void', ['pointer', 'pointer', 'int', 'float', 'float', 'float', 'float', 'float']],
       'wv_player_clear_rectangles': ['void', ['pointer']]
   });
   ```

## 常见问题

### 1. 找不到 vlc.h

**解决方案**：确保 VLC SDK 的 `include` 目录结构正确，应该是：
```
vlc-3.0.21/sdk/include/vlc/vlc.h
```

### 2. 链接错误：无法找到 libvlc.lib

**解决方案**：
- 检查 `vlc-3.0.21/sdk/lib/` 目录是否存在
- 确保使用的是与编译器匹配的库（64位 vs 32位）

### 3. 运行时错误：找不到 DLL

**解决方案**：确保以下文件在同一目录：
- WinVLCBridge.dll
- libvlc.dll
- libvlccore.dll
- plugins/ 目录

### 4. GDI+ 错误

**解决方案**：确保链接了 `gdiplus.lib`，这是 Windows SDK 的一部分。

## 调试

### 启用详细日志

代码中已包含详细的日志输出，使用 DebugView 工具查看：
1. 下载 DebugView：https://docs.microsoft.com/sysinternals/downloads/debugview
2. 运行 DebugView
3. 运行你的应用，日志会显示在 DebugView 中

### 使用 Visual Studio 调试器

1. 在 Visual Studio 中设置调试配置
2. 设置断点
3. 附加到 Electron 进程或直接调试 DLL

## 性能优化

### Release 编译优化

默认 Release 配置已启用优化，如需更激进的优化：

```cmake
# 在 CMakeLists.txt 中添加
target_compile_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Release>:/O2 /GL>
)
target_link_options(${PROJECT_NAME} PRIVATE 
    $<$<CONFIG:Release>:/LTCG>
)
```

## 更新日志

- **2025-10-07**：初始版本，支持基本的播放控制和矩形覆盖层

## 技术支持

如有问题，请检查：
1. VLC SDK 版本是否兼容（推荐 3.0.x）
2. Visual Studio 版本是否支持
3. Windows SDK 是否正确安装
4. 编译目标平台（x64/x86）是否一致

## API 参考

详细的 API 使用说明请参考 `WinVLCBridge.h` 中的注释。

所有函数名称以 `wv_` 开头，对应 macOS 版本的 `mv_` 前缀。

