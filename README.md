# 🍳 厨房安全警报系统

基于 STM32F103 的智能厨房安全监测系统，实时监测温度、湿度、气体浓度、光照强度等环境参数，并在危险情况下自动触发警报。

## 📋 功能特性

### 🔥 安全监测
- **火焰检测**：实时监测明火，及时发现火灾隐患
- **气体检测**：MQ2 传感器监测甲烷等可燃气体浓度（阈值：50 PPM）
- **温湿度监测**：DHT11 传感器实时采集环境温湿度数据
- **光照监测**：光敏传感器监测环境光照强度

### ⚡ 警报响应
- 危险情况下自动触发 LED 闪烁和蜂鸣器报警
- 自动启动风扇进行通风排气
- 支持手动/自动模式切换

### 📊 数据展示
- **OLED 显示屏**：循环显示各传感器数据
- **云端上传**：通过 UART 实时上传传感器数据至云端
- **状态监控**：实时显示设备运行状态

### 🔧 硬件配置
- **主控芯片**：STM32F103CBT6
- **显示屏**：OLED (I2C 接口)
- **传感器**：DHT11、MQ2、光敏电阻
- **执行器**：风扇（PWM 控制）、LED、蜂鸣器

## 📁 项目结构

```
Object1/
├── Core/                    # 用户代码
│   ├── Inc/                # 头文件
│   │   ├── main.h          # 主头文件
│   │   ├── dht11.h         # 温湿度传感器驱动
│   │   ├── mq2.h           # 气体传感器驱动
│   │   ├── light.h         # 光照传感器驱动
│   │   ├── oled.h          # OLED 显示屏驱动
│   │   ├── fan.h           # 风扇控制驱动
│   │   └── led_buzzer.h    # LED 和蜂鸣器驱动
│   └── Src/                # 源文件
│       ├── main.c          # 主程序
│       ├── dht11.c         # 温湿度传感器实现
│       ├── mq2.c           # 气体传感器实现
│       └── ...
├── Drivers/                # STM32 HAL 驱动库
├── MDK-ARM/               # Keil MDK 工程文件
├── Web.html               # 网页端控制系统
├── calculator.py          # MCP 接入模块（小智AI语音控制）
└── .gitignore             # Git 忽略规则
```

## 🎮 智能控制扩展

### 🌐 网页端控制
使用 `Web.html` 可以在网页端直接监控和控制整套系统，包括查看传感器数据、控制风扇开关、调整报警阈值等。

网页已部署到服务器，你可以直接访问：[www.ithuahua.xyz](https://www.ithuahua.xyz) 进行控制！

### 🎤 小智AI语音控制
通过 `calculator.py` 接入 MCP 协议，支持使用小智AI进行语音控制。详细配置请参考：
https://my.feishu.cn/wiki/HiPEwZ37XiitnwktX13cEM5KnSb

## 🔌 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| I2C1 (OLED) | PB6, PB7 | I2C 通信 |
| USART1 | PA9, PA10 | 调试串口 |
| USART2 | PA2, PA3 | 云端通信 |
| ADC1 | PC0, PC1, PC2 | 传感器采集 |
| TIM1 | PA8 | PWM 输出 |
| TIM2 | PA0, PA1 | 定时器 |
| TIM4 | PB6, PB7, PB8, PB9 | 风扇 PWM 控制 |
| GPIO | PA0-PA15, PC0-PC15 | 输入输出 |

## 🛠️ 开发环境

- **IDE**：Keil MDK-ARM / VS Code
- **编译器**：ARM GCC / ARMCC
- **芯片**：STM32F103CBTx

## 🚀 快速开始

### 1. 硬件准备
- STM32F103 开发板
- DHT11 温湿度传感器模块
- MQ2 气体传感器模块
- 0.96 寸 OLED 显示屏（I2C 接口）
- 光敏电阻模块
- 风扇模块（PWM 控制）
- LED、蜂鸣器

### 2. 软件配置
1. 安装 Keil MDK-ARM
2. 打开 `MDK-ARM/Kitchen Safety Alert.uvprojx` 工程文件

### 3. 编译与下载
```bash
# 使用 Keil 编译
# 1. 打开 MDK-ARM/Kitchen Safety Alert.uvprojx
# 2. 点击 Build (F7)
# 3. 点击 Download (F8)
```

## 📡 云端数据格式

设备通过 UART2 上传数据，格式如下：

```
# 温湿度数据
huahua/sensor/dht11 25.5_60

# 光照强度
huahua/sensor/light 500

# 气体浓度
huahua/sensor/mq2 45.23

# 设备状态
huahua/state/fan 3
huahua/state/buzzer 1
huahua/state/led 1
huahua/state/fire 0
```

## ⚙️ 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `DEVICE_ID` | huahua | 设备唯一标识 |
| `GAS_THRESHOLD_MQ2` | 50.0 PPM | 气体报警阈值 |
| `LOOP_DELAY_MS` | 2000 ms | 主循环延时 |

## 🔧 自定义修改

### 修改气体报警阈值
编辑 `Core/Src/main.c`：
```c
#define GAS_THRESHOLD_MQ2   50.0f  // 修改为你需要的阈值
```

### 修改设备 ID
```c
#define DEVICE_ID "huahua"  // 修改为你的设备ID
```
