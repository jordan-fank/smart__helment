@echo off
echo ========================================
echo 停止所有Python进程
echo ========================================
echo.

taskkill /F /IM python.exe 2>nul
if errorlevel 1 (
    echo 没有运行的Python进程
) else (
    echo Python进程已停止
)

echo.
echo 等待3秒...
timeout /t 3 /nobreak >nul

echo.
echo ========================================
echo 启动新的后端服务器
echo ========================================
echo.

python backend.py

pause
