/*
 * ICM20608 寄存器定义头文件
 * 包含所有传感器寄存器地址和配置参数
 */
#ifndef _ICM20608_REG_H
#define _ICM20608_REG_H

/* ICM20608寄存器地址定义 */
#define ICM20_SMPLRT_DIV        0x19    /* 采样率分频器 */
#define ICM20_CONFIG            0x1A    /* 低通滤波配置 */
#define ICM20_GYRO_CONFIG       0x1B    /* 陀螺仪配置 */
#define ICM20_ACCEL_CONFIG      0x1C    /* 加速度计配置 */
#define ICM20_ACCEL_CONFIG2     0x1D    /* 加速度计配置2 */
#define ICM20_LP_MODE_CFG       0x1E    /* 低功耗模式配置 */
#define ICM20_ACCEL_XOUT_H      0x3B    /* 加速度X高位 */
#define ICM20_ACCEL_YOUT_H      0x3D    /* 加速度Y高位 */
#define ICM20_ACCEL_ZOUT_H      0x3F    /* 加速度Z高位 */
#define ICM20_TEMP_OUT_H        0x41    /* 温度高位 */
#define ICM20_GYRO_XOUT_H       0x43    /* 陀螺仪X高位 */
#define ICM20_GYRO_YOUT_H       0x45    /* 陀螺仪Y高位 */
#define ICM20_GYRO_ZOUT_H       0x47    /* 陀螺仪Z高位 */
#define ICM20_PWR_MGMT_1        0x6B    /* 电源管理1 */
#define ICM20_PWR_MGMT_2        0x6C    /* 电源管理2 */
#define ICM20_INT_ENABLE        0x38    /* 中断使能 */
#define ICM20_INT_STATUS        0x3A    /* 中断状态 */
#define ICM20_WHO_AM_I          0x75    /* 芯片ID寄存器 */
#define ICM20_XG_OFFS_USRH      0x13    /* 陀螺仪X轴校准 */
#define ICM20_XA_OFFSET_H       0x77    /* 加速度计X轴校准 */

#endif /* _ICM20608_REG_H */

