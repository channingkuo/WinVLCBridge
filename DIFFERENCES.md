# MacVLCBridge vs WinVLCBridge - 平台差异说明

本文档说明 macOS 和 Windows 版本的 VLC 桥接库在实现上的差异。

## API 对比

| 功能 | macOS | Windows | 兼容性 |
|------|-------|---------|--------|
| 函数前缀 | `mv_` | `wv_` | ✅ 参数完全相同 |
| 窗口参数 | `NSView*` | `HWND` | ⚠️ 平台特定 |
| 返回类型 | `void*` | `void*` | ✅ 完全相同 |

## 实现差异

### 1. 视频渲染窗口

#### macOS (MacVLCBridge)
```objc
VLCVideoView *hostView = [[VLCVideoView alloc] initWithFrame:...];
[parentView addSubview:hostView];
player.drawable = hostView;
```

**特点：**
- 使用 VLCKit 提供的 `VLCVideoView`
- 直接作为 NSView 的子视图添加
- Cocoa 框架自动处理视图层级

#### Windows (WinVLCBridge)
```cpp
HWND videoWindow = CreateWindowExW(0, L"STATIC", L"Video Window", 
                                    WS_CHILD | WS_VISIBLE, ...);
libvlc_media_player_set_hwnd(mediaPlayer, videoWindow);
```

**特点：**
- 使用原生 Win32 API 创建子窗口
- 直接使用 libVLC C API（无高级封装）
- 需要手动管理窗口生命周期

---

### 2. 矩形覆盖层

#### macOS (MacVLCBridge)
```objc
@interface MVRectangleOverlayView : NSView
@end

- (void)drawRect:(NSRect)dirtyRect {
    [_strokeColor setStroke];
    NSBezierPath *path = [NSBezierPath bezierPathWithRect:rect];
    [path stroke];
}
```

**技术栈：**
- 自定义 NSView 子类
- 使用 Cocoa 的 `drawRect:` 绘图机制
- NSBezierPath 绘制矩形
- 通过 `layer.backgroundColor = clearColor` 实现透明

#### Windows (WinVLCBridge)
```cpp
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, ...) {
    case WM_PAINT:
        Graphics graphics(hdc);
        Pen pen(color, lineWidth);
        graphics.DrawRectangle(&pen, x, y, width, height);
}

// 创建分层窗口
CreateWindowExW(WS_EX_LAYERED | WS_EX_TRANSPARENT, ...);
SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
```

**技术栈：**
- 自定义窗口类（Window Class）
- 使用 GDI+ 绘图
- WM_PAINT 消息处理
- 分层窗口（Layered Window）+ 颜色键（Color Key）实现透明
- WS_EX_TRANSPARENT 实现鼠标穿透

---

### 3. 透明度实现

#### macOS
```objc
self.wantsLayer = YES;
self.layer.backgroundColor = [[NSColor clearColor] CGColor];
self.layer.opaque = NO;

// 鼠标事件穿透
- (NSView *)hitTest:(NSPoint)point {
    return nil;  // 不处理鼠标事件
}
```

**机制：** Core Animation 的 Layer-backed view

#### Windows
```cpp
// 创建分层窗口
WS_EX_LAYERED | WS_EX_TRANSPARENT

// 设置颜色键透明
SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);

// 鼠标穿透
case WM_NCHITTEST:
    return HTTRANSPARENT;
```

**机制：** Windows 分层窗口 + 颜色键

---

### 4. 线程模型

#### macOS
```objc
static void runOnMain(void (^block)(void)) {
    if ([NSThread isMainThread]) {
        block();
    } else {
        dispatch_sync(dispatch_get_main_queue(), block);
    }
}
```

**特点：**
- 使用 Grand Central Dispatch (GCD)
- block 语法
- 自动同步到主线程

#### Windows
```cpp
// 当前版本：直接调用（假设在主线程）
// 如需线程安全，可使用：
PostMessage(hwnd, WM_USER_CUSTOM, ...);
// 或
SendMessage(hwnd, WM_USER_CUSTOM, ...);
```

**特点：**
- 使用 Windows 消息队列
- 需要手动处理线程同步
- 当前实现假设在主线程调用

---

### 5. VLC 库接口

#### macOS
```objc
#import <VLCKit/VLCKit.h>

VLCMediaPlayer *player = [[VLCMediaPlayer alloc] initWithOptions:...];
VLCMedia *media = [VLCMedia mediaWithURL:url];
[player setMedia:media];
[player play];
```

**特点：**
- 使用 Objective-C 封装的 VLCKit
- 面向对象 API
- 自动内存管理（ARC）

#### Windows
```cpp
#include <vlc/vlc.h>

libvlc_instance_t *instance = libvlc_new(...);
libvlc_media_player_t *player = libvlc_media_player_new(instance);
libvlc_media_t *media = libvlc_media_new_path(instance, path);
libvlc_media_player_set_media(player, media);
libvlc_media_player_play(player);
```

**特点：**
- 使用 C API
- 手动内存管理（需要 release）
- 更底层的控制

---

### 6. 日志系统

#### macOS
```objc
NSLog(@"[MacVLCBridge] %@", message);
```

**特点：**
- 自动输出到系统日志
- 可通过 Console.app 查看
- 格式化字符串使用 `%@`

#### Windows
```cpp
std::cout << "[WinVLCBridge] " << message << std::endl;
OutputDebugStringA("[WinVLCBridge] ...");
```

**特点：**
- 输出到控制台和调试输出
- 使用 DebugView 查看
- 格式化使用 `vsnprintf`

---

## 性能对比

| 方面 | macOS | Windows |
|------|-------|---------|
| 绘图性能 | Core Animation（GPU加速） | GDI+（CPU绘制，可优化为 Direct2D） |
| 内存管理 | ARC 自动管理 | 手动管理 |
| 窗口层级 | NSView hierarchy（高效） | HWND hierarchy（标准） |
| 透明度 | Layer-backed view（高效） | Layered Window（开销稍大） |

---

## 代码复用性

### 高复用性部分
✅ **API 设计** - 完全相同的函数签名  
✅ **功能逻辑** - 播放控制、矩形管理逻辑相同  
✅ **使用方式** - JavaScript/Electron 调用代码几乎相同  

### 平台特定部分
⚠️ **窗口系统** - NSView vs HWND  
⚠️ **绘图系统** - Cocoa Drawing vs GDI+  
⚠️ **事件处理** - Cocoa Events vs Windows Messages  

---

## 未来优化方向

### macOS 优化
- [ ] 使用 Metal 加速绘制（替代 Core Animation）
- [ ] 支持 SwiftUI 集成

### Windows 优化
- [ ] 使用 Direct2D 替代 GDI+（更高性能）
- [ ] 使用 Windows.UI.Composition 实现更好的透明效果
- [ ] 添加线程安全的消息队列

### 跨平台优化
- [ ] 统一日志接口
- [ ] 共享核心逻辑代码
- [ ] 创建跨平台的 Node.js 包装层

---

## 使用建议

### 跨平台项目
```javascript
// 在 Electron 中自动选择平台
const bridge = process.platform === 'darwin' 
    ? require('./MacVLCBridge') 
    : require('./WinVLCBridge');

// API 相同，无需修改业务逻辑
const player = bridge.createPlayer(handle, 0, 0, 800, 600);
player.play(videoPath);
```

### 性能优化
- **macOS**: 尽量减少频繁的视图更新
- **Windows**: 考虑降低覆盖层更新频率（GDI+ 性能较低）

### 调试
- **macOS**: 使用 Console.app 或 Xcode
- **Windows**: 使用 DebugView 或 Visual Studio

---

## 总结

尽管底层实现差异很大，但通过精心设计的 API 接口，两个平台的使用体验保持一致。macOS 版本利用了 Cocoa 的高级特性，而 Windows 版本使用更底层的 Win32 API，但都实现了相同的功能。

选择哪个平台主要取决于你的目标用户，如果需要跨平台支持，两个版本可以无缝切换。

