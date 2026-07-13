@echo off
REM 校园网关管理系统 - 生产环境构建脚本
REM 此脚本用于在Windows系统上构建生产版本

echo ========================================
echo   校园网关管理系统 - 生产构建
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

echo 正在构建生产版本...
echo.

call npm run build

if errorlevel 1 (
    echo 构建失败！
    pause
    exit /b 1
)

echo.
echo ========================================
echo 构建成功！
echo ========================================
echo.
echo 生产文件已生成到 dist 文件夹
echo.
echo 后续部署方法：
echo 1. 使用 serve: npm install -g serve && serve -s dist
echo 2. 使用 Python: cd dist && python -m http.server 8000
echo 3. 复制 dist 文件夹到 IIS/Apache 网站目录
echo.
pause
