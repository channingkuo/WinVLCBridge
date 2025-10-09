//
//  WinVLCBridge.cpp
//  WinVLCBridge
//
//  Created by Channing Kuo on 2025/10/7.
//
//  VLC 视频播放器 Windows 桥接库 - 实现文件
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

// ==================== 模块1：日志工具 ====================

namespace Logger {
    /**
     * 输出日志消息到 stderr 和 Windows 调试器
     * @param format 格式化字符串
     * @param ... 可变参数
     */
    static void Log(const char* format, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        // 输出到 stderr（立即显示）
        fprintf(stderr, "[WinVLCBridge] %s\n", buffer);
        fflush(stderr);
        
        // 转换为宽字符并输出到 Windows 调试器（支持 DebugView）
        wchar_t wideBuffer[1024];
        wchar_t wideMessage[1100];
        MultiByteToWideChar(CP_UTF8, 0, buffer, -1, wideBuffer, 1024);
        swprintf(wideMessage, 1100, L"[WinVLCBridge] %s\n", wideBuffer);
        OutputDebugStringW(wideMessage);
    }
}

// ==================== 模块2：覆盖层窗口管理 ====================

namespace OverlayWindow {
    // 矩形数据结构
    struct Rectangle {
        float x, y, width, height;
    };
    
    // 覆盖层窗口数据
    struct Data {
        HWND hwnd;                          // 窗口句柄
        std::vector<Rectangle> rectangles;  // 矩形列表
        float lineWidth;                    // 矩形线宽
        float red, green, blue, alpha;      // 矩形颜色
        ULONG_PTR gdiplusToken;             // GDI+ 令牌
        
        // 统计信息显示
        std::wstring statsText;             // 统计信息文本
        bool showStats;                     // 是否显示统计信息
        
        Data() : hwnd(NULL), lineWidth(2.0f), red(0.0f), green(1.0f), 
                 blue(0.0f), alpha(1.0f), gdiplusToken(0), showStats(false) {}
    };
    
    const wchar_t* CLASS_NAME = L"WinVLCBridgeOverlayWindow";
    
    /**
     * 窗口过程函数
     */
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Data* data = reinterpret_cast<Data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        
        switch (msg) {
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                
                if (data) {
                    Graphics graphics(hdc);
                    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
                    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
                    
                    // 清除整个绘制区域（完全透明）
                    RECT clientRect;
                    GetClientRect(hwnd, &clientRect);
                    SolidBrush clearBrush(Color(0, 0, 0, 0));
                    graphics.FillRectangle(&clearBrush, 0, 0, clientRect.right, clientRect.bottom);
                    
                    // 绘制矩形
                    if (!data->rectangles.empty()) {
                        Color color(
                            static_cast<BYTE>(data->alpha * 255),
                            static_cast<BYTE>(data->red * 255),
                            static_cast<BYTE>(data->green * 255),
                            static_cast<BYTE>(data->blue * 255)
                        );
                        Pen pen(color, data->lineWidth);
                        
                        for (const auto& rect : data->rectangles) {
                            graphics.DrawRectangle(&pen, rect.x, rect.y, rect.width, rect.height);
                        }
                    }
                    
                    // 绘制统计信息（右上角）
                    if (data->showStats && !data->statsText.empty()) {
                        FontFamily fontFamily(L"Consolas");
                        Font font(&fontFamily, 14, FontStyleBold, UnitPixel);
                        
                        SolidBrush bgBrush(Color(180, 0, 0, 0));      // 半透明黑色背景
                        SolidBrush textBrush(Color(255, 0, 255, 0));  // 亮绿色文本
                        
                        // 测量文本大小
                        RectF layoutRect(0, 0, 500, 200);
                        RectF boundingBox;
                        graphics.MeasureString(data->statsText.c_str(), -1, &font, layoutRect, &boundingBox);
                        
                        // 计算右上角位置
                        float textX = clientRect.right - boundingBox.Width - 15;
                        float textY = 10;
                        
                        // 绘制背景和文本
                        RectF bgRect(textX - 5, textY - 3, boundingBox.Width + 10, boundingBox.Height + 6);
                        graphics.FillRectangle(&bgBrush, bgRect);
                        graphics.DrawString(data->statsText.c_str(), -1, &font, PointF(textX, textY), &textBrush);
                    }
                }
                
                EndPaint(hwnd, &ps);
                return 0;
            }
            
            case WM_ERASEBKGND:
                return 1;  // 已处理，防止闪烁
                
            case WM_NCHITTEST:
                return HTTRANSPARENT;  // 鼠标事件穿透
                
            case WM_DESTROY:
                if (data) {
                    if (data->gdiplusToken) {
                        GdiplusShutdown(data->gdiplusToken);
                    }
                    delete data;
                }
                return 0;
        }
        
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    /**
     * 注册窗口类
     */
    bool RegisterClass() {
        static bool registered = false;
        if (registered) return true;
        
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        
        if (!RegisterClassExW(&wc)) {
            Logger::Log("错误：无法注册覆盖层窗口类，错误码: %d", GetLastError());
            return false;
        }
        
        registered = true;
        return true;
    }
    
    /**
     * 创建覆盖层窗口
     */
    HWND Create(HWND parent, int x, int y, int width, int height) {
        if (!RegisterClass()) {
            return NULL;
        }
        
        // 初始化数据
        Data* data = new Data();
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&data->gdiplusToken, &gdiplusStartupInput, NULL);
        
        // 创建透明的分层子窗口
        HWND hwnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT,
            CLASS_NAME,
            L"Overlay",
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            parent,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd) {
            Logger::Log("错误：无法创建覆盖层窗口，错误码: %d", GetLastError());
            GdiplusShutdown(data->gdiplusToken);
            delete data;
            return NULL;
        }
        
        // 设置透明度
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        
        // 关联数据
        data->hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(data));
        
        Logger::Log("覆盖层窗口创建成功: HWND=0x%p, 位置=(%d,%d), 大小=%dx%d", hwnd, x, y, width, height);
        
        return hwnd;
    }
    
    /**
     * 获取窗口数据
     */
    Data* GetData(HWND hwnd) {
        if (!hwnd) return NULL;
        return reinterpret_cast<Data*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }
}

// ==================== 模块3：视频窗口管理 ====================

namespace VideoWindow {
    const wchar_t* CLASS_NAME = L"VLCVideoWindow";
    
    /**
     * 窗口过程函数（确保黑色背景）
     */
    LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_ERASEBKGND: {
                HDC hdc = (HDC)wParam;
                RECT rect;
                GetClientRect(hwnd, &rect);
                HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                FillRect(hdc, &rect, blackBrush);
                return 1;
            }
            case WM_PAINT: {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd, &ps);
                HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                FillRect(hdc, &ps.rcPaint, blackBrush);
                EndPaint(hwnd, &ps);
                return 0;
            }
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    /**
     * 注册窗口类
     */
    bool RegisterClass() {
        static bool registered = false;
        if (registered) return true;
        
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = CLASS_NAME;
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
    
    /**
     * 创建视频窗口
     */
    HWND Create(HWND parent, int x, int y, int width, int height) {
        if (!RegisterClass()) {
            Logger::Log("错误：无法注册视频窗口类");
            return NULL;
        }
        
        HWND hwnd = CreateWindowExW(
            WS_EX_NOACTIVATE,
            CLASS_NAME,
            L"Video Window",
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            x, y, width, height,
            parent,
            NULL,
            GetModuleHandle(NULL),
            NULL
        );
        
        if (!hwnd) {
            Logger::Log("错误：无法创建视频窗口，错误码: %d", GetLastError());
            return NULL;
        }
        
        Logger::Log("视频窗口创建成功: HWND=0x%p, 位置=(%d,%d), 大小=%dx%d", 
                   hwnd, x, y, width, height);
        
        return hwnd;
    }
}

// ==================== 模块4：工具函数 ====================

namespace Utils {
    /**
     * 判断是否为网络流地址
     */
    bool IsNetworkStream(const std::string& source) {
        std::string lower = source;
        for (auto& c : lower) c = tolower(c);
        
        return lower.find("http://") == 0 ||
               lower.find("https://") == 0 ||
               lower.find("rtsp://") == 0 ||
               lower.find("rtmp://") == 0 ||
               lower.find("rtmps://") == 0 ||
               lower.find("rtp://") == 0;
    }
    
    /**
     * 转换路径为标准格式（将反斜杠转为正斜杠）
     */
    std::string NormalizePath(const std::string& path) {
        if (IsNetworkStream(path)) {
            return path;
        }
        
        std::string normalized = path;
        for (auto& c : normalized) {
            if (c == '\\') c = '/';
        }
        return normalized;
    }
    
    /**
     * 获取 DLL 所在目录
     */
    std::string GetDllDirectory() {
        char dllPath[MAX_PATH];
        HMODULE hModule = NULL;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&wv_create_player_for_view, 
            &hModule
        );
        GetModuleFileNameA(hModule, dllPath, MAX_PATH);
        
        std::string dllDir = dllPath;
        size_t lastSlash = dllDir.find_last_of("\\/");
        if (lastSlash != std::string::npos) {
            dllDir = dllDir.substr(0, lastSlash);
        }
        
        return dllDir;
    }
}

// ==================== 模块5：VLC 播放器管理 ====================

/**
 * 播放器包装结构
 */
struct VLCPlayer {
    libvlc_instance_t* vlcInstance;
    libvlc_media_player_t* mediaPlayer;
    libvlc_media_t* currentMedia;
    libvlc_event_manager_t* eventManager;
    
    HWND videoWindow;      // 视频窗口
    HWND overlayWindow;    // 覆盖层窗口
    int videoWidth;        // 窗口宽度
    int videoHeight;       // 窗口高度
    
    bool videoScaleConfigured;  // 是否已配置视频缩放
    
    VLCPlayer() : vlcInstance(NULL), mediaPlayer(NULL), currentMedia(NULL),
                  eventManager(NULL), videoWindow(NULL), overlayWindow(NULL),
                  videoWidth(0), videoHeight(0), videoScaleConfigured(false) {}
};

namespace VLCPlayerManager {
    /**
     * VLC 事件回调：视频输出准备好时自动配置缩放
     */
    void OnMediaPlayerVout(const libvlc_event_t* event, void* userData) {
        VLCPlayer* player = static_cast<VLCPlayer*>(userData);
        if (!player || !player->mediaPlayer) return;
        
        // 避免重复配置
        if (player->videoScaleConfigured) {
            return;
        }
        
        Logger::Log("视频输出已准备好，正在设置视频适配模式...");
        
        // 获取视频实际尺寸
        unsigned int videoWidth = 0, videoHeight = 0;
        if (libvlc_video_get_size(player->mediaPlayer, 0, &videoWidth, &videoHeight) == 0) {
            if (videoWidth > 0 && videoHeight > 0) {
                Logger::Log("视频原始尺寸: %ux%u", videoWidth, videoHeight);
                Logger::Log("窗口尺寸: %dx%d", player->videoWidth, player->videoHeight);
                
                // 设置自动适配（scale=0 保持宽高比）
                libvlc_video_set_scale(player->mediaPlayer, 0);
                
                // 计算宽高比
                float videoAspect = (float)videoWidth / (float)videoHeight;
                float windowAspect = (float)player->videoWidth / (float)player->videoHeight;
                
                if (videoAspect > windowAspect) {
                    Logger::Log("视频更宽，将产生上下黑边（letterbox）");
                } else {
                    Logger::Log("视频更高，将产生左右黑边（pillarbox）");
                }
                
                player->videoScaleConfigured = true;
                Logger::Log("视频适配模式已设置完成");
            }
        } else {
            Logger::Log("警告：无法获取视频尺寸，视频可能还未准备好");
        }
    }
    
    /**
     * 初始化 VLC 实例
     */
    bool InitializeVLC(VLCPlayer* player) {
        std::string dllDir = Utils::GetDllDirectory();
        std::string pluginPath = "--plugin-path=" + dllDir + "\\plugins";
        
        Logger::Log("DLL 目录: %s", dllDir.c_str());
        Logger::Log("插件路径: %s", pluginPath.c_str());
        
        const char* vlc_args[] = {
            pluginPath.c_str(),
            "--file-caching=50",
            "--network-caching=100",
            "--avcodec-fast",
            "--no-sub-autodetect-file",
            "--no-video-title-show",
            "--no-snapshot-preview",
            "--no-osd",
            "--no-video-title",
            "--no-mouse-events",
            "--no-keyboard-events"
        };
        
        player->vlcInstance = libvlc_new(sizeof(vlc_args) / sizeof(vlc_args[0]), vlc_args);
        if (!player->vlcInstance) {
            Logger::Log("错误：无法初始化 libVLC");
            return false;
        }
        
        player->mediaPlayer = libvlc_media_player_new(player->vlcInstance);
        if (!player->mediaPlayer) {
            Logger::Log("错误：无法创建媒体播放器");
            libvlc_release(player->vlcInstance);
            return false;
        }
        
        return true;
    }
    
    /**
     * 注册事件监听器
     */
    void RegisterEventListeners(VLCPlayer* player) {
        player->eventManager = libvlc_media_player_event_manager(player->mediaPlayer);
        if (player->eventManager) {
            libvlc_event_attach(player->eventManager, libvlc_MediaPlayerVout, 
                              OnMediaPlayerVout, player);
            Logger::Log("已注册视频输出就绪事件监听器");
        }
    }
    
    /**
     * 播放媒体源
     */
    void Play(VLCPlayer* player, const char* source) {
        if (!source) {
            Logger::Log("错误：视频源为空");
            return;
        }
        
        std::string sourcePath = source;
        Logger::Log("原始路径: %s", source);
        
        // 创建媒体对象
        libvlc_media_t* media = NULL;
        
        if (Utils::IsNetworkStream(sourcePath)) {
            // 网络流
            Logger::Log("检测到网络流，使用 location 方式");
            media = libvlc_media_new_location(player->vlcInstance, sourcePath.c_str());
            
            if (media) {
                libvlc_media_add_option(media, ":network-caching=300");
                libvlc_media_add_option(media, ":live-caching=300");
                libvlc_media_add_option(media, ":clock-jitter=0");
                libvlc_media_add_option(media, ":clock-synchro=0");
            }
        } else {
            // 本地文件
            DWORD fileAttr = GetFileAttributesA(sourcePath.c_str());
            if (fileAttr == INVALID_FILE_ATTRIBUTES) {
                Logger::Log("错误：文件不存在: %s", sourcePath.c_str());
                return;
            }
            
            Logger::Log("文件存在，准备创建媒体对象");
            
            std::string normalizedPath = Utils::NormalizePath(sourcePath);
            std::string fileUri = "file:///" + normalizedPath;
            Logger::Log("使用 URI: %s", fileUri.c_str());
            
            media = libvlc_media_new_location(player->vlcInstance, fileUri.c_str());
            if (!media) {
                Logger::Log("location 方式失败，尝试 path 方式");
                media = libvlc_media_new_path(player->vlcInstance, sourcePath.c_str());
            }
        }
        
        if (!media) {
            Logger::Log("错误：无法创建媒体对象");
            const char* vlcError = libvlc_errmsg();
            if (vlcError) {
                Logger::Log("VLC 错误信息: %s", vlcError);
            }
            return;
        }
        
        Logger::Log("媒体对象创建成功");
        
        // 释放旧媒体
        if (player->currentMedia) {
            libvlc_media_release(player->currentMedia);
        }
        
        player->currentMedia = media;
        player->videoScaleConfigured = false;  // 重置缩放配置标志
        
        libvlc_media_player_set_media(player->mediaPlayer, media);
        
        int playResult = libvlc_media_player_play(player->mediaPlayer);
        
        if (playResult == 0) {
            Logger::Log("开始播放: %s", sourcePath.c_str());
            Logger::Log("等待视频输出准备就绪，将自动设置缩放模式...");
            
            // 确保窗口可见并在顶层
            ShowWindow(player->videoWindow, SW_SHOW);
            UpdateWindow(player->videoWindow);
            BringWindowToTop(player->videoWindow);
            SetWindowPos(player->videoWindow, HWND_TOP, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            
            if (player->overlayWindow) {
                SetWindowPos(player->overlayWindow, HWND_TOP, 0, 0, 0, 0, 
                           SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
            }
            
            Logger::Log("视频窗口已更新并设置 Z-order 为顶层");
        } else {
            Logger::Log("错误：播放失败，返回码: %d", playResult);
            const char* vlcError = libvlc_errmsg();
            if (vlcError) {
                Logger::Log("VLC 错误信息: %s", vlcError);
            }
        }
    }
    
    /**
     * 获取播放统计信息
     */
    bool GetStats(VLCPlayer* player, char* buffer, int bufferSize) {
        if (!player->mediaPlayer) {
            return false;
        }
        
        libvlc_media_t* media = libvlc_media_player_get_media(player->mediaPlayer);
        if (!media) {
            return false;
        }
        
        libvlc_media_stats_t stats;
        memset(&stats, 0, sizeof(stats));
        
        if (libvlc_media_get_stats(media, &stats)) {
            libvlc_time_t currentTime = libvlc_media_player_get_time(player->mediaPlayer);
            libvlc_state_t state = libvlc_media_player_get_state(player->mediaPlayer);
            
            const char* stateStr = "Unknown";
            switch (state) {
                case libvlc_Buffering: stateStr = "Buffering"; break;
                case libvlc_Playing: stateStr = "Playing"; break;
                case libvlc_Paused: stateStr = "Paused"; break;
                case libvlc_Stopped: stateStr = "Stopped"; break;
                default: break;
            }
            
            int decodeLag = stats.i_decoded_video - stats.i_displayed_pictures;
            
            snprintf(buffer, bufferSize,
                     "Status: %s\n"
                     "Bitrate: %.2f kb/s\n"
                     "Read: %.2f MB\n"
                     "Demux: %.2f MB\n"
                     "Decoded: %d frames\n"
                     "Displayed: %d frames\n"
                     "Decode lag: %d frames\n"
                     "Lost frames: %d\n"
                     "Time: %.1f s",
                     stateStr,
                     stats.f_input_bitrate,
                     stats.i_read_bytes / (1024.0f * 1024.0f),
                     stats.i_demux_read_bytes / (1024.0f * 1024.0f),
                     stats.i_decoded_video,
                     stats.i_displayed_pictures,
                     decodeLag,
                     stats.i_lost_pictures,
                     currentTime / 1000.0f
            );
            
            return true;
        }
        
        return false;
    }
}

// ==================== 模块6：公共 API 实现 ====================

void* wv_create_player_for_view(void* hwnd_ptr, float x, float y, float width, float height) {
    if (!hwnd_ptr) {
        Logger::Log("错误：父窗口句柄为空");
        return NULL;
    }
    
    HWND parentWindow = static_cast<HWND>(hwnd_ptr);
    VLCPlayer* player = new VLCPlayer();
    
    player->videoWidth = static_cast<int>(width);
    player->videoHeight = static_cast<int>(height);
    
    // 初始化 VLC
    if (!VLCPlayerManager::InitializeVLC(player)) {
        delete player;
        return NULL;
    }
    
    // 创建视频窗口
    player->videoWindow = VideoWindow::Create(
        parentWindow, 
        static_cast<int>(x), static_cast<int>(y),
        static_cast<int>(width), static_cast<int>(height)
    );
    
    if (!player->videoWindow) {
        libvlc_media_player_release(player->mediaPlayer);
        libvlc_release(player->vlcInstance);
        delete player;
        return NULL;
    }
    
    // 设置 VLC 渲染窗口
    SetWindowPos(player->videoWindow, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
    libvlc_media_player_set_hwnd(player->mediaPlayer, player->videoWindow);
    Logger::Log("已设置 VLC 渲染窗口句柄");
    
    // 注册事件监听器
    VLCPlayerManager::RegisterEventListeners(player);
    
    // 创建覆盖层窗口
    player->overlayWindow = OverlayWindow::Create(
        parentWindow,
        static_cast<int>(x), static_cast<int>(y),
        static_cast<int>(width), static_cast<int>(height)
    );
    
    if (player->overlayWindow) {
        SetWindowPos(player->overlayWindow, HWND_TOP, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        Logger::Log("覆盖层窗口 Z-order 已设置为顶层");
    }
    
    Logger::Log("播放器创建成功 - 窗口大小: %.0fx%.0f, 位置: (%.0f, %.0f)", width, height, x, y);
    
    return player;
}

void wv_player_play(void* playerHandle, const char* source) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    VLCPlayerManager::Play(player, source);
}

void wv_player_pause(void* playerHandle) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    libvlc_media_player_pause(player->mediaPlayer);
    Logger::Log("播放器已暂停");
}

void wv_player_resume(void* playerHandle) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    libvlc_state_t state = libvlc_media_player_get_state(player->mediaPlayer);
    
    if (state == libvlc_Paused) {
        libvlc_media_player_play(player->mediaPlayer);
        Logger::Log("播放器已恢复播放");
    } else if (state == libvlc_Stopped) {
        Logger::Log("警告：播放器处于停止状态，无法恢复播放");
    } else if (state == libvlc_Playing) {
        Logger::Log("提示：播放器已在播放中");
    } else {
        Logger::Log("播放器状态：%d，尝试恢复播放", state);
        libvlc_media_player_play(player->mediaPlayer);
    }
}

void wv_player_stop(void* playerHandle) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    libvlc_media_player_stop(player->mediaPlayer);
    Logger::Log("播放器已停止");
}

void wv_player_release(void* playerHandle) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    
    // 停止播放
    libvlc_media_player_stop(player->mediaPlayer);
    
    // 分离事件监听器
    if (player->eventManager) {
        libvlc_event_detach(player->eventManager, libvlc_MediaPlayerVout, 
                          VLCPlayerManager::OnMediaPlayerVout, player);
        Logger::Log("已分离事件监听器");
    }
    
    // 释放资源
    if (player->currentMedia) {
        libvlc_media_release(player->currentMedia);
    }
    
    if (player->mediaPlayer) {
        libvlc_media_player_release(player->mediaPlayer);
    }
    
    if (player->vlcInstance) {
        libvlc_release(player->vlcInstance);
    }
    
    // 销毁窗口
    if (player->overlayWindow) {
        DestroyWindow(player->overlayWindow);
    }
    
    if (player->videoWindow) {
        DestroyWindow(player->videoWindow);
    }
    
    delete player;
    Logger::Log("播放器资源已释放");
}

void wv_player_update_rectangles(void* playerHandle, const float* rects, int rectCount, 
                                  float lineWidth, float red, float green, float blue, float alpha) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    if (!rects || rectCount <= 0) {
        Logger::Log("错误：矩形数据无效");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    OverlayWindow::Data* overlayData = OverlayWindow::GetData(player->overlayWindow);
    
    if (!player->overlayWindow || !overlayData) {
        Logger::Log("警告：覆盖层窗口不存在");
        return;
    }
    
    // 更新矩形数据
    overlayData->rectangles.clear();
    for (int i = 0; i < rectCount; i++) {
        int offset = i * 4;
        OverlayWindow::Rectangle rect;
        rect.x = rects[offset];
        rect.y = rects[offset + 1];
        rect.width = rects[offset + 2];
        rect.height = rects[offset + 3];
        overlayData->rectangles.push_back(rect);
        
        Logger::Log("矩形 %d: x=%.1f, y=%.1f, w=%.1f, h=%.1f", 
                  i, rect.x, rect.y, rect.width, rect.height);
    }
    
    // 更新样式
    overlayData->lineWidth = lineWidth;
    overlayData->red = red;
    overlayData->green = green;
    overlayData->blue = blue;
    overlayData->alpha = alpha;
    
    // 刷新显示
    SetWindowPos(player->overlayWindow, HWND_TOP, 0, 0, 0, 0, 
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    InvalidateRect(player->overlayWindow, NULL, TRUE);
    UpdateWindow(player->overlayWindow);
    
    Logger::Log("已更新 %d 个矩形到覆盖层 (颜色: R=%.2f G=%.2f B=%.2f A=%.2f, 线宽=%.1f)", 
              rectCount, red, green, blue, alpha, lineWidth);
}

void wv_player_clear_rectangles(void* playerHandle) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    OverlayWindow::Data* overlayData = OverlayWindow::GetData(player->overlayWindow);
    
    if (!player->overlayWindow || !overlayData) {
        Logger::Log("警告：覆盖层窗口不存在");
        return;
    }
    
    overlayData->rectangles.clear();
    InvalidateRect(player->overlayWindow, NULL, TRUE);
    UpdateWindow(player->overlayWindow);
    
    Logger::Log("已清除覆盖层的所有矩形");
}

bool wv_player_get_stats(void* playerHandle, char* buffer, int bufferSize) {
    if (!playerHandle || !buffer || bufferSize <= 0) {
        return false;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    return VLCPlayerManager::GetStats(player, buffer, bufferSize);
}

void wv_player_update_stats_display(void* playerHandle, const char* statsText, bool show) {
    if (!playerHandle) {
        Logger::Log("错误：播放器句柄为空");
        return;
    }
    
    VLCPlayer* player = static_cast<VLCPlayer*>(playerHandle);
    OverlayWindow::Data* overlayData = OverlayWindow::GetData(player->overlayWindow);
    
    if (!player->overlayWindow || !overlayData) {
        Logger::Log("警告：覆盖层窗口不存在");
        return;
    }
    
    // 更新统计信息
    overlayData->showStats = show;
    
    if (statsText && show) {
        int len = MultiByteToWideChar(CP_UTF8, 0, statsText, -1, NULL, 0);
        if (len > 0) {
            overlayData->statsText.resize(len);
            MultiByteToWideChar(CP_UTF8, 0, statsText, -1, &overlayData->statsText[0], len);
        }
    } else {
        overlayData->statsText.clear();
    }
    
    // 刷新显示
    InvalidateRect(player->overlayWindow, NULL, TRUE);
    UpdateWindow(player->overlayWindow);
    
    if (show) {
        Logger::Log("已更新统计信息显示");
    }
}
