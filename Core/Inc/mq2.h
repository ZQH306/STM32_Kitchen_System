#ifndef _MQ2_H_
#define _MQ2_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* USER CODE BEGIN Private defines */
// 硬件配置
#define MQ2_RL                    10.0f   // 负载电阻 (KΩ) - MQ2模块通常为10K
#define MQ2_VCC                   5.0f    // 传感器工作电压 (V)
#define MQ2_ADC_RESOLUTION        4095.0f // ADC分辨率 (12位ADC)
#define MQ2_ADC_REF_VOLTAGE      3.3f    // ADC参考电压 (V)

// MQ2传感器曲线参数 (基于MQ2数据手册)
// 在洁净空气中 Rs/R0 ≈ 9.8
// 对于甲烷CH4: ppm = 1034.5 * (Rs/R0)^(-2.65)
#define MQ2_CURVE_A              1034.5f // 曲线参数a
#define MQ2_CURVE_B              2.65f   // 曲线参数b

// 洁净空气中Rs/R0的比值（来自MQ2数据手册）
#define MQ2_RO_RATIO             9.8f    // 洁净空气中Rs/R0比值

// 硬件配置 - 移植时只需修改这些宏定义
#define MQ2_ADC_HANDLE           hadc1   // ADC句柄
#define MQ2_ADC_CHANNEL          ADC_CHANNEL_6  // ADC通道

/* USER CODE END Private defines */

/* USER CODE BEGIN Prototypes */
void MQ2_Init(void);
float MQ2_ReadPPM(void);
uint16_t MQ2_ReadRawADC(void);
uint8_t MQ2_IsReady(void);           // 传感器是否就绪
uint8_t MQ2_GetState(void);          // 获取传感器状态
float MQ2_StabilityCheck(void);      // 稳定性检测（返回PPM波动值）
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __MQ2_H__ */
