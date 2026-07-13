import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

// https://vite.dev/config/
export default defineConfig({
  plugins: [vue()],
  server: {
    // 开发服务器配置
    proxy: {
      // 代理API请求到网关设备
      '/api': {
        target: 'http://192.168.1.101:5000',
        changeOrigin: true,
        rewrite: (path) => path
      }
    }
  }
})
