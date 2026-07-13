# 网关地址配置指南

## 快速配置

默认情况下，应用连接到网关地址：`http://192.168.1.101:5000`

如果你的网关地址不同，请按照下面的步骤进行配置。

## 方法一：修改源代码配置（推荐开发阶段）

### 步骤1：打开API配置文件

找到文件：`src/api/gateway.js`

### 步骤2：修改GATEWAY_URL

在第5行找到以下代码：
```javascript
const GATEWAY_URL = 'http://192.168.1.101:5000'
```

将其修改为你的实际网关地址：
```javascript
const GATEWAY_URL = 'http://your.gateway.ip:your.port'
```

例如：
```javascript
const GATEWAY_URL = 'http://192.168.0.100:5000'
const GATEWAY_URL = 'http://gateway.local:8080'
const GATEWAY_URL = 'http://10.0.0.50:5000'
```

### 步骤3：修改开发环境代理

打开文件：`vite.config.js`

在proxy配置中修改target：
```javascript
server: {
  proxy: {
    '/api': {
      target: 'http://your.gateway.ip:your.port',  // 修改此处
      changeOrigin: true,
      rewrite: (path) => path
    }
  }
}
```

## 方法二：运行时配置（生产环境推荐）

你可以创建一个配置文件来在运行时设置网关地址。

### 步骤1：创建配置文件

在项目根目录创建 `config.json` 文件：

```json
{
  "gatewayUrl": "http://192.168.1.101:5000",
  "timeout": 5000
}
```

### 步骤2：在应用启动时加载配置

修改 `src/main.js`：

```javascript
import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import './style.css'
import App from './App.vue'
import router from './router'

// 加载配置
fetch('/config.json')
  .then(response => response.json())
  .then(config => {
    // 在全局属性中存储配置
    window.$config = config
  })
  .catch(() => {
    // 如果文件不存在，使用默认配置
    window.$config = { gatewayUrl: 'http://192.168.1.101:5000' }
  })
  .finally(() => {
    const app = createApp(App)
    app.use(ElementPlus)
    app.use(router)
    app.mount('#app')
  })
```

### 步骤3：在API服务中使用配置

修改 `src/api/gateway.js`：

```javascript
// 使用全局配置的网关地址
const GATEWAY_URL = window.$config?.gatewayUrl || 'http://192.168.1.101:5000'
```

## 如何确定你的网关地址

### 在Windows系统中查看

1. 打开 `命令提示符` (Win+R，输入cmd)
2. 输入命令：`ipconfig`
3. 查看局域网中的IPv4地址
4. 网关通常与你的电脑在同一网段

### 示例输出：
```
以太网适配器 Ethernet:
   IPv4 地址 . . . . . . . . . . . . : 192.168.1.100
   子网掩码  . . . . . . . . . . . . : 255.255.255.0
   默认网关. . . . . . . . . . . . . : 192.168.1.1
```

你的校园网关应该在该网段内，例如 `192.168.1.101`

### 测试连接

在命令提示符中输入：
```bash
ping 192.168.1.101
```

如果能收到回复，说明网关设备在线且网络连接正常。

## 常见错误

### 错误1：Connection refused
```
错误信息：API错误: connect ECONNREFUSED 192.168.1.101:5000
```

**解决方案：**
1. 检查网关设备IP地址是否正确
2. 确认网关设备已启动
3. 确认你的计算机连接到了正确的WiFi网络
4. 尝试 `ping` 网关地址验证连接

### 错误2：CORS跨域错误
```
错误信息：Access to XMLHttpRequest blocked by CORS policy
```

**解决方案：**
1. 如果在开发环境：确保运行了 `npm run dev` (vite会代理请求)
2. 如果在生产环境：需要在网关侧配置CORS响应头

### 错误3：网络超时
```
错误信息：API错误: timeout of 5000ms exceeded
```

**解决方案：**
1. 检查网络连接质量
2. 尝试增加超时时间，修改 `src/api/gateway.js`：
```javascript
const api = axios.create({
  baseURL: GATEWAY_URL,
  timeout: 10000,  // 改为10秒
})
```

## 验证配置

### 在浏览器开发工具中查看

1. 打开应用，按 F12 打开开发者工具
2. 切换到 Network 标签页
3. 点击任意功能按钮
4. 查看请求的URL是否正确

### 查看网关连接状态

打开浏览器开发者工具的 Console 标签，输入：
```javascript
console.log('当前网关地址:', window.$config?.gatewayUrl || 'http://192.168.1.101:5000')
```

## 更多帮助

如果配置仍然无法连接，请检查：

1. 网关是否运行在HTTP模式（不支持HTTPS）
2. 防火墙是否阻止了连接（尝试临时关闭防火墙测试）
3. 网关是否已正确启动并监听5000端口
4. 局域网中是否有其他设备也运行在该IP地址上（IP冲突）
