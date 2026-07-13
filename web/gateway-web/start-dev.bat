@echo off
REM 校园网关管理系统 - 开发环境启动脚本
REM 此脚本用于在Windows系统上启动开发服务器

echo ========================================
echo   校园网关管理系统 - 开发服务器
echo ========================================
echo.

REM 检查Node.js是否安装
node --version >nul 2>&1
if errorlevel 1 (
    echo 错误：未检测到Node.js！
    echo 请先从 https://nodejs.org 下载并安装Node.js
    pause
    exit /b 1
)

echo Node.js 版本：
node --version
echo.

REM 检查依赖是否已安装
if not exist "node_modules" (
    echo 未检测到 node_modules 目录，正在安装依赖...
    echo.
    call npm install
    if errorlevel 1 (
        echo 依赖安装失败！
        pause
        exit /b 1
    )
    echo.
)

echo 正在启动开发服务器...
echo.
echo 服务器地址：http://localhost:5173
echo.
echo 按 Ctrl+C 停止服务器
echo.

call npm run dev
pause
