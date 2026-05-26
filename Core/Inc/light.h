#ifndef _LIGHT_H_
#define _LIGHT_H_

#include <stdint.h>
#include <stm32f1xx_hal.h>

/* USER CODE BEGIN Private defines */
// ADC配置
#define LIGHT_ADC_CHANNEL         ADC_CHANNEL_4  // ADC通道
#define LIGHT_ADC_RESOLUTION        4095.0f      // ADC分辨率 (12位)
#define LIGHT_ADC_REF_VOLTAGE      3300.0f      // ADC参考电压 (mV)

// 光敏电阻模块配置
// 实际电路：光敏电阻模块（如MH-Sensor-Series）
// 光照强 → 光敏电阻阻值小 → 电压升高 → ADC值高
// 光照弱 → 光敏电阻阻值大 → 电压降低 → ADC值低

// 传感器配置
#define LIGHT_MAX_LUX             100000.0f      // 最大lux值（晴天）
#define LIGHT_MIN_LUX             0.001f         // 最小lux值（黑夜）

// 滤波配置
#define LIGHT_FILTER_SIZE         5              // 滤波窗口大小

// 传感器校准参数
// 通过ADC值直接映射到lux值
/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
// 基本读取函数
uint32_t Light_Get(void);                    // 获取原始ADC值
uint32_t Light_Get_Lux(void);                // 获取lux值

// 高级功能
float Light_ADC_to_Lux(float adc_value);     // ADC转lux (float版本)
float Light_Get_Lux_Float(void);             // 获取lux值 (float版本)

// 校准功能
void Light_Calibrate_Dark(void);             // 校准黑暗环境
void Light_Calibrate_Light(void);            // 校准光照环境
uint8_t Light_Is_Calibrated(void);           // 检查是否已校准

// 滤波功能
void Light_Enable_Filter(uint8_t enable);    // 启用/禁用滤波
float Light_Get_Filtered_Lux(void);          // 获取滤波后的lux值

// 硬件控制
void Light_Init(void);                       // 初始化光照传感器
/* USER CODE END Prototypes */

#endif
