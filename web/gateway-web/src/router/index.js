import { createRouter, createWebHistory } from 'vue-router'
import Courses from '../views/Courses.vue'
import Messages from '../views/Messages.vue'
import Notices from '../views/Notices.vue'
import Library from '../views/Library.vue'
import Attendance from '../views/Attendance.vue'

// 路由配置
const routes = [
  {
    path: '/',
    redirect: '/courses'
  },
  {
    path: '/courses',
    name: 'Courses',
    component: Courses
  },
  {
    path: '/messages',
    name: 'Messages',
    component: Messages
  },
  {
    path: '/notices',
    name: 'Notices',
    component: Notices
  },
  {
    path: '/library',
    name: 'Library',
    component: Library
  },
  {
    path: '/attendance',
    name: 'Attendance',
    component: Attendance
  }
]

// 创建路由器实例
const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes
})

export default router
