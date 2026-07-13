<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { messageAPI } from '../api/gateway'

// 数据
const messages = ref([])
const loading = ref(false)

// 获取留言列表
const fetchMessages = async () => {
  loading.value = true
  try {
    const response = await messageAPI.getMessages()
    messages.value = response.data.messages || []
  } catch (error) {
    ElMessage.error('获取留言失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 删除留言
const handleDeleteMessage = async (messageId) => {
  try {
    // 确认删除
    if (!confirm('确定要删除这条留言吗？')) {
      return
    }

    loading.value = true
    await messageAPI.deleteMessage(messageId)
    ElMessage.success('留言删除成功')
    fetchMessages()
  } catch (error) {
    ElMessage.error('删除留言失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 页面挂载时获取数据
onMounted(() => {
  fetchMessages()
})
</script>

<template>
  <div class="messages-container">
    <!-- 页面标题 -->
    <div class="page-header">
      <h2>学生留言</h2>
      <el-button @click="fetchMessages" :loading="loading">刷新</el-button>
    </div>

    <!-- 留言列表 -->
    <div v-if="messages.length > 0" class="messages-list">
      <div v-for="message in messages" :key="message.message_id" class="message-card">
        <div class="message-header">
          <span class="message-id">留言ID: {{ message.message_id }}</span>
          <span class="message-student">学号: {{ message.student_id }}</span>
          <el-button link type="danger" size="small" @click="handleDeleteMessage(message.message_id)">
            删除
          </el-button>
        </div>
        <div class="message-title">
          <strong>标题：</strong> {{ message.title }}
        </div>
        <div class="message-content">
          <strong>内容：</strong>
          <p>{{ message.content }}</p>
        </div>
        <div class="message-time">
          <em>发布时间：{{ message.timestamp }}</em>
        </div>
      </div>
    </div>

    <!-- 空状态 -->
    <el-empty v-else description="暂无留言" />
  </div>
</template>

<style scoped>
.messages-container {
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

.messages-list {
  display: flex;
  flex-direction: column;
  gap: 15px;
  margin-top: 20px;
}

.message-card {
  border: 1px solid #e0e0e0;
  border-radius: 6px;
  padding: 15px;
  background: #f9f9f9;
  transition: box-shadow 0.3s ease;
}

.message-card:hover {
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.message-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  padding-bottom: 10px;
  border-bottom: 1px solid #e0e0e0;
}

.message-id,
.message-student {
  font-size: 12px;
  color: #666;
}

.message-title {
  margin-bottom: 10px;
  color: #333;
  font-size: 14px;
}

.message-content {
  margin-bottom: 10px;
  color: #555;
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-word;
}

.message-content p {
  margin: 0;
}

.message-time {
  font-size: 12px;
  color: #999;
}
</style>
