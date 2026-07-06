---
AIGC:
    Label: "1"
    ContentProducer: 001191440300708461136T1XGW3
    ProduceID: a9bc843330e7b9dfb48be9ad20e523a4_e29f36a6796511f1b3d35254007bceed
    ReservedCode1: Ay9s0OjE063Jw/g7QUbQE1GKvL/ii767SaBfNEkG5IRKe6SWLe/NGDjd4feS+RHaSi+1UwBOZ34Cty5GXsdwZxvR5wuK2+Ot5YVqWntRZsD2ybkZzExVtjRIhdQ0TY7AU+5L/Jur5FysjZk8YSma/aacQa7ZwY2YrsXgqwAwckUx/QruORONkmh63YQ=
    ContentPropagator: 001191440300708461136T1XGW3
    PropagateID: a9bc843330e7b9dfb48be9ad20e523a4_e29f36a6796511f1b3d35254007bceed
    ReservedCode2: Ay9s0OjE063Jw/g7QUbQE1GKvL/ii767SaBfNEkG5IRKe6SWLe/NGDjd4feS+RHaSi+1UwBOZ34Cty5GXsdwZxvR5wuK2+Ot5YVqWntRZsD2ybkZzExVtjRIhdQ0TY7AU+5L/Jur5FysjZk8YSma/aacQa7ZwY2YrsXgqwAwckUx/QruORONkmh63YQ=
---

# Proteus 仿真原理图搭建说明

## 所需元件清单

| 元件 | Proteus 关键字 | 数量 | 备注 |
|------|---------------|------|------|
| AT89C51 | `AT89C51` | 1 | 或 STC89C52 |
| L293D | `L293D` | 1 | 双H桥电机驱动 |
| DC Motor | `MOTOR-DC` | 2 | 左右驱动轮 |
| 红外对管 | `IRLINK` 或 `OPTOCOUPLER-NPN` | 2 | 循迹传感器 |
| 按键 | `BUTTON` | 1 | 启动键 |
| 电阻 10KΩ | `RES` | 3 | 上拉 |
| 排阻 10KΩ | `RESPACK-8` | 1 | P0口上拉 |
| 晶振 12MHz | `CRYSTAL` | 1 | 系统时钟 |
| 电容 30pF | `CAP` | 2 | 晶振负载 |
| 电容 10μF | `CAP-ELEC` | 1 | 复位 |
| 电阻 10KΩ | `RES` | 1 | 复位 |
| 电源 VCC/GND | `POWER` / `GROUND` | 若干 | |

## 引脚连接表

### L293D 电机驱动

| L293D 引脚 | 连接目标 | 说明 |
|-----------|---------|------|
| 1 (EN1,2) | P1^5 (ENA) | 左电机使能 |
| 2 (1A) | P1^4 (IN1) | 左电机方向A |
| 3 (1Y) | 左电机+ | |
| 6 (2Y) | 左电机- | |
| 7 (2A) | P1^3 (IN2) | 左电机方向B |
| 9 (EN3,4) | P1^0 (ENB) | 右电机使能 |
| 10 (3A) | P1^2 (IN3) | 右电机方向A |
| 11 (3Y) | 右电机+ | |
| 14 (4Y) | 右电机- | |
| 15 (4A) | P1^1 (IN4) | 右电机方向B |
| 8 (VCC2) | +5V 电机电源 | |
| 16 (VCC1) | +5V 逻辑电源 | |
| 4,5,12,13 | GND | |

### 红外循迹传感器

| 传感器 | 连接 | 说明 |
|--------|------|------|
| 左红外 OUT | P3^4 (DL) | 接10KΩ上拉电阻到VCC |
| 右红外 OUT | P3^5 (DR) | 接10KΩ上拉电阻到VCC |

> 注：红外对管检测到黑线时输出低电平，检测到白色地面时输出高电平。
*（内容由AI生成，仅供参考）*
