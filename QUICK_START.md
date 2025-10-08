# 快速开始指南 - WinVLCBridge

> ⚡ 5 分钟快速上手 Windows VLC 桥接库

## 📋 前置检查清单

- [ ] 安装 Visual Studio 2019+（包含 C++ 桌面开发）
- [ ] 准备 VLC SDK（vlc-3.0.21 目录）→ 参考下面的步骤 0
- [ ] 安装 Node.js（如果用于 Electron）

## 🚀 四步开始

### 步骤 0：获取 VLC SDK ⚠️

⚠️ **重要**：从 VLC 官网下载的普通安装包**不包含** SDK！

**一键设置（推荐）：**

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
- 从 NuGet 下载 VLC SDK
- 提取头文件（vlc.h）
- 组织目录结构
- 准备好编译所需的所有文件

✅ 成功后会生成：`vlc-3.0.21/sdk/include/vlc/vlc.h`

> 💡 **详细说明**：如果自动脚本失败，请查看 [VLC_SDK_GUIDE.md](VLC_SDK_GUIDE.md)

---

### 步骤 1：编译 DLL

**最简单方式：双击运行**
```
WinVLCBridge/build.bat
```

**或使用命令行：**
```cmd
cd WinVLCBridge
build.bat release
```

✅ 成功后会生成：`build/bin/Release/WinVLCBridge.dll`

---

### 步骤 2：准备文件

将以下文件复制到你的项目：

```
你的项目/
└── native/
    ├── WinVLCBridge.dll
    ├── libvlc.dll
    ├── libvlccore.dll
    └── plugins/              # 整个目录
```

这些文件位于：`WinVLCBridge/build/bin/Release/`

---

### 步骤 3：编写代码

**安装依赖：**
```bash
npm install ffi-napi ref-napi
```

**最小示例：**
```javascript
const ffi = require('ffi-napi');
const ref = require('ref-napi');

// 加载 DLL
const VLC = ffi.Library('native/WinVLCBridge.dll', {
    'wv_create_player_for_view': ['pointer', ['pointer', 'float', 'float', 'float', 'float']],
    'wv_player_play': ['void', ['pointer', 'string']],
    'wv_player_release': ['void', ['pointer']]
});

// 获取窗口句柄（Electron）
const { BrowserWindow } = require('electron');
const win = new BrowserWindow({ width: 1280, height: 720 });
const hwnd = win.getNativeWindowHandle();

// 创建播放器（x=0, y=0, 宽=800, 高=600）
const hwndBuffer = ref.alloc('pointer', hwnd);
const player = VLC.wv_create_player_for_view(hwndBuffer, 0, 0, 800, 600);

// 播放视频
VLC.wv_player_play(player, 'C:/Videos/sample.mp4');

// 清理
win.on('closed', () => {
    VLC.wv_player_release(player);
});
```

---

## 🎯 常用功能速查

### 播放本地视频
```javascript
VLC.wv_player_play(player, 'C:/Videos/sample.mp4');
```

### 播放网络流
```javascript
VLC.wv_player_play(player, 'rtsp://example.com/stream');
```

### 暂停/恢复
```javascript
VLC.wv_player_pause(player);   // 暂停
VLC.wv_player_resume(player);  // 恢复
```

### 添加矩形框
```javascript
const VLC_RECT = ffi.Library('native/WinVLCBridge.dll', {
    'wv_player_update_rectangles': ['void', ['pointer', 'pointer', 'int', 'float', 'float', 'float', 'float', 'float']]
});

// 定义矩形 [x, y, width, height, ...]
const rects = new Float32Array([
    100, 100, 200, 150,  // 第1个矩形
    400, 200, 150, 100   // 第2个矩形
]);

const buffer = Buffer.from(rects.buffer);

// 更新矩形（2个矩形，线宽3，绿色，不透明）
VLC_RECT.wv_player_update_rectangles(
    player, buffer, 2,    // 句柄、数据、数量
    3.0,                  // 线宽
    0.0, 1.0, 0.0, 1.0   // R, G, B, A
);
```

### 清除矩形
```javascript
const VLC_CLEAR = ffi.Library('native/WinVLCBridge.dll', {
    'wv_player_clear_rectangles': ['void', ['pointer']]
});

VLC_CLEAR.wv_player_clear_rectangles(player);
```

---

## 🎨 预设颜色

```javascript
// 红色警告框
{ r: 1.0, g: 0.0, b: 0.0, a: 1.0 }

// 绿色检测框
{ r: 0.0, g: 1.0, b: 0.0, a: 1.0 }

// 蓝色信息框
{ r: 0.0, g: 0.5, b: 1.0, a: 1.0 }

// 半透明黄色
{ r: 1.0, g: 1.0, b: 0.0, a: 0.5 }
```

---

## ❓ 问题排查

### DLL 加载失败
```javascript
// 使用绝对路径
const path = require('path');
const dllPath = path.join(__dirname, 'native', 'WinVLCBridge.dll');
const VLC = ffi.Library(dllPath, { ... });
```

### 视频不显示
1. 检查窗口句柄是否正确：
```javascript
console.log('HWND:', hwnd);  // 应该是一个 Buffer
```

2. 确认视频路径使用正斜杠：
```javascript
// ✅ 正确
'C:/Videos/sample.mp4'

// ❌ 错误（需要转义）
'C:\\Videos\\sample.mp4'
```

3. 查看日志：
- 下载 [DebugView](https://docs.microsoft.com/sysinternals/downloads/debugview)
- 运行 DebugView
- 启动你的应用，查看 `[WinVLCBridge]` 日志

### 矩形不显示
1. 确认坐标在视频窗口内：
```javascript
// 如果视频窗口是 800x600，矩形坐标应在此范围内
// x: 0-800, y: 0-600
```

2. 确认 alpha 不为 0：
```javascript
// ❌ 完全透明（看不见）
{ r: 1, g: 0, b: 0, a: 0.0 }

// ✅ 不透明
{ r: 1, g: 0, b: 0, a: 1.0 }
```

---

## 📚 更多资源

- 📖 [完整文档](README.md) - 详细的 API 说明
- 🔨 [编译指南](BUILD.md) - 深入的编译配置
- 💻 [使用示例](example_usage.js) - 完整的代码示例
- 🔄 [平台差异](DIFFERENCES.md) - macOS vs Windows

---

## 💡 实用代码片段

### 封装成类（推荐）
```javascript
class VideoPlayer {
    constructor(hwnd, x, y, width, height) {
        const VLC = ffi.Library('native/WinVLCBridge.dll', { ... });
        const hwndBuffer = ref.alloc('pointer', hwnd);
        this.player = VLC.wv_create_player_for_view(hwndBuffer, x, y, width, height);
        this.VLC = VLC;
    }
    
    play(source) {
        this.VLC.wv_player_play(this.player, source);
    }
    
    cleanup() {
        this.VLC.wv_player_release(this.player);
    }
}

// 使用
const player = new VideoPlayer(hwnd, 0, 0, 800, 600);
player.play('video.mp4');
```

### 错误处理
```javascript
try {
    const player = VLC.wv_create_player_for_view(hwndBuffer, 0, 0, 800, 600);
    if (player.isNull()) {
        throw new Error('Failed to create player');
    }
} catch (error) {
    console.error('VLC Error:', error);
}
```

### 自动清理
```javascript
process.on('exit', () => {
    if (player) {
        VLC.wv_player_release(player);
    }
});
```

---

## ⚡ 性能提示

1. **矩形更新**：不要超过 30 FPS
```javascript
// ✅ 限制更新频率
setInterval(() => {
    updateRectangles(detections);
}, 33);  // 约 30 FPS
```

2. **矩形数量**：单次不超过 50 个
```javascript
if (rectangles.length > 50) {
    rectangles = rectangles.slice(0, 50);
}
```

3. **使用 Release 版本**：
```bash
build.bat release  # 而不是 debug
```

---

## 🆘 获取帮助

遇到问题？

1. 查看 [README.md](README.md) 的常见问题部分
2. 检查 DebugView 日志（搜索 `[WinVLCBridge]`）
3. 确认 VLC SDK 版本兼容（推荐 3.0.x）

---

## ✅ 检查清单

完成了吗？

- [ ] DLL 编译成功
- [ ] 所有依赖文件已复制
- [ ] 视频能正常播放
- [ ] 矩形能正常显示
- [ ] 没有内存泄漏（调用了 release）

🎉 恭喜！你已经掌握了 WinVLCBridge 的基本使用！

---

**提示：** 更完整的示例请查看 `example_usage.js`

