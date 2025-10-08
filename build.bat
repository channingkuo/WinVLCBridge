@echo off
REM WinVLCBridge 快速编译脚本
REM 使用方法：双击运行或在命令行执行 build.bat [debug|release]

setlocal enabledelayedexpansion

echo ========================================
echo WinVLCBridge 编译脚本
echo ========================================
echo.

REM 设置编译类型（默认 Release）
set BUILD_TYPE=Release
if not "%1"=="" (
    if /i "%1"=="debug" set BUILD_TYPE=Debug
    if /i "%1"=="release" set BUILD_TYPE=Release
)

echo 编译类型: %BUILD_TYPE%
echo.

REM 检查是否存在 VLC SDK
if not exist "vlc-3.0.21\sdk\include\vlc\vlc.h" (
    echo [错误] 找不到 VLC SDK
    echo.
    echo 请确保 vlc-3.0.21\sdk 目录存在，包含：
    echo   - vlc-3.0.21\sdk\include\vlc\vlc.h
    echo   - vlc-3.0.21\sdk\lib\libvlc.lib
    echo   - vlc-3.0.21\sdk\lib\libvlccore.lib
    echo.
    echo 如果没有 SDK，请：
    echo 1. 访问 https://www.videolan.org/vlc/download-windows.html
    echo 2. 下载 VLC SDK
    echo 3. 解压到 vlc-3.0.21\ 目录
    echo.
    pause
    exit /b 1
)

REM 检查是否安装了 CMake
where cmake >nul 2>nul
if errorlevel 1 (
    echo [错误] 找不到 CMake
    echo.
    echo 请先安装 CMake:
    echo 1. 访问 https://cmake.org/download/
    echo 2. 下载并安装 CMake
    echo 3. 确保 CMake 已添加到 PATH
    echo.
    echo 或者使用 Visual Studio 自带的 CMake
    pause
    exit /b 1
)

REM 创建 build 目录
if not exist "build" (
    echo 创建 build 目录...
    mkdir build
)

cd build

REM 配置 CMake
echo.
echo ========================================
echo 正在配置 CMake...
echo ========================================
cmake .. -G "Visual Studio 16 2019" -A x64
if errorlevel 1 (
    echo.
    echo [错误] CMake 配置失败
    echo.
    echo 如果你使用的是 Visual Studio 2022，请修改脚本中的生成器为：
    echo   cmake .. -G "Visual Studio 17 2022" -A x64
    echo.
    echo 或者使用其他版本：
    echo   Visual Studio 15 2017
    echo   Visual Studio 14 2015
    echo.
    cd ..
    pause
    exit /b 1
)

REM 编译项目
echo.
echo ========================================
echo 正在编译项目 (%BUILD_TYPE%)...
echo ========================================
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo.
    echo [错误] 编译失败
    echo.
    echo 请检查：
    echo 1. Visual Studio 是否正确安装
    echo 2. Windows SDK 是否已安装
    echo 3. VLC SDK 路径是否正确
    echo.
    cd ..
    pause
    exit /b 1
)

cd ..

REM 检查输出文件
set OUTPUT_DIR=build\bin\%BUILD_TYPE%
if exist "%OUTPUT_DIR%\WinVLCBridge.dll" (
    echo.
    echo ========================================
    echo 编译成功！
    echo ========================================
    echo.
    echo 输出文件位于:
    echo   %OUTPUT_DIR%\WinVLCBridge.dll
    echo   %OUTPUT_DIR%\libvlc.dll
    echo   %OUTPUT_DIR%\libvlccore.dll
    echo   %OUTPUT_DIR%\plugins\
    echo.
    
    REM 显示文件大小
    for %%f in ("%OUTPUT_DIR%\WinVLCBridge.dll") do (
        set size=%%~zf
        set /a size_kb=!size! / 1024
        echo WinVLCBridge.dll: !size_kb! KB
    )
    
    echo.
    echo 下一步:
    echo 1. 查看 example_usage.js 了解如何使用
    echo 2. 将 DLL 文件复制到你的 Electron 项目
    echo 3. 使用 ffi-napi 加载 DLL
    echo.
) else (
    echo.
    echo [警告] 找不到输出文件: %OUTPUT_DIR%\WinVLCBridge.dll
    echo.
)

echo 按任意键退出...
pause >nul

