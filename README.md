# WinVLCBridge - Windows VLC 桥接库

这是一个用于在 Windows 平台上集成 VLC 媒体播放器的 C++ 桥接库，特别为 Electron 应用设计。它提供了简单的 C API，支持视频播放和实时矩形覆盖层绘制（适用于目标检测、视频分析等场景）。

## 功能特性

### 核心功能
- ✅ **视频播放**：支持本地文件和网络流（HTTP/RTSP/RTMP）
- ✅ **播放控制**：播放、暂停、恢复、停止
- ✅ **窗口集成**：无缝集成到任何 Windows 窗口（HWND）
- ✅ **矩形覆盖层**：实时绘制矩形框，支持自定义颜色和线宽
- ✅ **透明叠加**：覆盖层完全透明，不影响视频显示和交互
- ✅ **低延迟**：优化的缓存设置，适合直播流

### 技术特点
- 基于 libVLC 3.0.x
- 使用 GDI+ 绘制覆盖层
- 分层窗口（Layered Window）实现透明效果
- 详细的日志输出便于调试
- 线程安全的设计

## 项目结构

```
WinVLCBridge/
├── WinVLCBridge.h          # C API 头文件
├── WinVLCBridge.cpp        # 实现文件
├── CMakeLists.txt          # CMake 构建配置
├── BUILD.md                # 详细的编译指南
├── README.md               # 本文件
├── example_usage.js        # Node.js/Electron 使用示例
└── vlc-3.0.21/            # VLC SDK（需要自行下载）
    ├── libvlc.dll
    ├── libvlccore.dll
    ├── plugins/
    └── sdk/
        ├── include/
        └── lib/
```

## 快速开始

### 0. 准备 VLC SDK

⚠️ **重要**：首先需要获取 VLC SDK（包含头文件和库文件）。

**快速方法（推荐）：**
```bash
# Windows
cd WinVLCBridge
setup_vlc_sdk.bat

# Mac/Linux
cd WinVLCBridge
chmod +x setup_vlc_sdk.sh
./setup_vlc_sdk.sh
```

详细说明请参考 [VLC_SDK_GUIDE.md](VLC_SDK_GUIDE.md)。

### 1. 编译 DLL

详细步骤请参考 [BUILD.md](BUILD.md)。

**简要步骤（Visual Studio）：**
```cmd
cd WinVLCBridge
mkdir build && cd build
cmake .. -G "Visual Studio 16 2019" -A x64
cmake --build . --config Release
```

编译完成后，DLL 位于：`build/bin/Release/WinVLCBridge.dll`

### 2. 在 Node.js/Electron 中使用

安装依赖：
```bash
npm install ffi-napi ref-napi
```

基本使用：
```javascript
const ffi = require('ffi-napi');
const ref = require('ref-napi');
const path = require('path');

// 加载 DLL
const WinVLCBridge = ffi.Library('WinVLCBridge.dll', {
    'wv_create_player_for_view': ['pointer', ['pointer', 'float', 'float', 'float', 'float']],
    'wv_player_play': ['void', ['pointer', 'string']],
    'wv_player_pause': ['void', ['pointer']],
    'wv_player_stop': ['void', ['pointer']],
    'wv_player_release': ['void', ['pointer']],
});

// 创建播放器（hwnd 从 Electron BrowserWindow 获取）
const hwndBuffer = ref.alloc('pointer', hwnd);
const player = WinVLCBridge.wv_create_player_for_view(hwndBuffer, 0, 0, 800, 600);

// 播放视频
WinVLCBridge.wv_player_play(player, 'C:/Videos/sample.mp4');

// 清理
WinVLCBridge.wv_player_stop(player);
WinVLCBridge.wv_player_release(player);
```

完整示例请查看 [example_usage.js](example_usage.js)。

## API 参考

### 创建和释放

#### `wv_create_player_for_view`
```c
void* wv_create_player_for_view(void* hwnd_ptr, float x, float y, float width, float height);
```
创建播放器并关联到指定的窗口。

**参数：**
- `hwnd_ptr`: 父窗口句柄（HWND）
- `x, y`: 视频窗口位置
- `width, height`: 视频窗口大小

**返回：** 播放器句柄（失败返回 NULL）

#### `wv_player_release`
```c
void wv_player_release(void* playerHandle);
```
释放播放器资源。

### 播放控制

#### `wv_player_play`
```c
void wv_player_play(void* playerHandle, const char* source);
```
播放视频（自动识别本地文件或网络流）。

**支持的格式：**
- 本地文件：`C:/Videos/sample.mp4`
- HTTP(S)：`http://example.com/stream.m3u8`
- RTSP：`rtsp://example.com/stream`
- RTMP：`rtmp://example.com/stream`

#### `wv_player_pause`
```c
void wv_player_pause(void* playerHandle);
```
暂停播放。

#### `wv_player_resume`
```c
void wv_player_resume(void* playerHandle);
```
从暂停状态恢复播放。

#### `wv_player_stop`
```c
void wv_player_stop(void* playerHandle);
```
停止播放。

### 矩形覆盖层

#### `wv_player_update_rectangles`
```c
void wv_player_update_rectangles(
    void* playerHandle,
    const float* rects,
    int rectCount,
    float lineWidth,
    float red, float green, float blue, float alpha
);
```
更新覆盖层的矩形列表。

**参数：**
- `rects`: 矩形数组 `[x1, y1, w1, h1, x2, y2, w2, h2, ...]`
- `rectCount`: 矩形数量
- `lineWidth`: 边框宽度（像素）
- `red, green, blue, alpha`: 颜色分量（0.0 - 1.0）

**示例（JavaScript）：**
```javascript
const rectangles = [
    { x: 100, y: 100, width: 200, height: 150 },
    { x: 400, y: 200, width: 150, height: 100 }
];

const rectArray = new Float32Array(rectangles.length * 4);
rectangles.forEach((rect, i) => {
    rectArray[i * 4] = rect.x;
    rectArray[i * 4 + 1] = rect.y;
    rectArray[i * 4 + 2] = rect.width;
    rectArray[i * 4 + 3] = rect.height;
});

const rectBuffer = Buffer.from(rectArray.buffer);
WinVLCBridge.wv_player_update_rectangles(
    player, rectBuffer, rectangles.length,
    2.5,  // lineWidth
    1.0, 0.0, 0.0, 0.8  // 红色，80% 不透明
);
```

#### `wv_player_clear_rectangles`
```c
void wv_player_clear_rectangles(void* playerHandle);
```
清除所有矩形框。

## 应用场景

### 1. 视频监控
实时显示摄像头画面并标注检测目标：
```javascript
// 播放 RTSP 监控流
player.play('rtsp://192.168.1.100/stream');

// 每秒更新检测框
setInterval(() => {
    const detections = performObjectDetection(); // 你的检测逻辑
    player.updateRectangles(detections, 2.0, { r: 1, g: 0, b: 0, a: 1 });
}, 1000);
```

### 2. 视频分析
分析视频内容并标注感兴趣区域：
```javascript
player.play('analysis-video.mp4');

// 标注特定区域
const roi = [
    { x: 200, y: 150, width: 400, height: 300 }
];
player.updateRectangles(roi, 3.0, { r: 0, g: 1, b: 0, a: 0.5 });
```

### 3. 视频标注工具
创建用于机器学习数据标注的工具：
```javascript
// 用户点击视频创建标注框
canvas.on('mousedown', (e) => {
    const rect = createRectFromMouse(e);
    annotations.push(rect);
    player.updateRectangles(annotations, 1.5, { r: 0, g: 0.5, b: 1, a: 1 });
});
```

## 与 macOS 版本的对应关系

| macOS (MacVLCBridge)           | Windows (WinVLCBridge)           | 说明         |
|--------------------------------|----------------------------------|--------------|
| `mv_create_player_for_view`    | `wv_create_player_for_view`      | 创建播放器   |
| `mv_player_play`               | `wv_player_play`                 | 播放视频     |
| `mv_player_pause`              | `wv_player_pause`                | 暂停         |
| `mv_player_resume`             | `wv_player_resume`               | 恢复         |
| `mv_player_stop`               | `wv_player_stop`                 | 停止         |
| `mv_player_release`            | `wv_player_release`              | 释放资源     |
| `mv_player_update_rectangles`  | `wv_player_update_rectangles`    | 更新矩形     |
| `mv_player_clear_rectangles`   | `wv_player_clear_rectangles`     | 清除矩形     |

> **注意：** 函数签名完全相同，只是前缀不同（`mv_` vs `wv_`），便于跨平台代码复用。

## 调试

### 查看日志

所有日志通过 `OutputDebugString` 输出，可使用以下工具查看：

1. **DebugView**（推荐）
   - 下载：https://docs.microsoft.com/sysinternals/downloads/debugview
   - 运行 DebugView，日志会实时显示

2. **Visual Studio 输出窗口**
   - 调试时查看 "输出" 面板

日志格式：`[WinVLCBridge] <消息>`

### 常见问题

**Q: DLL 加载失败**
- 确保 `libvlc.dll` 和 `libvlccore.dll` 在同一目录
- 确保 `plugins/` 目录存在
- 检查是否安装了 Visual C++ 运行库

**Q: 视频不显示**
- 检查窗口句柄（HWND）是否正确
- 确认视频文件路径或流地址有效
- 查看 DebugView 中的错误日志

**Q: 矩形不显示**
- 确认矩形坐标在视频窗口范围内
- 检查颜色的 alpha 值是否大于 0
- 确保覆盖层窗口未被遮挡

**Q: 性能问题**
- 减少矩形更新频率
- 降低矩形数量
- 使用 Release 编译版本

## 性能建议

1. **矩形更新频率**：建议不超过 30 FPS（每 33ms 一次）
2. **矩形数量**：建议单次绘制不超过 50 个矩形
3. **编译优化**：生产环境使用 Release 编译
4. **网络流缓存**：根据网络状况调整缓存参数

## 许可证

本项目使用与 VLC 兼容的开源许可证。使用时请遵守 libVLC 的 LGPL 许可。

## 技术支持

- 查看 [BUILD.md](BUILD.md) 了解编译问题
- 查看 [example_usage.js](example_usage.js) 了解使用示例
- 检查 DebugView 日志进行故障排查

## 版本历史

- **v1.0.0** (2025-10-07)
  - 初始版本
  - 实现核心播放功能
  - 实现矩形覆盖层
  - 完整的日志系统

## 待办事项

- [ ] 添加音量控制
- [ ] 添加播放进度控制
- [ ] 支持截图功能
- [ ] 支持录制功能
- [ ] 添加更多覆盖层图形（圆形、多边形等）

## 贡献

欢迎提交 Issue 和 Pull Request！

---

**关联项目：** 
- macOS 版本：[MacVLCBridge](../MacVLCBridge/)
- Electron 演示：[electron-vlc-demo](../)

