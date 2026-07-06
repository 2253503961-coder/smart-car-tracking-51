#include <reg51.h>

typedef unsigned char uint8;
typedef unsigned int uint16;

/* ============ 引脚定义 ============ */
sbit ENB = P1^0;   // 右电机使能
sbit IN4 = P1^1;   // 右电机方向B
sbit IN3 = P1^2;   // 右电机方向A
sbit IN2 = P1^3;   // 左电机方向B
sbit IN1 = P1^4;   // 左电机方向A
sbit ENA = P1^5;   // 左电机使能

sbit DL  = P3^4;   // 左循迹传感器（黑线检测）
sbit DR  = P3^5;   // 右循迹传感器（黑线检测）
sbit KEY = P1^6;   // 启动按键

uint8 PwmL, PwmR;  // 左右电机PWM占空比（0~255）

/* ============ 延时函数 ============ */
void delay(uint16 ms)
{
    uint16 i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 120; j++);  // 约1ms（12MHz晶振）
}

/* ============ 电机控制函数 ============ */
// 前进：两电机正转
void go()
{
    IN1 = 1; IN2 = 0;   // 左电机正转
    IN3 = 1; IN4 = 0;   // 右电机正转
}

// 后退：两电机反转
void back()
{
    IN1 = 0; IN2 = 1;
    IN3 = 0; IN4 = 1;
}

// 左转：左轮反转 + 右轮正转（原地左转）
void left()
{
    IN1 = 0; IN2 = 1;   // 左电机反转
    IN3 = 1; IN4 = 0;   // 右电机正转
}

// 右转：左轮正转 + 右轮反转（原地右转）
void right()
{
    IN1 = 1; IN2 = 0;   // 左电机正转
    IN3 = 0; IN4 = 1;   // 右电机反转
}

// 停止
void stop()
{
    IN1 = 0; IN2 = 0;
    IN3 = 0; IN4 = 0;
}

/* ============ 定时器初始化 ============ */
void TimerInit()
{
    TMOD = 0x02;               // T0：8位自动重装模式
    TH0  = TL0 = 256 - 50;     // 50us定时周期（12MHz，不分频）
    ET0  = 1;                  // 使能T0中断
    EA   = 1;                  // 使能总中断
    TR0  = 1;                  // 启动T0
}

/* ============ 主函数 ============ */
void main()
{
    ENA = 1;    // 使能左电机
    ENB = 1;    // 使能右电机
    TimerInit();
    PwmR = PwmL = 180;   // 初始占空比 180/255 ≈ 70%

    while(KEY);          // 等待按键按下
    go();                // 开始前进

    while(1)
    {
        // ---- 循迹逻辑：两路红外对管检测黑线 ----
        // DL=0 表示左传感器在黑线上，DR=0 表示右传感器在黑线上
        // （根据实际传感器输出电平调整，此处假设检测到黑线输出低电平）

        if(DL == 0 && DR == 0)
        {
            // 两路都在黑线上 → 直行
            PwmR = PwmL = 180;
        }
        else if(DL == 1 && DR == 0)
        {
            // 左偏出黑线 → 右转修正
            PwmR = 50;
            PwmL = 220;
        }
        else if(DL == 0 && DR == 1)
        {
            // 右偏出黑线 → 左转修正
            PwmR = 220;
            PwmL = 50;
        }
        else
        {
            // 两路都偏出 → 减速寻找黑线
            PwmR = PwmL = 120;
        }
    }
}

/* ============ 定时器0中断：软件PWM生成 ============ */
void Timer0_ISR(void) interrupt 1
{
    static uint8 ct = 0;   // 0~255 循环计数
    ct++;

    // 右电机PWM
    if(ct < PwmR)  ENB = 1;
    else           ENB = 0;

    // 左电机PWM
    if(ct < PwmL)  ENA = 1;
    else           ENA = 0;

    // ct从255回到0时自动溢出，8位自动重装无需手动清零
}
