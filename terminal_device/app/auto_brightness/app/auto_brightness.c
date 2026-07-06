#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ctype.h>

/* 背光与日志路径 */
#define BACKLIGHT_PATH   "/sys/class/backlight/backlight/brightness"
#define LOG_FILE_PATH    "/var/log/als_autobright.log"

/* 亮度调节参数 */
#define DEFAULT_BRIGHT   4          // 7档中的中间值（约50%）
#define MAX_BRIGHT       7          // 7档最大亮度
#define MIN_BRIGHT       1           // 最低亮度 1（避免关闭屏幕）
#define INTERVAL_SEC     10         // 10分钟采样一次

/* IIO 相关常量 */
#define IIO_BASE_PATH    "/sys/bus/iio/devices"
#define IIO_DEVICE_PREFIX "iio:device"

/* 线性转换参数：将 IIO 原始值映射到 [10, 20] 范围 */
#define RAW_MIN         0
#define RAW_MAX         20000   // 对应强光下的典型原始值（可修改）
#define ALS_MIN         10
#define ALS_MAX         20

static volatile int keep_running = 1;
static int brightness_fd = -1;
static int default_brightness = DEFAULT_BRIGHT;
static FILE *log_fp = NULL;
static char iio_als_path[256] = {0};
static int foreground_mode = 0;     // 0:守护进程模式, 1:前台模式

/* 写日志函数（带时间戳） */
static void log_message(const char *level, const char *fmt, ...) {
    if (!log_fp) return;
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    
    fprintf(log_fp, "[%s] [%s] ", time_buf, level);
    
    va_list args;
    va_start(args, fmt);
    vfprintf(log_fp, fmt, args);
    va_end(args);
    
    fprintf(log_fp, "\n");
    fflush(log_fp);
}

/* 根据 ALS 值线性映射到 1~21 亮度档位 */
static int als_to_brightness(int als_value) {
    int brightness;
    if (als_value <= 10) {
        brightness = MIN_BRIGHT;
    } else if (als_value >= 20) {
        brightness = MAX_BRIGHT;
    } else {
        brightness = MIN_BRIGHT + (als_value - 10) * (MAX_BRIGHT - MIN_BRIGHT) / 10;
    }
    return brightness;
}

/**
 * 自动查找 AP3216C 的 IIO 设备路径
 * 返回值: 0 成功，-1 失败
 */
static int find_iio_als_path(void) {
    DIR *dir;
    struct dirent *entry;
    char name_path[256];
    char buf[64];
    int found = 0;

    dir = opendir(IIO_BASE_PATH);
    if (!dir) {
        log_message("ERROR", "无法打开 IIO 目录 %s: %s", IIO_BASE_PATH, strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, IIO_DEVICE_PREFIX, strlen(IIO_DEVICE_PREFIX)) != 0)
            continue;

        snprintf(name_path, sizeof(name_path), "%s/%s/name", IIO_BASE_PATH, entry->d_name);
        FILE *fp = fopen(name_path, "r");
        if (fp) {
            if (fgets(buf, sizeof(buf), fp) != NULL) {
                buf[strcspn(buf, "\n")] = 0;  // 去除换行符
                if (strcmp(buf, "ap3216c") == 0) {
                    snprintf(iio_als_path, sizeof(iio_als_path), "%s/%s/in_illuminance_raw",
                             IIO_BASE_PATH, entry->d_name);
                    found = 1;
                    fclose(fp);
                    break;
                }
            }
            fclose(fp);
        }
    }
    closedir(dir);

    if (!found) {
        log_message("ERROR", "未找到 ap3216c 的 IIO 设备，请确认驱动已加载");
        return -1;
    }

    log_message("INFO", "找到 ALS IIO 路径: %s", iio_als_path);
    return 0;
}

/**
 * 读取 IIO 原始 ALS 值，并映射到 10~20 范围
 * 成功返回 0，并将映射后的值存入 *value
 */
static int read_als(int *value) {
    if (iio_als_path[0] == '\0') {
        if (find_iio_als_path() != 0)
            return -1;
    }

    FILE *fp = fopen(iio_als_path, "r");
    if (!fp) {
        log_message("ERROR", "无法打开 %s: %s", iio_als_path, strerror(errno));
        return -1;
    }

    int raw;
    if (fscanf(fp, "%d", &raw) != 1) {
        log_message("ERROR", "读取 IIO ALS 原始值失败");
        fclose(fp);
        return -1;
    }
    fclose(fp);

    /* 将原始值线性映射到 [ALS_MIN, ALS_MAX] 区间 */
    int mapped;
    if (raw <= RAW_MIN) {
        mapped = ALS_MIN;
    } else if (raw >= RAW_MAX) {
        mapped = ALS_MAX;
    } else {
        mapped = ALS_MIN + (raw - RAW_MIN) * (ALS_MAX - ALS_MIN) / (RAW_MAX - RAW_MIN);
    }

    *value = mapped;
    if (foreground_mode) {
        printf("[DEBUG] 原始 ALS = %d, 映射后 = %d\n", raw, mapped);
    }
    return 0;
}

/* 设置背光亮度 (1~21) */
static int set_brightness(int level) {
    if (level < MIN_BRIGHT) level = MIN_BRIGHT;
    if (level > MAX_BRIGHT) level = MAX_BRIGHT;

    if (brightness_fd == -1) {
        brightness_fd = open(BACKLIGHT_PATH, O_WRONLY);
        if (brightness_fd == -1) {
            log_message("ERROR", "无法打开背光设备 %s: %s", BACKLIGHT_PATH, strerror(errno));
            return -1;
        }
    }
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", level);
    lseek(brightness_fd, 0, SEEK_SET);
    if (write(brightness_fd, buf, strlen(buf)) == -1) {
        log_message("ERROR", "写入亮度 %d 失败: %s", level, strerror(errno));
        return -1;
    }
    return 0;
}

/* 恢复默认亮度并退出 */
static void restore_and_exit(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (foreground_mode) {
            printf("\n收到退出信号，正在恢复亮度...\n");
        }
        keep_running = 0;
    }
}

/* 进入守护进程模式（并打开日志文件） */
static void daemonize(void) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(EXIT_SUCCESS);  // 父进程退出

    // 子进程成为会话组长
    if (setsid() < 0) {
        perror("setsid");
        exit(EXIT_FAILURE);
    }

    // 忽略终端I/O信号
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // 二次fork防止意外获得终端
    pid = fork();
    if (pid < 0) {
        perror("fork2");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) exit(EXIT_SUCCESS);

    // 修改工作目录到根
    chdir("/");

    // 关闭标准文件描述符
    close(0);
    close(1);
    close(2);

    // 打开日志文件（追加模式）
    log_fp = fopen(LOG_FILE_PATH, "a");
    if (!log_fp) {
        log_fp = fopen("/tmp/als_autobright.log", "a");
        if (!log_fp) {
            log_fp = NULL;
        }
    }
}

/* 打印使用说明 */
static void print_usage(const char *progname) {
    printf("用法: %s [选项]\n", progname);
    printf("选项:\n");
    printf("  -f         前台模式（显示调试信息，Ctrl+C退出）\n");
    printf("  -h         显示此帮助信息\n");
    printf("\n");
    printf("默认模式（无参数）：后台守护进程模式\n");
    printf("\n");
    printf("示例:\n");
    printf("  %s           # 后台守护进程模式\n", progname);
    printf("  %s -f        # 前台模式，实时显示亮度调节过程\n", progname);
}

int main(int argc, char *argv[]) {
    int als_value;
    int new_bright, last_bright = -1;
    int i= 0;
    // 解析命令行参数
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            foreground_mode = 1;
        } else if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        } else {
            printf("未知参数: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    // 根据模式初始化日志
    if (foreground_mode) {
        // 前台模式：使用标准输出
        log_fp = stdout;
        printf("===========================================\n");
        printf("自动亮度调节程序 - 前台模式\n");
        printf("===========================================\n");
        printf("按 Ctrl+C 退出程序并恢复亮度\n\n");
    } else {
        // 守护进程模式
        daemonize();
    }

    // 注册信号处理
    signal(SIGINT, restore_and_exit);
    signal(SIGTERM, restore_and_exit);

    // 打开背光设备（只打开一次）
    brightness_fd = open(BACKLIGHT_PATH, O_WRONLY);
    if (brightness_fd == -1) {
        if (log_fp) {
            log_message("ERROR", "无法打开背光设备 %s: %s", BACKLIGHT_PATH, strerror(errno));
            if (!foreground_mode && log_fp) fclose(log_fp);
        }
        exit(EXIT_FAILURE);
    }

    // 读取当前亮度作为默认值（用于退出恢复）
    int fd_tmp = open(BACKLIGHT_PATH, O_RDONLY);
    if (fd_tmp != -1) {
        char buf[8];
        int len = read(fd_tmp, buf, sizeof(buf)-1);
        if (len > 0) {
            buf[len] = '\0';
            default_brightness = atoi(buf);
            if (default_brightness < MIN_BRIGHT || default_brightness > MAX_BRIGHT)
                default_brightness = DEFAULT_BRIGHT;
        }
        close(fd_tmp);
    }

    log_message("INFO", "自动亮度守护进程启动 (PID=%d)", getpid());
    log_message("INFO", "模式: %s", foreground_mode ? "前台模式" : "守护进程模式");
    log_message("INFO", "背光设备: %s", BACKLIGHT_PATH);
    log_message("INFO", "亮度范围: %d~%d, 默认亮度: %d", MIN_BRIGHT, MAX_BRIGHT, default_brightness);
    log_message("INFO", "采样间隔: %d 秒", INTERVAL_SEC);
    log_message("INFO", "原始 ALS 映射范围: [%d, %d] -> 逻辑值 [%d, %d]", RAW_MIN, RAW_MAX, ALS_MIN, ALS_MAX);

    if (foreground_mode) {
        printf("\n开始监测环境光...\n");
        printf("-------------------------------------------\n");
    }

    while (keep_running) {
        if (read_als(&als_value) == 0) {
            new_bright = als_to_brightness(als_value);
            if (new_bright != last_bright) {
                if (set_brightness(new_bright) == 0) {
                    log_message("INFO", "逻辑 ALS = %4d -> 亮度 %d", als_value, new_bright);
                    last_bright = new_bright;
                }
            } else {
                log_message("DEBUG", "逻辑 ALS = %4d, 亮度保持 %d", als_value, new_bright);
            }
        } else {
            log_message("WARNING", "读取 ALS 失败，跳过本次采样");
        }
        
        if (foreground_mode) {
            printf("下次采样时间: %d 秒后...\n", INTERVAL_SEC);
        }
        
        // 使用循环睡眠，以便能及时响应退出信号
        int remaining = INTERVAL_SEC;
        while (remaining > 0 && keep_running) {
            remaining -= sleep(remaining);
        }
    }

    // 退出前恢复默认亮度
    if (foreground_mode) {
        printf("\n正在恢复亮度...\n");
    }
    log_message("INFO", "恢复默认亮度 %d", default_brightness);
    set_brightness(default_brightness);
    close(brightness_fd);
    log_message("INFO", "自动亮度守护进程已退出");
    
    if (foreground_mode) {
        printf("亮度已恢复为 %d，程序退出。\n", default_brightness);
    }
    
    if (log_fp && !foreground_mode) {
        fclose(log_fp);
    }
    return 0;
}
