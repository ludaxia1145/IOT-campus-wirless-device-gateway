# 校园网关管理系统 - 快速开始指南

## 5分钟快速启动

### 前置要求
- Windows 7/8/10/11
- 已安装 Node.js 14.0+ (下载地址：https://nodejs.org)
- WiFi连接到校园网络，能够连接到网关设备

### 快速启动

#### 方法一：使用批处理脚本（最简单）

1. 找到项目文件夹中的 `start-dev.bat` 文件
2. 双击 `start-dev.bat` 运行
3. 等待服务器启动，会自动打开浏览器
4. 在浏览器中访问 `http://localhost:5173`

#### 方法二：使用命令行

```bash
# 打开命令提示符，进入项目目录
cd path/to/gateway-web

# 首次需要安装依赖
npm install

# 启动开发服务器
npm run dev

# 服务器启动后，在浏览器中访问 http://localhost:5173
```

## 功能说明

### 1. 课表管理 📚
- **查看课表**：显示所有已添加的课程信息
- **添加课表**：输入课程名称、教室、教师、时间和容量
- **删除课表**：移除不需要的课表记录

### 2. 学生留言 💬
- **查看留言**：显示所有学生留言，包括标题、内容和时间
- **删除留言**：移除学生留言记录
- **刷新列表**：手动更新留言列表

### 3. 公告管理 📢
- **查看公告**：显示所有发布的公告
- **发布公告**：创建新公告（标题、内容、发布者）
- **删除公告**：移除旧公告

### 4. 图书馆座位 📖
- **座位预约状态**：查看所有座位的预约情况
- **统计信息**：显示总座位数、已预约数和可用座位数
- **座位详情**：查看每个座位的位置、预约人和预约时间

### 5. 考勤管理 ✓
- **查看考勤**：显示详细的考勤记录
- **按状态筛选**：支持筛选出席、缺席、迟到、请假等状态
- **统计数据**：自动统计各类型考勤数据

## 网关地址配置

默认网关地址为 `http://192.168.1.101:5000`

### 如果需要修改网关地址

1. 打开 `src/api/gateway.js` 文件
2. 找到第5行的代码：
   ```javascript
   const GATEWAY_URL = 'http://192.168.1.101:5000'
   ```
3. 修改为你的实际网关地址，例如：
   ```javascript
   const GATEWAY_URL = 'http://192.168.0.100:5000'
   ```
4. 保存文件，重新启动开发服务器

详细配置说明请查看 `GATEWAY_CONFIG.md` 文件

## 项目文件结构

```
gateway-web/
├── src/                          # 源代码目录
│   ├── api/
│   │   └── gateway.js           # API服务接口封装
│   ├── router/
│   │   └── index.js             # 路由配置
│   ├── views/                   # 页面组件
│   │   ├── Courses.vue          # 课表管理
│   │   ├── Messages.vue         # 学生留言
│   │   ├── Notices.vue          # 公告管理
│   │   ├── Library.vue          # 图书馆座位
│   │   └── Attendance.vue       # 考勤管理
│   ├── App.vue                  # 主应用组件
│   ├── main.js                  # 应用入口
│   └── style.css                # 全局样式
├── dist/                         # 生产版本输出目录（npm run build 生成）
├── node_modules/                # npm 依赖包
├── index.html                   # HTML模板文件
├── package.json                 # 项目配置和依赖清单
├── vite.config.js               # Vite 构建工具配置
├── start-dev.bat                # Windows 快速启动脚本
├── build-prod.bat               # Windows 构建脚本
├── README.md                    # 完整项目文档
├── GATEWAY_CONFIG.md            # 网关地址配置指南
└── QUICK_START.md               # 本文件

```

## 常用命令

```bash
# 安装依赖（首次需要）
npm install

# 启动开发服务器
npm run dev

# 构建生产版本
npm run build

# 预览生产版本（必须先运行 npm run build）
npm run preview
```

## 故障排除

### 问题：无法连接到网关

**错误信息：** `API错误: connect ECONNREFUSED`

**解决方案：**
1. 检查网关IP地址是否正确
2. 确认网关设备已启动
3. 检查是否连接到正确的WiFi网络
4. 在命令提示符中运行 `ping 192.168.1.101` 测试连接

### 问题：Node.js 不是内部或外部命令

**解决方案：**
1. 确认已安装 Node.js
2. 重新启动计算机
3. 确保 Node.js 已添加到系统 PATH 环境变量中

### 问题：端口 5173 已被占用

**错误信息：** `EADDRINUSE: address already in use :::5173`

**解决方案：**
```bash
# 关闭已占用端口的程序，或使用其他端口
npm run dev -- --port 3000
```

### 问题：页面加载缓慢

**解决方案：**
1. 检查网络连接质量
2. 确认网关设备负载不过高
3. 尝试关闭浏览器扩展程序

## 生产环境部署

### 方式一：使用 serve（推荐）

```bash
# 全局安装 serve
npm install -g serve

# 启动生产服务器
serve -s dist

# 访问 http://localhost:3000
```

### 方式二：使用 Python

```bash
cd dist
python -m http.server 8000
# 访问 http://localhost:8000
```

### 方式三：IIS 部署

1. 复制 `dist` 文件夹中的所有文件
2. 在 IIS 中创建新网站，指向该文件夹
3. 配置 IIS 返回 index.html 用于路由

详细部署说明请查看 `README.md` 文件的部署方法部分

## 技术支持

如遇到问题，请检查：

1. **网络连接**
   - 确认 WiFi 已连接
   - 尝试 ping 网关地址

2. **Node.js 环境**
   - 运行 `node --version` 检查版本
   - 运行 `npm --version` 检查 npm 版本

3. **浏览器**
   - 尝试清空浏览器缓存
   - 尝试使用其他浏览器
   - 按 F12 查看控制台是否有错误信息

4. **网关设备**
   - 确认网关已启动
   - 尝试重启网关设备
   - 检查网关上是否有错误日志

## 进阶：添加新功能

如需添加新功能（例如新的管理模块），请参考以下步骤：

1. **创建新页面**
   - 在 `src/views/` 目录创建新 `.vue` 文件

2. **添加 API**
   - 在 `src/api/gateway.js` 中添加新的 API 方法

3. **配置路由**
   - 在 `src/router/index.js` 中添加新路由

4. **更新菜单**
   - 在 `src/App.vue` 的 `menuItems` 数组中添加新菜单项

详细开发指南请查看 `README.md`

---

**祝你使用愉快！** 🎉
