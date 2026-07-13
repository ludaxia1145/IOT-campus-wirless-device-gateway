# 智慧教务网关服务器

## 产品功能
1.本项目是一个部署在 **ARM 嵌入式开发板** 上的网关，作为局域网内嵌入式设备与Web端的桥梁，通过http协议连接web浏览器，通过TCP协议连接imx6ull终端设备，处理多台设备的请求
2.嵌入式设备的请求会存储到网关设备的sqlite数据库中
3.web浏览器也可以查看、修改网关设备sqlite数据库中的信息


业务实现包含以下类型:
1.课表查询：接收终端设备的教室号，查询sqlite的对应课表，返回给终端设备
2.公告查询：接收终端设备的公告请求，查询sqlite的最新公告，返回给终端设备
3.教学留言：接收终端设备的学生ID、姓名信息，网关验证身份后存入sqlite数据库
4.自习室预约：接收终端设备的预约，验证学生学号之后将预约信息存入sqlite数据库
5.课堂考勤：老师选择缺勤学生，网关批量记录缺勤名单，关联课程与学号。
6.Web课程管理：管理员通过 Web 页面增删课程信息，修改网关设备的sqlite数据库
7.Web公告管理：管理员通过 Web 页面发布新公告或删除已有公告，修改网关设备的sqlite数据库
8.Web留言管理：管理员通过 Web 页面查看所有学生留言列表，通过网关设备的sqlite查询
9.Web考勤记录查询：管理员通过 Web 页面联合查询考勤记录与课程信息，通过网关设备的sqlite查询


## 编译条件
### 1.交叉编译器
/usr/local/arm/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc

### 2.Buildroot 已编译 SQLite
/home/doge/buildroot2021/buildroot-2021.02.x/output/staging/usr/include/sqlite3.h 

### 3.Buildroot 已编译 Mongoose
/home/doge/buildroot2021/buildroot-2021.02.x/output/staging/usr/include/mongoose.h
/home/doge/buildroot2021/buildroot-2021.02.x/output/staging/usr/lib/libmongoose.so

## 编译方法
make
./gateway

## 技术实现:
1.I/O多路复用：管理数百个客户端连接，避免多线程切换开销，适合 ARM 嵌入式资源受限环境
2.JSON 轻量级通信协议：TCP设备端统一使用 {"type":"...", "data":...}格式的JSON消息
3.Mongoose 嵌入式 HTTP 服务器：集成工业级 Mongoose 库，自动处理 HTTP 解析
4.TCP网络编程，实现和终端设备的网络通信








