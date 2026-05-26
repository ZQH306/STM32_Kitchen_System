/**
  ******************************************************************************
  * @file    led_buzzer.h
  * @brief   LED和蜂鸣器控制函数头文件
  ******************************************************************************
  */

#ifndef __LED_BUZZER_H
#define __LED_BUZZER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* 蜂鸣器控制函数 */
void Buzzer_On(void);
void Buzzer_Off(void);
uint8_t Buzzer_Is_Active(void);

/* 警报控制函数 */
void Trigger_Alarm(void);      // 启动警报（点亮LED）
void Stop_Alarm(void);         // 停止警报（关闭LED和蜂鸣器）
void Update_Alarm_Sound(void); // 更新警报音频（在主循环中调用）
void Set_Alarm_Active(uint8_t state);  // 设置警报激活状态
uint8_t Get_Alarm_Active(void);  // 获取警报激活状态

/* LED控制函数 */
void LED1_On(void);
void LED1_Off(void);
void LED1_Toggle(void);
void LED2_On(void);
void LED2_Off(void);
void LED2_Toggle(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_BUZZER_H */
