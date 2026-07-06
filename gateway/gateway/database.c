/*
 * database.c  ——  数据库操作模块（SQLite3 封装）
 * 初始化sqlite，建表、插入实例数据
 * 封装所有 SQLite 数据库操作，提供课程、公告、座位、考勤、留言等业务的数据访问接口。
 */
#include "database.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/*
 * 功能：打开数据库连接
 * @db: 数据库句柄指针（输出参数）
 */
int db_init(sqlite3 **db) {
    int rc = sqlite3_open("teaching_office.db", db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "无法打开数据库: %s\n", sqlite3_errmsg(*db));
        return -1;
    }
    printf("数据库打开成功\n");
    return 0;
}

/*
 * 功能：创建所有数据表
 * @db: 数据库句柄
 */
int db_create_tables(sqlite3 *db) {
    char *err_msg = NULL;
    
    // 课程表 - 添加weekday和period字段
    const char *sql_courses = 
        "CREATE TABLE IF NOT EXISTS courses ("
        "course_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "classroom TEXT NOT NULL,"
        "course_name TEXT NOT NULL,"
        "teacher TEXT NOT NULL,"
        "class_name TEXT NOT NULL,"
        "weekday INTEGER NOT NULL,"  // 1=周一, 2=周二, ..., 7=周日
        "period INTEGER NOT NULL"     // 1-5对应5个时间段
        ");";
    
    // 图书馆座位表
    const char *sql_library = 
        "CREATE TABLE IF NOT EXISTS library_seats ("
        "seat_number INTEGER PRIMARY KEY,"
        "is_reserved INTEGER DEFAULT 0,"
        "student_id TEXT,"
        "time_slot TEXT,"
        "reserved_time TEXT"
        ");";
    
    // 公告表
    const char *sql_notices = 
        "CREATE TABLE IF NOT EXISTS notices ("
        "notice_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "title TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "publish_time TEXT NOT NULL"
        ");";
    
    // 学生表
    const char *sql_students = 
        "CREATE TABLE IF NOT EXISTS students ("
        "student_id TEXT PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "class_name TEXT NOT NULL"
        ");";
    
    // 考勤表
    const char *sql_attendance = 
        "CREATE TABLE IF NOT EXISTS attendance ("
        "attendance_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "course_id INTEGER NOT NULL,"
        "absent_student_id TEXT NOT NULL,"
        "record_time TEXT NOT NULL"
        ");";
    
    // 留言表
    const char *sql_messages = 
        "CREATE TABLE IF NOT EXISTS messages ("
        "message_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "student_id TEXT NOT NULL,"
        "student_name TEXT NOT NULL,"
        "message TEXT NOT NULL,"
        "create_time TEXT NOT NULL"
        ");";
    
    // 教师表
    const char *sql_teachers = 
        "CREATE TABLE IF NOT EXISTS teachers ("
        "teacher_id TEXT PRIMARY KEY,"
        "teacher_name TEXT NOT NULL"
        ");";

    /* 集合建表语句 */
    const char *sqls[] = {sql_courses, sql_library, sql_notices, 
                         sql_students, sql_attendance, sql_messages, sql_teachers, NULL};
    /*  按顺序执行所有建表语句  */
    for (int i = 0; sqls[i] != NULL; i++) {
        int rc = sqlite3_exec(db, sqls[i], NULL, NULL, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "创建表失败: %s\n", err_msg);
            sqlite3_free(err_msg);
            return -1;
        }
    }
    
    printf("数据表创建成功\n");
    return 0;
}

/*
 * 功能：插入示例数据（课程、座位、公告、学生）
 * @db: 数据库句柄
 */
int db_insert_sample_data(sqlite3 *db) {
    char *err_msg = NULL;
    
    // 删除旧数据
    sqlite3_exec(db, "DELETE FROM courses", NULL, NULL, &err_msg);
    
    // 插入周一到周五的实例课程数据
    // weekday: 1=周一, 2=周二, ..., 5=周五
    // period: 1=8:00-9:40, 2=9:50-11:30, 3=14:00-15:40, 4=15:50-17:30, 5=19:00-20:40
    const char *sql_courses = 
        "INSERT INTO courses (classroom, course_name, teacher, class_name, weekday, period) VALUES "
        "('4-106', '高等数学', '王教授', '工商管理2201班', 1, 1),"
        "('4-106', '线性代数', '李教授', '工商管理2201班', 1, 2),"
        "('4-106', '大学英语', '张老师', '大数据管理与应用2201班', 1, 3),"
        "('4-106', '计算机基础', '刘老师', '工商管理2201班,大数据管理与应用2201班', 1, 4),"
        "('4-106', '运筹学', '张林兰', '工商管理2201班,大数据管理与应用2201班', 1, 5),"
        
        "('4-106', '数据结构', '赵教授', '大数据管理与应用2201班', 2, 1),"
        "('4-106', '概率论', '孙教授', '工商管理2201班', 2, 2),"
        "('4-106', '管理学原理', '周教授', '工商管理2201班', 2, 4),"
        
        "('4-106', '数据库原理', '吴教授', '大数据管理与应用2201班', 3, 2),"
        "('4-106', '市场营销', '郑教授', '工商管理2201班', 3, 3),"
        "('4-106', 'Python编程', '陈老师', '大数据管理与应用2201班', 3, 5),"
        
        "('4-106', '经济学基础', '钱教授', '工商管理2201班', 4, 1),"
        "('4-106', '机器学习', '马教授', '大数据管理与应用2201班', 4, 3),"
        "('4-106', '企业战略管理', '许教授', '工商管理2201班', 4, 4),"
        
        "('4-106', '大数据分析', '韩教授', '大数据管理与应用2201班', 5, 2),"
        "('4-106', '人力资源管理', '冯教授', '工商管理2201班', 5, 3),"
        
        "('4-107', '软件工程', '赵丽', '软件工程2201班', 1, 1),"
        "('4-107', '操作系统', '孙强', '软件工程2201班', 1, 2),"
        "('4-107', '计算机网络', '张伟', '软件工程2201班', 1, 3),"
        "('4-107', '算法设计', '刘芳', '软件工程2201班', 1, 4),"
        "('4-107', '数据挖掘', '钱勇', '软件工程2201班', 1, 5),"
        
        "('4-107', '软件项目管理', '赵刚', '软件工程2201班', 2, 1),"
        "('4-107', '软件测试', '孙敏', '软件工程2201班', 2, 2),"
        "('4-107', '系统分析与设计', '周磊', '软件工程2201班', 2, 4),"
        
        "('4-107', '数据库系统', '吴霞', '软件工程2201班', 3, 2),"
        "('4-107', '软件架构', '郑涛', '软件工程2201班', 3, 3),"
        "('4-107', '人工智能', '陈静', '软件工程2201班', 3, 5),"
        
        "('4-107', '经济学原理', '钱华', '软件工程2201班', 4, 1),"
        "('4-107', '金融学', '马丽', '软件工程2201班', 4, 3),"
        "('4-107', '投资学', '许伟', '软件工程2201班', 4, 4),"
        
        "('4-107', '数据分析', '韩敏', '软件工程2201班', 5, 2),"
        "('4-107', '数据可视化', '冯涛', '软件工程2201班', 5, 3);";
    /* 插入课程数据 */
    if (sqlite3_exec(db, sql_courses, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "插入课程数据失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    // 初始化200个图书馆座位，全部初始化为空闲
    for (int i = 1; i <= 200; i++) {
        char sql[256];
        snprintf(sql, sizeof(sql), 
                "INSERT OR IGNORE INTO library_seats (seat_number, is_reserved) VALUES (%d, 0);", i);
        sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    }
    
    // 插入公告
    const char *sql_notices = 
        "INSERT OR IGNORE INTO notices (notice_id, title, content, publish_time) VALUES "
        "(1, '关于期末考试安排的通知', '各位同学：本学期期末考试将于下周一开始，请大家做好复习准备。考试时间和地点详见教务系统。祝大家考试顺利！', '2026-01-05 10:00:00'),"
        "(2, '人工智能发展前沿讲座通知', '讲座主题：人工智能在教育领域的应用与发展\\n主讲人：李教授（清华大学）\\n时间：2026年1月10日 14:00-16:00\\n地点：学术报告厅\\n欢迎全校师生参加！', '2026-01-06 14:00:00'),"
        "(3, '校园文化艺术节活动通知', '为丰富校园文化生活，学校将举办第十届校园文化艺术节。活动包括歌唱比赛、舞蹈表演、书画展览等。欢迎同学们踊跃报名参加！报名截止时间：1月15日。', '2026-01-08 09:00:00'),"
        "(4, '图书馆开放时间调整通知', '因馆内设备维护，图书馆本周六（1月13日）闭馆一天，周日恢复正常开放。给您带来不便，敬请谅解。', '2026-01-09 15:30:00'),"
        "(5, '寒假放假通知', '根据校历安排，本学期将于1月20日结束，1月21日开始放寒假。下学期报到时间为2月28日，3月1日正式上课。祝大家寒假愉快！', '2026-01-10 11:00:00');";
    sqlite3_exec(db, sql_notices, NULL, NULL, &err_msg);
    
    const char *sql_students = 
    "INSERT OR IGNORE INTO students (student_id, name, class_name) VALUES "
    // 工商管理2201班（16人）
    "('2022001', '张三', '工商管理2201班'),"
    "('2022002', '李四', '工商管理2201班'),"
    "('2022003', '王五', '工商管理2201班'),"
    "('2022004', '赵六', '工商管理2201班'),"
    "('2022005', '孙阳', '工商管理2201班'),"
    "('2022006', '周彤', '工商管理2201班'),"
    "('2022007', '吴磊', '工商管理2201班'),"
    "('2022008', '郑佳', '工商管理2201班'),"
    "('2022009', '冯宇', '工商管理2201班'),"
    "('2022010', '陈思', '工商管理2201班'),"
    "('2022011', '褚晓', '工商管理2201班'),"
    "('2022012', '卫华', '工商管理2201班'),"
    "('2022013', '蒋明', '工商管理2201班'),"
    "('2022014', '沈浩', '工商管理2201班'),"
    "('2022015', '韩雪', '工商管理2201班'),"
    "('2022016', '杨阳', '工商管理2201班'),"
    // 大数据管理与应用2201班（4人，学号后移避免冲突）
    "('2022017', '孙七', '大数据管理与应用2201班'),"
    "('2022018', '周八', '大数据管理与应用2201班'),"
    "('2022019', '吴九', '大数据管理与应用2201班'),"
    "('2022020', '郑十', '大数据管理与应用2201班');";
    
    if (sqlite3_exec(db, sql_students, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "插入学生数据失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    // 插入教师数据（10位工号）
    sqlite3_exec(db, "DELETE FROM teachers", NULL, NULL, &err_msg);
    const char *sql_teachers = 
        "INSERT OR IGNORE INTO teachers (teacher_id, teacher_name) VALUES "
        "('1001000001', '王教授'),"
        "('1001000002', '李教授'),"
        "('1001000003', '张老师'),"
        "('1001000004', '刘老师'),"
        "('1001000005', '赵丽'),"
        "('1001000006', '孙强'),"
        "('1001000007', '陈静'),"
        "('1001000008', '钱华'),"
        "('1001000009', '马丽'),"
        "('1001000010', '许伟');";
    
    if (sqlite3_exec(db, sql_teachers, NULL, NULL, &err_msg) != SQLITE_OK) {
        fprintf(stderr, "插入教师数据失败: %s\n", err_msg);
        sqlite3_free(err_msg);
    }
    
    printf("示例数据插入成功\n");
    return 0;
}

/*
 * 功能：验证学生身份（学号和姓名是否匹配）
 * @db: 数据库句柄
 * @student_id: 学号
 * @student_name: 姓名
 * @return: 0 验证成功，-1 验证失败
 */
int db_validate_student(sqlite3 *db, const char *student_id, const char *student_name) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT COUNT(*) FROM students WHERE student_id = ? AND name = ?";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "准备验证SQL失败: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, student_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, student_name, -1, SQLITE_STATIC);
    
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    
    printf("学生验证: 学号=%s, 姓名=%s, 结果=%s\n", 
           student_id, student_name, count > 0 ? "通过" : "失败");
    
    return count > 0 ? 0 : -1;  // 返回0表示验证成功，-1表示失败
}

/*
 * 功能：根据教室、星期、时间段精确查询课程
 * @db: 数据库句柄
 * @classroom: 教室名称
 * @weekday: 星期几（1=周一, 2=周二, ..., 7=周日）
 * @period: 时间段（1=8:00-9:40, 2=9:50-11:30, 3=14:00-15:40, 4=15:50-17:30, 5=19:00-20:40）
 * @course: 课程信息结构体（输出参数）
 */
int db_get_course_by_time(sqlite3 *db, const char *classroom, int weekday, int period, Course *course) {
    sqlite3_stmt *stmt;
     /* 使用 ? 作为占位符，后续通过 sqlite3_bind_text 绑定具体值 */
    const char *sql = "SELECT course_id, classroom, course_name, teacher, class_name, weekday, period "
                     "FROM courses WHERE classroom = ? AND weekday = ? AND period = ?";

    const char *weekdays[] = {"", "周一", "周二", "周三", "周四", "周五", "周六", "周日"};
    /* 预编译 SQL 语句 */
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL准备失败: %s\n", sqlite3_errmsg(db));
        return -1;
    }
    
    /* 分别绑定第1，2，3个占位符，查询条件为教室号、星期几、时间 */
    sqlite3_bind_text(stmt, 1, classroom, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, weekday);
    sqlite3_bind_int(stmt, 3, period);
    
    /* 执行预编译语句 */
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        // 第 0 列：课程ID（整数）
        course->course_id = sqlite3_column_int(stmt, 0);
        // 第 1 列：教室名称（字符串）
        strncpy(course->classroom, (const char*)sqlite3_column_text(stmt, 1), sizeof(course->classroom) - 1);
        // 第 2 列：课程名称（字符串）
        strncpy(course->course_name, (const char*)sqlite3_column_text(stmt, 2), sizeof(course->course_name) - 1);
        // 第 3 列：教师姓名（字符串）        
        strncpy(course->teacher, (const char*)sqlite3_column_text(stmt, 3), sizeof(course->teacher) - 1);
        // 第 4 列：班级名称（字符串）
        strncpy(course->class_name, (const char*)sqlite3_column_text(stmt, 4), sizeof(course->class_name) - 1);
        
        // 根据时间段设置具体时间
        const char *time_slots[][2] = {
            {"08:00", "09:40"},
            {"09:50", "11:30"},
            {"14:00", "15:40"},
            {"15:50", "17:30"},
            {"19:00", "20:40"}
        };
        /* 将数据库查询结果填充到 course 结构体中 */
        int p = sqlite3_column_int(stmt, 6) - 1;
        if (p >= 0 && p < 5) {
            strncpy(course->start_time, time_slots[p][0], sizeof(course->start_time) - 1);
            strncpy(course->end_time, time_slots[p][1], sizeof(course->end_time) - 1);
        }
        
        // 设置星期
        /* 将数据库查询结果填充到 course 结构体中 */ 
        const char *weekdays[] = {"", "周一", "周二", "周三", "周四", "周五", "周六", "周日"};
        int wd = sqlite3_column_int(stmt, 5);
        if (wd >= 1 && wd <= 7) {
            strncpy(course->weekday, weekdays[wd], sizeof(course->weekday) - 1);
        }
        
        // 释放数据库对象
        sqlite3_finalize(stmt);
        return 0;    // 查询成功
    }
    // 释放数据库对象 
    sqlite3_finalize(stmt);
    return -1;  // 查询失败
}

/*
 * @param db: SQLite数据库句柄，用于执行查询操作
 * @param teacher_id: 待验证的教师工号（字符串格式，如 "1001000001"）
 * @param classroom: 教室名称（如 "4-106"），用于定位课程
 * @return: 0 表示验证通过（教师工号匹配当前课程），-1 表示验证失败
*/
int db_validate_teacher(sqlite3 *db, const char *teacher_id, const char *classroom) {
    sqlite3_stmt *stmt;         /* SQL预处理语句句柄，用于安全执行查询 */
    /* 获取系统时间，用于数据库查询 */
    time_t now = time(NULL);    
    struct tm *tm_info = localtime(&now); 

    /* 获取当前星期几，并进行转换 */
    int weekday = tm_info->tm_wday;
    if (weekday == 0) weekday = 7;

    /* 根据当前所在的时间计算上课时间段（第几节课） */
    int hour = tm_info->tm_hour;
    int period = 1;
    if (hour >= 8 && hour < 10) period = 1;
    else if (hour >= 10 && hour < 12) period = 2;
    else if (hour >= 14 && hour < 16) period = 3;
    else if (hour >= 16 && hour < 18) period = 4;
    else if (hour >= 18 && hour < 20) period = 5;
    else if (hour >= 20 && hour < 22) period = 6;
    /* 构造SQL查询语句：查询指定教室、星期、时间段的课程任课教师 */
    const char *sql = 
        "SELECT teacher FROM courses WHERE classroom = ? AND weekday = ? AND period = ?";
    
    /* 准备SQL预处理语句 */
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "准备教师验证SQL失败: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    /* 绑定查询参数到SQL语句中的占位符（?） */
    sqlite3_bind_text(stmt, 1, classroom, -1, SQLITE_STATIC);  /* 第1个参数：教室名称（字符串） */
    sqlite3_bind_int(stmt, 2, weekday);     /* 第2个参数：星期几（整数 1-7） */
    sqlite3_bind_int(stmt, 3, period);       /* 第3个参数：时间段（整数 1-5） */
    
    int result = -1;
    /* 从查询结果中提取任课教师姓名（或工号） */
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *course_teacher = (const char*)sqlite3_column_text(stmt, 0);
        /* 比较查询到的教师与输入的教师工号是否匹配 */
        if (course_teacher && strcmp(course_teacher, teacher_id) == 0) {
            result = 0;
            printf("[教师验证] 教室:%s, 当堂教师:%s, 输入工号:%s, 结果:通过\n", 
                   classroom, course_teacher, teacher_id);
        } else {
            printf("[教师验证] 教室:%s, 当堂教师:%s, 输入工号:%s, 结果:失败\n", 
                   classroom, course_teacher ? course_teacher : "无", teacher_id);
        }
    } else {
        printf("[教师验证] 教室:%s 当前时间无课程\n", classroom);
    }
    
    sqlite3_finalize(stmt); /* 释放SQL预处理语句占用的资源 */
    return result;  /* 返回验证结果：0=通过，-1=失败 */
}

void db_close(sqlite3 *db) {
    sqlite3_close(db);  
    printf("数据库已关闭\n");
}
