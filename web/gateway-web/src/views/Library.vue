<script setup>
import { ref, onMounted, computed } from 'vue'
import { ElMessage } from 'element-plus'
import { libraryAPI } from '../api/gateway'

// 数据
const seats = ref([])
const loading = ref(false)

// 获取座位信息
const fetchSeats = async () => {
  loading.value = true
  try {
    const response = await libraryAPI.getLibrarySeats()
    seats.value = response.data.seats || []
  } catch (error) {
    ElMessage.error('获取座位信息失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 计算已预约座位数
const reservedCount = computed(() => {
  return seats.value.filter(seat => seat.is_reserved === 1).length
})

// 计算可用座位数
const availableCount = computed(() => {
  return seats.value.length - reservedCount.value
})

// 获取座位状态的显示文字
const getSeatStatus = (seat) => {
  if (seat.is_reserved === 1) {
    return `已预约 (${seat.reserved_by})`
  }
  return '可用'
}

// 获取座位状态的标签类型
const getSeatStatusType = (seat) => {
  if (seat.is_reserved === 1) {
    return 'danger'
  }
  return 'success'
}

// 页面挂载时获取数据
onMounted(() => {
  fetchSeats()
})
</script>

<template>
  <div class="library-container">
    <!-- 页面标题 -->
    <div class="page-header">
      <h2>图书馆座位预约信息</h2>
      <el-button @click="fetchSeats" :loading="loading">刷新</el-button>
    </div>

    <!-- 统计信息 -->
    <div class="stats-container">
      <div class="stat-card">
        <div class="stat-label">总座位数</div>
        <div class="stat-value">{{ seats.length }}</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">已预约</div>
        <div class="stat-value reserved">{{ reservedCount }}</div>
      </div>
      <div class="stat-card">
        <div class="stat-label">可用座位</div>
        <div class="stat-value available">{{ availableCount }}</div>
      </div>
    </div>

    <!-- 座位列表 -->
    <div class="seats-wrapper" v-loading="loading">
      <el-table :data="seats" stripe class="seats-table">
        <el-table-column prop="seat_id" label="座位号" width="100" />
        <el-table-column prop="location" label="位置" width="150" />
        <el-table-column label="状态" width="180">
          <template #default="scope">
            <el-tag :type="getSeatStatusType(scope.row)">
              {{ getSeatStatus(scope.row) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="reserved_by" label="预约人" />
        <el-table-column prop="reservation_time" label="预约时间" />
      </el-table>
    </div>
  </div>
</template>

<style scoped>
.library-container {
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
  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
  gap: 15px;
  margin: 20px 0;
}

.stat-card {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  border-radius: 8px;
  padding: 20px;
  color: white;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

.stat-card.available {
  background: linear-gradient(135deg, #84fab0 0%, #8fd3f4 100%);
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

.stat-value.reserved {
  color: #ff6b6b;
}

.stat-value.available {
  color: #51cf66;
}

.seats-wrapper {
  margin-top: 20px;
}

.seats-table {
  width: 100%;
}
</style>
