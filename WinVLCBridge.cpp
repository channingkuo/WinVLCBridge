//
//  WinVLCBridge.cpp
//  WinVLCBridge
//
//  Created by Channing Kuo on 2025/10/7.
//

#include "WinVLCBridge.h"
#include <windows.h>

// 在包含 VLC 头文件之前，定义缺失的类型
// 这是 VLC SDK 3.0.20 的一个已知问题的解决方案
#ifndef _SSIZE_T_DEFINED
#define _SSIZE_T_DEFINED
#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef int ssize_t;
#endif
#endif

#include <vlc/vlc.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// ==================== 日志辅助函数 ====================

static void LogMessage(const char* format, ...) {
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // 使用 stderr 输出，因为它是无缓冲的，能立即显示在命令行中
    fprintf(stderr, "[WinVLCBridge] %s\n", buffer);
    fflush(stderr);  // 强制刷新输出缓冲区
    
    // 转换为宽字符并输出到 Windows 调试器（避免 DebugView 中文乱码）
    wchar_t wideBuffer[1024];
    wchar_t wideMessage[1100];
    
    // 将 UTF-8 字符串转换为宽字符（假设输入是 UTF-8 编码）
    MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wideBuffer, 1024);
    
    // 添加前缀
    swprintf(wideMessage, 1100, L"[WinVLCBridge] %s\n", wideBuffer);
    
    // 输出到 Windows 调试器
    OutputDebugStringW(wideMessage);
}

// ==================== 矩形覆盖层窗口 ====================

// 矩形数据结构 (重命名以避免与 GDI+ Rectangle 冲突)
struct OverlayRectangle {
    float x, y, width, height;
};

// 覆盖层窗口数据
struct OverlayWindowData {
    HWND hwnd;
    std::vector<OverlayRectangle> rectangles;
    float lineWidth;
    float red, green, blue, alpha;
    ULONG_PTR gdiplusToken;
    
    // 延迟信息显示
    std::wstring statsText;      // 统计信息文本
    bool showStats;              // 是否显示统计信息
};

// 覆盖层窗口类名
const wchar_t* OVERLAY_CLASS_NAME = L"WinVLCBridgeOverlayWindow";

// 覆盖层窗口过程
LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayWindowData* data = reinterpret_cast<OverlayWindowData*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (data) {
                Graphics graphics(hdc);
                graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
                
                // 绘制矩形
                if (!data->rectangles.empty()) {
                    // 创建画笔
                    Color color(
                        static_cast<BYTE>(data->alpha * 255),
                        static_cast<BYTE>(data->red * 255),
                        static_cast<BYTE>(data->green * 255),
                        static_cast<BYTE>(data->blue * 255)
                    );
                    Pen pen(color, data->lineWidth);
                    
                    // 绘制所有矩形
                    for (const auto& rect : data->rectangles) {
                        graphics.DrawRectangle(&pen, rect.x, rect.y, rect.width, rect.height);
                    }
                }
                
                // 绘制统计信息文本（右上角）
                if (data->showStats && !data->statsText.empty()) {
                    // 创建字体
                    FontFamily fontFamily(L"Consolas");
                    Font font(&fontFamily, 14, FontStyleBold, UnitPixel);
                    
                    // 半透明黑色背景
                    SolidBrush bgBrush(Color(180, 0, 0, 0));
                    // 亮绿色文本
                    SolidBrush textBrush(Color(255, 0, 255, 0));
                    
                    // 测量文本大小
                    RectF layoutRect(0, 0, 500, 200);
                    RectF boundingBox;
                    graphics.MeasureString(data->statsText.c_str(), -1, &font, layoutRect, &boundingBox);
                    
                    // 获取窗口尺寸
                    RECT clientRect;
                    GetClientRect(hwnd, &clientRect);
                    
                    // 计算右上角位置（距离右边和上边各10像素）
                    float textX = clientRect.right - boundingBox.Width - 15;
                    float textY = 10;
                    
                    // 绘制背景矩形（添加一些内边距）
                    RectF bgRect(textX - 5, textY - 3, boundingBox.Width + 10, boundingBox.Height + 6);
                    graphics.FillRectangle(&bgBrush, bgRect);
                    
                    // 绘制文本
                    PointF textPos(textX, textY);
                    graphics.DrawString(data->statsText.c_str(), -1, &font, textPos, &textBrush);
                }
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_ERASEBKGND:
            // 返回 1 表示已处理，防止闪烁
            return 1;
            
        case WM_NCHITTEST:
            // 让鼠标事件穿透到下层窗口
            return HTTRANSPARENT;
            
        case WM_DESTROY:
            if (data) {
                // 清理 GDI+
                if (data->gdiplusToken) {
                    GdiplusShutdown(data->gdiplusToken);
                }
                delete data;
            }
            return 0;
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 注册覆盖层窗口类
static bool RegisterOverlayWindowClass() {
    static bool registered = false;
    if (registered) return true;
    
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = OVERLAY_CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);  // 透明背景
    
    if (!RegisterClassExW(&wc)) {
        LogMessage("错误：无法注册覆盖层窗口类，错误码: %d", GetLastError());
        return false;
    }
    
    registered = true;
    return true;
}

// 创建覆盖层窗口
static HWND CreateOverlayWindow(HWND parent, int x, int y, int width, int height) {
    if (!RegisterOverlayWindowClass()) {
        return NULL;
    }
    
    // 创建 GDI+ 令牌和数据
    OverlayWindowData* data = new OverlayWindowData();
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&data->gdiplusToken, &gdiplusStartupInput, NULL);
    
    data->lineWidth = 2.0f;
    data->red = 0.0f;
    data->green = 1.0f;
    data->blue = 0.0f;
    data->alpha = 1.0f;
    data->showStats = false;  // 默认不显示统计信息
    
    // 创建透明的、可层叠的子窗口
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT,  // 分层窗口和透明事件
        OVERLAY_CLASS_NAME,
        L"Overlay",
        WS_CHILD | WS_VISIBLE,
        x, y, width, height,
        parent,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!hwnd) {
        LogMessage("错误：无法创建覆盖层窗口，错误码: %d", GetLastError());
        GdiplusShutdown(data->gdiplusToken);
        delete data;
        return NULL;
    }
    
    // 设置窗口透明度（关键：背景完全透明，只显示绘制的内容）
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // 关联数据
    data->hwnd = hwnd;
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
    
    LogMessage("覆盖层窗口创建成功: HWND=0x%p, 位置=(%d,%d), 大小=%dx%d", hwnd, x, y, width, height);
    
    return hwnd;
}

// ==================== 播放器包装结构 ====================

struct WVPlayerWrapper {
    libvlc_instance_t* vlcInstance;
    libvlc_media_player_t* mediaPlayer;
    libvlc_media_t* currentMedia;  // 当前媒体对象
    HWND containerWindow;  // 黑色背景容器窗口（固定大小）
    HWND videoWindow;      // VLC 视频窗口（可调整大小，在容器内居中）
    HWND overlayWindow;    // 覆盖层窗口
    OverlayWindowData* overlayData;
    int videoWidth;        // 容器窗口宽度
    int videoHeight;       // 容器窗口高度
    int videoX;            // 容器窗口 X 坐标（相对于父窗口）
    int videoY;            // 容器窗口 Y 坐标（相对于父窗口）
    libvlc_event_manager_t* eventManager;  // 事件管理器
    COLORREF backgroundColor;  // 背景颜色（RGB）
    HBRUSH backgroundBrush;    // 背景画刷
};

// ==================== 工具函数 ====================

// 视频窗口过程（使用自定义背景色）
static LRESULT CALLBACK VideoWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // 获取 wrapper 数据
    WVPlayerWrapper* wrapper = (WVPlayerWrapper*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    
    switch (msg) {
        case WM_ERASEBKGND: {
            // 用背景色填充
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hwnd, &rect);
            
            // 如果有自定义背景画刷，使用它；否则使用黑色
            HBRUSH brush = wrapper && wrapper->backgroundBrush ? 
                          wrapper->backgroundBrush : 
                          (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &rect, brush);
            return 1; // 表示已处理
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 如果有自定义背景画刷，使用它；否则使用黑色
            HBRUSH brush = wrapper && wrapper->backgroundBrush ? 
                          wrapper->backgroundBrush : 
                          (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &ps.rcPaint, brush);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// 注册视频窗口类
static bool RegisterVideoWindowClass() {
    static bool registered = false;
    if (registered) return true;
    
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = VideoWindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"VLCVideoWindow";
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    
    if (RegisterClassExW(&wc)) {
        registered = true;
        return true;
    }
    
    DWORD error = GetLastError();
    if (error == ERROR_CLASS_ALREADY_EXISTS) {
        registered = true;
        return true;
    }
    
    return false;
}

// 计算并应用视频居中缩放（参考 macOS centerVideoLayerInHostView 实现）
static bool ApplyVideoCenterScaling(WVPlayerWrapper* wrapper) {
    if (!wrapper || !wrapper->mediaPlayer || !wrapper->videoWindow) {
        LogMessage("错误：wrapper 或 mediaPlayer 或 videoWindow 为空");
        return false;
    }
    
    // 获取视频实际尺寸
    unsigned int videoWidth = 0, videoHeight = 0;
    if (libvlc_video_get_size(wrapper->mediaPlayer, 0, &videoWidth, &videoHeight) != 0 || 
        videoWidth == 0 || videoHeight == 0) {
        LogMessage("无法获取视频尺寸或尺寸无效，使用默认设置");
        return false;
    }
    
    // 获取容器窗口尺寸（原始创建时的尺寸）
    float hostWidth = (float)wrapper->videoWidth;
    float hostHeight = (float)wrapper->videoHeight;
    
    LogMessage("视频原始尺寸: %ux%u", videoWidth, videoHeight);
    LogMessage("容器窗口尺寸: %.0fx%.0f", hostWidth, hostHeight);
    
    // 计算缩放比例（aspect fit - 类似 macOS 实现）
    // scaleW = hostWidth / videoWidth
    // scaleH = hostHeight / videoHeight
    // scale = MIN(scaleW, scaleH)  取较小的缩放比例，确保完整显示
    float scaleW = hostWidth / (float)videoWidth;
    float scaleH = hostHeight / (float)videoHeight;
    float scale = (scaleW < scaleH) ? scaleW : scaleH;  // MIN
    
    // 计算缩放后的视频尺寸
    int scaledWidth = (int)((float)videoWidth * scale);
    int scaledHeight = (int)((float)videoHeight * scale);
    
    // 计算居中位置（相对于容器窗口，0-based）
    int centerX = (int)((hostWidth - (float)scaledWidth) / 2.0f);
    int centerY = (int)((hostHeight - (float)scaledHeight) / 2.0f);
    
    LogMessage("缩放计算: scaleW=%.3f, scaleH=%.3f, 最终scale=%.3f", scaleW, scaleH, scale);
    LogMessage("缩放后尺寸: %dx%d", scaledWidth, scaledHeight);
    LogMessage("居中位置（相对于容器）: (%d, %d)", centerX, centerY);
    
    // 判断 letterbox 或 pillarbox
    float videoAspect = (float)videoWidth / (float)videoHeight;
    float windowAspect = hostWidth / hostHeight;
    
    if (videoAspect > windowAspect) {
        LogMessage("视频更宽（宽高比 %.2f > %.2f），将产生上下黑边（letterbox）", videoAspect, windowAspect);
    } else if (videoAspect < windowAspect) {
        LogMessage("视频更高（宽高比 %.2f < %.2f），将产生左右黑边（pillarbox）", videoAspect, windowAspect);
    } else {
        LogMessage("视频宽高比完美匹配窗口（%.2f）", videoAspect);
    }
    
    // 调整视频窗口的位置和大小以实现居中
    // 这样视频会完全填充这个较小的窗口，而周围会露出父窗口的黑色背景
    BOOL moveResult = MoveWindow(
        wrapper->videoWindow,
        centerX, centerY,
        scaledWidth, scaledHeight,
        TRUE  // 重绘
    );
    
    if (!moveResult) {
        LogMessage("错误：无法调整视频窗口位置和大小");
        return false;
    }
    
    // 设置 VLC 为填充模式（scale=0 表示填满窗口）
    // 由于我们已经调整了窗口大小为正确的宽高比，VLC 会填满整个窗口
    libvlc_video_set_scale(wrapper->mediaPlayer, 0);
    
    // 不设置固定宽高比，让 VLC 使用视频原始宽高比
    libvlc_video_set_aspect_ratio(wrapper->mediaPlayer, NULL);
    
    // 确保窗口可见并在顶层
    ShowWindow(wrapper->videoWindow, SW_SHOW);
    SetWindowPos(wrapper->videoWindow, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    LogMessage("视频窗口已调整并居中 - 新位置: (%d, %d), 新尺寸: %dx%d", 
               centerX, centerY, scaledWidth, scaledHeight);
    
    return true;
}

// VLC 事件回调：当视频开始播放时调整缩放
static void OnMediaPlayerPlaying(const libvlc_event_t* event, void* userData) {
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(userData);
    if (!wrapper || !wrapper->mediaPlayer) return;
    
    LogMessage("视频开始播放事件触发，正在计算居中缩放参数...");
    
    // 等待视频输出准备好
    Sleep(100);
    
    // 应用视频居中缩放
    ApplyVideoCenterScaling(wrapper);
}

// 判断字符串是否为网络流地址
static bool IsNetworkStream(const std::string& source) {
    std::string lower = source;
    for (auto& c : lower) c = tolower(c);
    
    return lower.find("http://") == 0 ||
           lower.find("https://") == 0 ||
           lower.find("rtsp://") == 0 ||
           lower.find("rtmp://") == 0 ||
           lower.find("rtmps://") == 0 ||
           lower.find("rtp://") == 0;
}

// 转换路径为标准格式
static std::string NormalizePath(const std::string& path) {
    if (IsNetworkStream(path)) {
        return path;
    }
    
    // 对于本地文件，确保使用正斜杠
    std::string normalized = path;
    for (auto& c : normalized) {
        if (c == '\\') c = '/';
    }
    return normalized;
}

// ==================== 公共 API 实现 ====================

void* wv_create_player_for_view(void* hwnd_ptr, float x, float y, float width, float height) {
    if (!hwnd_ptr) {
        LogMessage("错误：父窗口句柄为空");
        return NULL;
    }
    
    HWND parentWindow = static_cast<HWND>(hwnd_ptr);
    
    // 创建播放器包装对象
    WVPlayerWrapper* wrapper = new WVPlayerWrapper();
    memset(wrapper, 0, sizeof(WVPlayerWrapper));
    
    // 保存视频窗口位置和尺寸
    wrapper->videoX = static_cast<int>(x);
    wrapper->videoY = static_cast<int>(y);
    wrapper->videoWidth = static_cast<int>(width);
    wrapper->videoHeight = static_cast<int>(height);
    
    // 设置背景颜色（默认黑色，可以修改为其他颜色）
    wrapper->backgroundColor = RGB(0, 0, 0);  // 黑色
    // 如果需要其他颜色，可以改为：
    // wrapper->backgroundColor = RGB(255, 0, 0);  // 红色
    // wrapper->backgroundColor = RGB(0, 255, 0);  // 绿色
    // wrapper->backgroundColor = RGB(30, 30, 30); // 深灰色
    
    // 创建背景画刷
    wrapper->backgroundBrush = CreateSolidBrush(wrapper->backgroundColor);
    LogMessage("背景色设置为: RGB(%d, %d, %d)", 
               GetRValue(wrapper->backgroundColor),
               GetGValue(wrapper->backgroundColor),
               GetBValue(wrapper->backgroundColor));
    
    // 获取 DLL 所在目录，用于定位 VLC 插件
    char dllPath[MAX_PATH];
    HMODULE hModule = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&wv_create_player_for_view, &hModule);
    GetModuleFileNameA(hModule, dllPath, MAX_PATH);
    
    // 获取 DLL 所在目录
    std::string dllDir = dllPath;
    size_t lastSlash = dllDir.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        dllDir = dllDir.substr(0, lastSlash);
    }
    
    // 构建插件路径
    std::string pluginPath = "--plugin-path=" + dllDir + "\\plugins";
    
    LogMessage("DLL 目录: %s", dllDir.c_str());
    LogMessage("插件路径: %s", pluginPath.c_str());
    
    // 初始化 libVLC
    const char* vlc_args[] = {
        pluginPath.c_str(),
        "--file-caching=50",
        "--network-caching=100",
        "--avcodec-fast",
        "--no-sub-autodetect-file",
        "--no-video-title-show",
        "--no-snapshot-preview",
        "--no-osd",
        "--no-video-title",           // 不显示视频标题
        "--no-mouse-events",          // 禁用鼠标事件
        "--no-keyboard-events"        // 禁用键盘事件
    };
    
    wrapper->vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
    if (!wrapper->vlcInstance) {
        LogMessage("错误：无法初始化 libVLC");
        delete wrapper;
        return NULL;
    }
    
    // 创建媒体播放器
    wrapper->mediaPlayer = libvlc_media_player_new(wrapper->vlcInstance);
    if (!wrapper->mediaPlayer) {
        LogMessage("错误：无法创建媒体播放器");
        libvlc_release(wrapper->vlcInstance);
        delete wrapper;
        return NULL;
    }
    
    // 注册自定义视频窗口类（带黑色背景）
    if (!RegisterVideoWindowClass()) {
        LogMessage("错误：无法注册视频窗口类");
        libvlc_media_player_release(wrapper->mediaPlayer);
        libvlc_release(wrapper->vlcInstance);
        delete wrapper;
        return NULL;
    }
    
    // 第一步：创建黑色背景容器窗口（固定大小）
    wrapper->containerWindow = CreateWindowExW(
        WS_EX_NOACTIVATE,
        L"VLCVideoWindow",  // 使用自定义窗口类（黑色背景）
        L"Video Container",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        static_cast<int>(x), static_cast<int>(y),
        static_cast<int>(width), static_cast<int>(height),
        parentWindow,  // 父窗口是 Electron 窗口
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!wrapper->containerWindow) {
        LogMessage("错误：无法创建容器窗口，错误码: %d", GetLastError());
        libvlc_media_player_release(wrapper->mediaPlayer);
        libvlc_release(wrapper->vlcInstance);
        delete wrapper;
        return NULL;
    }
    
    LogMessage("容器窗口创建成功（黑色背景）: HWND=0x%p, 位置=(%d,%d), 大小=%dx%d", 
               wrapper->containerWindow, static_cast<int>(x), static_cast<int>(y),
               static_cast<int>(width), static_cast<int>(height));
    
    // 将 wrapper 指针保存到容器窗口
    SetWindowLongPtr(wrapper->containerWindow, GWLP_USERDATA, (LONG_PTR)wrapper);
    
    // 强制重绘容器窗口以显示黑色背景
    InvalidateRect(wrapper->containerWindow, NULL, TRUE);
    UpdateWindow(wrapper->containerWindow);
    LogMessage("容器窗口已强制重绘背景");
    
    // 第二步：在容器窗口内创建视频窗口（初始时填满容器，稍后会调整）
    wrapper->videoWindow = CreateWindowExW(
        WS_EX_NOACTIVATE,
        L"STATIC",  // 使用 STATIC 窗口类用于 VLC 渲染
        L"Video Window",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0,  // 初始位置 (0, 0) 相对于容器
        static_cast<int>(width), static_cast<int>(height),
        wrapper->containerWindow,  // 父窗口是容器窗口
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!wrapper->videoWindow) {
        LogMessage("错误：无法创建视频窗口，错误码: %d", GetLastError());
        DestroyWindow(wrapper->containerWindow);
        libvlc_media_player_release(wrapper->mediaPlayer);
        libvlc_release(wrapper->vlcInstance);
        delete wrapper;
        return NULL;
    }
    
    LogMessage("视频窗口创建成功: HWND=0x%p, 初始大小=%dx%d", 
               wrapper->videoWindow, static_cast<int>(width), static_cast<int>(height));
    
    // 将 wrapper 指针保存到窗口的用户数据中，以便窗口过程访问
    SetWindowLongPtr(wrapper->videoWindow, GWLP_USERDATA, (LONG_PTR)wrapper);
    
    // 将容器窗口置于 Z-order 顶层（在 Chromium WebView 之上）
    SetWindowPos(wrapper->containerWindow, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    LogMessage("容器窗口已设置为 Z-order 顶层");
    
    // 设置 VLC 使用视频窗口进行渲染
    libvlc_media_player_set_hwnd(wrapper->mediaPlayer, wrapper->videoWindow);
    LogMessage("已设置 VLC 渲染窗口句柄");
    
    // 注册事件监听器
    wrapper->eventManager = libvlc_media_player_event_manager(wrapper->mediaPlayer);
    if (wrapper->eventManager) {
        libvlc_event_attach(wrapper->eventManager, libvlc_MediaPlayerPlaying, OnMediaPlayerPlaying, wrapper);
        LogMessage("已注册视频播放事件监听器");
    }
    
    // 创建覆盖层窗口（作为容器窗口的子窗口，覆盖整个容器）
    wrapper->overlayWindow = CreateOverlayWindow(
        wrapper->containerWindow,  // 父窗口改为容器窗口
        0, 0,  // 相对于容器窗口的位置
        static_cast<int>(width), static_cast<int>(height)
    );
    
    if (wrapper->overlayWindow) {
        wrapper->overlayData = reinterpret_cast<OverlayWindowData*>(
            GetWindowLongPtr(wrapper->overlayWindow, GWLP_USERDATA)
        );
        
        // 确保覆盖层窗口在视频窗口上方
        SetWindowPos(wrapper->overlayWindow, HWND_TOP, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        
        LogMessage("覆盖层窗口创建成功: HWND=0x%p (透明，黑色区域会穿透)", wrapper->overlayWindow);
        LogMessage("覆盖层窗口 Z-order 已设置为顶层");
    } else {
        LogMessage("警告：覆盖层窗口创建失败");
    }
    
    LogMessage("播放器创建成功 - 窗口大小: %.0fx%.0f, 位置: (%.0f, %.0f)", width, height, x, y);
    
    return wrapper;
}

void wv_player_play(void* playerHandle, const char* source) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    if (!source) {
        LogMessage("错误：视频源为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    std::string sourcePath = source;
    
    LogMessage("原始路径: %s", source);
    
    // 创建媒体对象
    libvlc_media_t* media = NULL;
    
    if (IsNetworkStream(sourcePath)) {
        // 网络流
        LogMessage("检测到网络流，使用 location 方式");
        media = libvlc_media_new_location(wrapper->vlcInstance, sourcePath.c_str());
        
        // 设置网络流选项
        if (media) {
            libvlc_media_add_option(media, ":network-caching=300");
            libvlc_media_add_option(media, ":live-caching=300");
            libvlc_media_add_option(media, ":clock-jitter=0");
            libvlc_media_add_option(media, ":clock-synchro=0");
        }
    } else {
        // 本地文件 - 检查文件是否存在
        DWORD fileAttr = GetFileAttributesA(sourcePath.c_str());
        if (fileAttr == INVALID_FILE_ATTRIBUTES) {
            LogMessage("错误：文件不存在: %s", sourcePath.c_str());
            return;
        }
        
        LogMessage("文件存在，准备创建媒体对象");
        
        // 将路径转换为 file:/// URI 格式（VLC 更可靠地支持这种格式）
        std::string normalizedPath = NormalizePath(sourcePath);
        std::string fileUri = "file:///" + normalizedPath;
        
        LogMessage("使用 URI: %s", fileUri.c_str());
        
        // 使用 location 方式创建本地文件媒体（比 new_path 更可靠）
        media = libvlc_media_new_location(wrapper->vlcInstance, fileUri.c_str());
        
        if (!media) {
            LogMessage("location 方式失败，尝试 path 方式");
            // 如果失败，尝试使用 new_path（使用原始路径）
            media = libvlc_media_new_path(wrapper->vlcInstance, sourcePath.c_str());
        }
    }
    
    if (!media) {
        LogMessage("错误：无法创建媒体对象");
        const char* vlcError = libvlc_errmsg();
        if (vlcError) {
            LogMessage("VLC 错误信息: %s", vlcError);
        }
        return;
    }
    
    LogMessage("媒体对象创建成功");
    
    // 设置媒体并播放
    // 释放旧的媒体对象（如果存在）
    if (wrapper->currentMedia) {
        libvlc_media_release(wrapper->currentMedia);
    }
    
    // 保存当前媒体对象的引用
    wrapper->currentMedia = media;
    
    libvlc_media_player_set_media(wrapper->mediaPlayer, media);
    
    int playResult = libvlc_media_player_play(wrapper->mediaPlayer);
    
    if (playResult == 0) {
        LogMessage("开始播放: %s", sourcePath.c_str());
        LogMessage("等待视频准备就绪，将在播放事件中设置缩放模式...");
        
        // 获取并记录视频窗口的实际位置和大小
        RECT rect;
        GetWindowRect(wrapper->videoWindow, &rect);
        POINT pt = {rect.left, rect.top};
        ScreenToClient(GetParent(wrapper->videoWindow), &pt);
        LogMessage("视频窗口实际位置: (%d,%d), 大小: %dx%d", 
                   pt.x, pt.y, rect.right - rect.left, rect.bottom - rect.top);
        
        // 确保视频窗口可见并在顶层（覆盖 WebView）
        ShowWindow(wrapper->videoWindow, SW_SHOW);
        UpdateWindow(wrapper->videoWindow);
        BringWindowToTop(wrapper->videoWindow);
        SetWindowPos(wrapper->videoWindow, HWND_TOP, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        
        // 确保覆盖层窗口在视频窗口之上
        if (wrapper->overlayWindow) {
            SetWindowPos(wrapper->overlayWindow, HWND_TOP, 0, 0, 0, 0, 
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        }
        
        LogMessage("视频窗口已更新并设置 Z-order 为顶层");
    } else {
        LogMessage("错误：播放失败，返回码: %d", playResult);
        const char* vlcError = libvlc_errmsg();
        if (vlcError) {
            LogMessage("VLC 错误信息: %s", vlcError);
        }
    }
}

void wv_player_pause(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    libvlc_media_player_pause(wrapper->mediaPlayer);
    
    LogMessage("播放器已暂停");
}

void wv_player_resume(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    // 检查播放器状态
    libvlc_state_t state = libvlc_media_player_get_state(wrapper->mediaPlayer);
    
    if (state == libvlc_Paused) {
        libvlc_media_player_play(wrapper->mediaPlayer);
        LogMessage("播放器已恢复播放");
    } else if (state == libvlc_Stopped) {
        LogMessage("警告：播放器处于停止状态，无法恢复播放");
    } else if (state == libvlc_Playing) {
        LogMessage("提示：播放器已在播放中");
    } else {
        LogMessage("播放器状态：%d，尝试恢复播放", state);
        libvlc_media_player_play(wrapper->mediaPlayer);
    }
}

void wv_player_stop(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    libvlc_media_player_stop(wrapper->mediaPlayer);
    
    LogMessage("播放器已停止");
}

void wv_player_release(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    // 停止播放
    libvlc_media_player_stop(wrapper->mediaPlayer);
    
    // 分离事件监听器
    if (wrapper->eventManager) {
        libvlc_event_detach(wrapper->eventManager, libvlc_MediaPlayerPlaying, OnMediaPlayerPlaying, wrapper);
        LogMessage("已分离事件监听器");
    }
    
    // 释放当前媒体对象
    if (wrapper->currentMedia) {
        libvlc_media_release(wrapper->currentMedia);
        wrapper->currentMedia = NULL;
    }
    
    // 释放媒体播放器
    if (wrapper->mediaPlayer) {
        libvlc_media_player_release(wrapper->mediaPlayer);
    }
    
    // 释放 VLC 实例
    if (wrapper->vlcInstance) {
        libvlc_release(wrapper->vlcInstance);
    }
    
    // 销毁覆盖层窗口
    if (wrapper->overlayWindow) {
        DestroyWindow(wrapper->overlayWindow);
    }
    
    // 销毁视频窗口
    if (wrapper->videoWindow) {
        DestroyWindow(wrapper->videoWindow);
    }
    
    // 销毁容器窗口（会自动销毁其子窗口）
    if (wrapper->containerWindow) {
        DestroyWindow(wrapper->containerWindow);
        LogMessage("已销毁容器窗口");
    }
    
    // 删除背景画刷
    if (wrapper->backgroundBrush) {
        DeleteObject(wrapper->backgroundBrush);
        LogMessage("已删除背景画刷");
    }
    
    delete wrapper;
    
    LogMessage("播放器资源已释放");
}

void wv_player_set_background_color(void* playerHandle, int red, int green, int blue) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    // 限制 RGB 值在 0-255 范围内
    red = max(0, min(255, red));
    green = max(0, min(255, green));
    blue = max(0, min(255, blue));
    
    // 删除旧的画刷
    if (wrapper->backgroundBrush) {
        DeleteObject(wrapper->backgroundBrush);
    }
    
    // 创建新的背景色和画刷
    wrapper->backgroundColor = RGB(red, green, blue);
    wrapper->backgroundBrush = CreateSolidBrush(wrapper->backgroundColor);
    
    LogMessage("背景色已更新为: RGB(%d, %d, %d)", red, green, blue);
    
    // 强制重绘窗口以应用新背景色
    if (wrapper->videoWindow) {
        InvalidateRect(wrapper->videoWindow, NULL, TRUE);
        UpdateWindow(wrapper->videoWindow);
    }
}

bool wv_player_recalculate_video_center(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return false;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    LogMessage("手动触发视频居中缩放计算");
    return ApplyVideoCenterScaling(wrapper);
}

void wv_player_update_rectangles(void* playerHandle, const float* rects, int rectCount, 
                                  float lineWidth, float red, float green, float blue, float alpha) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    if (!rects || rectCount <= 0) {
        LogMessage("错误：矩形数据无效");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    if (!wrapper->overlayWindow || !wrapper->overlayData) {
        LogMessage("警告：覆盖层窗口不存在");
        return;
    }
    
    // 更新矩形数据
    wrapper->overlayData->rectangles.clear();
    for (int i = 0; i < rectCount; i++) {
        int offset = i * 4;
        OverlayRectangle rect;
        rect.x = rects[offset];
        rect.y = rects[offset + 1];
        rect.width = rects[offset + 2];
        rect.height = rects[offset + 3];
        wrapper->overlayData->rectangles.push_back(rect);
        
        LogMessage("矩形 %d: x=%.1f, y=%.1f, w=%.1f, h=%.1f", 
                  i, rect.x, rect.y, rect.width, rect.height);
    }
    
    // 更新样式
    wrapper->overlayData->lineWidth = lineWidth;
    wrapper->overlayData->red = red;
    wrapper->overlayData->green = green;
    wrapper->overlayData->blue = blue;
    wrapper->overlayData->alpha = alpha;
    
    // 确保覆盖层在最上层
    SetWindowPos(wrapper->overlayWindow, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    
    // 触发重绘
    InvalidateRect(wrapper->overlayWindow, NULL, TRUE);
    
    LogMessage("已更新 %d 个矩形到覆盖层 (颜色: R=%.2f G=%.2f B=%.2f A=%.2f, 线宽=%.1f)", 
              rectCount, red, green, blue, alpha, lineWidth);
}

void wv_player_clear_rectangles(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    if (!wrapper->overlayWindow || !wrapper->overlayData) {
        LogMessage("警告：覆盖层窗口不存在");
        return;
    }
    
    // 清除矩形数据
    wrapper->overlayData->rectangles.clear();
    
    // 触发重绘
    InvalidateRect(wrapper->overlayWindow, NULL, TRUE);
    
    LogMessage("已清除覆盖层的所有矩形");
}

bool wv_player_get_stats(void* playerHandle, char* buffer, int bufferSize) {
    if (!playerHandle || !buffer || bufferSize <= 0) {
        return false;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    if (!wrapper->mediaPlayer) {
        return false;
    }
    
    // 获取 VLC 播放统计信息
    libvlc_media_t* media = libvlc_media_player_get_media(wrapper->mediaPlayer);
    if (!media) {
        return false;
    }
    
    libvlc_media_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    
    if (libvlc_media_get_stats(media, &stats)) {
        // 格式化统计信息
        snprintf(buffer, bufferSize,
                 "Bitrate: %.2f kb/s\n"
                 "Read: %.2f MB\n"
                 "Demux: %.2f MB\n"
                 "Lost frames: %d",
                 stats.f_input_bitrate,
                 stats.i_read_bytes / (1024.0f * 1024.0f),
                 stats.i_demux_read_bytes / (1024.0f * 1024.0f),
                 stats.i_lost_pictures
        );
        
        return true;
    }
    
    return false;
}

void wv_player_update_stats_display(void* playerHandle, const char* statsText, bool show) {
    if (!playerHandle) {
        LogMessage("错误：播放器句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    if (!wrapper->overlayWindow || !wrapper->overlayData) {
        LogMessage("警告：覆盖层窗口不存在");
        return;
    }
    
    // 更新统计信息文本
    wrapper->overlayData->showStats = show;
    
    if (statsText && show) {
        // 将 UTF-8 转换为宽字符
        int len = MultiByteToWideChar(CP_UTF8, 0, statsText, -1, NULL, 0);
        if (len > 0) {
            wrapper->overlayData->statsText.resize(len);
            MultiByteToWideChar(CP_UTF8, 0, statsText, -1, &wrapper->overlayData->statsText[0], len);
        }
    } else {
        wrapper->overlayData->statsText.clear();
    }
    
    // 刷新窗口
    InvalidateRect(wrapper->overlayWindow, NULL, TRUE);
    
    if (show) {
        LogMessage("已更新统计信息显示");
    }
}

