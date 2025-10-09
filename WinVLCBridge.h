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
 * 创建窗口并关联到指定的窗口句柄
 * @param hwnd_ptr 父窗口句柄
 * @param x 窗口 X 坐标
 * @param y 窗口 Y 坐标
 * @param width 窗口宽度
 * @param height 窗口高度
 * @return 窗口句柄
 */
WINVLCBRIDGE_API void* wv_create_player_for_view(void* hwnd_ptr, float x, float y, float width, float height);

/**
 * 释放窗口资源
 * @param playerHandle 窗口句柄
 */
WINVLCBRIDGE_API void wv_player_release(void* playerHandle);

#ifdef __cplusplus
}
#endif

#endif // WIN_VLC_BRIDGE_H
