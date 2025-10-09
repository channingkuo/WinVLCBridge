//
//  WinVLCBridge.h
//  WinVLCBridge
//
//  Created by Channing Kuo on 2025/10/7.
//
//  VLC 视频播放器 Windows 桥接库 - 公共API接口
//  支持视频播放、覆盖层绘制、统计信息显示等功能
//

#ifndef WIN_VLC_BRIDGE_H
#define WIN_VLC_BRIDGE_H

// ==================== DLL 导出宏定义 ====================
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

// ==================== 播放器生命周期管理 ====================

/**
 * 创建播放器实例并关联到指定的父窗口
 * 
 * @param hwnd_ptr 父窗口句柄（HWND类型转为void*）
 * @param x 视频窗口在父窗口中的X坐标（像素）
 * @param y 视频窗口在父窗口中的Y坐标（像素）
 * @param width 视频窗口宽度（像素）
 * @param height 视频窗口高度（像素）
 * @return 播放器句柄，失败返回NULL
 * 
 * @note 该函数会创建：
 *       - VLC媒体播放器实例
 *       - 视频渲染窗口（带黑色背景）
 *       - 透明覆盖层窗口（用于绘制矩形和统计信息）
 */
WINVLCBRIDGE_API void* wv_create_player_for_view(
    void* hwnd_ptr, 
    float x, 
    float y, 
    float width, 
    float height
);

/**
 * 释放播放器资源并销毁关联窗口
 * 
 * @param playerHandle 播放器句柄
 * 
 * @note 该函数会清理：
 *       - 停止播放
 *       - 释放媒体对象和播放器
 *       - 销毁视频窗口和覆盖层窗口
 *       - 释放VLC实例
 */
WINVLCBRIDGE_API void wv_player_release(void* playerHandle);

// ==================== 播放控制 ====================

/**
 * 播放视频源（本地文件或网络流）
 * 
 * @param playerHandle 播放器句柄
 * @param source 视频源路径
 *               - 本地文件：支持绝对路径和相对路径
 *               - 网络流：支持 http://, https://, rtsp://, rtmp://, rtp:// 等协议
 * 
 * @note 播放过程：
 *       - 自动检测是本地文件还是网络流
 *       - 创建对应的媒体对象
 *       - 开始播放并等待视频输出准备就绪
 *       - 自动设置视频缩放模式（保持宽高比）
 */
WINVLCBRIDGE_API void wv_player_play(void* playerHandle, const char* source);

/**
 * 暂停播放
 * 
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_pause(void* playerHandle);

/**
 * 恢复播放（从暂停状态继续）
 * 
 * @param playerHandle 播放器句柄
 * 
 * @note 仅在暂停状态下有效
 */
WINVLCBRIDGE_API void wv_player_resume(void* playerHandle);

/**
 * 停止播放
 * 
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_stop(void* playerHandle);

// ==================== 覆盖层矩形绘制 ====================

/**
 * 更新覆盖层的矩形列表（实时绘制矩形框）
 * 
 * @param playerHandle 播放器句柄
 * @param rects 矩形数组，格式为 [x1, y1, width1, height1, x2, y2, width2, height2, ...]
 *              坐标系：左上角为原点(0,0)，单位为像素
 * @param rectCount 矩形的数量
 * @param lineWidth 矩形边框宽度（像素）
 * @param red 红色分量 (0.0 - 1.0)
 * @param green 绿色分量 (0.0 - 1.0)
 * @param blue 蓝色分量 (0.0 - 1.0)
 * @param alpha 透明度 (0.0 - 1.0, 1.0为完全不透明)
 * 
 * @note 该函数会立即刷新覆盖层窗口显示新的矩形
 */
WINVLCBRIDGE_API void wv_player_update_rectangles(
    void* playerHandle, 
    const float* rects, 
    int rectCount, 
    float lineWidth, 
    float red, 
    float green, 
    float blue, 
    float alpha
);

/**
 * 清除覆盖层的所有矩形
 * 
 * @param playerHandle 播放器句柄
 */
WINVLCBRIDGE_API void wv_player_clear_rectangles(void* playerHandle);

// ==================== 统计信息 ====================

/**
 * 获取播放统计信息
 * 
 * @param playerHandle 播放器句柄
 * @param buffer 用于存储统计信息文本的缓冲区
 * @param bufferSize 缓冲区大小（字节）
 * @return 是否成功获取统计信息
 * 
 * @note 统计信息包括：
 *       - 播放状态（Playing/Paused/Buffering等）
 *       - 码率
 *       - 读取和解复用的数据量
 *       - 解码和显示的帧数
 *       - 丢帧数和解码延迟
 *       - 当前播放时间
 */
WINVLCBRIDGE_API bool wv_player_get_stats(
    void* playerHandle, 
    char* buffer, 
    int bufferSize
);

/**
 * 更新统计信息在覆盖层的显示
 * 
 * @param playerHandle 播放器句柄
 * @param statsText 统计信息文本（UTF-8编码），显示在视频右上角
 * @param show 是否显示统计信息（true=显示，false=隐藏）
 * 
 * @note 统计信息会以半透明黑色背景、亮绿色文本显示在视频右上角
 */
WINVLCBRIDGE_API void wv_player_update_stats_display(
    void* playerHandle, 
    const char* statsText, 
    bool show
);

#ifdef __cplusplus
}
#endif

#endif // WIN_VLC_BRIDGE_H
