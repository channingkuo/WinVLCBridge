//
//  WinVLCBridge.h
//  WinVLCBridge
//
//  Created by Channing Kuo on 2025/10/7.
//

#ifndef WIN_VLC_BRIDGE_H
#define WIN_VLC_BRIDGE_H

#ifdef _WIN32
    #ifdef WINVLCBRIDGE_EXPORTS
        #define WINVLCBRIDGE_API __declspec(dllexport)
    #else
        #define WINVLCBRIDGE_API __declspec(dllimport)
    #endif
#else
    #define WINVLCBRIDGE_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 创建播放器并关联到指定的窗口句柄
 * @param hwnd_ptr 父窗口句柄
 * @param x 视频窗口 X 坐标
 * @param y 视频窗口 Y 坐标
 * @param width 视频窗口宽度
 * @param height 视频窗口高度
 * @return 播放器句柄
 */
WINVLCBRIDGE_API void* wv_create_player_for_view(void* hwnd_ptr, float x, float y, float width, float height);

/**
 * 播放视频（自动识别本地文件或网络流）
 * @param playerHandle 播放器句柄
 * @param source 视频源路径（本地文件路径或网络流地址 http/rtsp）
 */
WINVLCBRIDGE_API void wv_player_play(void* playerHandle, const char* source);

/**
 * 暂停播放
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_pause(void* playerHandle);

/**
 * 恢复播放（从暂停状态继续播放）
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_resume(void* playerHandle);

/**
 * 停止播放
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_stop(void* playerHandle);

/**
 * 释放播放器资源
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_release(void* playerHandle);

#ifdef __cplusplus
}
#endif

#endif // WIN_VLC_BRIDGE_H
