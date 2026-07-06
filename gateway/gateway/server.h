/*
 * server.h  ——  TCP 协议服务器（epoll 事件驱动版）
 *
 * 原版使用 pthread，每连接一个线程；改造后改用 epoll I/O 多路复用，
 * 单线程处理所有并发连接，适合 imx6ull 等资源受限的嵌入式网关平台。
 *
 * 并发模型：
 *   epoll 边缘触发（EPOLLET）监听所有 fd，事件就绪后在主循环内
 *   依次处理，避免线程切换开销；SQLite 操作在同一线程完成，无需加锁。
 */

#ifndef SERVER_H
#define SERVER_H

#include <sqlite3.h>
#include <sys/epoll.h>

/* epoll_wait 单次最多返回的就绪事件数 */
#define MAX_EPOLL_EVENTS  64

/* 每个客户端接收缓冲区大小（字节），须大于单条最长 JSON 消息 */
#define CLIENT_BUF_SIZE   8192

/* 服务器同时支持的最大客户端连接数 */
#define MAX_CLIENTS       128

/*
 * ClientConn —— 单个客户端连接的上下文
 *
 * epoll 模型下不再依赖线程局部存储，每条连接维护独立接收缓冲区，
 * 用于跨多次 recv() 调用拼接一条完整的以 '\n' 结尾的 JSON 消息。
 */
typedef struct {
    int  fd;                    /* 客户端 socket fd，-1 表示槽位空闲 */
    char buf[CLIENT_BUF_SIZE];  /* 接收缓冲区（跨 recv 累积） */
    int  buf_len;               /* 缓冲区中已有效字节数 */
} ClientConn;

/*
 * Server —— TCP 服务器主结构体
 */
typedef struct {
    int        port;                   /* 监听端口 */
    sqlite3   *db;                     /* SQLite 数据库句柄（共享） */
    int        server_fd;              /* 监听 socket fd */
    int        epoll_fd;               /* epoll 实例 fd */
    int        running;                /* 运行标志：1=运行，0=停止 */
    ClientConn clients[MAX_CLIENTS];   /* 客户端连接池（静态分配） */
} Server;

/* ------------------------------------------------------------------ */
/*  对外接口                                                            */
/* ------------------------------------------------------------------ */

/* 初始化：创建 socket、绑定端口、创建 epoll 实例，返回 0 成功/-1 失败 */
int  server_init(Server *server, int port, sqlite3 *db);

/* 启动事件循环（阻塞运行，直到 server->running 被置 0） */
int  server_start(Server *server);

/* 处理一条完整的 JSON 文本消息，按 "type" 字段分发到各业务处理函数 */
void handle_client_message(int client_fd, const char *message, sqlite3 *db);

/* 停止服务器：关闭 epoll fd 和监听 fd，退出事件循环 */
void server_stop(Server *server);

#endif /* SERVER_H */
