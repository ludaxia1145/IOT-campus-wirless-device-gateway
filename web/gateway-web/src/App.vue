<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()
const activeTab = ref('courses')

// 导航菜单项
const menuItems = [
  { label: '课表管理', value: 'courses', icon: 'CalendarDate' },
  { label: '学生留言', value: 'messages', icon: 'ChatDotRound' },
  { label: '公告管理', value: 'notices', icon: 'Notification' },
  { label: '图书馆座位', value: 'library', icon: 'Document' },
  { label: '考勤记录', value: 'attendance', icon: 'User' }
]

// 处理菜单项点击
const handleMenuClick = (item) => {
  activeTab.value = item.value
  router.push(`/${item.value}`)
}

// 路由变化时更新activeTab
router.beforeEach((to) => {
  const path = to.path.replace('/', '')
  if (path) {
    activeTab.value = path
  }
})
</script>

<template>
  <div class="app-container">
    <!-- 侧边栏导航 -->
    <aside class="sidebar">
      <div class="logo">
        <h1>校园网关系统</h1>
      </div>
      <nav class="menu">
        <div
          v-for="item in menuItems"
          :key="item.value"
          :class="['menu-item', { active: activeTab === item.value }]"
          @click="handleMenuClick(item)"
        >
          <span class="menu-label">{{ item.label }}</span>
        </div>
      </nav>
    </aside>

    <!-- 主内容区域 -->
    <main class="main-content">
      <router-view />
    </main>
  </div>
</template>

<style scoped>
.app-container {
  display: flex;
  height: 100vh;
  background-color: #f5f7fa;
}

/* 侧边栏样式 */
.sidebar {
  width: 250px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  display: flex;
  flex-direction: column;
  box-shadow: 2px 0 10px rgba(0, 0, 0, 0.1);
}

.logo {
  padding: 30px 20px;
  border-bottom: 1px solid rgba(255, 255, 255, 0.2);
  text-align: center;
}

.logo h1 {
  margin: 0;
  font-size: 20px;
  font-weight: 600;
}

.menu {
  flex: 1;
  padding: 20px 0;
  overflow-y: auto;
}

.menu-item {
  padding: 16px 20px;
  cursor: pointer;
  transition: all 0.3s ease;
  border-left: 3px solid transparent;
  display: flex;
  align-items: center;
}

.menu-item:hover {
  background-color: rgba(255, 255, 255, 0.1);
}

.menu-item.active {
  background-color: rgba(255, 255, 255, 0.2);
  border-left-color: white;
  font-weight: 600;
}

.menu-label {
  flex: 1;
}

/* 主内容区域 */
.main-content {
  flex: 1;
  overflow-y: auto;
  padding: 30px;
}
</style>
