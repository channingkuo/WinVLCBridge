# WinVLCBridge 文档索引

> 📚 快速找到你需要的文档

## 🎯 我想...

### 快速开始
- **5分钟上手** → [QUICK_START.md](QUICK_START.md) ⚡
- **查看完整文档** → [README.md](README.md) 📖
- **了解 API** → [WinVLCBridge.h](WinVLCBridge.h) 📄

### 编译和构建
- **获取 VLC SDK** → [VLC_SDK_GUIDE.md](VLC_SDK_GUIDE.md) 📦
- **在 Windows 上编译** → [BUILD.md](BUILD.md) 🔨
- **在 Mac 上编译** → [MAC_BUILD_GUIDE.md](MAC_BUILD_GUIDE.md) 🍎
- **快速编译** → 双击 `build.bat` 或 [build.bat](build.bat) 🚀
- **清理编译** → 双击 `clean.bat` 或 [clean.bat](clean.bat) 🧹

### 使用和集成
- **代码示例** → [example_usage.js](example_usage.js) 💻
- **打包分发** → [打包说明.md](打包说明.md) 📦
- **Electron 集成** → [example_usage.js](example_usage.js) 中的 `electronExample()` 🖥️

### 高级主题
- **平台差异** → [DIFFERENCES.md](DIFFERENCES.md) 🔄
- **性能优化** → [README.md](README.md) 的性能建议部分 ⚡
- **问题排查** → [README.md](README.md) 的常见问题部分 🔧

---

## 📚 文档列表

### 核心文档

| 文档 | 用途 | 阅读时间 | 重要性 |
|------|------|----------|--------|
| [README.md](README.md) | 完整的项目文档和 API 参考 | 15 分钟 | ⭐⭐⭐⭐⭐ |
| [QUICK_START.md](QUICK_START.md) | 快速开始指南 | 5 分钟 | ⭐⭐⭐⭐⭐ |
| [VLC_SDK_GUIDE.md](VLC_SDK_GUIDE.md) | VLC SDK 获取和设置 | 10 分钟 | ⭐⭐⭐⭐⭐ |
| [BUILD.md](BUILD.md) | 详细的编译指南 | 10 分钟 | ⭐⭐⭐⭐ |

### 参考文档

| 文档 | 用途 | 阅读时间 | 重要性 |
|------|------|----------|--------|
| [example_usage.js](example_usage.js) | 完整的代码示例和最佳实践 | 10 分钟 | ⭐⭐⭐⭐⭐ |
| [打包说明.md](打包说明.md) | 打包和分发指南 | 8 分钟 | ⭐⭐⭐⭐ |
| [MAC_BUILD_GUIDE.md](MAC_BUILD_GUIDE.md) | 在 Mac 上构建 Windows DLL | 12 分钟 | ⭐⭐⭐⭐ |
| [DIFFERENCES.md](DIFFERENCES.md) | macOS 和 Windows 版本的差异 | 12 分钟 | ⭐⭐⭐ |

### 工具脚本

| 文件 | 用途 | 使用方式 |
|------|------|----------|
| [build.bat](build.bat) | 自动编译脚本 | 双击运行或 `build.bat release` |
| [clean.bat](clean.bat) | 清理编译文件 | 双击运行 |
| [CMakeLists.txt](CMakeLists.txt) | CMake 构建配置 | 由 CMake 使用 |

### 源代码

| 文件 | 说明 | 行数 |
|------|------|------|
| [WinVLCBridge.h](WinVLCBridge.h) | C API 头文件 | ~100 |
| [WinVLCBridge.cpp](WinVLCBridge.cpp) | 实现文件 | ~600 |

---

## 🎓 学习路径

### 初学者路径

1. **第一步：了解项目**
   - 阅读 [README.md](README.md) 的功能特性部分
   - 大致了解能做什么

2. **第二步：编译 DLL**
   - 按照 [QUICK_START.md](QUICK_START.md) 的步骤 1 编译
   - 或直接阅读 [BUILD.md](BUILD.md) 了解详细步骤

3. **第三步：运行示例**
   - 查看 [example_usage.js](example_usage.js)
   - 复制最小示例到你的项目

4. **第四步：深入学习**
   - 阅读 [README.md](README.md) 的 API 参考部分
   - 了解每个函数的用法

### 进阶用户路径

1. **优化性能**
   - 阅读 [README.md](README.md) 的性能建议
   - 了解 [DIFFERENCES.md](DIFFERENCES.md) 中的性能对比

2. **跨平台开发**
   - 比较 [DIFFERENCES.md](DIFFERENCES.md) 了解平台差异
   - 设计统一的接口层

3. **生产部署**
   - 阅读 [打包说明.md](打包说明.md)
   - 设置 CI/CD 流程

### 问题排查路径

遇到问题时：

1. **编译问题** → [BUILD.md](BUILD.md) 的常见问题部分
2. **使用问题** → [README.md](README.md) 的常见问题部分
3. **性能问题** → [README.md](README.md) 的性能建议部分
4. **示例代码** → [example_usage.js](example_usage.js) 的完整示例

---

## 🔍 按场景查找

### 场景 1：我想在 Electron 中播放视频

**推荐阅读顺序：**
1. [QUICK_START.md](QUICK_START.md) - 了解基本用法
2. [example_usage.js](example_usage.js) - 查看 `electronExample()` 函数
3. [README.md](README.md) - 了解 API 详情

**关键代码：**
```javascript
const hwnd = mainWindow.getNativeWindowHandle();
const player = new VLCPlayer(hwnd, 0, 0, 800, 600);
player.play('video.mp4');
```

---

### 场景 2：我想在视频上绘制矩形框

**推荐阅读顺序：**
1. [README.md](README.md) - 查看矩形覆盖层 API
2. [example_usage.js](example_usage.js) - 查看 `updateRectangles` 示例
3. [QUICK_START.md](QUICK_START.md) - 查看速查表

**关键代码：**
```javascript
player.updateRectangles(
    [{ x: 100, y: 100, width: 200, height: 150 }],
    2.0,  // 线宽
    { r: 1, g: 0, b: 0, a: 1 }  // 红色
);
```

---

### 场景 3：我想播放 RTSP 网络流

**推荐阅读顺序：**
1. [README.md](README.md) - 查看支持的格式
2. [example_usage.js](example_usage.js) - 查看网络流示例
3. [BUILD.md](BUILD.md) - 确保编译时包含网络插件

**关键代码：**
```javascript
player.play('rtsp://example.com/stream');
```

---

### 场景 4：我想实现目标检测标注

**推荐阅读顺序：**
1. [example_usage.js](example_usage.js) - 查看 `VideoPlayerWithDetection` 类
2. [README.md](README.md) - 了解应用场景部分
3. [DIFFERENCES.md](DIFFERENCES.md) - 了解性能特点

**关键代码：**
```javascript
const detector = new VideoPlayerWithDetection(hwnd, 0, 0, 800, 600);
detector.playWithDetection('rtsp://camera.local/stream');
```

---

### 场景 5：我想打包分发我的应用

**推荐阅读顺序：**
1. [打包说明.md](打包说明.md) - 完整的打包流程
2. [BUILD.md](BUILD.md) - 确保使用 Release 配置
3. [README.md](README.md) - 检查依赖要求

**关键步骤：**
```cmd
build.bat release
cd build\bin\Release
# 复制所有文件到分发目录
```

---

### 场景 6：我想从 macOS 迁移到 Windows

**推荐阅读顺序：**
1. [DIFFERENCES.md](DIFFERENCES.md) - 详细的平台差异说明
2. [README.md](README.md) - 查看 API 对应关系表
3. [example_usage.js](example_usage.js) - 查看 Windows 特定代码

**主要差异：**
- 函数前缀：`mv_` → `wv_`
- 窗口参数：`NSView*` → `HWND`
- 其他 API 完全相同

---

### 场景 7：我在 Mac 上开发，需要编译 Windows DLL

**推荐阅读顺序：**
1. [MAC_BUILD_GUIDE.md](MAC_BUILD_GUIDE.md) - 完整的 Mac 构建指南
2. 选择方案：GitHub Actions（推荐）或虚拟机
3. [BUILD.md](BUILD.md) - 了解 Windows 编译细节

**推荐方案：**
- **开源项目** → GitHub Actions（免费、自动）
- **频繁编译** → Parallels Desktop（快速、方便）
- **预算有限** → VirtualBox（免费）

**快速开始：**
```bash
# 推送到 GitHub，自动编译
git push origin master
# 从 Actions 下载 DLL
```

---

## 📖 API 速查

快速查找 API：

| API | 功能 | 文档位置 |
|-----|------|----------|
| `wv_create_player_for_view` | 创建播放器 | [WinVLCBridge.h](WinVLCBridge.h#L24) |
| `wv_player_play` | 播放视频 | [WinVLCBridge.h](WinVLCBridge.h#L32) |
| `wv_player_pause` | 暂停播放 | [WinVLCBridge.h](WinVLCBridge.h#L38) |
| `wv_player_resume` | 恢复播放 | [WinVLCBridge.h](WinVLCBridge.h#L44) |
| `wv_player_stop` | 停止播放 | [WinVLCBridge.h](WinVLCBridge.h#L50) |
| `wv_player_release` | 释放资源 | [WinVLCBridge.h](WinVLCBridge.h#L56) |
| `wv_player_update_rectangles` | 更新矩形 | [WinVLCBridge.h](WinVLCBridge.h#L68) |
| `wv_player_clear_rectangles` | 清除矩形 | [WinVLCBridge.h](WinVLCBridge.h#L74) |

---

## 🛠️ 工具和资源

### 必备工具

- **编译器**: Visual Studio 2019+ (免费的 Community 版本)
- **构建工具**: CMake 3.15+ (通常 VS 自带)
- **调试工具**: [DebugView](https://docs.microsoft.com/sysinternals/downloads/debugview) - 查看日志

### 依赖库

- **VLC SDK**: [下载地址](https://www.videolan.org/vlc/download-windows.html)
- **ffi-napi**: `npm install ffi-napi` - Node.js FFI 绑定
- **ref-napi**: `npm install ref-napi` - Node.js 引用类型

### 相关链接

- **libVLC 文档**: https://www.videolan.org/developers/vlc/doc/doxygen/html/group__libvlc.html
- **GDI+ 文档**: https://docs.microsoft.com/windows/win32/gdiplus/
- **CMake 文档**: https://cmake.org/documentation/

---

## 💡 提示和技巧

### 快速提示

1. **首次使用？** 直接跳到 [QUICK_START.md](QUICK_START.md)
2. **遇到错误？** 先查看日志（使用 DebugView）
3. **性能问题？** 查看 [README.md](README.md) 的性能建议
4. **需要示例？** 所有示例都在 [example_usage.js](example_usage.js)

### 最佳实践

- ✅ 始终调用 `wv_player_release()` 释放资源
- ✅ 使用 Release 配置进行生产构建
- ✅ 限制矩形更新频率（< 30 FPS）
- ✅ 矩形数量不超过 50 个
- ✅ 网络流使用 RTSP 而不是 HTTP（更低延迟）

---

## 📞 获取帮助

### 问题排查顺序

1. **查看文档** - 先在相关文档中搜索
2. **查看日志** - 使用 DebugView 查看 `[WinVLCBridge]` 日志
3. **查看示例** - 对比 [example_usage.js](example_usage.js)
4. **创建 Issue** - 如果问题仍未解决

### 报告问题时请提供

- 操作系统版本
- Visual Studio 版本
- VLC SDK 版本
- 完整的错误日志
- 最小复现代码

---

## 📊 文档统计

- **总文档数**: 11 个
- **总代码行数**: ~700 行（C++）+ ~400 行（JavaScript）
- **支持平台**: Windows 7/8/10/11 (x64)
- **项目版本**: v1.0.0

---

## 🔄 更新日志

- **2025-10-07**: 初始版本
  - 完整的文档集
  - 核心功能实现
  - 示例代码和构建脚本

---

## ⭐ 推荐阅读流程

### 快速流程（30 分钟）

1. [QUICK_START.md](QUICK_START.md) - 5 分钟
2. 编译 DLL - 10 分钟
3. [example_usage.js](example_usage.js) - 15 分钟

### 完整流程（2 小时）

1. [README.md](README.md) - 15 分钟
2. [BUILD.md](BUILD.md) - 10 分钟
3. 编译和测试 - 30 分钟
4. [example_usage.js](example_usage.js) - 20 分钟
5. [DIFFERENCES.md](DIFFERENCES.md) - 12 分钟
6. [打包说明.md](打包说明.md) - 8 分钟
7. 实践和调试 - 25 分钟

---

**提示**: 将此文档加入书签，方便快速查找！🔖

