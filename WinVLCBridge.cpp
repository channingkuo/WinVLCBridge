//
//  WinVLCBridge.cpp
//  WinVLCBridge
//
//  Created by Channing Kuo on 2025/10/7.
//

#include "WinVLCBridge.h"
#include <windows.h>
#include <iostream>

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

// ==================== 窗口包装结构 ====================

struct WVPlayerWrapper {
    HWND videoWindow;    // 视频窗口
    HWND parentWindow;   // 父窗口
    int videoWidth;      // 视频窗口宽度
    int videoHeight;     // 视频窗口高度
    int offsetX;         // 相对于父窗口的X偏移
    int offsetY;         // 相对于父窗口的Y偏移
};

// ==================== 工具函数 ====================

// 视频窗口过程（确保黑色背景正确显示）
static LRESULT CALLBACK VideoWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_ERASEBKGND: {
            // 用黑色填充背景
            HDC hdc = (HDC)wParam;
            RECT rect;
            GetClientRect(hwnd, &rect);
            HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &rect, blackBrush);
            return 1; // 表示已处理
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // 用黑色填充
            HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &ps.rcPaint, blackBrush);
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


// ==================== 公共 API 实现 ====================

void* wv_create_player_for_view(void* hwnd_ptr, float x, float y, float width, float height) {
    if (!hwnd_ptr) {
        LogMessage("错误：父窗口句柄为空");
        return NULL;
    }
    
    HWND parentWindow = static_cast<HWND>(hwnd_ptr);
    
    // 创建窗口包装对象
    WVPlayerWrapper* wrapper = new WVPlayerWrapper();
    memset(wrapper, 0, sizeof(WVPlayerWrapper));
    
    // 保存窗口尺寸和父窗口信息
    wrapper->parentWindow = parentWindow;
    wrapper->videoWidth = static_cast<int>(width);
    wrapper->videoHeight = static_cast<int>(height);
    wrapper->offsetX = static_cast<int>(x);
    wrapper->offsetY = static_cast<int>(y);
    
    // 注册自定义视频窗口类（带黑色背景）
    if (!RegisterVideoWindowClass()) {
        LogMessage("错误：无法注册视频窗口类");
        delete wrapper;
        return NULL;
    }
    
    // 创建子窗口（模仿 macOS 的 addSubview 逻辑）
    // 使用相对坐标，子窗口会自动跟随父窗口移动
    wrapper->videoWindow = CreateWindowExW(
        WS_EX_NOACTIVATE,  // 不激活窗口
        L"VLCVideoWindow",  // 使用自定义窗口类
        L"Video Window",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,  // 子窗口样式
        static_cast<int>(x), static_cast<int>(y),  // 相对于父窗口的坐标
        static_cast<int>(width), static_cast<int>(height),
        parentWindow,  // 父窗口
        NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    LogMessage("创建子窗口（模仿 macOS addSubview）- 相对坐标: (%d,%d)", 
               static_cast<int>(x), static_cast<int>(y));
    
    if (!wrapper->videoWindow) {
        LogMessage("错误：无法创建窗口，错误码: %d", GetLastError());
        delete wrapper;
        return NULL;
    }
    
    LogMessage("窗口创建成功（带黑色背景）: HWND=0x%p, 位置=(%d,%d), 大小=%dx%d", 
               wrapper->videoWindow, static_cast<int>(x), static_cast<int>(y),
               static_cast<int>(width), static_cast<int>(height));
    
    // 强制绘制黑色背景
    HDC hdc = GetDC(wrapper->videoWindow);
    if (hdc) {
        RECT rect;
        GetClientRect(wrapper->videoWindow, &rect);
        HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        FillRect(hdc, &rect, blackBrush);
        ReleaseDC(wrapper->videoWindow, hdc);
        LogMessage("已强制绘制黑色背景");
    }
    
    // 将子窗口置于同级窗口的最上方（覆盖 Chromium 渲染层）
    SetWindowPos(wrapper->videoWindow, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    
    // 强制刷新窗口
    InvalidateRect(wrapper->videoWindow, NULL, TRUE);
    UpdateWindow(wrapper->videoWindow);
    
    // 再次确保窗口可见
    ShowWindow(wrapper->videoWindow, SW_SHOW);
    BringWindowToTop(wrapper->videoWindow);
    
    LogMessage("窗口已设置为 Z-order 最顶层并强制刷新");
    LogMessage("窗口创建成功 - 窗口大小: %.0fx%.0f, 位置: (%.0f, %.0f)", width, height, x, y);
    
    return wrapper;
}

void wv_player_release(void* playerHandle) {
    if (!playerHandle) {
        LogMessage("错误：窗口句柄为空");
        return;
    }
    
    WVPlayerWrapper* wrapper = static_cast<WVPlayerWrapper*>(playerHandle);
    
    // 销毁窗口
    if (wrapper->videoWindow) {
        DestroyWindow(wrapper->videoWindow);
    }
    
    delete wrapper;
    
    LogMessage("窗口资源已释放");
}
