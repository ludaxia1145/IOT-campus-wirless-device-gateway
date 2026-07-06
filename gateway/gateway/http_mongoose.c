/*
 * http_mongoose.c  ——  Mongoose 嵌入式 HTTP 服务器实现
 */

#include "http_mongoose.h"
#include "http_api.h"
#include "cJSON.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mongoose.h"

/*
 * 函数功能：Mongoose HTTP 事件回调函数
 * @c: Mongoose 连接对象
 * @ev: 事件类型（MG_EV_HTTP_MSG 表示 HTTP 请求到达）
 * @ev_data: 事件数据（指向 mg_http_message 结构体）
 * @fn_data: 用户数据指针（指向 SQLite 数据库句柄）
 */
static void http_event_handler(struct mg_connection *c, int ev,
                                void *ev_data, void *fn_data) {
    if (ev != MG_EV_HTTP_MSG) return;  /* 只关心 HTTP 请求事件 */
    
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    sqlite3 *db = (sqlite3 *)fn_data;

    /* HTTP API的核心处理函数 */
    http_api_dispatch_request(c, hm, db);
}


/*
 * 函数功能：初始化 Mongoose HTTP 服务器
 * @mgr: Mongoose 管理器指针（输出参数）
 * @db: SQLite 数据库句柄（传递给回调函数）
 * @port: 监听端口
 * @return: 0 成功，-1 失败
 */
int http_mongoose_init(struct mg_mgr *mgr, sqlite3 *db, int port) {
    /* 1. 初始化 Mongoose 管理器 */
    mg_mgr_init(mgr);

    /* 2. 组装监听地址 */
    char addr[64];
    snprintf(addr, sizeof(addr), "0.0.0.0:%d", port);

    /* 3. 创建监听连接，绑定回调处理器 */
    struct mg_connection *c = mg_http_listen(
        mgr,                    /* Mongoose 管理器 */
        addr,                   /* 监听地址：主机:端口 比如，192.168.1.100:500 */
        http_event_handler,     /* 事件回调函数 */
        (void *)db              /* 用户数据指针（这里传 db） */
    );

    /* 检查监听是否成功，如果返回 NULL 表示创建失败 */
    if (c == NULL) {
        fprintf(stderr, "[HTTP] 无法在 %s 监听\n", addr);
        return -1;
    }

    printf("[HTTP] Mongoose 服务器初始化成功，监听 %s\n", addr);
    return 0;
}

/*
 * 函数功能：处理一轮 HTTP 事件（非阻塞）
 * @mgr: Mongoose 管理器指针
 */
void http_mongoose_poll(struct mg_mgr *mgr) {
    /* 处理一系列的网络活动 */ 
    mg_mgr_poll(mgr, 1);  // 最多等待 1 毫秒
}


/*  http_mongoose_cleanup() —— 清理 HTTP 服务器                      */
void http_mongoose_cleanup(struct mg_mgr *mgr) {
    mg_mgr_free(mgr);
}


/*  http_mongoose_send_json_success() —— 发送成功响应                */
void http_mongoose_send_json_success(struct mg_connection *c,
                                      const char *json_body) {
    mg_http_reply(
        c,                   /* 连接 */
        200,                 /* HTTP 状态码 */
        "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
        "%s",                /* 响应体格式 */
        json_body            /* 响应体内容 */
    );
}



/*
 * 函数功能：发送 JSON 格式的错误响应
 * @c: Mongoose 连接对象
 * @http_code: HTTP 状态码（400, 404, 405, 500 等）
 * @error_msg: 错误描述文本
 */
void http_mongoose_send_json_error(struct mg_connection *c,
                                    int http_code,
                                    const char *error_msg) {
    /* 构建错误 JSON */
    cJSON *error = cJSON_CreateObject();
    cJSON_AddItemToObject(error, "success", cJSON_CreateBool(0));
    cJSON_AddStringToObject(error, "error", error_msg);
    char *json_str = cJSON_Print(error);

    /* 发送 HTTP 响应 */
    mg_http_reply(
        c,
        http_code,
        "Content-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\n",
        "%s",
        json_str
    );

    free(json_str);
    cJSON_Delete(error);
}

/*
 * 函数功能：从 HTTP 请求 URL 中提取查询参数
 * @hm: Mongoose HTTP 消息对象
 * @key: 参数名（如 "student_id"）
 * @return: 参数值字符串，不存在返回 NULL
 */
const char *http_mongoose_get_query_param(struct mg_http_message *hm,
                                           const char *key) {
    static char buf[256];
    int n = mg_http_get_var(&hm->query, key, buf, sizeof(buf));
    return (n > 0) ? buf : NULL;
}
