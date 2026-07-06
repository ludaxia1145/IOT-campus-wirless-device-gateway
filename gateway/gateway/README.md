# 智慧教务网关服务器

## 产品功能
本项目是一个部署在 **ARM 嵌入式开发板** 上的网关
作为局域网内嵌入式设备与Web端的桥梁，通过http协议连接web浏览器，通过tcp原生协议连接imx6ull终端设备，处理多台设备的请求


## 编译条件
### 1.交叉编译器存在
/usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc

### 2.Buildroot 已编译 SQLite
/home/doge/buildroot2021/buildroot-2021.02.x/output/staging/usr/include/sqlite3.h   存在

### 3.Buildroot 已编译 Mongoose
/home/doge/buildroot2021/buildroot-2021.02.x/output/staging/usr/include/mongoose.h
/home/doge/buildroot2021/buildroot-2021.02.x/output/staging/usr/lib/libmongoose.so



## Qt方法：使用QTcpsocket和Qjson(type + data方式)进行网络通信

## sqlite：存储课程、公告、留言、考勤、预约数据
SELECT  查询数据  查询当前课程、公告
INSERT  插入数据  添加课程、发布公告
UPDATE  更新数据  取消预约
DELETE  删除数据  删除课程、删除公告
CREAT_TABLE   创建表

## http方法：基于mongoose库，额外开线程实现http服务器
GET    → 查询（SELECT）
POST   → 更新（INSERT）
DELETE → 删除（DELETE）

/api/courses 课程路由
/api/notices  公告路由
/api/messages 留言路由
/api/attendance 考勤路由

## 数据库表及其结构
课程表	classroom, course_name, teacher, class_name, weekday, period
图书馆座位表	seat_number, is_reserved, student_id, time_slot, reserved_time
公告表	notice_id, title, content, publish_time
学生表	student_id, name, class_name
考勤表	attendance_id, course_id, absent_student_id, record_time
留言表	message_id, student_id, student_name, message, create_time

## 锁机制
锁机制用在座位预约模块，防止两个设备同时预约同一个座位  乐观锁实现（代码简单、并发低）


## 业务功能汇总
### 1.课表查询
Qt 设备 (IMX6ULL)               网关 (TCP 8888)              SQLite
       │                              │                          │
       │  ① {"type":"get_course",     │                          │
       │     "data":{"classroom":"4-106"}}                       │
       │ ─────────────────────────▶  │                          │
       │                              │ ② SELECT FROM courses
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │                              │  ③ 返回课程数据           │
       │                              │ ◀─────────────────────  │
       │  ④ {"type":"course_info",    │                          │
       │     "data":{...}}            │                          │
       │ ◀─────────────────────────  │                          │

### 2.公告查询
Qt 设备 (IMX6ULL)               网关 (TCP 8888)              SQLite
       │                              │                          │
       │  ① {"type":"get_notices",    │                          │
       │     "data":{}}               │                          │
       │ ─────────────────────────▶  │                          │
       │                              │  ② SELECT notice_id,    │
       │                              │     title, content      │
       │                              │     FROM notices        │
       │                              │     ORDER BY id DESC    │
       │                              │     LIMIT 10            │
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │                              │  ③ 返回公告列表          │
       │                              │ ◀─────────────────────  │
       │  ④ {"type":"notice_list",    │                          │
       │     "data":{"notices":[...]}}│                          │
       │ ◀─────────────────────────  │                          │
       │                              │                          │
       │  ⑤ 用户点击某条公告                                      │
       │                              │                          │
       │  ⑥ {"type":"get_notice_detail",│                       │
       │     "data":{"notice_id":1}}  │                          │
       │ ─────────────────────────▶  │                          │
       │                              │  ⑦ SELECT * FROM notices│
       │                              │     WHERE notice_id = 1 │
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │                              │  ⑧ 返回公告详情          │
       │                              │ ◀─────────────────────  │
       │  ⑨ {"type":"notice_detail",  │                          │
       │     "data":{"title":"...",   │                          │
       │             "content":"..."}}│                          │
       │ ◀─────────────────────────  │                          │

### 3.教学留言
Qt 设备 (IMX6ULL)               网关 (TCP 8888)              SQLite
       │                              │                          │
       │  ① {"type":"send_message",   │                          │
       │     "data":{                 │                          │
       │       "student_id":"2022001",│                          │
       │       "student_name":"张三", │                          │
       │       "message":"老师好..."  │                          │
       │     }}                       │                          │
       │ ─────────────────────────▶  │                          │
       │                              │  ② 验证学生身份          │
       │                              │  SELECT FROM students   │
       │                              │  WHERE student_id=?     │
       │                              │    AND name=?           │
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │                              │  ③ 验证通过则插入留言    │
       │                              │  INSERT INTO messages   │
       │                              │  (student_id,           │
       │                              │   student_name,         │
       │                              │   message, create_time) │
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │  ④ {"type":"message_response",│                        │
       │     "data":{"status":"success"}}│                       │
       │ ◀─────────────────────────  │                          │

### 4.自习室预约
Qt 设备 (IMX6ULL)               网关 (TCP 8888)              SQLite
       │                              │                          │
       │  ① {"type":"get_seat_status",│                          │
       │     "data":{}}               │                          │
       │ ─────────────────────────▶  │                          │
       │                              │  ② SELECT FROM          │
       │                              │     library_seats       │
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │  ③ 返回 200 个座位状态        │                          │
       │ ◀─────────────────────────  │                          │
       │                              │                          │
       │  ④ 用户选择座位 + 输入学号     │                          │
       │                              │                          │
       │  ⑤ {"type":"reserve_seat",   │                          │
       │     "data":{                 │                          │
       │       "seat_number": 23,     │                          │
       │       "student_id":"2022001",│                          │
       │       "time_slot":"morning"  │                          │
       │     }}                       │                          │
       │ ─────────────────────────▶  │                          │
       │                              │  ⑥ 执行预约              │
       │                              │  UPDATE library_seats   │
       │                              │  SET is_reserved=1,     │
       │                              │  student_id=?,          │
       │                              │  time_slot=?            │
       │                              │  WHERE seat_number=?    │
       │                              │    AND is_reserved=0    │
       │                              │ ─────────────────────▶  │
       │                              │                          │
       │  ⑦ 返回预约结果               │                          │
       │ ◀─────────────────────────  │                          │

### 5.课堂考勤
Qt 设备                         网关                           SQLite
   │                              │                              │
   │  ① {"type":"get_attendance_list",│                         │
   │     "data":{"classroom":"4-106"}}│                         │
   │ ─────────────────────────▶  │                              │
   │                              │  ② 计算当前 weekday/period   │
   │                              │     + SELECT FROM courses   │
   │                              │     + SELECT FROM students  │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ③ 返回学生名单              │                              │
   │ ◀─────────────────────────  │                              │
   │                              │                              │
   │  ④ 老师点击缺勤学生          │                              │
   │     (绿色→红色)              │                              │
   │                              │                              │
   │  ⑤ {"type":"submit_attendance",│                           │
   │     "data":{                 │                              │
   │       "course_id": 1,        │                              │
   │       "absent_students": [   │                              │
   │         "2022003",           │                              │
   │         "2022005"            │                              │
   │       ]                      │                              │
   │     }}                       │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ⑥ INSERT INTO attendance    │
   │                              │     (循环插入缺勤学生)        │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ⑦ 返回提交结果               │                              │
   │ ◀─────────────────────────  │                              │

### 6.web端课程管理
Web 浏览器                       网关 (HTTP 5000)              SQLite
   │                              │                              │
   │  ① GET /api/courses          │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ② SELECT FROM courses      │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ③ 返回课程列表              │                              │
   │ ◀─────────────────────────  │                              │
   │                              │                              │
   │  ④ POST /api/courses         │                              │
   │     {classroom,course_name,  │                              │
   │      teacher,class_name,     │                              │
   │      weekday,period}         │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ⑤ INSERT INTO courses      │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ⑥ 返回添加成功              │                              │
   │ ◀─────────────────────────  │                              │
   │                              │                              │
   │  ⑦ DELETE /api/courses/1    │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ⑧ DELETE FROM courses      │
   │                              │     WHERE course_id = 1     │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ⑨ 返回删除成功              │                              │
   │ ◀─────────────────────────  │                              │

### 7.web端公告管理
Web 浏览器                       网关 (HTTP 5000)              SQLite
   │                              │                              │
   │  ① GET /api/notices          │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ② SELECT FROM notices      │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ③ 返回公告列表              │                              │
   │ ◀─────────────────────────  │                              │
   │                              │                              │
   │  ④ POST /api/notices         │                              │
   │     {title, content}         │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ⑤ INSERT INTO notices      │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ⑥ 返回发布成功              │                              │
   │ ◀─────────────────────────  │                              │
   │                              │                              │
   │  ⑦ DELETE /api/notices/1    │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ⑧ DELETE FROM notices      │
   │                              │     WHERE notice_id = 1     │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ⑨ 返回删除成功              │                              │
   │ ◀─────────────────────────  │                              │

### 8. Web端留言管理
Web 浏览器                       网关 (HTTP 5000)              SQLite
   │                              │                              │
   │  ① GET /api/messages         │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ② SELECT FROM messages     │
   │                              │     ORDER BY id DESC        │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ③ 返回留言列表              │                              │
   │ ◀─────────────────────────  │                              │
   │                              │                              │
   │  ④ DELETE /api/messages/1   │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ⑤ DELETE FROM messages     │
   │                              │     WHERE message_id = 1    │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ⑥ 返回删除成功              │                               │
   │ ◀─────────────────────────  │                              │

### 9.Web端考勤记录查询
Web 浏览器                       网关 (HTTP 5000)              SQLite
   │                              │                              │
   │  ① GET /api/attendance       │                              │
   │ ─────────────────────────▶  │                              │
   │                              │  ② SELECT FROM attendance   │
   │                              │     + JOIN courses          │
   │                              │ ─────────────────────────▶  │
   │                              │                              │
   │  ③ 返回考勤记录列表           │                              │
   │ ◀─────────────────────────  │                              │

## Qt方法：使用QTcpsocket和Qjson(type + data方式)进行网络通信

## sqlite：存储课程、公告、留言、考勤、预约数据
SELECT  查询数据  查询当前课程、公告
INSERT  插入数据  添加课程、发布公告
UPDATE  更新数据  取消预约
DELETE  删除数据  删除课程、删除公告
CREAT_TABLE   创建表

## http方法：基于mongoose库，额外开线程实现http服务器
GET    → 查询（SELECT）
POST   → 更新（INSERT）
DELETE → 删除（DELETE）

/api/courses 课程路由
/api/notices  公告路由
/api/messages 留言路由
/api/attendance 考勤路由

## 数据库表及其结构
课程表	classroom, course_name, teacher, class_name, weekday, period
图书馆座位表	seat_number, is_reserved, student_id, time_slot, reserved_time
公告表	notice_id, title, content, publish_time
学生表	student_id, name, class_name
考勤表	attendance_id, course_id, absent_student_id, record_time
留言表	message_id, student_id, student_name, message, create_time

## 锁机制
锁机制用在座位预约模块，防止两个设备同时预约同一个座位  乐观锁实现（代码简单、并发低）


### 核心技术栈
#### 1. TCP 服务器（epoll  IO多路复用）
##### 服务器 & epoll的使用方法:
1）初始化服务器(socket->bind->listen)，创建epoll实例(epoll_create)，添加服务端监听fd进入epoll实例
2）循环处理请求，epoll_wait处理两个分支：
   分支1：新的客户端来了，accept函数添加客户端的fd到epoll实例
   分支2：老的客户端来需求了，处理客户端需求
3）使用边缘触发（必须一次性把所有数据读完），减少epoll通知次数

##### 重要代码实例
``` c
while (running) {
    /* 
        等待epoll_fd里面的所有文件描述符，有以下两类:
        监听 fd（server_fd） → 表示有新连接；
        客户端 fd（某个教室设备） → 表示有数据到达 
    */
    int nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, 500);
    for (int i = 0; i < nfds; i++) {
        /* 有客户端来了，被服务端监听了 */
        if (events[i].data.fd == listen_fd) {
            int cli_fd = accept(...);
            /* 将客户端 fd 注册到 epoll */
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_fd, &ev);
        /* 客户端有事件过来了，服务端需要处理 */
        } else {
            handle_client_readable(events[i].data.fd);
        }
    }
}
```

#### 2. HTTP 服务器（Mongoose 嵌入式库）

**库选择**：Mongoose（嵌入式 HTTP/WebSocket 服务器）
##### http 使用方法
1）初始化http服务器、开启监听  mg_mgr_init();  mg_http_listen()；
2）创建http子线程，在里面处理http网络IO   mg_mgr_poll(mgr, 1); 
3）具体实现回调函数与http业务逻辑    http_event_handler

**关键代码示例**：

```c 
void http_event_handler(struct mg_connection *c, int ev,
                        void *ev_data, void *fn_data) {
    if (ev != MG_EV_HTTP_MSG) return;
    
    struct mg_http_message *hm = (struct mg_http_message *)ev_data;
    sqlite3 *db = (sqlite3 *)fn_data;
    
    /* 路由分发（Mongoose 已完成 HTTP 解析） */
    http_api_dispatch_request(c, hm, db);
}

```

#### 3. http路由匹配（Mongoose + 字符串匹配）

**实现方式**：`http_api.c`  
**匹配流程**:
**示例1**: 课表查询
1）用户点击查询课程，Web端调用 fetch('http://localhost:5000/api/courses')
2）网络传输: GET /api/courses
3）http_api_dispatch_request(): 核心分发：
(strcmp(uri, "/api/courses") == 0) {
        if (strcmp(method, "GET") == 0) {
    }
}


**示例2**: 座位预约
1）用户点击"预约座位"按钮，Web端调用fetch('http://localhost:5000/api/library/reserve’)
2）网络传输：POST /api/library/reserve
3）http_api_dispatch_request():核心分发：
    else if (strcmp(uri, "/api/library/reserve") == 0) {
        if (strcmp(method, "POST") == 0) {
    }
}



#### 4. 数据库（SQLite）

**实现方式**：`database.c` / `database.h`

- **库选择**：SQLite3（轻量级嵌入式 SQL 数据库）
- **并发模型**：`SQLITE_OPEN_FULLMUTEX` 序列化模式


### 设计细节
1.打开数据库使用SQLITE_OPEN_FULLMUTEX，启用完全互斥锁，贴合多线程访问数据库的安全性
（但是有一定性能开销）

2.addr.sin_port  = htons(port);  网络传输，小端要转化为大端

3.TCP服务器非阻塞:
set_nonblocking(server->server_fd);
int cli_fd = accept(server->server_fd,(struct sockaddr*)&client_addr,&client_len);

如果 server->server_fd 是阻塞模式，accept() 在没有连接时，会永久阻塞, 程序停在这一行，直到有客户端连接为止，期间啥也干不了 

所以，设置为阻塞，后面的epoll都不能执行

4.使用连接池而不是动态内存分配
// ❌ 动态分配（危险）：并发可能导致内存泄漏
ClientConn *conn = malloc(sizeof(ClientConn));

// ✅ 连接池（安全）
ClientConn clients[MAX_CLIENTS];  // 预分配 128 个


### 设计缺陷:
1.课表查询提高兼容性，可以改成日期查询，而不是单纯时间段查询
