@echo off
REM 清理编译文件

echo ========================================
echo 清理 WinVLCBridge 编译文件
echo ========================================
echo.

if exist "build" (
    echo 删除 build 目录...
    rmdir /s /q build
    echo 已删除 build 目录
) else (
    echo build 目录不存在，无需清理
)

echo.
echo 清理完成！
echo.
pause

