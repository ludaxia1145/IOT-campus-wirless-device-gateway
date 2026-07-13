<script setup>
import { ref, onMounted, computed } from 'vue'
import { ElMessage } from 'element-plus'
import { attendanceAPI } from '../api/gateway'

// 数据
const attendanceRecords = ref([])
const loading = ref(false)
const filterStatus = ref('all')

// 获取考勤记录
const fetchAttendance = async () => {
  loading.value = true
  try {
    const response = await attendanceAPI.getAttendance()
    attendanceRecords.value = response.data.attendance || []
  } catch (error) {
    ElMessage.error('获取考勤记录失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 过滤后的考勤记录
const filteredRecords = computed(() => {
  if (filterStatus.value === 'all') {
    return attendanceRecords.value
  }
  return attendanceRecords.value.filter(record => record.status === filterStatus.value)
})

// 获取状态的显示文字
const getStatusLabel = (status) => {
  const statusMap = {
    'present': '出席',
    'absent': '缺席',
    'late': '迟到',
    'leave': '请假'
  }
  return statusMap[status] || status
}

// 获取状态的标签类型
const getStatusType = (status) => {
  const typeMap = {
    'present': 'success',
    'absent': 'danger',
    'late': 'warning',
    'leave': 'info'
  }
  return typeMap[status] || 'info'
}

// 计算统计数据
const statistics = computed(() => {
  const stats = {
    total: attendanceRecords.value.length,
    present: 0,
    absent: 0,
    late: 0,
    leave: 0
  }

  attendanceRecords.value.forEach(record => {
    stats[record.status]++
  })

  return stats
})

// 页面挂载时获取数据
onMounted(() => {
  fetchAttendance()
})
</script>

<template>
  <div class="attendance-container">
    <!-- 页面标题 -->
    <div class="page-header">
      <h2>考勤记录</h2>
      <el-button @click="fetchAttendance" :loading="loading">刷新</el-button>
    </div>

    <!-- 统计信息 -->
    <div class="stats-container">
      <div class="stat-card">
        <div class="stat-label">总人数</div>
        <div class="stat-value">{{ statistics.total }}</div>
      </div>
      <div class="stat-card present">
        <div class="stat-label">出席</div>
        <div class="stat-value">{{ statistics.present }}</div>
      </div>
      <div class="stat-card absent">
        <div class="stat-label">缺席</div>
        <div class="stat-value">{{ statistics.absent }}</div>
      </div>
      <div class="stat-card late">
        <div class="stat-label">迟到</div>
        <div class="stat-value">{{ statistics.late }}</div>
      </div>
      <div class="stat-card leave">
        <div class="stat-label">请假</div>
        <div class="stat-value">{{ statistics.leave }}</div>
      </div>
    </div>

    <!-- 过滤选项 -->
    <div class="filter-container">
      <span class="filter-label">筛选状态：</span>
      <el-select v-model="filterStatus" style="width: 200px">
        <el-option label="全部" value="all" />
        <el-option label="出席" value="present" />
        <el-option label="缺席" value="absent" />
        <el-option label="迟到" value="late" />
        <el-option label="请假" value="leave" />
      </el-select>
    </div>

    <!-- 考勤表格 -->
    <div class="table-wrapper">
      <el-table :data="filteredRecords" :loading="loading" stripe>
        <el-table-column prop="student_id" label="学号" width="150" />
        <el-table-column prop="student_name" label="姓名" width="150" />
        <el-table-column prop="course_name" label="课程" />
        <el-table-column label="考勤状态" width="120">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">
              {{ getStatusLabel(scope.row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="check_in_time" label="签到时间" />
        <el-table-column prop="remarks" label="备注" />
      </el-table>
    </div>
  </div>
</template>

<style scoped>
.attendance-container {
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

/* 统计信息卡片 */
.stats-container {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
  gap: 15px;
  margin: 20px 0;
}

.stat-card {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border-radius: 8px;
  padding: 20px;
  color: white;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
  text-align: center;
}

.stat-card.present {
  background: linear-gradient(135deg, #51cf66 0%, #37b24d 100%);
}

.stat-card.absent {
  background: linear-gradient(135deg, #ff6b6b 0%, #fa5252 100%);
}

.stat-card.late {
  background: linear-gradient(135deg, #ffd43b 0%, #ffed4e 100%);
  color: #333;
}

.stat-card.leave {
  background: linear-gradient(135deg, #748ffc 0%, #0ba5ec 100%);
}

.stat-label {
  font-size: 14px;
  opacity: 0.9;
  margin-bottom: 10px;
}

.stat-value {
  font-size: 32px;
  font-weight: bold;
}

/* 过滤容器 */
.filter-container {
  display: flex;
  align-items: center;
  gap: 15px;
  margin: 20px 0;
  padding: 15px;
  background: #f5f7fa;
  border-radius: 6px;
}

.filter-label {
  font-weight: 500;
  color: #333;
}

/* 表格容器 */
.table-wrapper {
  margin-top: 20px;
}
</style>
