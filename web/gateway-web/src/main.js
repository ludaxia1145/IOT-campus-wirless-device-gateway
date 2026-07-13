import { createApp } from 'vue'
import ElementPlus from 'element-plus'
import 'element-plus/dist/index.css'
import './style.css'
import App from './App.vue'
import router from './router'

// 创建Vue应用实例
const app = createApp(App)

// 使用Element Plus UI组件库
app.use(ElementPlus)

// 使用路由
app.use(router)

// 挂载应用到DOM
app.mount('#app')
