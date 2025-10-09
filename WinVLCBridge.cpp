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
            
            if (data && !data->rectangles.empty()) {
                Graphics graphics(hdc);
                graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                
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
    HWND videoWindow;      // VLC 视频窗口
    HWND overlayWindow;    // 覆盖层窗口
    OverlayWindowData* overlayData;
};

// ==================== 工具函数 ====================

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
        "--no-osd"
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
    
    // 创建视频子窗口
    wrapper->videoWindow = CreateWindowExW(
        0,
        L"STATIC",
        L"Video Window",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
        static_cast<int>(x), static_cast<int>(y),
        static_cast<int>(width), static_cast<int>(height),
        parentWindow,
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (!wrapper->videoWindow) {
        LogMessage("错误：无法创建视频窗口，错误码: %d", GetLastError());
        libvlc_media_player_release(wrapper->mediaPlayer);
        libvlc_release(wrapper->vlcInstance);
        delete wrapper;
        return NULL;
    }
    
    // 设置 VLC 使用该窗口进行渲染
    libvlc_media_player_set_hwnd(wrapper->mediaPlayer, wrapper->videoWindow);
    
    // 创建覆盖层窗口
    wrapper->overlayWindow = CreateOverlayWindow(
        parentWindow,
        static_cast<int>(x), static_cast<int>(y),
        static_cast<int>(width), static_cast<int>(height)
    );
    
    if (wrapper->overlayWindow) {
        wrapper->overlayData = reinterpret_cast<OverlayWindowData*>(
            GetWindowLongPtr(wrapper->overlayWindow, GWLP_USERDATA)
        );
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
    libvlc_media_player_set_media(wrapper->mediaPlayer, media);
    libvlc_media_release(media);  // 播放器会持有引用
    
    int playResult = libvlc_media_player_play(wrapper->mediaPlayer);
    
    if (playResult == 0) {
        LogMessage("开始播放: %s", sourcePath.c_str());
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
    
    delete wrapper;
    
    LogMessage("播放器资源已释放");
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

