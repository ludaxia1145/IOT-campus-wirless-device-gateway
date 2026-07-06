#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

/**
 * 课程结构体
 * 对应数据库中的 courses 表，存储课程安排信息
 * 用于查询和返回课程详情
 */
typedef struct {
    int course_id;
    char classroom[64];
    char course_name[128];
    char teacher[64];
    char class_name[256];
    char start_time[32];
    char end_time[32];
    char weekday[32];
} Course;

/**
 * 图书馆座位结构体
 * 对应数据库中的 library_seats 表，存储座位预约状态
 */
typedef struct {
    int seat_number;
    int is_reserved;
    char student_id[32];
    char time_slot[32];
    char reserved_time[64];
} LibrarySeat;

/**
 * 公告结构体
 * 对应数据库中的 notices 表，存储系统公告信息
 */
typedef struct {
    int notice_id;
    char title[256];
    char content[1024];
    char publish_time[64];
} Notice;

/**
 * 学生结构体
 * 对应数据库中的 students 表，存储学生基本身份信息
 * 用于学生身份验证和考勤名单生成
 */
typedef struct {
    char student_id[32];
    char name[64];
    char class_name[64];
} Student;

/**
 * 留言结构体
 * 对应数据库中的 messages 表，存储学生提交的教学留言
 */
typedef struct {
    char student_id[32];
    char student_name[64];
    char message[1024];
    char create_time[64];
} Message;

/**
 * 教师结构体
 * 对应数据库中的 teachers 表，存储教师身份信息
 * 用于教师身份验证和权限管理
 */
typedef struct {
    char teacher_id[16];
    char teacher_name[64];
} Teacher;

/* 数据库初始化 */
int db_init(sqlite3 **db);

/* 创建所有业务数据表 */
int db_create_tables(sqlite3 *db);

/* 插入示例数据 */
int db_insert_sample_data(sqlite3 *db);
/* 关闭数据库连接 */
void db_close(sqlite3 *db);

/* 学生提交留言、作为预约身份的核验 */
int db_validate_student(sqlite3 *db, const char *student_id, const char *student_name);

/* 教师进行课堂考勤操作前的身份校验 */
int db_validate_teacher(sqlite3 *db, const char *teacher_id, const char *classroom);

/* 根据当前时间查询对应课程 */
int db_get_course_by_time(sqlite3 *db, const char *classroom, int weekday, int period, Course *course);

#endif
