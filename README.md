# 🍳 AI语音厨房安全管控系统

基于 STM32F103 的智能厨房安全监测系统，集成温湿度、气体、火焰等多类传感器实时监测厨房环境，通过 ESP8266 以 MQTT 协议上云；基于 FastMCP 搭建 MCP Server 将硬件能力暴露为 AI 工具接口，接入小智语音助手实现自然语言控制硬件。

## 功能特性

### 安全监测
- **火焰检测**（PC14）：实时监测明火，及时发现火灾隐患
- **气体检测**（PA6/ADC）：MQ2 传感器监测甲烷等可燃气体浓度（阈值：50 PPM）
- **温湿度监测**（PC13/DHT11）：单总线微秒级时序驱动采集环境温湿度
- **光照监测**（PA4/ADC）：光敏电阻 ADC 采集 + 七段非线性 Lux 映射 + 移动平均滤波

### 警报响应
- 超限时触发 LED 常亮告警 + 500~2000Hz 变频蜂鸣器扫频警报 + 风扇强制满速排烟
- 标志位锁定机制保证安全优先级——警报状态下按键/网页无法关闭风扇、自动退出手动模式
- KEY1（PB3）手动警报开关，一键控制 LED + 蜂鸣器防空警报音效
- KEY2（PA0）手动风扇档位切换

### 系统启动流程
- MQ2 预热期间自动屏蔽所有报警（防止上电误触发）
- OLED 显示开机画面 + 播放小星星开机音乐（利用预热等待时间）
- 启动后上报自检结果（selftest）、上线消息（system online）、就绪通知（system ready）

### 数据展示与通信
- **OLED 显示屏**（PB6/PB7 I2C）：循环显示各传感器数据
- **MQTT 云端通信**（ESP8266）：按 Topic 分层发布传感器数据和设备状态，订阅云端控制指令
- **USART 中断驱动**：单字节指令解析（a~h），毫秒级响应

### AI 语音控制
- FastMCP 封装 14 个标准化硬件工具接口（灯/风扇/蜂鸣器/温湿度/气体/火焰）
- 打通"语音指令→小智→LLM→MCP Tool Call→paho-mqtt→MQTT Broker→ESP8266→STM32中断→硬件执行"完整闭环

## 引脚分配与电压要求

| 模块 | 引脚 | 供电电压 | 功能说明 |
|------|------|---------|---------|
| **STM32F103C8T6** 主控 | — | **3.3V** | Cortex-M3, 72MHz |
| **DHT11** 温湿度 | **PC13** | 3.3V / 5V 均可 | 单总线时序模拟（开漏输出+上拉） |
| **火焰传感器** | **PC14** | 3.3V / 5V 均可 | 数字输入（浮空） |
| **KEY2** 风扇按键 | **PA0** (EXTI0) | **3.3V** | 外部中断，下拉，上升/下降沿 |
| **光照传感器** | **PA4** (ADC1_IN4) | 3.3V / 5V 均可 | ADC 模拟输入，DMA 循环采集 |
| **风扇方向控制** | **PA5** (GPIO) | **3.3V**（仅信号） | 推挽输出，固定高电平向外排风 |
| **MQ2** 气体传感器 | **PA6** (ADC1_IN6) | **5V（必须）** | ADC 模拟输入，DMA 循环采集 |
| **蜂鸣器** | **PA8** (TIM1_CH1 PWM) | 3.3V / 5V | PWM 输出，500Hz~2000Hz 变频 |
| **USART2_TX** → ESP8266 RX | **PA2** | **ESP8266 接 3.3V（严禁 5V！）** | 复用推挽 |
| **USART2_RX** ← ESP8266 TX | **PA3** | 同上 | 输入 |
| **USART1_TX** 调试串口 | **PA9** | **3.3V** | 复用推挽 |
| **USART1_RX** 调试串口 | **PA10** | **3.3V** | 输入 |
| **LED1** | **PB0** | **3.3V** | 推挽输出 |
| **LED2** | **PB1** | **3.3V** | 推挽输出 |
| **KEY1** 警报按键 | **PB3** (EXTI3) | **3.3V** | 外部中断，上拉，上升/下降沿 |
| **OLED SCL** | **PB6** (I2C1) | 3.3V / 5V 均可 | 复用开漏，400kHz Fast Mode |
| **OLED SDA** | **PB7** (I2C1) | 同上 | 复用开漏 |
| **风扇 PWM** | **PB8** (TIM4_CH3 PWM) | **外部独立供电**（PB8 仅 3.3V 信号） | H桥反转驱动，四档调速 |

> ⚠️ 关键注意事项：
> - **ESP8266 绝对不能接 5V**，必须 3.3V 供电，否则直接烧毁
> - **MQ2 必须 5V** 供电，否则加热丝功率不足导致传感器不准
> - **风扇需额外独立电源**，PB8 只输出 PWM 控制信号
> - ADC 参考电压为 3.3V，模拟输入不应超过此值

## 项目结构

```
Core/
├── Inc/                          # 头文件
│   ├── main.h                    # 主头文件（含系统状态宏、设备ID、阈值配置）
│   ├── dht11.h                   # DHT11 温湿度传感器驱动
│   ├── mq2.h                     # MQ2 气体传感器驱动（含 IsReady/StabilityCheck API）
│   ├── light.h                   # 光照传感器驱动（含校准参数）
│   ├── oled.h                    # SSD1306 OLED 显示屏驱动（I2C）
│   ├── fan.h                     # 风扇 PWM 控制（H桥反转，四档调速）
│   ├── led_buzzer.h              # LED 和蜂鸣器驱动（防空警报音效）
│   ├── gpio.h                    # GPIO 初始化 + 按键中断回调
│   ├── usart.h                   # USART 配置 + 中断接收 + 单字节指令解析
│   ├── tim.h                     # TIM1(蜂鸣器)/TIM2(DHT11延时)/TIM4(风扇PWM)
│   ├── adc.h / dma.h             # ADC 双通道 DMA 循环采集
│   ├── i2c.h / oled.h            # I2C1 (400kHz) + OLED 驱动
│   └── delay.h                   # TIM2 微秒级延时
├── Src/                          # 源文件
│   ├── main.c                    # 主程序（初始化 + 主循环 + 报警联动逻辑）
│   └── ...                       # 各模块实现
Drivers/                           # STM32 HAL 驱动库
MDK-ARM/                          # Keil MDK 工程文件
Web.html                          # Web 端实时控制系统（mqtt.js WebSocket-MQTT）
calculator.py                     # FastMCP Server（14个 @mcp.tool() + paho-mqtt）
```

## 开发环境

| 项目 | 版本/选型 |
|------|---------|
| IDE | Keil MDK-ARM V5 |
| MCU | STM32F103C8T6 (Cortex-M3, 72MHz, 64KB Flash) |
| 编译器 | ARMCC V5.06 update 7 |
| 框架 | STM32 HAL 库 |

## 快速开始

### 硬件准备
- STM32F103C8T6 最小系统板
- DHT11 温湿度传感器模块
- MQ2 气体传感器模块
- 0.96 寸 OLED 显示屏（SSD1306, I2C 接口）
- 光敏电阻传感器模块
- 火焰传感器模块
- 直流风扇 + H 桥/L298N 驱动板（或带驱动的风扇模块）
- LED × 2、有源/无源蜂鸣器 × 1
- 按键 × 2
- ESP8266 WiFi 模块
- 外部 5V 电源（供 MQ2 和风扇）

### 编译与下载
1. 安装 Keil MDK-ARM 并配置 STM32F1xx Device Family Pack
2. 打开 `MDK-ARM/Kitchen Safety Alert.uvprojx`
3. Build (F7) → Download (F8)

## MQTT 数据格式

### 发布（STM32 → 云端）

```
huahua/sensor/dht11    25.5_60          # 温度_湿度
huahua/sensor/light    500              # 光照原始ADC值
huahua/sensor/mq2      45.23            # 气体浓度PPM
huahua/state/fan       3                # 风扇档位(0=关,1=3=三档)
huahua/state/buzzer    1                # 蜂鸣器状态(0/1)
huahua/state/led       1                # LED状态(0/1)
huahua/state/fire      0                # 火焰检测(0/1)
huahua/system          online           # 设备上线
huahua/selftest        MQ2_OK           # 自检结果(MQ2_OK/MQ2_ERR_stateX)
huahua/system          ready             # 系统就绪(暖机完成)
```

### 订阅（云端 → STM32）

```
huahua/cmd              a                # 单字节指令:
                                           a=灯开 b=灯关 c=蜂鸣器开 d=蜂鸣器关
                                           e=风扇开 f=风扇关 g=风扇2档 h=风扇3档
```

## 配置参数

| 参数 | 默认值 | 说明 | 定义位置 |
|------|--------|------|---------|
| `DEVICE_ID` | `huahua` | 设备唯一标识（MQTT Topic前缀） | main.h |
| `GAS_THRESHOLD_MQ2` | `50.0f` PPM | 气体报警阈值 | main.c |
| `LOOP_DELAY_MS` | `2000` ms | 主循环周期 | main.c |
| `MQ2_PREHEAT_TIME_SEC` | `6` 秒 | MQ2预热时长 | main.h |
| `SYSTEM_WARMUP_CYCLES` | `3` 次 | 暖机屏蔽报警循环数(~6秒) | main.c |
| `MQ2_VCC` | `5.0f` V | MQ2参考电压（必须5V供电） | mq2.h |
| `UART_TIMEOUT_CMD` | `100` ms | USART指令超时 | usart.h |
| `UART_TIMEOUT_DATA` | `200` ms | USART数据超时 | usart.h |

## MCP 工具接口列表（AI 控制）

| 工具名 | 功能 | 对应指令 |
|--------|------|---------|
| `lamp_turn_on` | 开灯 | a |
| `lamp_turn_off` | 关灯 | b |
| `buzzer_turn_on` | 开启警报 | c |
| `buzzer_turn_off` | 关闭警报 | d |
| `fan_turn_on` | 开启风扇(默认1档) | e |
| `fan_turn_off` | 关闭风扇 | f |
| `fan_level_2` | 风扇调到2档 | g |
| `fan_level_3` | 风扇调到3档(最大) | h |
| `get_temperature` | 查询温度 | 读取 sensor/dht11 |
| `get_humidity` | 查询湿度 | 同上 |
| `get_light_level` | 查询光照强度 | 读取 sensor/light |
| `get_gas_concentration` | 查询气体浓度 | 读取 sensor/mq2 |
| `get_fire_status` | 查询火焰状态 | 读取 state/fire |
| `lamp_get_state` / `buzzer_get_state` / `fan_get_state` | 查询设备状态 | 读取 state/* |
