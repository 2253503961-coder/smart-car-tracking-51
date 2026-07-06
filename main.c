/**
 * @file    main.c
 * @brief   基于AT89C51的双轮差速智能循迹小车
 * @author  Reinovo
 * @date    2025
 *
 * 核心功能：
 *   1. 双路红外循迹传感器信号采集
 *   2. 软件PWM调速（T0定时器，8位自动重装，50us周期）
 *   3. 双模式控制算法：比例修正 / 增量式PID
 *   4. L293D双H桥电机驱动
 */

#include <reg51.h>

/* ==================== 类型定义 ==================== */
typedef unsigned char  uint8;
typedef unsigned int   uint16;
typedef signed   int   int16;

/* ==================== 硬件引脚映射 ==================== */
/* ---------- 电机驱动 (L293D) ---------- */
sbit ENB = P1^0;   /* 右电机使能 (PWM)      */
sbit IN4 = P1^1;   /* 右电机方向B           */
sbit IN3 = P1^2;   /* 右电机方向A           */
sbit IN2 = P1^3;   /* 左电机方向B           */
sbit IN1 = P1^4;   /* 左电机方向A           */
sbit ENA = P1^5;   /* 左电机使能 (PWM)      */

/* ---------- 传感器 ---------- */
sbit DL  = P3^4;   /* 左红外传感器 (0=黑线) */
sbit DR  = P3^5;   /* 右红外传感器 (0=黑线) */
sbit KEY = P1^6;   /* 启动按键              */

/* ==================== 系统参数配置 ==================== */
#define PWM_BASE       180    /* 基准占空比 (70%)             */
#define PWM_PERIOD     256    /* PWM周期 (定时器计数)         */
#define T0_RELOAD      (256-50) /* 50us 中断周期 @12MHz       */

/* ---------- PID 参数 (可调) ---------- */
#define PID_KP         60     /* 比例系数                     */
#define PID_KI         2      /* 积分系数                     */
#define PID_KD         15     /* 微分系数                     */
#define PID_DEADBAND   3      /* 死区：误差<3视为在线上       */

/* ---------- 控制模式选择 ---------- */
#define MODE_PROPORTIONAL  0   /* 比例修正模式                */
#define MODE_PID           1   /* PID控制模式                 */
#define CTRL_MODE  MODE_PID    /* 当前控制模式（可切换）      */

/* ==================== 全局变量 ==================== */
uint8  PwmL, PwmR;              /* 左右电机PWM占空比 (0~255)  */
int16  pid_error     = 0;       /* 当前误差                    */
int16  pid_last_error = 0;      /* 上次误差                    */
int16  pid_integral  = 0;       /* 积分累加                    */
int16  pid_derivative = 0;      /* 微分项                      */
int16  pid_output    = 0;       /* PID输出 (叠加到PWM的增量)   */

/* ==================== 基础函数 ==================== */

/**
 * @brief 毫秒级延时 (12MHz晶振, 约1ms/循环)
 */
void delay(uint16 ms)
{
    uint16 i, j;
    for (i = 0; i < ms; i++)
        for (j = 0; j < 120; j++);
}

/* ==================== 运动控制 ==================== */

/**
 * @brief 前进：双电机正转
 */
void go(void)
{
    IN1 = 1; IN2 = 0;
    IN3 = 1; IN4 = 0;
}

/**
 * @brief 后退：双电机反转
 */
void back(void)
{
    IN1 = 0; IN2 = 1;
    IN3 = 0; IN4 = 1;
}

/**
 * @brief 原地左转：左反 + 右正
 */
void turn_left(void)
{
    IN1 = 0; IN2 = 1;
    IN3 = 1; IN4 = 0;
}

/**
 * @brief 原地右转：左正 + 右反
 */
void turn_right(void)
{
    IN1 = 1; IN2 = 0;
    IN3 = 0; IN4 = 1;
}

/**
 * @brief 停止：四管截止
 */
void stop(void)
{
    IN1 = 0; IN2 = 0;
    IN3 = 0; IN4 = 0;
}

/* ==================== 传感器读取 ==================== */

/**
 * @brief 读取红外传感器状态，计算偏差
 * @return 偏差值: -1=偏右, 0=居中, +1=偏左
 *
 * 传感器输出逻辑: 0=检测到黑线, 1=白底
 */
int8 read_sensors(void)
{
    uint8 sl = (DL == 0) ? 1 : 0;  /* 1=在黑线上 */
    uint8 sr = (DR == 0) ? 1 : 0;

    if (sl && sr)      return  0;   /* 居中   */
    else if (sl && !sr) return -1;   /* 偏右   */
    else if (!sl && sr) return +1;   /* 偏左   */
    else                return  2;   /* 完全偏离 */
}

/* ==================== 控制算法 ==================== */

/**
 * @brief 模式0: 比例修正控制
 *
 * 根据传感器状态直接设定两轮PWM差值。
 * 优点: 响应快，无超调
 * 缺点: 无法消除稳态误差，弯道适应性差
 */
static void ctrl_proportional(int8 deviation)
{
    switch (deviation) {
        case -1:  /* 偏右 → 左转修正 */
            PwmR = 220; PwmL = 50;  break;
        case +1:  /* 偏左 → 右转修正 */
            PwmR = 50;  PwmL = 220; break;
        case 0:   /* 居中 → 直行 */
            PwmR = PwmL = PWM_BASE; break;
        default:  /* 完全偏离 → 减速搜索 */
            PwmR = PwmL = 120; break;
    }
}

/**
 * @brief 模式1: 增量式PID控制
 *
 * 增量式PID公式:
 *   Δu(k) = Kp * [e(k) - e(k-1)] + Ki * e(k) + Kd * [e(k) - 2e(k-1) + e(k-2)]
 *
 * 输出为叠加到基准PWM的增量，通过差速实现转向。
 */
static void ctrl_pid(int8 deviation)
{
    int16  delta;
    int16  last_error_backup;

    /* 偏差映射: -1 → +1 映射到误差值 */
    pid_error = (int16)deviation * 20;  /* 放大以增强控制灵敏度 */

    /* 死区判断 */
    if (pid_error <= PID_DEADBAND && pid_error >= -PID_DEADBAND) {
        pid_error = 0;
        pid_integral = 0;
    }

    /* 积分抗饱和 */
    pid_integral += pid_error;
    if (pid_integral >  200) pid_integral =  200;
    if (pid_integral < -200) pid_integral = -200;

    /* 微分计算 */
    pid_derivative = pid_error - pid_last_error;

    /* 增量式PID输出 */
    delta = PID_KP * pid_derivative
          + PID_KI * pid_error
          + PID_KD * (pid_derivative - (pid_last_error - 0)); /* 简化二阶差分 */

    /* 保存历史 */
    pid_last_error = pid_error;

    /* 输出叠加: 正输出=右转(左快右慢), 负输出=左转 */
    delta = delta / 4;  /* 缩放至PWM合理范围 */
    PwmL = PWM_BASE + delta;
    PwmR = PWM_BASE - delta;

    /* 限幅 */
    if (PwmL > 250) PwmL = 250;
    if (PwmL <  30) PwmL = 30;
    if (PwmR > 250) PwmR = 250;
    if (PwmR <  30) PwmR = 30;

    /* 完全偏离时减速 */
    if (deviation == 2) {
        PwmL = PwmR = 120;
        pid_integral = 0;
    }
}

/* ==================== 定时器初始化 ==================== */

/**
 * @brief T0初始化：模式2, 8位自动重装, 50us中断周期
 *
 * PWM频率: f_pwm = 12MHz / 12 / 256 ≈ 3.9kHz (不分频)
 *           实际 = 12MHz / 12 / 50us / 256 ≈ 78Hz
 */
void Timer0_Init(void)
{
    TMOD &= 0xF0;          /* 清除T0配置位        */
    TMOD |= 0x02;          /* T0: 模式2, 自动重装  */
    TH0   = T0_RELOAD;     /* 重装值               */
    TL0   = T0_RELOAD;     /* 初值                 */
    ET0   = 1;             /* 使能T0中断           */
    EA    = 1;             /* 使能总中断           */
    TR0   = 1;             /* 启动T0               */
}

/* ==================== 主程序 ==================== */

void main(void)
{
    int8   dev;
    uint16 loop_cnt = 0;

    ENA = 1;
    ENB = 1;
    Timer0_Init();
    PwmR = PwmL = PWM_BASE;

    /* 等待启动按键 */
    while (KEY);
    go();

    while (1)
    {
        dev = read_sensors();

#if CTRL_MODE == MODE_PROPORTIONAL
        ctrl_proportional(dev);
#elif CTRL_MODE == MODE_PID
        ctrl_pid(dev);
#endif

        /* 简单延时防止过于频繁的传感器读取 */
        delay(2);
    }
}

/* ==================== T0中断服务: 软件PWM生成 ==================== */

void Timer0_ISR(void) interrupt 1
{
    static uint8 ct = 0;
    ct++;

    /* 右电机PWM */
    ENB = (ct < PwmR) ? 1 : 0;

    /* 左电机PWM */
    ENA = (ct < PwmL) ? 1 : 0;

    /* ct: 0→255, 256周期自动溢出 (8位模式) */
}
