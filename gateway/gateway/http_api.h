/*
 * http_api.h  ——  REST API 路由处理（Mongoose 版）
 *
 * 本文件定义 bridge_api.py 中全部 16 个 @app.route 的 C 实现。
 * 改用 Mongoose 后，回调签名改为 mg_event_handler_t：
 *    void handler(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
 *
 * REST API 列表（与 Python 版完全兼容）：
 *
 *  课程管理：
 *    GET  /api/courses          —— 获取所有课程
 *    POST /api/courses          —— 添加课程
 *    DELETE /api/courses/:id    —— 删除课程
 *
 *  公告管理：
 *    GET  /api/notices          —— 获取所有公告
 *    POST /api/notices          —— 添加公告
 *    DELETE /api/notices/:id    —— 删除公告
 *
 *  图书馆座位：
 *    GET  /api/library/seats            —— 获取所有座位状态
 *    POST /api/library/reserve          —— 预约座位
 *    POST /api/library/cancel           —— 取消预约
 *    GET  /api/library/my-reservations  —— 查询学生预约
 *
 *  教学留言：
 *    GET  /api/messages         —— 获取所有留言
 *    DELETE /api/messages/:id   —— 删除留言
 *
 *  考勤记录：
 *    GET  /api/attendance       —— 获取考勤记录
 *
 *  健康检查：
 *    GET  /api/health           —— 服务存活检查
 */

#ifndef HTTP_API_H
#define HTTP_API_H

#include "mongoose.h"
#include <sqlite3.h>

/*
 * http_api_dispatch_request() —— Mongoose HTTP 请求总分发器
 *
 * 由 http_mongoose.c 中的 http_event_handler() 回调触发。
 * 根据请求方法和路径，分发到对应的业务处理函数。
 *
 * 参数：
 *   c：Mongoose 连接
 *   hm：HTTP 消息（包含方法、路径、头部、body）
 *   db：SQLite 数据库句柄
 */
void http_api_dispatch_request(struct mg_connection *c,
                                struct mg_http_message *hm,
                                sqlite3 *db);

#endif /* HTTP_API_H */
