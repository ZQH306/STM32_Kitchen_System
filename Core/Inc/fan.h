/**
  ******************************************************************************
  * @file    fan.h
  * @brief   风扇驱动控制函数头文件
  ******************************************************************************
  */

#ifndef __FAN_H
#define __FAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 风扇档位定义 */
typedef enum {
    FAN_OFF = 0,      // 关闭
    FAN_LEVEL_1 = 1,  // 低速档
    FAN_LEVEL_2 = 2,  // 中速档
    FAN_LEVEL_3 = 3   // 高速档（满速）
} FanLevel_t;

/* 风扇方向定义 */
/* 风扇控制函数 */
void Fan_Init(void);                    // 初始化风扇驱动
void Fan_On(void);                      // 开启风扇
void Fan_Off(void);                     // 关闭风扇
void Fan_Set_Level(FanLevel_t level);   // 设置风扇档位
FanLevel_t Fan_Get_Level(void);         // 获取当前风扇档位
void Fan_Next_Level(void);              // 切换到下一档
void Fan_Update(void);                  // 更新风扇状态（在主循环中调用）
uint8_t Fan_Is_On(void);                // 检查风扇是否开启

/* 警报模式控制 */
void Fan_Set_Alarm_Mode(uint8_t alarm_mode);  // 设置警报模式
uint8_t Fan_Get_Alarm_Mode(void);             // 获取警报模式状态

#ifdef __cplusplus
}
#endif

#endif /* __FAN_H */
