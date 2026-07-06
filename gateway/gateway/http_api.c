#include "http_api.h"
#include "http_mongoose.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/*
 * 函数功能：获取当前时间字符串
 * @buf: 输出缓冲区
 * @size: 缓冲区大小
 */
static void now_str(char *buf, int size) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", tm);
}

/*
 * 函数功能：从数据库查询所有课程，以JSON格式返回给Web前端
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求信息
 * @db: SQLite数据库句柄，用于执行查询
 */
static void handle_get_courses(struct mg_connection *c,
                                struct mg_http_message *hm,
                                sqlite3 *db) {
    const char *weekday_map[] = {"", "周一", "周二", "周三", "周四",
                                  "周五", "周六", "周日"};
    const char *period_map[]  = {"", "08:00-09:40", "09:50-11:30",
                                  "14:00-15:40", "15:50-17:30", "19:00-20:40"};

    sqlite3_stmt *stmt;
    const char *sql = "SELECT course_id, classroom, course_name, teacher, "
                      "class_name, weekday, period FROM courses ORDER BY course_id ASC";

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();
    
    /* 执行SQL，遍历结果集 */
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *item = cJSON_CreateObject();
            int course_id = sqlite3_column_int(stmt, 0);
            int weekday   = sqlite3_column_int(stmt, 5);
            int period    = sqlite3_column_int(stmt, 6);

            cJSON_AddNumberToObject(item, "course_id", course_id);
            cJSON_AddStringToObject(item, "classroom",
                (const char *)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(item, "course_name",
                (const char *)sqlite3_column_text(stmt, 2));
            cJSON_AddStringToObject(item, "teacher",
                (const char *)sqlite3_column_text(stmt, 3));
            cJSON_AddStringToObject(item, "class_name",
                (const char *)sqlite3_column_text(stmt, 4));
            cJSON_AddStringToObject(item, "weekday",
                (weekday >= 1 && weekday <= 7) ? weekday_map[weekday] : "");
            cJSON_AddNumberToObject(item, "weekday_num", weekday);
            cJSON_AddStringToObject(item, "period",
                (period >= 1 && period <= 5) ? period_map[period] : "");
            cJSON_AddNumberToObject(item, "period_num", period);

            cJSON_AddItemToArray(data, item);
        }
        sqlite3_finalize(stmt);
    }

    /* 构造响应: {success: true, data: [...]} */
    cJSON_AddItemToObject(root, "success", cJSON_CreateBool(1));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    /* 将 JSON 数据作为 HTTP 200 OK 响应发送给客户端 */
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(root);
}

/*
 * 函数功能：添加新课程，将前端提交的课程数据插入数据库
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含POST请求的JSON数据
 * @db: SQLite数据库句柄，用于执行插入操作
 */
static void handle_add_course(struct mg_connection *c,
                               struct mg_http_message *hm,
                               sqlite3 *db) {
    /* 从 POST body 解析 JSON */
    cJSON *req_json = cJSON_Parse((const char *)hm->body.ptr);
    if (!req_json) {
        http_mongoose_send_json_error(c, 400, "无效的 JSON");
        return;
    }

    const char *classroom  = (cJSON_GetObjectItem(req_json, "classroom")->valuestring);
    const char *course_name = (cJSON_GetObjectItem(req_json, "course_name")->valuestring);
    const char *teacher    = (cJSON_GetObjectItem(req_json, "teacher")->valuestring);
    const char *class_name = (cJSON_GetObjectItem(req_json, "class_name")->valuestring);
    cJSON *weekday_obj = cJSON_GetObjectItem(req_json, "weekday_num");
    cJSON *period_obj  = cJSON_GetObjectItem(req_json, "period_num");

    if (!classroom || !course_name || !teacher || !class_name || !weekday_obj || !period_obj) {
        http_mongoose_send_json_error(c, 400, "缺少必要参数");
        cJSON_Delete(req_json);
        return;
    }

    int weekday = weekday_obj->valueint;
    int period  = period_obj->valueint;

    const char *sql = "INSERT INTO courses (classroom, course_name, teacher, class_name, weekday, period) "
                      "VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, classroom, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, course_name, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, teacher, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, class_name, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 5, weekday);
        sqlite3_bind_int(stmt, 6, period);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }

    cJSON_Delete(req_json);
}

/*
 * 函数功能：删除指定ID的课程，从数据库中移除课程记录
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含URL路径中的课程ID
 * @db: SQLite数据库句柄，用于执行删除操作
 */
static void handle_delete_course(struct mg_connection *c,
                                  struct mg_http_message *hm,
                                  sqlite3 *db) {
    /* 从路径中提取 course_id (e.g., /api/courses/123 → 123) */
    int course_id = 0;
    sscanf(hm->uri.ptr, "/api/courses/%d", &course_id);

    if (course_id <= 0) {
        http_mongoose_send_json_error(c, 400, "无效的 course_id");
        return;
    }

    const char *sql = "DELETE FROM courses WHERE course_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, course_id);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：获取所有公告列表，按ID降序返回最新公告
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求信息
 * @db: SQLite数据库句柄，用于执行查询
 */
static void handle_get_notices(struct mg_connection *c,
                                struct mg_http_message *hm,
                                sqlite3 *db) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT notice_id, title, content, created_at FROM notices ORDER BY notice_id DESC";

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "notice_id", sqlite3_column_int(stmt, 0));
            cJSON_AddStringToObject(item, "title", (const char *)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(item, "content", (const char *)sqlite3_column_text(stmt, 2));
            cJSON_AddStringToObject(item, "created_at", (const char *)sqlite3_column_text(stmt, 3));
            cJSON_AddItemToArray(data, item);
        }
        sqlite3_finalize(stmt);
    }

    cJSON_AddItemToObject(root, "success", cJSON_CreateBool(1));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(root);
}

/*
 * 函数功能：发布新公告，将公告数据插入数据库
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含POST请求的JSON数据
 * @db: SQLite数据库句柄，用于执行插入操作
 */
static void handle_add_notice(struct mg_connection *c,
                               struct mg_http_message *hm,
                               sqlite3 *db) {
    cJSON *req_json = cJSON_Parse((const char *)hm->body.ptr);
    if (!req_json) {
        http_mongoose_send_json_error(c, 400, "无效的 JSON");
        return;
    }

    const char *title = (cJSON_GetObjectItem(req_json, "title")->valuestring);
    const char *content = (cJSON_GetObjectItem(req_json, "content")->valuestring);

    if (!title || !content) {
        http_mongoose_send_json_error(c, 400, "缺少必要参数");
        cJSON_Delete(req_json);
        return;
    }

    char time_str[32];
    now_str(time_str, sizeof(time_str));

    const char *sql = "INSERT INTO notices (title, content, created_at) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, content, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, time_str, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }

    cJSON_Delete(req_json);
}

/*
 * 函数功能：删除指定ID的公告，从数据库中移除公告记录
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含URL路径中的公告ID
 * @db: SQLite数据库句柄，用于执行删除操作
 */
static void handle_delete_notice(struct mg_connection *c,
                                  struct mg_http_message *hm,
                                  sqlite3 *db) {
    int notice_id = 0;
    sscanf(hm->uri.ptr, "/api/notices/%d", &notice_id);

    if (notice_id <= 0) {
        http_mongoose_send_json_error(c, 400, "无效的 notice_id");
        return;
    }

    const char *sql = "DELETE FROM notices WHERE notice_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, notice_id);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            // 发送成功响应
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：获取所有图书馆座位的状态信息
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求信息（本函数未使用）
 * @db: SQLite数据库句柄，用于执行查询
 */
static void handle_get_library_seats(struct mg_connection *c,
                                      struct mg_http_message *hm,
                                      sqlite3 *db) {
    (void)hm;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT seat_id, is_available FROM library_seats ORDER BY seat_id ASC";

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "seat_id", sqlite3_column_int(stmt, 0));
            cJSON_AddItemToObject(item, "is_available", cJSON_CreateBool(sqlite3_column_int(stmt, 1)));
            cJSON_AddItemToArray(data, item);
        }
        sqlite3_finalize(stmt);
    }

    cJSON_AddItemToObject(root, "success", cJSON_CreateBool(1));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(root);
}

/*
 * 函数功能：预约图书馆座位，在预约表中插入记录并更新座位状态
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含POST请求的JSON数据
 * @db: SQLite数据库句柄，用于执行插入和更新操作
 */
static void handle_reserve_seat(struct mg_connection *c,
                                 struct mg_http_message *hm,
                                 sqlite3 *db) {
    cJSON *req_json = cJSON_Parse((const char *)hm->body.ptr);
    if (!req_json) {
        http_mongoose_send_json_error(c, 400, "无效的 JSON");
        return;
    }

    cJSON *seat_id_obj = cJSON_GetObjectItem(req_json, "seat_id");
    const char *student_id = (cJSON_GetObjectItem(req_json, "student_id")->valuestring);

    if (!seat_id_obj || !student_id) {
        http_mongoose_send_json_error(c, 400, "缺少必要参数");
        cJSON_Delete(req_json);
        return;
    }

    int seat_id = seat_id_obj->valueint;
    char time_str[32];
    now_str(time_str, sizeof(time_str));

    const char *sql = "INSERT INTO library_reservations (seat_id, student_id, reserved_at) VALUES (?, ?, ?)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, seat_id);
        sqlite3_bind_text(stmt, 2, student_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, time_str, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            /* 标记座位不可用 */
            const char *update_sql = "UPDATE library_seats SET is_available = 0 WHERE seat_id = ?";
            sqlite3_stmt *upd_stmt;
            if (sqlite3_prepare_v2(db, update_sql, -1, &upd_stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(upd_stmt, 1, seat_id);
                sqlite3_step(upd_stmt);
                sqlite3_finalize(upd_stmt);
            }

            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }

    cJSON_Delete(req_json);
}

/*
 * 函数功能：取消座位预约，删除预约记录并恢复座位可用状态
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含POST请求的JSON数据
 * @db: SQLite数据库句柄，用于执行查询、删除和更新操作
 */
static void handle_cancel_reservation(struct mg_connection *c,
                                       struct mg_http_message *hm,
                                       sqlite3 *db) {
    cJSON *req_json = cJSON_Parse((const char *)hm->body.ptr);
    if (!req_json) {
        http_mongoose_send_json_error(c, 400, "无效的 JSON");
        return;
    }

    cJSON *reservation_id_obj = cJSON_GetObjectItem(req_json, "reservation_id");
    if (!reservation_id_obj) {
        http_mongoose_send_json_error(c, 400, "缺少必要参数");
        cJSON_Delete(req_json);
        return;
    }

    int reservation_id = reservation_id_obj->valueint;

    /* 查询座位 ID */
    const char *sel_sql = "SELECT seat_id FROM library_reservations WHERE reservation_id = ?";
    sqlite3_stmt *sel_stmt;
    int seat_id = 0;

    if (sqlite3_prepare_v2(db, sel_sql, -1, &sel_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(sel_stmt, 1, reservation_id);
        if (sqlite3_step(sel_stmt) == SQLITE_ROW) {
            seat_id = sqlite3_column_int(sel_stmt, 0);
        }
        sqlite3_finalize(sel_stmt);
    }

    if (seat_id <= 0) {
        http_mongoose_send_json_error(c, 404, "预约不存在");
        cJSON_Delete(req_json);
        return;
    }

    /* 删除预约 */
    const char *del_sql = "DELETE FROM library_reservations WHERE reservation_id = ?";
    sqlite3_stmt *del_stmt;

    if (sqlite3_prepare_v2(db, del_sql, -1, &del_stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(del_stmt, 1, reservation_id);

        if (sqlite3_step(del_stmt) == SQLITE_DONE) {
            /* 标记座位可用 */
            const char *upd_sql = "UPDATE library_seats SET is_available = 1 WHERE seat_id = ?";
            sqlite3_stmt *upd_stmt;
            if (sqlite3_prepare_v2(db, upd_sql, -1, &upd_stmt, NULL) == SQLITE_OK) {
                sqlite3_bind_int(upd_stmt, 1, seat_id);
                sqlite3_step(upd_stmt);
                sqlite3_finalize(upd_stmt);
            }

            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(del_stmt);
    }

    cJSON_Delete(req_json);
}

/*
 * 函数功能：查询指定学生的所有预约记录
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含URL查询参数中的学号
 * @db: SQLite数据库句柄，用于执行查询
 */
static void handle_get_my_reservations(struct mg_connection *c,
                                        struct mg_http_message *hm,
                                        sqlite3 *db) {
    /* 从查询参数提取 student_id */
    char student_id_buf[256];
    int n = mg_http_get_var(&hm->query, "student_id", student_id_buf, sizeof(student_id_buf));

    if (n <= 0) {
        http_mongoose_send_json_error(c, 400, "缺少 student_id 参数");
        return;
    }

    sqlite3_stmt *stmt;
    const char *sql = "SELECT reservation_id, seat_id, reserved_at FROM library_reservations WHERE student_id = ? ORDER BY reserved_at DESC";

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, student_id_buf, -1, SQLITE_STATIC);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "reservation_id", sqlite3_column_int(stmt, 0));
            cJSON_AddNumberToObject(item, "seat_id", sqlite3_column_int(stmt, 1));
            cJSON_AddStringToObject(item, "reserved_at", (const char *)sqlite3_column_text(stmt, 2));
            cJSON_AddItemToArray(data, item);
        }
        sqlite3_finalize(stmt);
    }

    cJSON_AddItemToObject(root, "success", cJSON_CreateBool(1));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(root);
}


/*
 * 函数功能：获取所有留言列表，按ID降序返回最新留言
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求信息（本函数未使用）
 * @db: SQLite数据库句柄，用于执行查询
 */
static void handle_get_messages(struct mg_connection *c,
                                 struct mg_http_message *hm,
                                 sqlite3 *db) {
    (void)hm;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT message_id, student_name, content, created_at FROM messages ORDER BY message_id DESC";

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "message_id", sqlite3_column_int(stmt, 0));
            cJSON_AddStringToObject(item, "student_name", (const char *)sqlite3_column_text(stmt, 1));
            cJSON_AddStringToObject(item, "content", (const char *)sqlite3_column_text(stmt, 2));
            cJSON_AddStringToObject(item, "created_at", (const char *)sqlite3_column_text(stmt, 3));
            cJSON_AddItemToArray(data, item);
        }
        sqlite3_finalize(stmt);
    }

    cJSON_AddItemToObject(root, "success", cJSON_CreateBool(1));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(root);
}

/*
 * 函数功能：删除指定ID的留言，从数据库中移除留言记录
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含URL路径中的留言ID
 * @db: SQLite数据库句柄，用于执行删除操作
 */
static void handle_delete_message(struct mg_connection *c,
                                   struct mg_http_message *hm,
                                   sqlite3 *db) {
    int message_id = 0;
    sscanf(hm->uri.ptr, "/api/messages/%d", &message_id);

    if (message_id <= 0) {
        http_mongoose_send_json_error(c, 400, "无效的 message_id");
        return;
    }

    const char *sql = "DELETE FROM messages WHERE message_id = ?";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, message_id);

        if (sqlite3_step(stmt) == SQLITE_DONE) {
            cJSON *resp = cJSON_CreateObject();
            cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
            char *json_str = cJSON_PrintUnformatted(resp);
            http_mongoose_send_json_success(c, json_str);
            free(json_str);
            cJSON_Delete(resp);
        } else {
            http_mongoose_send_json_error(c, 500, sqlite3_errmsg(db));
        }
        sqlite3_finalize(stmt);
    }
}

/*
 * 函数功能：获取所有考勤记录，按签到时间降序返回
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求信息（本函数未使用）
 * @db: SQLite数据库句柄，用于执行查询
 */
static void handle_get_attendance(struct mg_connection *c,
                                   struct mg_http_message *hm,
                                   sqlite3 *db) {
    (void)hm;
    sqlite3_stmt *stmt;
    const char *sql = "SELECT attendance_id, student_name, course_id, checked_in_at FROM attendance ORDER BY checked_in_at DESC";

    cJSON *root = cJSON_CreateObject();
    cJSON *data = cJSON_CreateArray();

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            cJSON *item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "attendance_id", sqlite3_column_int(stmt, 0));
            cJSON_AddStringToObject(item, "student_name", (const char *)sqlite3_column_text(stmt, 1));
            cJSON_AddNumberToObject(item, "course_id", sqlite3_column_int(stmt, 2));
            cJSON_AddStringToObject(item, "checked_in_at", (const char *)sqlite3_column_text(stmt, 3));
            cJSON_AddItemToArray(data, item);
        }
        sqlite3_finalize(stmt);
    }

    cJSON_AddItemToObject(root, "success", cJSON_CreateBool(1));
    cJSON_AddItemToObject(root, "data", data);

    char *json_str = cJSON_PrintUnformatted(root);
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(root);
}


/*
 * 函数功能：健康检查接���，返回服务运行状态
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求信息（本函数未使用）
 * @db: SQLite数据库句柄（本函数未使用）
 */
static void handle_health_check(struct mg_connection *c,
                                 struct mg_http_message *hm,
                                 sqlite3 *db) {
    (void)hm;
    (void)db;
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddItemToObject(resp, "success", cJSON_CreateBool(1));
    cJSON_AddStringToObject(resp, "status", "healthy");

    char *json_str = cJSON_PrintUnformatted(resp);
    http_mongoose_send_json_success(c, json_str);
    free(json_str);
    cJSON_Delete(resp);
}


/*
 * HTTP API的核心处理函数
 * 函数功能：HTTP请求分发器，根据请求方法和URL路径路由到对应的处理函数
 * @c: Mongoose连接对象，用于发送HTTP响应
 * @hm: Mongoose HTTP消息对象，包含请求方法和URL路径
 * @db: SQLite数据库句柄，传递给各个业务处理函数
 */
void http_api_dispatch_request(struct mg_connection *c,
                                struct mg_http_message *hm,
                                sqlite3 *db) {
    /* 提取 HTTP 方法和路径 */
    char method[16] = {0};
    char uri[512] = {0};

    /* 提取/api/courses、GET等等，然后进行匹配 */
    strncpy(method, (const char*)hm->method.ptr, hm->method.len > 15 ? 15 : hm->method.len);
    strncpy(uri, (const char*)hm->uri.ptr, hm->uri.len > 511 ? 511 : hm->uri.len);

    /* 简单路由匹配：按 URI 前缀和方法判断 */

    /* ---- 课程管理 ---- */
    if (strcmp(uri, "/api/courses") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_get_courses(c, hm, db);
        } else if (strcmp(method, "POST") == 0) {
            handle_add_course(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    } else if (strcmp(uri, "/api/courses/*") == 0) {
        if (strcmp(method, "DELETE") == 0) {
            handle_delete_course(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    }
    /* ---- 公告管理 ---- */
    else if (strcmp(uri, "/api/notices") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_get_notices(c, hm, db);
        } else if (strcmp(method, "POST") == 0) {
            handle_add_notice(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    } else if (strcmp(uri, "/api/notices/*") == 0) {
        if (strcmp(method, "DELETE") == 0) {
            handle_delete_notice(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    }
    /* ---- 图书馆座位 ---- */
    else if (strcmp(uri, "/api/library/seats") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_get_library_seats(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    } else if (strcmp(uri, "/api/library/reserve") == 0) {
        if (strcmp(method, "POST") == 0) {
            handle_reserve_seat(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    } else if (strcmp(uri, "/api/library/cancel") == 0) {
        if (strcmp(method, "POST") == 0) {
            handle_cancel_reservation(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    } else if (strcmp(uri, "/api/library/my-reservations") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_get_my_reservations(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    }
    /* ---- 教学留言 ---- */
    else if (strcmp(uri, "/api/messages") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_get_messages(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    } else if (strcmp(uri, "/api/messages/*") == 0) {
        if (strcmp(method, "DELETE") == 0) {
            handle_delete_message(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    }
    /* ---- 考勤记录 ---- */
    else if (strcmp(uri, "/api/attendance") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_get_attendance(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    }
    /* ---- 健康检查 ---- */
    else if (strcmp(uri, "/api/health") == 0) {
        if (strcmp(method, "GET") == 0) {
            handle_health_check(c, hm, db);
        } else {
            http_mongoose_send_json_error(c, 405, "方法不允许");
        }
    }
    /* ---- 未找到路由 ---- */
    else {
        http_mongoose_send_json_error(c, 404, "接口不存在");
    }
}
