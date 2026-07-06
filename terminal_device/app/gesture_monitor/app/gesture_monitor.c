/*
 * 文件名: attitude_detect.c
 * 说明: 姿态角检测应用程序（后台监测模式）
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

/* IIO的sysfs路径 */
#define ACCEL_X_PATH    "/sys/bus/iio/devices/iio:device0/in_accel_x_raw"
#define ACCEL_Y_PATH    "/sys/bus/iio/devices/iio:device0/in_accel_y_raw"
#define ACCEL_Z_PATH    "/sys/bus/iio/devices/iio:device0/in_accel_z_raw"
#define ANGLE_THRESHOLD 15.0        /* 姿态角阈值（度数） */
#define MONITOR_INTERVAL 10         /* 监测周期（秒） */
#define ACCEL_SCALE    488281       /* 加速度计分辨率(±16g) */
#define ACCEL_LSB      (9.8 / (16384 / 16.0))  /* 加速度LSB转换为m/s^2 */

/* 全局变量 */
int g_debug_mode = 0;               /* 0=后台模式, 1=前台调试 */
int g_protect_ui_pid = -1;          /* protect_ui进程ID */

/*
 * 函数: read_accel_data
 * 说明: 从sysfs读取单个加速度计轴数据
 * 参数: path - sysfs文件路径
 * 返回: 加速度值（原始值）
 */
static int read_accel_data(const char *path)
{
    FILE *fp;
    int value = 0;
    
    fp = fopen(path, "r");
    if (fp == NULL) {
        perror("fopen");
        return 0;
    }
    /* 读取IIO信息到value中 */
    fscanf(fp, "%d", &value);
    fclose(fp);
    
    return value;
}

/*
 * 函数: calculate_tilt_angle
 * 说明: 根据加速度计XY轴数据计算倾角
 *       仅考虑X、Y两个轴，不考虑Z轴
 * 参数: accel_x - X轴加速度（原始值）
 *       accel_y - Y轴加速度（原始值）
 *       accel_z - Z轴加速度（原始值）
 */
static float calculate_tilt_angle(int accel_x, int accel_y, int accel_z)
{
    float angle_x, angle_y, tilt_angle;
    
    /* 避免除以零 */
    if (accel_z == 0)
        accel_z = 1;
    
    /* 计算绕各轴的倾角 */
    angle_x = atan2((float)accel_y, (float)accel_z) * 180.0 / M_PI;
    angle_y = atan2((float)accel_x, (float)accel_z) * 180.0 / M_PI;
    
    /* 综合倾角（欧几里得距离） */
    tilt_angle = sqrt(angle_x * angle_x + angle_y * angle_y);
    
    return tilt_angle;
}

/*
 * 函数: get_process_pid
 * 说明: 根据进程名查找进程ID
 * 参数: proc_name - 进程名称
 * 返回: 进程ID，未找到返回-1
 */
static pid_t get_process_pid(const char *proc_name)
{
    FILE *fp;
    char cmd[256], line[256];
    pid_t pid = -1;
    
    snprintf(cmd, sizeof(cmd), "pgrep -f %s | head -1", proc_name);
    fp = popen(cmd, "r");
    if (fp == NULL)
        return -1;
    
    if (fgets(line, sizeof(line), fp) != NULL) {
        pid = atoi(line);
    }
    
    pclose(fp);
    return pid;
}

/*
 * 函数: kill_process
 * 说明: 杀死指定进程
 * 参数: pid - 进程ID
 * 返回: 0成功, -1失败
 */
static int kill_process(pid_t pid)
{
    if (pid <= 0)
        return -1;
    
    if (kill(pid, SIGKILL) != 0) {
        perror("kill");
        return -1;
    }
    
    return 0;
}

/*
 * 函数: start_protect_ui
 * 说明: 启动保护页面进程
 * 返回: 子进程PID
 */
static pid_t start_protect_ui(void)
{
    pid_t pid = fork();
    
    if (pid == 0) {
        /* 子进程：执行protect_ui */
        execl("/usr/app/protect_ui", "protect_ui", NULL);
        fprintf(stderr, "Failed to execute protect_ui\n");
        exit(1);
    } else if (pid > 0) {
        /* 父进程 */
        return pid;
    } else {
        perror("fork");
        return -1;
    }
}

/*
 * 函数: monitor_attitude
 * 说明: 后台监测模式 - 定期检测姿态角，超过阈值则触发保护
 * 参数: 无
 * 返回: 无
 */
static void monitor_attitude(void)
{
    int accel_x, accel_y, accel_z;
    float tilt_angle;
    pid_t smart_schedule_pid;
    time_t last_time = 0, current_time;
    
    while (1) {
        /* 等待监测周期 */
        sleep(MONITOR_INTERVAL);
        
        /* 读取加速度计数据 */
        accel_x = read_accel_data(ACCEL_X_PATH);
        accel_y = read_accel_data(ACCEL_Y_PATH);
        accel_z = read_accel_data(ACCEL_Z_PATH);
        
        /* 计算倾角 */
        tilt_angle = calculate_tilt_angle(accel_x, accel_y, accel_z);
        
        current_time = time(NULL);
        
        /* 检查是否超过阈值 */
        if (tilt_angle > ANGLE_THRESHOLD) {
            /* 时间防抖：确保在MONITOR_INTERVAL秒内只触发一次 */
            if (current_time - last_time > MONITOR_INTERVAL) {
                fprintf(stderr, "[ALERT] Tilt angle exceeded: %.2f°\n", tilt_angle);
                
                /* 杀死smart_schedule进程 */
                smart_schedule_pid = get_process_pid("smart_schedule");
                if (smart_schedule_pid > 0) {
                    kill_process(smart_schedule_pid);
                    fprintf(stderr, "[ACTION] Killed smart_schedule (PID: %d)\n", 
                            smart_schedule_pid);
                }
                
                /* 启动保护页面 */
                g_protect_ui_pid = start_protect_ui();
                fprintf(stderr, "[ACTION] Started protect_ui (PID: %d)\n", g_protect_ui_pid);
                
                last_time = current_time;
            }
        }
    }
}

/*
 * 函数: debug_mode
 * 说明: 前台调试模式 - 实时输出姿态角到控制台
 * 参数: 无
 * 返回: 无
 */
static void debug_mode(void)
{
    int accel_x, accel_y, accel_z;
    float tilt_angle;
    
    printf("========================================\n");
    printf("ICM20608 姿态角检测 - 前台调试模式\n");
    printf("角度阈值: %.2f°\n", (float)ANGLE_THRESHOLD);
    printf("按 Ctrl+C 退出\n");
    printf("========================================\n\n");
    
    while (1) {
        /* 读取加速度计数据 */
        accel_x = read_accel_data(ACCEL_X_PATH);
        accel_y = read_accel_data(ACCEL_Y_PATH);
        accel_z = read_accel_data(ACCEL_Z_PATH);
        
        /* 计算倾角 */
        tilt_angle = calculate_tilt_angle(accel_x, accel_y, accel_z);
        
        /* 输出信息 */
        printf("[%ld] Accel: X=%6d Y=%6d Z=%6d | Tilt: %.2f° ", 
               time(NULL), accel_x, accel_y, accel_z, tilt_angle);
        
        if (tilt_angle > ANGLE_THRESHOLD) {
            printf("[WARNING] EXCEED THRESHOLD!\n");
        } else {
            printf("[OK]\n");
        }
        
        sleep(2);  /* 前台调试：2秒输出一次 */
    }
}

/*
 * 函数: daemonize
 * 说明: 将进程转换为后台守护进程
 * 参数: 无
 * 返回: 无
 */
static void daemonize(void)
{
    pid_t pid;
    int fd;
    
    /* 脱离终端控制 ，这样，用户退出终端时，守护进程不会收到 SIGHUP 信号 */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid > 0) {
        exit(0);  /* 父进程退出 */
    }
    
    /* 创建新的会话，脱离控制终端 */
    if (setsid() < 0) {
        perror("setsid");
        exit(1);
    }
    
    /* 第二次fork：确保不会重新获得控制终端 */
    pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }
    
    /* 设置文件掩码 */
    umask(0);
}

/*
 * 函数: signal_handler
 * 说明: 信号处理函数，优雅退出
 * 参数: sig - 信号编号
 * 返回: 无
 */
static void signal_handler(int sig)
{
    if (sig == SIGTERM || sig == SIGINT) {
        if (!g_debug_mode && g_protect_ui_pid > 0) {
            kill(g_protect_ui_pid, SIGTERM);
        }
        exit(0);
    }
}

/*
 * 函数: print_usage
 * 说明: 打印使用说明
 * 参数: prog_name - 程序名
 * 返回: 无
 */
static void print_usage(const char *prog_name)
{
    printf("使用方法:\n");
    printf("  %s debug    - 前台调试模式（实时输出姿态角）\n", prog_name);
    printf("  %s daemon   - 后台监测模式（监测并自动处理警报）\n", prog_name);
    printf("\n");
    printf("前台调试模式:\n");
    printf("  实时显示加速度计XYZ数据和计算出的倾角\n");
    printf("  2秒更新一次\n");
    printf("\n");
    printf("后台监测模式:\n");
    printf("  以%d秒为周期监测姿态角\n", MONITOR_INTERVAL);
    printf("  当倾角 > %.1f° 时:\n", (float)ANGLE_THRESHOLD);
    printf("    - 杀死 smart_schedule 进程\n");
    printf("    - 启动 protect_ui 保护页面\n");
}

/*
 * 主函数
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* 注册信号处理函数  SIGTERM: kill命令默认信号  SIGINT: Ctrl+C键盘中断   */
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    if (strcmp(argv[1], "debug") == 0) {
        /* 前台调试模式 */
        g_debug_mode = 1;
        debug_mode();
    } else if (strcmp(argv[1], "daemon") == 0) {
        /* 后台监测模式 */
        g_debug_mode = 0;
        daemonize();
        monitor_attitude();
    } else {
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}

