/**
 * WinVLCBridge 使用示例
 * 
 * 本文件展示如何在 Node.js/Electron 中使用 WinVLCBridge.dll
 */

const ffi = require('ffi-napi');
const ref = require('ref-napi');
const path = require('path');

// DLL 路径
const dllPath = path.join(__dirname, 'build', 'bin', 'Release', 'WinVLCBridge.dll');

// 定义 FFI 接口
const WinVLCBridge = ffi.Library(dllPath, {
    // 创建播放器
    'wv_create_player_for_view': ['pointer', ['pointer', 'float', 'float', 'float', 'float']],
    
    // 播放控制
    'wv_player_play': ['void', ['pointer', 'string']],
    'wv_player_pause': ['void', ['pointer']],
    'wv_player_resume': ['void', ['pointer']],
    'wv_player_stop': ['void', ['pointer']],
    'wv_player_release': ['void', ['pointer']],
    
    // 矩形覆盖层
    'wv_player_update_rectangles': ['void', ['pointer', 'pointer', 'int', 'float', 'float', 'float', 'float', 'float']],
    'wv_player_clear_rectangles': ['void', ['pointer']]
});

// ==================== 封装类 ====================

class VLCPlayer {
    constructor(hwnd, x, y, width, height) {
        // hwnd 是 Windows 窗口句柄（从 Electron 的 BrowserWindow 获取）
        const hwndBuffer = ref.alloc('pointer', hwnd);
        this.playerHandle = WinVLCBridge.wv_create_player_for_view(hwndBuffer, x, y, width, height);
        
        if (this.playerHandle.isNull()) {
            throw new Error('Failed to create VLC player');
        }
        
        console.log('[VLCPlayer] 播放器创建成功');
    }
    
    /**
     * 播放视频
     * @param {string} source - 视频源路径（本地文件或网络流 URL）
     */
    play(source) {
        if (!this.playerHandle) {
            throw new Error('Player not initialized');
        }
        WinVLCBridge.wv_player_play(this.playerHandle, source);
        console.log('[VLCPlayer] 开始播放:', source);
    }
    
    /**
     * 暂停播放
     */
    pause() {
        if (!this.playerHandle) return;
        WinVLCBridge.wv_player_pause(this.playerHandle);
        console.log('[VLCPlayer] 已暂停');
    }
    
    /**
     * 恢复播放
     */
    resume() {
        if (!this.playerHandle) return;
        WinVLCBridge.wv_player_resume(this.playerHandle);
        console.log('[VLCPlayer] 已恢复');
    }
    
    /**
     * 停止播放
     */
    stop() {
        if (!this.playerHandle) return;
        WinVLCBridge.wv_player_stop(this.playerHandle);
        console.log('[VLCPlayer] 已停止');
    }
    
    /**
     * 更新矩形框（用于目标检测等场景）
     * @param {Array<Object>} rectangles - 矩形数组，每个对象包含 {x, y, width, height}
     * @param {number} lineWidth - 线宽
     * @param {Object} color - 颜色对象 {r, g, b, a}，值范围 0.0-1.0
     */
    updateRectangles(rectangles, lineWidth = 2.0, color = {r: 0, g: 1, b: 0, a: 1}) {
        if (!this.playerHandle) return;
        
        // 将矩形数组转换为 float 数组
        const rectCount = rectangles.length;
        const rectArray = new Float32Array(rectCount * 4);
        
        rectangles.forEach((rect, index) => {
            rectArray[index * 4] = rect.x;
            rectArray[index * 4 + 1] = rect.y;
            rectArray[index * 4 + 2] = rect.width;
            rectArray[index * 4 + 3] = rect.height;
        });
        
        // 创建 Buffer
        const rectBuffer = Buffer.from(rectArray.buffer);
        
        WinVLCBridge.wv_player_update_rectangles(
            this.playerHandle,
            rectBuffer,
            rectCount,
            lineWidth,
            color.r,
            color.g,
            color.b,
            color.a
        );
        
        console.log(`[VLCPlayer] 已更新 ${rectCount} 个矩形`);
    }
    
    /**
     * 清除所有矩形框
     */
    clearRectangles() {
        if (!this.playerHandle) return;
        WinVLCBridge.wv_player_clear_rectangles(this.playerHandle);
        console.log('[VLCPlayer] 已清除所有矩形');
    }
    
    /**
     * 释放播放器资源
     */
    release() {
        if (!this.playerHandle) return;
        WinVLCBridge.wv_player_release(this.playerHandle);
        this.playerHandle = null;
        console.log('[VLCPlayer] 播放器已释放');
    }
}

// ==================== Electron 集成示例 ====================

/**
 * 在 Electron 主进程中使用
 */
function electronExample() {
    const { BrowserWindow } = require('electron');
    
    // 创建 Electron 窗口
    const win = new BrowserWindow({
        width: 1280,
        height: 720,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    
    // 获取窗口句柄（Windows 特定）
    const hwnd = win.getNativeWindowHandle();
    
    // 创建播放器（位置 x=50, y=50, 大小 800x600）
    const player = new VLCPlayer(hwnd, 50, 50, 800, 600);
    
    // 播放本地视频
    player.play('C:/Videos/sample.mp4');
    
    // 或播放网络流
    // player.play('rtsp://example.com/stream');
    
    // 3秒后添加一些矩形框（模拟目标检测）
    setTimeout(() => {
        const rectangles = [
            { x: 100, y: 100, width: 200, height: 150 },
            { x: 400, y: 200, width: 150, height: 100 }
        ];
        
        // 绿色矩形，线宽3
        player.updateRectangles(rectangles, 3.0, { r: 0, g: 1, b: 0, a: 1 });
    }, 3000);
    
    // 6秒后清除矩形
    setTimeout(() => {
        player.clearRectangles();
    }, 6000);
    
    // 10秒后暂停
    setTimeout(() => {
        player.pause();
    }, 10000);
    
    // 12秒后恢复
    setTimeout(() => {
        player.resume();
    }, 12000);
    
    // 窗口关闭时清理
    win.on('closed', () => {
        player.stop();
        player.release();
    });
    
    return { win, player };
}

// ==================== 完整的使用示例 ====================

/**
 * 完整示例：播放视频并实时显示矩形框
 */
class VideoPlayerWithDetection {
    constructor(hwnd, x, y, width, height) {
        this.player = new VLCPlayer(hwnd, x, y, width, height);
        this.detectionInterval = null;
    }
    
    playWithDetection(videoSource) {
        // 播放视频
        this.player.play(videoSource);
        
        // 模拟实时检测（每秒更新一次矩形）
        this.startDetectionSimulation();
    }
    
    startDetectionSimulation() {
        let frameCount = 0;
        
        this.detectionInterval = setInterval(() => {
            frameCount++;
            
            // 模拟检测结果：随机生成一些矩形
            const rectangles = [];
            const numObjects = Math.floor(Math.random() * 3) + 1; // 1-3个目标
            
            for (let i = 0; i < numObjects; i++) {
                rectangles.push({
                    x: Math.random() * 600 + 100,
                    y: Math.random() * 400 + 100,
                    width: Math.random() * 100 + 50,
                    height: Math.random() * 100 + 50
                });
            }
            
            // 更新矩形（红色警告框）
            this.player.updateRectangles(rectangles, 2.5, { r: 1, g: 0, b: 0, a: 0.8 });
            
            console.log(`帧 ${frameCount}: 检测到 ${numObjects} 个目标`);
        }, 1000);
    }
    
    stopDetection() {
        if (this.detectionInterval) {
            clearInterval(this.detectionInterval);
            this.detectionInterval = null;
        }
        this.player.clearRectangles();
    }
    
    cleanup() {
        this.stopDetection();
        this.player.stop();
        this.player.release();
    }
}

// ==================== 导出 ====================

module.exports = {
    VLCPlayer,
    VideoPlayerWithDetection,
    electronExample
};

// ==================== 使用说明 ====================

/*

1. 安装依赖：
   npm install ffi-napi ref-napi

2. 确保 DLL 文件在正确位置：
   WinVLCBridge/build/bin/Release/WinVLCBridge.dll
   WinVLCBridge/build/bin/Release/libvlc.dll
   WinVLCBridge/build/bin/Release/libvlccore.dll
   WinVLCBridge/build/bin/Release/plugins/

3. 在 Electron 主进程中使用：
   const { VLCPlayer } = require('./example_usage.js');
   const hwnd = mainWindow.getNativeWindowHandle();
   const player = new VLCPlayer(hwnd, 0, 0, 800, 600);
   player.play('path/to/video.mp4');

4. 矩形坐标说明：
   - x, y: 矩形左上角坐标（相对于视频窗口）
   - width, height: 矩形宽度和高度
   - 坐标系统：左上角为原点 (0, 0)

5. 支持的视频源：
   - 本地文件：'C:/Videos/sample.mp4'
   - HTTP流：'http://example.com/stream.m3u8'
   - RTSP流：'rtsp://example.com/stream'
   - RTMP流：'rtmp://example.com/stream'

*/

