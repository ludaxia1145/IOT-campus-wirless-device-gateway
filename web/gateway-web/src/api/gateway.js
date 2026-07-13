import axios from 'axios'

// 网关服务器地址和端口
const GATEWAY_URL = 'http://192.168.1.101:5000'

// 创建axios实例
const api = axios.create({
  baseURL: GATEWAY_URL,
  timeout: 5000,
  headers: {
    'Content-Type': 'application/json'
  }
})

// 错误处理拦截器
api.interceptors.response.use(
  response => response,
  error => {
    console.error('API错误:', error.message)
    return Promise.reject(error)
  }
)

/**
 * 课表相关API
 */
export const courseAPI = {
  // 获取所有课表
  getCourses: () => api.get('/api/courses'),
  
  // 添加课表
  addCourse: (data) => api.post('/api/courses', data),
  
  // 删除课表
  deleteCourse: (courseId) => api.delete(`/api/courses/${courseId}`)
}

/**
 * 学生留言相关API
 */
export const messageAPI = {
  // 获取所有留言
  getMessages: () => api.get('/api/messages'),
  
  // 删除留言
  deleteMessage: (messageId) => api.delete(`/api/messages/${messageId}`)
}

/**
 * 公告相关API
 */
export const noticeAPI = {
  // 获取所有公告
  getNotices: () => api.get('/api/notices'),
  
  // 添加公告
  addNotice: (data) => api.post('/api/notices', data),
  
  // 删除公告
  deleteNotice: (noticeId) => api.delete(`/api/notices/${noticeId}`)
}

/**
 * 图书馆座位相关API
 */
export const libraryAPI = {
  // 获取所有座位信息
  getLibrarySeats: () => api.get('/api/library/seats'),
  
  // 获取我的预约信息
  getMyReservations: () => api.get('/api/library/my-reservations'),
  
  // 预约座位
  reserveSeat: (data) => api.post('/api/library/reserve', data),
  
  // 取消预约
  cancelReservation: (data) => api.post('/api/library/cancel', data)
}

/**
 * 考勤相关API
 */
export const attendanceAPI = {
  // 获取考勤记录
  getAttendance: () => api.get('/api/attendance')
}

/**
 * 健康检查
 */
export const healthCheck = () => api.get('/api/health')

export default api
