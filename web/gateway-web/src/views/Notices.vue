<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { noticeAPI } from '../api/gateway'

// 数据
const notices = ref([])
const loading = ref(false)
const showDialog = ref(false)

// 表单数据
const formData = ref({
  title: '',
  content: '',
  author: ''
})

// 获取公告列表
const fetchNotices = async () => {
  loading.value = true
  try {
    const response = await noticeAPI.getNotices()
    notices.value = response.data.notices || []
  } catch (error) {
    ElMessage.error('获取公告失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 打开发布公告对话框
const openAddDialog = () => {
  formData.value = {
    title: '',
    content: '',
    author: ''
  }
  showDialog.value = true
}

// 发布公告
const handleAddNotice = async () => {
  try {
    // 验证表单数据
    if (!formData.value.title || !formData.value.content || !formData.value.author) {
      ElMessage.warning('请填写所有字段')
      return
    }

    loading.value = true
    await noticeAPI.addNotice(formData.value)
    ElMessage.success('公告发布成功')
    showDialog.value = false
    fetchNotices()
  } catch (error) {
    ElMessage.error('发布公告失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 删除公告
const handleDeleteNotice = async (noticeId) => {
  try {
    // 确认删除
    if (!confirm('确定要删除这条公告吗？')) {
      return
    }

    loading.value = true
    await noticeAPI.deleteNotice(noticeId)
    ElMessage.success('公告删除成功')
    fetchNotices()
  } catch (error) {
    ElMessage.error('删除公告失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 页面挂载时获取数据
onMounted(() => {
  fetchNotices()
})
</script>

<template>
  <div class="notices-container">
    <!-- 页面标题 -->
    <div class="page-header">
      <h2>公告管理</h2>
      <el-button type="primary" @click="openAddDialog">发布公告</el-button>
    </div>

    <!-- 公告列表 -->
    <div v-if="notices.length > 0" class="notices-list">
      <div v-for="notice in notices" :key="notice.notice_id" class="notice-card">
        <div class="notice-header">
          <h3 class="notice-title">{{ notice.title }}</h3>
          <el-button link type="danger" size="small" @click="handleDeleteNotice(notice.notice_id)">
            删除
          </el-button>
        </div>
        <div class="notice-meta">
          <span class="notice-author">发布者：{{ notice.author }}</span>
          <span class="notice-time">{{ notice.timestamp }}</span>
        </div>
        <div class="notice-content">
          {{ notice.content }}
        </div>
      </div>
    </div>

    <!-- 空状态 -->
    <el-empty v-else description="暂无公告" />

    <!-- 发布公告对话框 -->
    <el-dialog v-model="showDialog" title="发布公告" width="600px" @close="showDialog = false">
      <div class="form-group">
        <label class="form-label">公告标题</label>
        <el-input v-model="formData.title" placeholder="请输入公告标题" />
      </div>

      <div class="form-group">
        <label class="form-label">公告内容</label>
        <el-input
          v-model="formData.content"
          placeholder="请输入公告内容"
          type="textarea"
          :rows="6"
        />
      </div>

      <div class="form-group">
        <label class="form-label">发布者</label>
        <el-input v-model="formData.author" placeholder="请输入发布者名称" />
      </div>

      <template #footer>
        <el-button @click="showDialog = false">取消</el-button>
        <el-button type="primary" @click="handleAddNotice" :loading="loading">
          发布
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<style scoped>
.notices-container {
  background: white;
  border-radius: 8px;
  padding: 20px;
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.page-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding-bottom: 15px;
  border-bottom: 1px solid #e0e0e0;
}

.page-header h2 {
  margin: 0;
  color: #333;
  font-size: 24px;
}

.notices-list {
  display: flex;
  flex-direction: column;
  gap: 15px;
  margin-top: 20px;
}

.notice-card {
  border-left: 4px solid #667eea;
  background: #f9f9f9;
  border-radius: 6px;
  padding: 15px;
  transition: box-shadow 0.3s ease;
}

.notice-card:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.notice-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
}

.notice-title {
  margin: 0;
  color: #333;
  font-size: 16px;
  flex: 1;
}

.notice-meta {
  display: flex;
  gap: 20px;
  margin-bottom: 10px;
  font-size: 12px;
  color: #999;
}

.notice-author,
.notice-time {
  display: inline-block;
}

.notice-content {
  color: #555;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
  padding-top: 10px;
  border-top: 1px solid #e0e0e0;
}

.form-group {
  margin-bottom: 20px;
}

.form-label {
  display: block;
  margin-bottom: 8px;
  font-weight: 500;
  color: #333;
}
</style>
