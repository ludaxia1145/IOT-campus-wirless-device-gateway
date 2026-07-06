/*
 * server.c  ——  TCP 协议服务器（epoll 事件驱动版）
 */

#include "server.h"
#include "protocol.h"
#include "database.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>            /* fcntl() —— 设置非阻塞 fd */
#include <errno.h>            /* EAGAIN / EWOULDBLOCK 判断 */
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

/*
 * 函数功能：处理获取课程请求，根据当前时间和教室查询对应课程
 * @client_fd: 客户端 socket 文件描述符
 * @data: 客户端发来的JSON 数据
 * @db: SQLite 数据库句柄
 */
static void handle_get_course(int client_fd, cJSON *data, sqlite3 *db) {
    cJSON *classroom_item = cJSON_GetObjectItem(data, "classroom");
    if (!classroom_item) return;
    
    const char *classroom = classroom_item->valuestring;
    
    // 获取当前时间
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    int current_hour = tm_info->tm_hour;
    int current_min = tm_info->tm_min;
    int weekday = (tm_info->tm_wday == 0) ? 7 : tm_info->tm_wday; // 转换为1-7
    
    // 计算当前时间段 (1-5)
    int period = 0;
    if (current_hour == 8 || (current_hour == 9 && current_min <= 40)) {
        period = 1; // 8:00-9:40
    } else if ((current_hour == 9 && current_min >= 50) || (current_hour == 10) || (current_hour == 11 && current_min <= 30)) {
        period = 2; // 9:50-11:30
    } else if (current_hour == 14 || (current_hour == 15 && current_min <= 40)) {
        period = 3; // 14:00-15:40
    } else if ((current_hour == 15 && current_min >= 50) || (current_hour == 16) || (current_hour == 17 && current_min <= 30)) {
        period = 4; // 15:50-17:30
    } else if (current_hour == 19 || (current_hour == 20 && current_min <= 40)) {
        period = 5; // 19:00-20:40
    }
    
    printf("当前时间: %02d:%02d, 星期%d, 时间段%d\n", current_hour, current_min, weekday, period);
    
    Course course;
    cJSON *response = cJSON_CreateObject();
    cJSON *response_data = cJSON_CreateObject();
    
    if (period > 0 && db_get_course_by_time(db, classroom, weekday, period, &course) == 0) {
        // 找到对应课程
        cJSON_AddStringToObject(response_data, "course_name", course.course_name);
        cJSON_AddStringToObject(response_data, "teacher", course.teacher);
        cJSON_AddStringToObject(response_data, "class", course.class_name);
        
        char time_str[100];
        snprintf(time_str, sizeof(time_str), "%s~%s", course.start_time, course.end_time);
        cJSON_AddStringToObject(response_data, "time", time_str);
        cJSON_AddStringToObject(response_data, "room_type", "教室");
        cJSON_AddStringToObject(response_data, "weekday", course.weekday);
        cJSON_AddNumberToObject(response_data, "period", period);
        
        printf("发送课程信息: %s (教师: %s)\n", course.course_name, course.teacher);
    } else {
        // 当前时间段无课程
        cJSON_AddStringToObject(response_data, "course_name", "暂无课程");
        cJSON_AddStringToObject(response_data, "teacher", "");
        cJSON_AddStringToObject(response_data, "class", "");
        cJSON_AddStringToObject(response_data, "time", "休息时间");
        cJSON_AddStringToObject(response_data, "room_type", "");
        cJSON_AddStringToObject(response_data, "weekday", "");
        cJSON_AddNumberToObject(response_data, "period", 0);
        
        printf("当前无课程安排\n");
    }
    
    cJSON_AddStringToObject(response, "type", "course_info");
    cJSON_AddItemToObject(response, "data", response_data);
    
    char *json_str = cJSON_PrintUnformatted(response);
    char send_buf[2048];
    snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
    /* 通过网络套接字把json数据发送给客户端 */
    write(client_fd, send_buf, strlen(send_buf));
    
    free(json_str);
    cJSON_Delete(response);
}

/*
 * 函数功能：处理发送留言请求，验证学生身份后插入留言记录
 * @client_fd: 客户端 socket 文件描述符
 * @data: 客户端JSON 数据（包含 student_id, student_name, message）
 * @db: SQLite 数据库句柄
 */
static void handle_send_message(int client_fd, cJSON *data, sqlite3 *db) {
    cJSON *student_id = cJSON_GetObjectItem(data, "student_id");
    cJSON *name = cJSON_GetObjectItem(data, "student_name");
    if (!name) {
        name = cJSON_GetObjectItem(data, "name");
    }
    cJSON *message = cJSON_GetObjectItem(data, "message");
    
    if (!student_id || !name || !message) {
        printf("留言数据不完整\n");
        return;
    }
    
    // 验证学生身份
    if (db_validate_student(db, student_id->valuestring, name->valuestring) != 0) {
        printf("学生身份验证失败: 学号=%s, 姓名=%s\n", student_id->valuestring, name->valuestring);
        
        cJSON *response = cJSON_CreateObject();
        cJSON *response_data = cJSON_CreateObject();
        cJSON_AddStringToObject(response_data, "status", "error");
        cJSON_AddStringToObject(response_data, "error", "学号或姓名错误，请检查后重试");
        cJSON_AddStringToObject(response, "type", "message_response");
        cJSON_AddItemToObject(response, "data", response_data);
        
        char *json_str = cJSON_PrintUnformatted(response);
        char send_buf[512];
        snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
        write(client_fd, send_buf, strlen(send_buf));
        
        free(json_str);
        cJSON_Delete(response);
        return;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO messages (student_id, student_name, message, create_time) VALUES (?, ?, ?, datetime('now', 'localtime'))";
    /* 执行留言插入 */
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, student_id->valuestring, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, name->valuestring, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, message->valuestring, -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_DONE) {
            printf("\n=== 收到教学办留言 ===\n");
            printf("学号: %s\n", student_id->valuestring);
            printf("姓名: %s\n", name->valuestring);
            printf("留言: %s\n", message->valuestring);
            printf("====================\n\n");
            
            cJSON *response = cJSON_CreateObject();
            cJSON *response_data = cJSON_CreateObject();
            cJSON_AddStringToObject(response_data, "status", "success");
            cJSON_AddStringToObject(response, "type", "message_response");
            cJSON_AddItemToObject(response, "data", response_data);
            
            char *json_str = cJSON_PrintUnformatted(response);
            char send_buf[512];
            snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
            write(client_fd, send_buf, strlen(send_buf));
            
            free(json_str);
            cJSON_Delete(response);
        }
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：处理获取座位状态请求，返回所有座位的预约信息
 * @client_fd: 客户端 socket 文件描述符
 * @data: 客户端的JSON 数据
 * @db: SQLite 数据库句柄
 */
static void handle_get_seat_status(int client_fd, cJSON *data, sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT seat_number, is_reserved, student_id, time_slot FROM library_seats ORDER BY seat_number";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        cJSON *response = cJSON_CreateObject();
        cJSON *response_data = cJSON_CreateObject();
        cJSON *seats_array = cJSON_CreateArray();
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *seat = cJSON_CreateObject();
            cJSON_AddNumberToObject(seat, "seat_number", sqlite3_column_int(stmt, 0));
            cJSON_AddNumberToObject(seat, "is_reserved", sqlite3_column_int(stmt, 1));
            
            const char *student_id = (const char*)sqlite3_column_text(stmt, 2);
            const char *time_slot = (const char*)sqlite3_column_text(stmt, 3);
            
            cJSON_AddStringToObject(seat, "reserved_by", student_id ? student_id : "");
            cJSON_AddStringToObject(seat, "time_slot", time_slot ? time_slot : "");
            cJSON_AddItemToArray(seats_array, seat);
        }
        
        cJSON_AddItemToObject(response_data, "seats", seats_array);
        cJSON_AddStringToObject(response, "type", "seat_status");
        cJSON_AddItemToObject(response, "data", response_data);
        
        char *json_str = cJSON_PrintUnformatted(response);
        char send_buf[32768];
        snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
        write(client_fd, send_buf, strlen(send_buf));
        
        free(json_str);
        cJSON_Delete(response);
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：处理座位预约请求，验证学号和座位后更新数据库
 * @client_fd: 客户端 socket 文件描述符
 * @data: 客户端的JSON 数据（包含 seat_number, student_id, time_slot）
 * @db: SQLite 数据库句柄
 */
static void handle_reserve_seat(int client_fd, cJSON *data, sqlite3 *db) {
    cJSON *seat_number = cJSON_GetObjectItem(data, "seat_number");
    cJSON *student_id = cJSON_GetObjectItem(data, "student_id");
    cJSON *time_slot = cJSON_GetObjectItem(data, "time_slot");
    
    printf("\n[座位预约] ========== 收到预约请求 ==========\n");
    printf("[座位预约] 座位号: %d\n", seat_number ? seat_number->valueint : -1);
    printf("[座位预约] 学号: %s\n", student_id ? student_id->valuestring : "NULL");
    printf("[座位预约] 时间段: %s\n", time_slot ? time_slot->valuestring : "NULL");
    
    if (!seat_number || !student_id || !time_slot) {
        printf("[座位预约] *** 缺少必要参数 ***\n");
        return;
    }
    
    cJSON *response = cJSON_CreateObject();
    cJSON *response_data = cJSON_CreateObject();
    
    // 第一步：验证学号是否存在于students表中
    sqlite3_stmt *validate_stmt;
    const char *validate_sql = "SELECT COUNT(*) FROM students WHERE student_id = ?";
    int student_exists = 0;
    
    printf("[座位预约] 开始验证学号...\n");
    if (sqlite3_prepare_v2(db, validate_sql, -1, &validate_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(validate_stmt, 1, student_id->valuestring, -1, SQLITE_STATIC);
        if (sqlite3_step(validate_stmt) == SQLITE_ROW) {
            student_exists = sqlite3_column_int(validate_stmt, 0) > 0;
            printf("[座位预约] 查询结果: 找到 %d 个匹配学生\n", sqlite3_column_int(validate_stmt, 0));
        }
        sqlite3_finalize(validate_stmt);
    } else {
        printf("[座位预约] SQL准备失败: %s\n", sqlite3_errmsg(db));
    }
    
    if (!student_exists) {
        printf("[座位预约] *** 学号验证失败: %s (学号不存在) ***\n", student_id->valuestring);
        printf("[座位预约] 拒绝预约请求\n");
        cJSON_AddStringToObject(response_data, "status", "error");
        cJSON_AddStringToObject(response_data, "error", "学号不存在，请输入正确的学号");
        
        cJSON_AddStringToObject(response, "type", "reservation_response");
        cJSON_AddItemToObject(response, "data", response_data);
        
        char *json_str = cJSON_PrintUnformatted(response);
        printf("[座位预约] 发送响应: %s\n", json_str);
        char send_buf[512];
        snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
        write(client_fd, send_buf, strlen(send_buf));
        
        free(json_str);
        cJSON_Delete(response);
        printf("[座位预约] ==========================================\n\n");
        return;
    }
    
    printf("[座位预约] ✓ 学号验证通过: %s\n", student_id->valuestring);
    
    // 第二步：检查该学生是否已有预约
    sqlite3_stmt *check_stmt;
    const char *check_sql = "SELECT COUNT(*) FROM library_seats WHERE student_id = ? AND is_reserved = 1";
    int has_reservation = 0;
    
    if (sqlite3_prepare_v2(db, check_sql, -1, &check_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(check_stmt, 1, student_id->valuestring, -1, SQLITE_STATIC);
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            has_reservation = sqlite3_column_int(check_stmt, 0) > 0;
        }
        sqlite3_finalize(check_stmt);
    }
    
    if (has_reservation) {
        cJSON_AddStringToObject(response_data, "status", "error");
        cJSON_AddStringToObject(response_data, "error", "您已有预约，不能重复预约");
    } else {
        // 检查座位是否可用
        sqlite3_stmt *seat_stmt;
        const char *seat_sql = "SELECT is_reserved FROM library_seats WHERE seat_number = ?";
        int seat_reserved = 0;
        
        if (sqlite3_prepare_v2(db, seat_sql, -1, &seat_stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(seat_stmt, 1, seat_number->valueint);
            if (sqlite3_step(seat_stmt) == SQLITE_ROW) {
                seat_reserved = sqlite3_column_int(seat_stmt, 0);
            }
            sqlite3_finalize(seat_stmt);
        }
        //座位已经被预约
        if (seat_reserved) {
            cJSON_AddStringToObject(response_data, "status", "error");
            cJSON_AddStringToObject(response_data, "error", "该座位已被预约");
        //座位没被预约，执行预约
        } else {
            sqlite3_stmt *stmt;
            const char *sql = "UPDATE library_seats SET is_reserved = 1, student_id = ?, time_slot = ?, reserved_time = datetime('now', 'localtime') WHERE seat_number = ?";
            
            if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_text(stmt, 1, student_id->valuestring, -1, SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, time_slot->valuestring, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, seat_number->valueint);
                
                if (sqlite3_step(stmt) == SQLITE_DONE) {
                    cJSON_AddStringToObject(response_data, "status", "success");
                    cJSON_AddNumberToObject(response_data, "seat_number", seat_number->valueint);
                    printf("图书馆预约成功: 座位%d, 学号%s, 时间段%s\n", 
                           seat_number->valueint, student_id->valuestring, time_slot->valuestring);
                } else {
                    cJSON_AddStringToObject(response_data, "status", "error");
                    cJSON_AddStringToObject(response_data, "error", "预约失败");
                }
                sqlite3_finalize(stmt);
            }
        }
    }
    
    cJSON_AddStringToObject(response, "type", "reservation_response");
    cJSON_AddItemToObject(response, "data", response_data);
    
    char *json_str = cJSON_PrintUnformatted(response);
    char send_buf[512];
    snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
    // 网络传输
    write(client_fd, send_buf, strlen(send_buf));
    
    free(json_str);
    cJSON_Delete(response);
}

/*
 * 函数功能：处理取消座位预约请求，将座位恢复为可用状态
 * @client_fd: 客户端 socket 文件描述符
 * @data: 客户端的JSON 数据
 * @db: SQLite 数据库句柄
 */
static void handle_cancel_reservation(int client_fd, cJSON *data, sqlite3 *db) {
    cJSON *seat_number = cJSON_GetObjectItem(data, "seat_number");
    cJSON *student_id = cJSON_GetObjectItem(data, "student_id");
    
    if (!seat_number || !student_id) return;
    
    sqlite3_stmt *stmt;
    const char *sql = "UPDATE library_seats SET is_reserved = 0, student_id = '', time_slot = '' WHERE seat_number = ? AND student_id = ?";
    
    cJSON *response = cJSON_CreateObject();
    cJSON *response_data = cJSON_CreateObject();
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, seat_number->valueint);
        sqlite3_bind_text(stmt, 2, student_id->valuestring, -1, SQLITE_STATIC);
        
        if (sqlite3_step(stmt) == SQLITE_DONE && sqlite3_changes(db) > 0) {
            cJSON_AddStringToObject(response_data, "status", "success");
            printf("取消预约成功: 座位%d, 学号%s\n", seat_number->valueint, student_id->valuestring);
        } else {
            cJSON_AddStringToObject(response_data, "status", "error");
            cJSON_AddStringToObject(response_data, "error", "取消失败，请确认是您的预约");
        }
        sqlite3_finalize(stmt);
    }
    
    cJSON_AddStringToObject(response, "type", "cancel_response");
    cJSON_AddItemToObject(response, "data", response_data);
    
    char *json_str = cJSON_PrintUnformatted(response);
    char send_buf[512];
    snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
    write(client_fd, send_buf, strlen(send_buf));
    
    free(json_str);
    cJSON_Delete(response);
}

/*
 * 函数功能：处理获取公告列表请求，返回最新 10 条公告
 * @client_fd: 客户端 socket 文件描述符
 * @data: JSON 数据（本函数未使用）
 * @db: SQLite 数据库句柄
 */
static void handle_get_notices(int client_fd, cJSON *data, sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT notice_id, title, content FROM notices ORDER BY notice_id DESC LIMIT 10";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        cJSON *response = cJSON_CreateObject();
        cJSON *response_data = cJSON_CreateObject();
        cJSON *notices_array = cJSON_CreateArray();
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *notice = cJSON_CreateObject();
            cJSON_AddNumberToObject(notice, "notice_id", sqlite3_column_int(stmt, 0));
            cJSON_AddStringToObject(notice, "title", (const char*)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(notice, "content", (const char*)sqlite3_column_text(stmt, 2));
            cJSON_AddItemToArray(notices_array, notice);
        }
        
        cJSON_AddItemToObject(response_data, "notices", notices_array);
        cJSON_AddStringToObject(response, "type", "notice_list");
        cJSON_AddItemToObject(response, "data", response_data);
        
        char *json_str = cJSON_PrintUnformatted(response);
        char send_buf[16384];
        snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
        write(client_fd, send_buf, strlen(send_buf));
        
        free(json_str);
        cJSON_Delete(response);
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：处理获取公告详情请求，返回指定 ID 的公告内容
 * @client_fd: 客户端 socket 文件描述符
 * @data: JSON 数据（包含 notice_id）
 * @db: SQLite 数据库句柄
 */
static void handle_get_notice_detail(int client_fd, cJSON *data, sqlite3 *db) {
    cJSON *notice_id = cJSON_GetObjectItem(data, "notice_id");
    if (!notice_id) return;
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT notice_id, title, content FROM notices WHERE notice_id = ?";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, notice_id->valueint);
        
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *response = cJSON_CreateObject();
            cJSON *response_data = cJSON_CreateObject();
            
            cJSON_AddNumberToObject(response_data, "notice_id", sqlite3_column_int(stmt, 0));
            cJSON_AddStringToObject(response_data, "title", (const char*)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(response_data, "content", (const char*)sqlite3_column_text(stmt, 2));
            
            cJSON_AddStringToObject(response, "type", "notice_detail");
            cJSON_AddItemToObject(response, "data", response_data);
            
            char *json_str = cJSON_PrintUnformatted(response);
            char send_buf[8192];
            snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
            write(client_fd, send_buf, strlen(send_buf));
            
            free(json_str);
            cJSON_Delete(response);
        }
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：处理获取考勤名单请求，根据教室和当前时间查询课程对应的学生列表
 * @client_fd: 客户端 socket 文件描述符
 * @data: JSON 数据（包含 classroom）
 * @db: SQLite 数据库句柄
 */
static void handle_get_attendance_list(int client_fd, cJSON *data, sqlite3 *db) {
    cJSON *classroom = cJSON_GetObjectItem(data, "classroom");
    if (!classroom) {
        printf("缺少classroom参数\n");
        cJSON *response = cJSON_CreateObject();
        cJSON *response_data = cJSON_CreateObject();
        cJSON *students_array = cJSON_CreateArray();
        
        cJSON_AddItemToObject(response_data, "students", students_array);
        cJSON_AddNumberToObject(response_data, "course_id", 0);
        cJSON_AddStringToObject(response, "type", "attendance_list");
        cJSON_AddItemToObject(response, "data", response_data);
        
        char *json_str = cJSON_PrintUnformatted(response);
        char send_buf[512];
        snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
        write(client_fd, send_buf, strlen(send_buf));
        
        free(json_str);
        cJSON_Delete(response);
        return;
    }
    
    // 获取当前时间和课程
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    int current_hour = tm_info->tm_hour;
    int current_min = tm_info->tm_min;
    int weekday = (tm_info->tm_wday == 0) ? 7 : tm_info->tm_wday;
    
    // 计算当前时间段
    int period = 0;
    if (current_hour == 8 || (current_hour == 9 && current_min <= 40)) {
        period = 1;
    } else if ((current_hour == 9 && current_min >= 50) || (current_hour == 10) || (current_hour == 11 && current_min <= 30)) {
        period = 2;
    } else if (current_hour == 14 || (current_hour == 15 && current_min <= 40)) {
        period = 3;
    } else if ((current_hour == 15 && current_min >= 50) || (current_hour == 16) || (current_hour == 17 && current_min <= 30)) {
        period = 4;
    } else if (current_hour == 19 || (current_hour == 20 && current_min <= 40)) {
        period = 5;
    }
    
    printf("[考勤查询] 当前时间: %02d:%02d, 星期%d, 时间段%d, 教室%s\n", 
           current_hour, current_min, weekday, period, classroom->valuestring);
    
    // 查询当前时间段的课程
    Course course;
    int course_id = 0;
    char class_names[512] = "";
    
    if (period > 0 && db_get_course_by_time(db, classroom->valuestring, weekday, period, &course) == 0) {
        course_id = course.course_id;
        strncpy(class_names, course.class_name, sizeof(class_names) - 1);
        printf("[考勤查询] 找到课程: %s, 上课班级: %s\n", course.course_name, class_names);
    } else {
        printf("[考勤查询] 当前时间段没有课程\n");
    }
    
    cJSON *response = cJSON_CreateObject();
    cJSON *response_data = cJSON_CreateObject();
    cJSON *students_array = cJSON_CreateArray();
    
    // 如果找到了课程，根据班级查询学生
    if (course_id > 0) {
        sqlite3_stmt *stmt;
        // 处理多个班级（用逗号分隔）
        const char *sql = "SELECT s.student_id, s.name, s.class_name FROM students s "
                         "WHERE s.class_name IN (";
        
        // 构建SQL的IN子句
        char full_sql[1024];
        strcpy(full_sql, sql);
        
        // 分割班级名称
        char *class_list = strdup(class_names);
        char *token = strtok(class_list, ",");
        int first = 1;
        
        while (token != NULL) {
            // 去除首尾空格
            while (*token == ' ') token++;
            char *end = token + strlen(token) - 1;
            while (end > token && *end == ' ') end--;
            *(end + 1) = '\0';
            
            if (!first) {
                strcat(full_sql, ",");
            }
            strcat(full_sql, "'");
            strcat(full_sql, token);
            strcat(full_sql, "'");
            first = 0;
            
            token = strtok(NULL, ",");
        }
        strcat(full_sql, ") ORDER BY s.student_id");
        
        printf("[考勤查询] SQL: %s\n", full_sql);
        
        if (sqlite3_prepare_v2(db, full_sql, -1, &stmt, NULL) == SQLITE_OK) {
            int count = 0;
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                cJSON *student = cJSON_CreateObject();
                cJSON_AddStringToObject(student, "student_id", (const char*)sqlite3_column_text(stmt, 0));
                cJSON_AddStringToObject(student, "name", (const char*)sqlite3_column_text(stmt, 1));
                cJSON_AddStringToObject(student, "class", (const char*)sqlite3_column_text(stmt, 2));
                cJSON_AddItemToArray(students_array, student);
                count++;
            }
            
            printf("[考勤查询] 返回%d名学生\n", count);
            sqlite3_finalize(stmt);
        }
        
        free(class_list);
    }
    
    cJSON_AddItemToObject(response_data, "students", students_array);
    cJSON_AddNumberToObject(response_data, "course_id", course_id);
    cJSON_AddStringToObject(response, "type", "attendance_list");
    cJSON_AddItemToObject(response, "data", response_data);
    
    char *json_str = cJSON_PrintUnformatted(response);
    char send_buf[8192];
    snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
    write(client_fd, send_buf, strlen(send_buf));
    
    free(json_str);
    cJSON_Delete(response);
}

/*
 * 函数功能：处理提交考勤请求，将缺勤学生记录插入数据库
 * @client_fd: 客户端 socket 文件描述符
 * @data: JSON 数据（包含 course_id, absent_students）
 * @db: SQLite 数据库句柄
 */
static void handle_submit_attendance(int client_fd, cJSON *data, sqlite3 *db) {
    printf("\n[考勤提交] 收到提交请求\n");
    printf("[考勤提交] 原始data: %s\n", cJSON_PrintUnformatted(data));
    
    cJSON *course_id = cJSON_GetObjectItem(data, "course_id");
    cJSON *absent_students = cJSON_GetObjectItem(data, "absent_students");
    
    printf("[考勤提交] course_id存在: %s\n", course_id ? "是" : "否");
    printf("[考勤提交] absent_students存在: %s\n", absent_students ? "是" : "否");
    
    if (!course_id || !absent_students) {
        printf("[考勤提交] *** 缺少必要参数 ***\n");
        if (!course_id) printf("[考勤提交] 缺少course_id\n");
        if (!absent_students) printf("[考勤提交] 缺少absent_students\n");
        return;
    }
    
    int course_id_val = course_id->valueint;
    printf("[考勤提交] 课程ID值: %d\n", course_id_val);
    
    // 查询课程信息用于日志输出
    sqlite3_stmt *course_stmt;
    const char *course_sql = "SELECT course_name, teacher, class_name FROM courses WHERE course_id = ?";
    char course_name[100] = "未知课程";
    char teacher[50] = "未知教师";
    char class_name[100] = "未知班级";
    
    if (sqlite3_prepare_v2(db, course_sql, -1, &course_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(course_stmt, 1, course_id_val);
        if (sqlite3_step(course_stmt) == SQLITE_ROW) {
            strncpy(course_name, (const char*)sqlite3_column_text(course_stmt, 0), sizeof(course_name) - 1);
            strncpy(teacher, (const char*)sqlite3_column_text(course_stmt, 1), sizeof(teacher) - 1);
            strncpy(class_name, (const char*)sqlite3_column_text(course_stmt, 2), sizeof(class_name) - 1);
        }
        sqlite3_finalize(course_stmt);
    }
    
    printf("\n╔════════════════════════════════════════════╗\n");
    printf("║          收到考勤提交记录                  ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("  课程: %s\n", course_name);
    printf("  教师: %s\n", teacher);
    printf("  班级: %s\n", class_name);
    printf("  课程ID: %d\n", course_id_val);
    
    // 处理absent_students：如果是字符串则解析成数组
    cJSON *absent_array = NULL;
    int need_free_array = 0;
    
    if (absent_students->type & cJSON_String) {
        printf("[考勤提交] absent_students是字符串，尝试解析: %s\n", absent_students->valuestring);
        // 解析字符串为JSON数组
        absent_array = cJSON_Parse(absent_students->valuestring);
        if (absent_array) {
            printf("[考勤提交] 字符串解析成功\n");
            need_free_array = 1;
        } else {
            printf("[考勤提交] 字符串解析失败\n");
        }
    } else if (absent_students->type & cJSON_Array) {
        printf("[考勤提交] absent_students已经是数组\n");
        absent_array = absent_students;
    } else {
        printf("[考勤提交] absent_students类型未知: %d\n", absent_students->type);
    }
    
    // 遍历absent_students数组，为每个缺勤学生插入一条记录
    if (absent_array && (absent_array->type & cJSON_Array)) {
        int absent_count = cJSON_GetArraySize(absent_array);
        printf("[考勤提交] absent_students数组大小: %d\n", absent_count);
        
        if (absent_count == 0) {
            printf("  ✓ 全员出勤！无缺勤学生\n");
        } else {
            printf("  ✗ 缺勤人数: %d\n", absent_count);
            printf("  缺勤学生名单:\n");
            
            sqlite3_stmt *stmt;
            const char *sql = "INSERT INTO attendance (course_id, absent_student_id, record_time) "
                            "VALUES (?, ?, datetime('now', 'localtime'))";
            
            for (int i = 0; i < absent_count; i++) {
                cJSON *student_item = cJSON_GetArrayItem(absent_array, i);
                const char *sid = NULL;
                const char *sname = NULL;
                const char *sclass = NULL;
                
                // 判断是字符串（只有学号）还是对象（完整信息）
                if (student_item && (student_item->type & cJSON_String)) {
                    // 只有学号，需要查询学生信息
                    sid = student_item->valuestring;
                    
                    sqlite3_stmt *query_stmt;
                    const char *query_sql = "SELECT name, class_name FROM students WHERE student_id = ?";
                    if (sqlite3_prepare_v2(db, query_sql, -1, &query_stmt, NULL) == SQLITE_OK) {
                        sqlite3_bind_text(query_stmt, 1, sid, -1, SQLITE_STATIC);
                        if (sqlite3_step(query_stmt) == SQLITE_ROW) {
                            sname = (const char*)sqlite3_column_text(query_stmt, 0);
                            sclass = (const char*)sqlite3_column_text(query_stmt, 1);
                            
                            printf("    %d. %s (%s) - %s\n", i + 1, sname, sid, sclass);
                        } else {
                            printf("    %d. 学号%s (未找到学生信息)\n", i + 1, sid);
                        }
                        sqlite3_finalize(query_stmt);
                    }
                } else if (student_item && (student_item->type & cJSON_Object)) {
                    // 完整的学生对象
                    cJSON *student_id = cJSON_GetObjectItem(student_item, "student_id");
                    cJSON *name = cJSON_GetObjectItem(student_item, "name");
                    cJSON *class_item = cJSON_GetObjectItem(student_item, "class");
                    
                    if (student_id && name) {
                        sid = student_id->valuestring;
                        sname = name->valuestring;
                        sclass = class_item ? class_item->valuestring : "未知班级";
                        
                        printf("    %d. %s (%s) - %s\n", i + 1, sname, sid, sclass);
                    }
                }
                
                // 插入数据库
                if (sid && sname) {
                    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
                        sqlite3_bind_int(stmt, 1, course_id_val);
                        sqlite3_bind_text(stmt, 2, sid, -1, SQLITE_TRANSIENT);
                        
                        int step_result = sqlite3_step(stmt);
                        if (step_result == SQLITE_DONE) {
                            printf("       [成功] 数据已插入数据库\n");
                        } else {
                            printf("       [错误] 插入数据库失败: %s (错误码: %d)\n", 
                                   sqlite3_errmsg(db), step_result);
                        }
                        sqlite3_finalize(stmt);
                    } else {
                        printf("       [错误] SQL准备失败: %s\n", sqlite3_errmsg(db));
                    }
                }
            }
        }
    }
    
    printf("════════════════════════════════════════════\n\n");
    
    // 释放解析的数组
    if (need_free_array && absent_array) {
        cJSON_Delete(absent_array);
    }
    
    // 发送响应
    cJSON *response = cJSON_CreateObject();
    cJSON *response_data = cJSON_CreateObject();
    cJSON_AddStringToObject(response_data, "status", "success");
    cJSON_AddStringToObject(response, "type", "attendance_response");
    cJSON_AddItemToObject(response, "data", response_data);
    
    char *json_str = cJSON_PrintUnformatted(response);
    char send_buf[512];
    snprintf(send_buf, sizeof(send_buf), "%s\n", json_str);
    write(client_fd, send_buf, strlen(send_buf));
    
    free(json_str);
    cJSON_Delete(response);
}

/*
 * 函数功能：TCP 消息分发器，根据 type 字段路由到对应的业务处理函数，把data给客户端
 * @client_fd: 客户端 socket 文件描述符
 * @message: 客户端发送的 JSON 字符串（以 \n 结尾）
 * @db: SQLite 数据库句柄
 */
void handle_client_message(int client_fd, const char *message, sqlite3 *db) {
    cJSON *json = cJSON_Parse(message);
    if (!json) {
        printf("JSON解析失败: %s\n", message);
        return;
    }
    
    cJSON *type_item = cJSON_GetObjectItem(json, "type");
    cJSON *data_item = cJSON_GetObjectItem(json, "data");
    
    if (!type_item || !data_item) {
        cJSON_Delete(json);
        return;
    }
    
    const char *type = type_item->valuestring;
    printf("收到消息类型: %s\n", type);
    
    if (strcmp(type, "get_course") == 0) {
        handle_get_course(client_fd, data_item, db);
    } else if (strcmp(type, "send_message") == 0) {
        handle_send_message(client_fd, data_item, db);
    } else if (strcmp(type, "get_seat_status") == 0) {
        handle_get_seat_status(client_fd, data_item, db);
    } else if (strcmp(type, "reserve_seat") == 0) {
        handle_reserve_seat(client_fd, data_item, db);
    } else if (strcmp(type, "cancel_reservation") == 0) {
        handle_cancel_reservation(client_fd, data_item, db);
    } else if (strcmp(type, "get_notices") == 0) {
        handle_get_notices(client_fd, data_item, db);
    } else if (strcmp(type, "get_notice_detail") == 0) {
        handle_get_notice_detail(client_fd, data_item, db);
    } else if (strcmp(type, "get_attendance_list") == 0) {
        handle_get_attendance_list(client_fd, data_item, db);
    } else if (strcmp(type, "submit_attendance") == 0) {
        handle_submit_attendance(client_fd, data_item, db);
    }
    
    cJSON_Delete(json);
}

/*  将 fd 设置为非阻塞模式                                   */
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/*   在连接池中寻找一个空闲槽位，返回下标；找不到返回 -1      */
static int alloc_client_slot(Server *server) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].fd == -1)
            return i;
    }
    return -1;
}

/*  内部辅助：根据 fd 找到连接池下标，找不到返回 -1                    */
static int find_client_slot(Server *server, int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].fd == fd)
            return i;
    }
    return -1;
}


/*  内部辅助：关闭一个客户端连接，从 epoll 注销并清理槽位              */
static void close_client(Server *server, int slot) {
    int fd = server->clients[slot].fd;
    if (fd < 0) return;

    /* 从 epoll 实例中移除该 fd */
    epoll_ctl(server->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);

    /* 清空槽位，标记为空闲 */
    server->clients[slot].fd      = -1;
    server->clients[slot].buf_len = 0;
    memset(server->clients[slot].buf, 0, CLIENT_BUF_SIZE);

    printf("[TCP] 客户端断开连接 (fd=%d)\n", fd);
}

/* ------------------------------------------------------------------ */
/*  内部辅助：处理一个客户端 fd 的可读事件                             */
/*                                                                      */
/*  循环调用 recv() 直到返回 EAGAIN（非阻塞 fd），每读到完整行（'\n'   */
/*  结尾）就调用 handle_client_message() 处理业务逻辑。                */
/* ------------------------------------------------------------------ */
static void handle_client_readable(Server *server, int slot) {
    ClientConn *conn = &server->clients[slot];
    char tmp[1024];

    while (1) {
        /* 非阻塞读，防止单连接占用过长时间 */
        int n = recv(conn->fd, tmp, sizeof(tmp) - 1, 0);

        if (n > 0) {
            /* 防溢出：剩余空间不足时丢弃旧数据（异常客户端保护） */
            if (conn->buf_len + n >= CLIENT_BUF_SIZE) {
                fprintf(stderr, "[TCP] fd=%d 缓冲区溢出，清空重置\n", conn->fd);
                conn->buf_len = 0;
            }
            memcpy(conn->buf + conn->buf_len, tmp, n);
            conn->buf_len += n;
            conn->buf[conn->buf_len] = '\0';

            /* 扫描缓冲区，提取所有以 '\n' 结尾的完整消息 */
            char *newline;
            while ((newline = strchr(conn->buf, '\n')) != NULL) {
                *newline = '\0';
                /* 业务分发（与原版逻辑完全一致） */
                handle_client_message(conn->fd, conn->buf, server->db);
                /* 将剩余数据移到缓冲区头部 */
                int remaining = conn->buf_len - (int)(newline - conn->buf + 1);
                if (remaining > 0)
                    memmove(conn->buf, newline + 1, remaining);
                conn->buf_len = (remaining > 0) ? remaining : 0;
                conn->buf[conn->buf_len] = '\0';
            }

        } else if (n == 0) {
            /* 对端正常关闭连接 */
            close_client(server, slot);
            return;
        } else {
            /* n < 0 */
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* 数据已读完，等待下次 epoll 通知 */
                break;
            }
            /* 其他错误：关闭连接 */
            perror("[TCP] recv 错误");
            close_client(server, slot);
            return;
        }
    }
}


/*                  初始化 TCP 服务器                                 */
int server_init(Server *server, int port, sqlite3 *db) {
    server->port      = port;
    server->db        = db;
    server->running   = 0;
    server->epoll_fd  = -1;      /* epoll 文件描述符初始为 -1（无效值） */

    /* 初始化连接池：所有槽位标记为空闲 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        server->clients[i].fd      = -1;
        server->clients[i].buf_len = 0;
    }

    /* 1. 创建监听 socket */
    /* AF_INET: IPv4 协议族，SOCK_STREAM: 面向连接的 TCP 协议，0: 自动选择协议（TCP） */
    server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->server_fd < 0) {
        perror("[TCP] 创建 socket 失败");
        return -1;
    }

    /* 允许地址复用，避免重启时 "Address already in use" */
    int opt = 1;
    setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 2. 绑定端口 */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;             /* 地址族：IPv4 */
    addr.sin_addr.s_addr = INADDR_ANY;          /* 绑定到所有网卡（0.0.0.0），接受任何 IP 的连接 */
    addr.sin_port        = htons(port);         /* 端口号：主机字节序 → 网络字节序（大端） */

    if (bind(server->server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[TCP] 绑定端口失败");
        close(server->server_fd);
        return -1;
    }

    /* 3. 开始监听，积压队列设为 SOMAXCONN (Linux内核规定 128) */
    /* 设置小了并发能力差，设置大了，性能无法保障 */
    if (listen(server->server_fd, SOMAXCONN) < 0) {
        perror("[TCP] listen 失败");
        close(server->server_fd);
        return -1;
    }

    /* 4. 将监听 fd 设为非阻塞 */
    if (set_nonblocking(server->server_fd) < 0) {
        perror("[TCP] 设置非阻塞失败");
        close(server->server_fd);
        return -1;
    }

    /* 5. 创建 epoll 实例 */
    server->epoll_fd = epoll_create(1);
    if (server->epoll_fd < 0) {
        perror("[TCP] epoll_create 失败");
        close(server->server_fd);
        return -1;
    }
    /* 定义 epoll 事件结构体，用于描述要监听的事件类型和关联数据 */
    struct epoll_event ev;  
    ev.events  = EPOLLIN; // 监听可读事件
    ev.data.fd = server->server_fd;  // 通过 data.fd 可以知道是哪个 fd 发生了事件
    // 控制 epoll 实例，执行添加、修改、删除操作
    if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server->server_fd, &ev) < 0) {
        perror("[TCP] epoll_ctl 注册监听 fd 失败");
        close(server->epoll_fd);
        close(server->server_fd);
        return -1;
    }

    printf("[TCP] 服务器初始化成功，端口: %d，最大并发连接: %d\n", port, MAX_CLIENTS);
    return 0;
}

/* ------------------------------------------------------------------ */
/*  server_start() —— 启动 epoll 事件循环                                */
/* ------------------------------------------------------------------ */
int server_start(Server *server) {
    server->running = 1;
    printf("[TCP] 事件循环启动，等待客户端连接...\n");

    /* 定义 epoll 事件数组，用于存放 epoll_wait() 返回的就绪事件列表 */
    struct epoll_event events[MAX_EPOLL_EVENTS];

    /* 用于 accept() 时获取客户端地址信息 */
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (server->running) {
        /* 
        * @server->epoll_fd    epoll 实例句柄
        * @events              输出：存放就绪事件的数组（函数将其赋值）  
        * @MAX_EPOLL_EVENTS    events 数组最大容量（自定义64个）
        * @500                 超时时间（毫秒），0.5秒返回一次
        * @return              返回就绪事件数量，>0有事件，0超时，-1出错
        */
        int nfds = epoll_wait(server->epoll_fd, events, MAX_EPOLL_EVENTS, 500);

        if (nfds < 0) {
            if (errno == EINTR) continue;   /* 被信号中断，重试 */
            perror("[TCP] epoll_wait 错误");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            /* 
            从事件中提取文件描述符,文件描述符可能是客户端或者服务端 
            监听 fd（server_fd） → 表示有新连接；
            客户端 fd（某个教室设备） → 表示有数据到达 
            */
            int fd = events[i].data.fd;

            /* ---- 监听 fd 可读：有新连接到来 ---- */
            if (fd == server->server_fd) {
                /* 非阻塞 accept()，一次性接受所有排队连接 */
                while (1) {
                    int cli_fd = accept(server->server_fd,
                                        (struct sockaddr*)&client_addr,
                                        &client_len);
                    if (cli_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;  /* 队列已空 */
                        perror("[TCP] accept 失败");
                        break;
                    }

                    /* 为新连接分配连接池槽位 */
                    int slot = alloc_client_slot(server);
                    if (slot < 0) {
                        fprintf(stderr, "[TCP] 连接池已满，拒绝新连接 fd=%d\n", cli_fd);
                        close(cli_fd);
                        continue;
                    }

                    /* 将客户端 fd 设为非阻塞 */
                    set_nonblocking(cli_fd);

                    /* 初始化连接槽位 */
                    server->clients[slot].fd      = cli_fd;
                    server->clients[slot].buf_len = 0;

                    /* 将客户端 fd 注册到 epoll */
                    struct epoll_event cev;
                    cev.events  = EPOLLIN | EPOLLET;  /* 边缘触发，高效 */
                    cev.data.fd = cli_fd;
                    /* 把客户端的fd加入监听列表 */
                    if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, cli_fd, &cev) < 0) {
                        perror("[TCP] epoll_ctl 注册客户端 fd 失败");
                        close(cli_fd);
                        server->clients[slot].fd = -1;
                        continue;
                    }

                    printf("[TCP] 新连接: %s:%d (fd=%d, slot=%d)\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port),
                           cli_fd, slot);
                }

            /* ---- 客户端 fd 可读：处理消息 ---- */
            } else if (events[i].events & EPOLLIN) {
                int slot = find_client_slot(server, fd);
                if (slot >= 0) {
                    // 处理客户的核心逻辑
                    handle_client_readable(server, slot);
                }

            /* ---- 连接异常（对端关闭或出错） ---- */
            } else if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                int slot = find_client_slot(server, fd);
                if (slot >= 0) {
                    fprintf(stderr, "[TCP] 连接异常 fd=%d，关闭\n", fd);
                    close_client(server, slot);
                }
            }
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  server_stop() —— 停止服务器                                        */
/* ------------------------------------------------------------------ */
void server_stop(Server *server) {
    server->running = 0;

    /* 关闭所有客户端连接 */
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (server->clients[i].fd >= 0)
            close_client(server, i);
    }

    /* 关闭 epoll 实例和监听 fd */
    if (server->epoll_fd >= 0) {
        close(server->epoll_fd);
        server->epoll_fd = -1;
    }
    close(server->server_fd);

    printf("[TCP] 服务器已停止\n");
}
