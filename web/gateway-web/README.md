# 校园网关管理系统 - Web前端

## 项目介绍

本项目是一个运行在Windows浏览器中的校园网关管理系统前端应用。通过局域网WiFi连接校园网关设备(网关默认地址：`http://192.168.1.101:5000`)，提供了一套完整的管理界面，支持课表管理、学生留言查询、公告发布、图书馆座位预约信息查询和考勤记录查看等功能。

所有数据操作都通过HTTP API与网关设备的SQLite数据库交互，实现数据的实时读写。

## 功能特性

### 1. 课表管理
- 查看所有课程信息（课程名称、教室、教师、上课时间、容量）
- 添加新课表记录
- 删除课表记录

### 2. 学生留言
- 查看所有学生留言（标题、内容、学号、发布时间）
- 删除留言记录
- 支持留言信息刷新

### 3. 公告管理
- 查看所有公告信息
- 发布新公告（标题、内容、发布者）
- 删除公告记录

### 4. 图书馆座位预约
- 查看图书馆所有座位的预约状态
- 统计已预约和可用座位数
- 显示座位详细信息（座位号、位置、预约人、预约时间）

### 5. 考勤管理
- 查看详细的考勤记录
- 按考勤状态筛选（出席、缺席、迟到、请假）
- 统计各类型考勤数据
- 显示学生签到时间和备注信息

## 技术栈

| 技术 | 版本 | 说明 |
|------|------|------|
| Vue | 3+ | 前端框架 |
| Vite | 最新 | 构建工具 |
| Element Plus | 最新 | UI组件库 |
| Axios | 最新 | HTTP请求库 |
| Vue Router | 4+ | 路由管理 |

## 项目结构

```
gateway-web/
├── src/
│   ├── api/
│   │   └── gateway.js          # API服务模块（封装所有HTTP请求）
│   ├── router/
│   │   └── index.js            # 路由配置
│   ├── views/
│   │   ├── Courses.vue         # 课表管理页面
│   │   ├── Messages.vue        # 学生留言页面
│   │   ├── Notices.vue         # 公告管理页面
│   │   ├── Library.vue         # 图书馆座位页面
│   │   └── Attendance.vue      # 考勤管理页面
│   ├── App.vue                 # 主应用组件（导航栏 + 页面容器）
│   ├── main.js                 # 应用入口
│   └── style.css               # 全局样式
├── index.html                  # HTML模板
├── package.json                # 项目配置
├── vite.config.js              # Vite配置（包含API代理配置）
└── README.md                   # 本文件
```

## 部署方法

### 1. 前置要求
- Node.js 14.0+ 和 npm/pnpm 已安装
- 校园网关设备已启动且IP为 `192.168.1.101`（如有变动，修改 `src/api/gateway.js` 中的 `GATEWAY_URL`）
- Windows系统具有WiFi连接功能

### 2. 开发环境运行

```bash
# 安装依赖
npm install
# 或使用pnpm
pnpm install

# 启动开发服务器
npm run dev
# 或使用pnpm
pnpm dev

# 访问应用
# 在浏览器中打开 http://localhost:5173
```

### 3. 生产环境打包

```bash
# 构建项目
npm run build
# 或使用pnpm
pnpm build

# 构建完成后，dist 文件夹中包含所有静态文件
```

### 4. 部署到Windows

#### 方式一：使用Node.js内置服务器（推荐快速部署）

```bash
# 全局安装serve
npm install -g serve

# 进入项目目录，启动服务器
serve -s dist

# 默认在 http://localhost:3000 启动
```

#### 方式二：使用Python简单服务器

```bash
# 进入dist目录
cd dist

# Python 3.x
python -m http.server 8000

# 访问 http://localhost:8000
```

#### 方式三：使用IIS部署（适合企业环境）

1. 安装IIS Web Server
2. 将dist文件夹内容复制到IIS网站目录
3. 配置IIS为该应用作为静态文件服务器

#### 方式四：使用Apache部署

1. 将dist文件夹内容复制到Apache网站目录
2. 配置.htaccess文件以支持Vue Router单页应用路由

## 配置修改

### 修改网关地址

如果网关地址不是 `http://192.168.1.101:5000`，需要修改以下文件：

**文件路径：** `src/api/gateway.js`

```javascript
// 第5行 - 修改为实际的网关地址
const GATEWAY_URL = 'http://your.gateway.ip:port'
```

**文件路径：** `vite.config.js`（开发环境代理）

```javascript
// 修改代理目标
proxy: {
  '/api': {
    target: 'http://your.gateway.ip:port',  // 修改此处
    changeOrigin: true,
    rewrite: (path) => path
  }
}
```

## API接口说明

所有API调用都通过 `src/api/gateway.js` 封装，具体接口如下：

### 课表API
- `GET /api/courses` - 获取所有课表
- `POST /api/courses` - 添加课表
- `DELETE /api/courses/{id}` - 删除课表

### 留言API
- `GET /api/messages` - 获取所有留言
- `DELETE /api/messages/{id}` - 删除留言

### 公告API
- `GET /api/notices` - 获取所有公告
- `POST /api/notices` - 发布公告
- `DELETE /api/notices/{id}` - 删除公告

### 图书馆API
- `GET /api/library/seats` - 获取座位信息
- `GET /api/library/my-reservations` - 获取我的预约

### 考勤API
- `GET /api/attendance` - 获取考勤记录

## 故障排除

### 问题1：无法连接到网关

**症状：** 页面加载时出现"API错误"或"连接超时"

**解决方案：**
1. 检查网关设备是否已启动
2. 检查Windows计算机是否已连接到WiFi网络
3. 检查网关IP地址是否正确（运行 `ipconfig` 查看局域网设置）
4. 修改 `src/api/gateway.js` 中的 `GATEWAY_URL` 为正确的网关地址

### 问题2：跨域错误

**症状：** 浏览器控制台出现CORS错误

**解决方案：**
1. 确保开发环境正在运行 `npm run dev`（此时vite会代理请求）
2. 如果已部署到生产环境，需要在网关侧配置CORS头，或使用代理服务器

### 问题3：页面加载缓慢

**症状：** 应用启动或数据加载速度慢

**解决方案：**
1. 检查网络连接质量
2. 检查网关设备是否过载
3. 尝试关闭浏览器扩展程序

## 开发建议

### 添加新功能步骤

1. **创建新页面组件**
   - 在 `src/views/` 目录下创建 `.vue` 文件

2. **添加API调用**
   - 在 `src/api/gateway.js` 中添加新的API方法

3. **配置路由**
   - 在 `src/router/index.js` 中添加新路由

4. **更新主菜单**
   - 在 `src/App.vue` 中的 `menuItems` 数组添加新菜单项

### 代码规范

- 使用Vue 3 Composition API（`<script setup>` 语法）
- 所有组件都应该有中文注释说明功能
- 使用Element Plus组件保持UI统一风格
- API错误需要用ElMessage显示给用户

## 许可证

内部使用项目

## 联系方式

如有任何问题或建议，请联系系统管理员。
