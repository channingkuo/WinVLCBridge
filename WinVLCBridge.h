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

/**
 * 更新覆盖层的矩形列表（实时绘制矩形框）
 * @param playerHandle 播放器句柄
 * @param rects 矩形数组，格式为 [x1, y1, width1, height1, x2, y2, width2, height2, ...]
 * @param rectCount 矩形的数量
 * @param lineWidth 矩形边框宽度
 * @param red 红色分量 (0.0 - 1.0)
 * @param green 绿色分量 (0.0 - 1.0)
 * @param blue 蓝色分量 (0.0 - 1.0)
 * @param alpha 透明度 (0.0 - 1.0)
 */
WINVLCBRIDGE_API void wv_player_update_rectangles(void* playerHandle, const float* rects, int rectCount, float lineWidth, float red, float green, float blue, float alpha);

/**
 * 清除覆盖层的所有矩形
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_clear_rectangles(void* playerHandle);

/**
 * 获取播放统计信息
 * @param playerHandle 播放器句柄
 * @param buffer 用于存储统计信息文本的缓冲区
 * @param bufferSize 缓冲区大小
 * @return 是否成功获取
 */
WINVLCBRIDGE_API bool wv_player_get_stats(void* playerHandle, char* buffer, int bufferSize);

/**
 * 更新统计信息显示
 * @param playerHandle 播放器句柄
 * @param statsText 统计信息文本（UTF-8编码）
 * @param show 是否显示统计信息
 */
WINVLCBRIDGE_API void wv_player_update_stats_display(void* playerHandle, const char* statsText, bool show);

/**
 * 设置视频窗口背景色
 * @param playerHandle 播放器句柄
 * @param red 红色分量 (0-255)
 * @param green 绿色分量 (0-255)
 * @param blue 蓝色分量 (0-255)
 */
WINVLCBRIDGE_API void wv_player_set_background_color(void* playerHandle, int red, int green, int blue);

#ifdef __cplusplus
}
#endif

#endif // WIN_VLC_BRIDGE_H
