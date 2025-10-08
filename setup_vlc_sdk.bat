@echo off
REM VLC SDK 自动设置脚本
REM 适用于 Windows

setlocal enabledelayedexpansion

echo ==========================================
echo VLC SDK 自动设置脚本
echo ==========================================
echo.

set VLC_VERSION=3.0.20
set NUGET_URL=https://www.nuget.org/api/v2/package/VideoLAN.LibVLC.Windows/%VLC_VERSION%
set TARGET_DIR=vlc-3.0.21

echo 步骤 1: 下载 VLC SDK (NuGet 包)...
powershell -Command "Invoke-WebRequest -Uri '%NUGET_URL%' -OutFile 'libvlc.zip'"
if errorlevel 1 (
    echo [错误] 下载失败，请检查网络连接
    pause
    exit /b 1
)

echo.
echo 步骤 2: 解压...
powershell -Command "Expand-Archive -Path 'libvlc.zip' -DestinationPath 'libvlc-nuget' -Force"

echo.
echo 步骤 3: 创建目录结构...
if not exist "%TARGET_DIR%\sdk\include" mkdir "%TARGET_DIR%\sdk\include"
if not exist "%TARGET_DIR%\sdk\lib" mkdir "%TARGET_DIR%\sdk\lib"

echo.
echo 步骤 4: 复制头文件...
xcopy /E /I /Y "libvlc-nuget\build\x64\include\*" "%TARGET_DIR%\sdk\include\"

echo.
echo 步骤 5: 复制库文件...
copy /Y "libvlc-nuget\build\x64\libvlc.lib" "%TARGET_DIR%\sdk\lib\"
copy /Y "libvlc-nuget\build\x64\libvlccore.lib" "%TARGET_DIR%\sdk\lib\"

echo.
echo 步骤 6: 复制 DLL 和插件...
copy /Y "libvlc-nuget\build\x64\*.dll" "%TARGET_DIR%\"
xcopy /E /I /Y "libvlc-nuget\build\x64\plugins" "%TARGET_DIR%\plugins\"

echo.
echo 步骤 7: 清理临时文件...
rmdir /s /q "libvlc-nuget"
del /q "libvlc.zip"

echo.
echo ==========================================
echo ✅ VLC SDK 设置完成！
echo ==========================================
echo.
echo 目录结构：
echo %TARGET_DIR%\
echo ├── sdk\
echo │   ├── include\vlc\
echo │   │   └── vlc.h ✅
echo │   └── lib\
echo │       ├── libvlc.lib ✅
echo │       └── libvlccore.lib ✅
echo ├── libvlc.dll ✅
echo ├── libvlccore.dll ✅
echo └── plugins\ ✅
echo.
echo 现在可以运行 build.bat 编译项目了！
echo.
pause

