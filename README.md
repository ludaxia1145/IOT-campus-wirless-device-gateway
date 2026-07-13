<<<<<<< HEAD
# IOT-campus-wirless-device-gateway
=======
# IoT Campus wireless device and gateway - 智能校园终端系统

> 一套完整的嵌入式IoT解决方案：校园无线显示终端 + 网关设备 + Web管理平台

## 项目概述

本项目是一个**三层架构**的智能校园系统，用于教室课程展示、考勤管理和学生服务：

```
┌─────────────────────────────────────────────────────────┐
│  Web管理平台 (React + Next.js)                           │
│  └─ 数据查看、公告发布、课程管理                          │
└────────────────┬────────────────────────────────────────┘
                 │ HTTP/JSON
┌────────────────▼────────────────────────────────────────┐
│  网关设备 (Linux epoll + SQLite)                         │
│  └─ 数据集中、协议转换、数据同步                          │
└────────────────┬────────────────────────────────────────┘
                 │ 自定义协议
        ┌────────┴─────────┬──────────┐
        │                  │          │
    ┌───▼────┐      ┌──────▼──┐  ┌──▼──────┐
    │终端1   │      │ 终端2   │   │ 终端n   │
    │imx6ull │      │imx6ull  │  │imx6ull  │
    └────────┘      └─────────┘  └─────────┘
```

## 项目结构

```
IOT campus wirless device and gateway/
│
├── terminal_device/             # 校园无线通信终端触摸设备
│   ├── app/
│   │   ├── smart-schedule/      # 主GUI应用 (Qt5)
│   │   ├── auto-brightness/     # 亮度调节应用
│   │   └── gesture-monitor/     # 姿态检测应用 + 保护UI
│   ├── drivers/
│   │   ├── ap3216c-driver/      # 光传感器驱动
│   │   └── icm20608-driver/     # IMU传感器驱动
│   ├── bsp/                     # 芯片支持包
│   │   ├── linux-kernel/        # Linux内核配置
│   │   └── device-tree/         # 设备树
│   |   |__ rootfs
|   |            |------defconfig  # 根文件系统配置
|   |            |------my_rootfs_additions  # 自定义应用程序、资源库、模块驱动、启动脚本等配置
│   |
|   |—— mfgtool/                 # 烧写工具
|
├── gateway/                     # 校园网关设备
│   ├── app/
│   │   ├── server.c/h           # TCP服务器 (epoll)
│   │   ├── http_api.c/h         # HTTP API 接口
│   │   ├── http_mongoose.c/h    # HTTP框架 (Mongoose)
│   │   ├── database.c/h         # SQLite 数据库操作
│   │   ├── protocol.h           # 通信协议定义
│   │   ├── cJSON.c/h            # JSON 序列化/反序列化
│   │   ├── main.c               # 主程序
│   │   └── Makefile
│   ├── rootfs/                  # 根文件系统配置
│   └── mfgtool/                # 烧写工具
│                    
│
├── web/                         # Web管理平台
│   ├── app/                     # Next.js 应用路由
│   ├── components/              # UI组件
│   ├── hooks/                   # 自定义Hooks
│   ├── lib/                     # 工具函数
│   ├── package.json
│   └── next.config.mjs
│
├── protocol/                    # 通信协议文档
│   ├── device-gateway.md        # 设备 ↔ 网关
│   └── gateway-web.md           # 网关 ↔ Web
│
├── docs/                        # 项目文档
│   ├── architecture.md          # 系统架构详解
│   ├── hardware.md              # 硬件配置
│   └── build-guide.md           # 完整构建指南
│
├── tools/                       # 构建和部署脚本
│   ├── build.sh                 # 一键构建所有组件
│   ├── deploy.sh                # 部署脚本
│   └── test.sh                  # 测试脚本
│
└── README.md                    # 本文件
```

## 根文件系统配置
my_rootfs_additions/
├── etc
│   ├── init.d
│   │   ├── S11modules
│   │   └── S99myapps
│   └── systemd
│       └── system
│           ├── auto_brightness.service
│           ├── gesture_monitor.service
│           └── smart_schedule.service
├── lib
│   └── modules
│       └── 4.1.15
│           ├── ap3216c_iio.ko
│           └── icm20608.ko
└── usr
    ├── app
    │   ├── auto_brightness
    │   ├── gesture_monitor
    │   ├── protect_ui
    │   └── smart_schedule
    └── audio
        └── success.wav

---

>>>>>>> 119b451 (首次提交：IOT campus wirless device and gateway)
