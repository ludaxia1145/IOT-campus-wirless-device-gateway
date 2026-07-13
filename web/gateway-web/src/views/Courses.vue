<script setup>
import { ref, onMounted } from 'vue'
import { ElMessage } from 'element-plus'
import { courseAPI } from '../api/gateway'

// 数据
const courses = ref([])
const loading = ref(false)
const showDialog = ref(false)

// 表单数据
const formData = ref({
  course_name: '',
  classroom: '',
  teacher: '',
  time: '',
  capacity: ''
})

// 表单验证规则
const rules = {
  course_name: [{ required: true, message: '请输入课程名称', trigger: 'blur' }],
  classroom: [{ required: true, message: '请输入教室', trigger: 'blur' }],
  teacher: [{ required: true, message: '请输入教师名称', trigger: 'blur' }],
  time: [{ required: true, message: '请输入上课时间', trigger: 'blur' }],
  capacity: [{ required: true, message: '请输入容量', trigger: 'blur' }]
}

// 获取课表列表
const fetchCourses = async () => {
  loading.value = true
  try {
    const response = await courseAPI.getCourses()
    courses.value = response.data.courses || []
  } catch (error) {
    ElMessage.error('获取课表失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 打开添加课表对话框
const openAddDialog = () => {
  formData.value = {
    course_name: '',
    classroom: '',
    teacher: '',
    time: '',
    capacity: ''
  }
  showDialog.value = true
}

// 添加课表
const handleAddCourse = async () => {
  try {
    // 验证表单数据
    if (!formData.value.course_name || !formData.value.classroom || 
        !formData.value.teacher || !formData.value.time || !formData.value.capacity) {
      ElMessage.warning('请填写所有字段')
      return
    }

    loading.value = true
    await courseAPI.addCourse(formData.value)
    ElMessage.success('课表添加成功')
    showDialog.value = false
    fetchCourses()
  } catch (error) {
    ElMessage.error('添加课表失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 删除课表
const handleDeleteCourse = async (courseId) => {
  try {
    // 确认删除
    if (!confirm('确定要删除这条课表记录吗？')) {
      return
    }

    loading.value = true
    await courseAPI.deleteCourse(courseId)
    ElMessage.success('课表删除成功')
    fetchCourses()
  } catch (error) {
    ElMessage.error('删除课表失败：' + error.message)
  } finally {
    loading.value = false
  }
}

// 页面挂载时获取数据
onMounted(() => {
  fetchCourses()
})
</script>

<template>
  <div class="courses-container">
    <!-- 页面标题 -->
    <div class="page-header">
      <h2>课表管理</h2>
      <el-button type="primary" @click="openAddDialog">添加课表</el-button>
    </div>

    <!-- 课表列表 -->
    <div class="table-wrapper">
      <el-table :data="courses" :loading="loading" stripe>
        <el-table-column prop="course_id" label="课程ID" width="100" />
        <el-table-column prop="course_name" label="课程名称" />
        <el-table-column prop="classroom" label="教室" width="120" />
        <el-table-column prop="teacher" label="教师" width="120" />
        <el-table-column prop="time" label="上课时间" />
        <el-table-column prop="capacity" label="容量" width="100" />
        <el-table-column label="操作" width="100" fixed="right">
          <template #default="scope">
            <el-button link type="danger" @click="handleDeleteCourse(scope.row.course_id)">
              删除
            </el-button>
          </template>
        </el-table-column>
      </el-table>
    </div>

    <!-- 添加课表对话框 -->
    <el-dialog v-model="showDialog" title="添加课表" width="500px" @close="showDialog = false">
      <div class="form-group">
        <label class="form-label">课程名称</label>
        <el-input v-model="formData.course_name" placeholder="请输入课程名称" />
      </div>

      <div class="form-group">
        <label class="form-label">教室</label>
        <el-input v-model="formData.classroom" placeholder="请输入教室号" />
      </div>

      <div class="form-group">
        <label class="form-label">教师名称</label>
        <el-input v-model="formData.teacher" placeholder="请输入教师名称" />
      </div>

      <div class="form-group">
        <label class="form-label">上课时间</label>
        <el-input v-model="formData.time" placeholder="例如：周一 08:00-09:30" />
      </div>

      <div class="form-group">
        <label class="form-label">容量</label>
        <el-input v-model.number="formData.capacity" placeholder="请输入教室容量" type="number" />
      </div>

      <template #footer>
        <el-button @click="showDialog = false">取消</el-button>
        <el-button type="primary" @click="handleAddCourse" :loading="loading">
          添加
        </el-button>
      </template>
    </el-dialog>
  </div>
</template>

<style scoped>
.courses-container {
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

.table-wrapper {
  margin-top: 20px;
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
