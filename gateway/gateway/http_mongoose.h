/*
 * http_mongoose.h  ——  Mongoose 嵌入式 HTTP 服务器框架头文件
 *
 * 本文件定义 Mongoose 库在网关中的使用接口。
 * Mongoose 是工业级嵌入式 HTTP/WebSocket 服务器，广泛应用于 IoT/智能设备。
 *
 * 相比手写框架的优势：
 *   - 内置 HTTP/1.1 完整实现（请求解析、响应构建、头部管理等）
 *   - 自动 epoll/kqueue/IOCP 事件循环（跨平台）
 *   - 内置 Keep-Alive、分块传输、WebSocket 等高级特性
 *   - 代码行数从 500+ 减到 100+ 行
 *   - 性能更优（经过优化的事件调度）
 */

#ifndef HTTP_MONGOOSE_H
#define HTTP_MONGOOSE_H

#include "mongoose.h"
#include <sqlite3.h>

/* ================================================================== */
/*  Mongoose HTTP 服务器上下文                                        */
/* ================================================================== */

typedef struct {
    struct mg_mgr mgr;    /* Mongoose 管理器（事件循环、连接管理） */
    sqlite3       *db;    /* SQLite 数据库句柄（业务共享） */
    const char    *addr;  /* 监听地址 (e.g., "0.0.0.0:5000") */
    int           running;/* 运行标志 */
} HttpMongooseServer;

/* ================================================================== */
/*  公开接口                                                           */
/* ================================================================== */

/*
 * http_mongoose_init() 初始化 Mongoose HTTP 服务器
 *   - mgr：Mongoose 管理器
 *   - db：SQLite 数据库句柄
 *   - port：监听端口
 *   返回 0 成功，-1 失败
 */
int http_mongoose_init(struct mg_mgr *mgr, sqlite3 *db, int port);

/*
 * http_mongoose_poll() 处理一轮 HTTP 事件（非阻塞）
 *   应在主循环或定时器中调用
 */
void http_mongoose_poll(struct mg_mgr *mgr);

/*
 * http_mongoose_cleanup() 清理 HTTP 服务器资源
 */
void http_mongoose_cleanup(struct mg_mgr *mgr);

/* ================================================================== */
/*  JSON 响应工具函数                                                  */
/* ================================================================== */

/*
 * 向 HTTP 连接发送成功 JSON 响应（自动 Content-Type、Content-Length）
 *   msg：指向 HTTP 消息的指针
 *   json_body：JSON 对象文本
 */
void http_mongoose_send_json_success(struct mg_connection *c, 
                                      const char *json_body);

/*
 * 向 HTTP 连接发送错误 JSON 响应
 *   msg：指向 HTTP 消息的指针
 *   http_code：HTTP 状态码 (400, 404, 500 等)
 *   error_msg：错误描述文本
 */
void http_mongoose_send_json_error(struct mg_connection *c,
                                    int http_code,
                                    const char *error_msg);

/*
 * 从 HTTP 请求 URL 中提取查询参数
 *   hm：Mongoose HTTP 消息
 *   key：参数名
 *   返回参数值，如不存在返回 NULL
 */
const char *http_mongoose_get_query_param(struct mg_http_message *hm,
                                           const char *key);

#endif /* HTTP_MONGOOSE_H */
