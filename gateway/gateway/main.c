/*
 * main.c  ——  网关主入口
 * 本程序同时运行两个独立的网络服务：
 *
 *   1. TCP 服务器（默认端口 8888）
 *      接收来自嵌入式设备（教室课表屏、签到终端）发来的 JSON 协议消息，
 *      处理课程查询、座位预约、考勤提交、留言发送等业务。
 *      使用 epoll 事件驱动（server.c 中实现）。
 *
 *   2. HTTP 服务器（默认端口 5000）
 *      提供 REST API，供 library_web 前端管理页面调用。
 *      使用 Mongoose 库（内置 epoll 事件循环）。
 */

#include "server.h"
#include "database.h"
#include "http_mongoose.h"
#include "http_api.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>


static Server             g_tcp_server;     /* TCP 服务器（epoll）*/
static struct mg_mgr      g_http_mgr;       /* Mongoose HTTP 管理器 */
static sqlite3           *g_db;             /* SQLite 数据库 */
static volatile int       g_http_running;   /* HTTP 线程运行标志 */


/*  Mongoose HTTP 事件循环线程入口                                    */
static void *http_mongoose_thread(void *arg) {
    (void)arg; /* 显式标记参数未使用，消除编译警告 */

    printf("[HTTP] Mongoose 事件循环线程启动\n");

    /* 在子线程中持续轮询 Mongoose 事件 */
    while (g_http_running) {
        /* 处理网络IO的所有事件 */
        http_mongoose_poll(&g_http_mgr);
        usleep(10000);  /* 10ms 轮询间隔 */
    }

    printf("[HTTP] Mongoose 事件循环线程退出\n");
    return NULL;
}


/* 
 * 当用户按下 Ctrl+C (SIGINT) 或系统发送终止信号(SIGTERM)时，执行优雅关闭流程。
 */
static void signal_handler(int signum) {
    (void)signum; /* 显式标记参数未使用，消除编译警告 */
    printf("\n收到停止信号，正在关闭所有服务...\n");
    
    /* 标记 HTTP 线程停止 */
    g_http_running = 0;
    
    /* 停止 TCP 服务器 */
    server_stop(&g_tcp_server);
    
    /* 清理资源 */
    usleep(100000);  /* 给 HTTP 线程反应时间 */
    http_mongoose_cleanup(&g_http_mgr);  /* 释放 Mongoose HTTP 服务器占用的资源 */
    db_close(g_db); /* 关闭 SQLite 数据库连接 */
    
    printf("所有服务已停止，再见。\n");
    exit(0);     /* 退出进程，返回状态码 0 表示正常结束 */
}


/* 启动说明 */
static void print_banner(int tcp_port, int http_port) {
    printf("\n");
    printf("║       智慧教务网关服务器 v2.1 (Mongoose 版)             ║\n");
    printf("║  TCP  服务 (设备协议, epoll)  : 端口 %-5d             ║\n", tcp_port);
    printf("║  HTTP 服务 (REST API, Mongoose) : 端口 %-5d             ║\n", http_port);
    printf("║  数据库 (SQLite FULLMUTEX)    : teaching_office.db   ║\n");
    printf("\n");
}


int main(int argc, char *argv[]) {
    /* 解析命令行参数，用户无输入则用默认 */
    int tcp_port  = (argc > 1) ? atoi(argv[1]) : 8888;  /* 默认 TCP 端口 8888 */
    int http_port = (argc > 2) ? atoi(argv[2]) : 5000;  /* 默认 HTTP 端口 5000 */

    print_banner(tcp_port, http_port);

    /* ---- 1. 初始化 SQLite 数据库 ---- */
    /* 以读写 + 创建模式打开数据库文件，FULLMUTEX 确保多线程安全，适用于多线程访问数据库 */
    if (sqlite3_open_v2("teaching_office.db", &g_db,
                         SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
                         | SQLITE_OPEN_FULLMUTEX, NULL) != SQLITE_OK) {
        fprintf(stderr, "[DB] 数据库打开失败: %s\n", sqlite3_errmsg(g_db));
        return 1;
    }
    printf("[DB] 数据库打开成功 (序列化模式 FULLMUTEX)\n");

    if (db_create_tables(g_db) != 0) {
        fprintf(stderr, "[DB] 创建数据表失败\n");
        db_close(g_db);
        return 1;
    }

    /* 插入示例数据 */
    db_insert_sample_data(g_db);
    printf("[DB] 示例数据已加载\n\n");

    /* ---- 2. 初始化 TCP 服务器（epoll 事件驱动）---- */
    if (server_init(&g_tcp_server, tcp_port, g_db) != 0) {
        fprintf(stderr, "[TCP] 初始化失败\n");
        db_close(g_db);     /* 关闭数据库 */
        return 1;
    }

    /* ---- 3. 初始化 HTTP 服务器（Mongoose）---- */
    if (http_mongoose_init(&g_http_mgr, g_db, http_port) != 0) {
        fprintf(stderr, "[HTTP] 初始化失败\n");
        server_stop(&g_tcp_server);  /* 停止 TCP 服务器 */
        db_close(g_db);     /* 关闭数据库 */
        return 1;
    }

    /* ---- 4. 注册信号处理 ---- */
    signal(SIGINT,  signal_handler);  /* 注册 Ctrl+C 信号处理函数 */
    signal(SIGTERM, signal_handler);  /* 注册终止信号处理函数（kill 默认信号） */


    /* ---- 5. 启动 HTTP 服务器线程 ---- */
    g_http_running = 1;  /* 设置 HTTP 线程运行标志为真 */
    pthread_t http_tid;  /* 存储 HTTP 线程 ID */
    if (pthread_create(&http_tid, NULL, http_mongoose_thread, NULL) != 0) {
        perror("[HTTP] 创建线程失败");
        server_stop(&g_tcp_server);     
        http_mongoose_cleanup(&g_http_mgr); /* 清理 HTTP 服务器资源 */
        db_close(g_db);
        return 1;
    }
    /* 线程设置为分离状态，让线程在结束时自动释放资源，不需要其他线程回收 */
    pthread_detach(http_tid);      

    printf("提示: 按 Ctrl+C 停止所有服务\n");
    printf("════════════════════════════════════════════════════════\n\n");

    /* ---- 6. 主线程运行 TCP 服务器 ---- */
    server_start(&g_tcp_server);

    /* 正常退出 */
    return 0;
}
