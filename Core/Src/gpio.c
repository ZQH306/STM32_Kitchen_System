/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   This file provides code for the configuration
  *          of all used GPIO pins.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "gpio.h"

/* USER CODE BEGIN 0 */
#include "main.h"
#include "led_buzzer.h"  // LED和蜂鸣器控制
#include "fan.h"         // 风扇控制
#include <string.h> // 用于strlen

/* USER CODE END 0 */

/*----------------------------------------------------------------------------*/
/* Configure GPIO                                                             */
/*----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
 
  /*Configure GPIO pin : DHT11_Pin */
  GPIO_InitStruct.Pin = DHT11_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : FIRE_Pin */
  GPIO_InitStruct.Pin = FIRE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(FIRE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : KEY2_Pin */
  GPIO_InitStruct.Pin = KEY2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(KEY2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LED1_Pin LED2_Pin */
  GPIO_InitStruct.Pin = LED1_Pin|LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 (风扇方向控制 - 固定向外) */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Set fan direction to outward (CCW) */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

  /*Configure GPIO pin : KEY1_Pin */
  GPIO_InitStruct.Pin = KEY1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(KEY1_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

}

/* USER CODE BEGIN 2 */
// 按键状态定义
typedef enum {
    KEY_PRESSED,   // 按下
    KEY_RELEASED   // 松开
} KeyStatus_t;

// KEY2长按状态机定义
typedef enum {
    KEY2_STATE_IDLE = 0,     // 空闲状态
    KEY2_STATE_PRESSED = 1,  // 按键按下
    KEY2_STATE_LONG = 2      // 已识别为长按
} Key2State_t;

static KeyStatus_t key1_status = KEY_RELEASED;
static KeyStatus_t key2_status = KEY_RELEASED;
static Key2State_t key2_state = KEY2_STATE_IDLE;  // KEY2长按状态机

static uint32_t key1_time = 0;
static uint32_t key2_time = 0;

// 按键控制的LED和蜂鸣器状态
static uint8_t key_control_state = 0;  // 0: 关, 1: 开
static uint8_t key_override = 0;      // 按键覆盖标志：1=按键控制优先，0=自动检测优先

/**
 * @brief  GPIO外部中断回调函数 - 按键消抖处理
 * @param  GPIO_Pin: 触发中断的GPIO引脚
 * @retval None
 * @note   KEY1: 上拉电阻(下降沿按下，上升沿松开)
 *          KEY2: 下拉电阻(上升沿按下，下降沿松开)
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == KEY1_Pin)
    {
        // 处理引脚 KEY1 的中断 (上拉电阻)
        if(HAL_GPIO_ReadPin(KEY1_GPIO_Port, KEY1_Pin))
        {
            //上升沿(松开)
            if(key1_status == KEY_PRESSED)
            {
                if(HAL_GetTick() - key1_time > 10)
                {
                    key1_status = KEY_RELEASED;
                    key1_time = HAL_GetTick();
                }
            }
        }
        else
        {
            //下降沿(按下)
            if(key1_status == KEY_RELEASED)
            {
                if(HAL_GetTick() - key1_time > 10)
                {
                    key1_status = KEY_PRESSED;
                    key1_time = HAL_GetTick();

                    // 执行动作 - 控制LED和蜂鸣器开关
                    key_control_state = !key_control_state;
                    key_override = 1;  // 设置按键覆盖标志

                    if(key_control_state)
                    {
                        // 打开LED和启动防空警报循环（但不触发警报模式，避免启动风扇）
                        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);     // LED1高电平驱动
                        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);   // LED2低电平驱动
                        Set_Alarm_Active(1);  // 启动警报状态，让Update_Alarm_Sound()播放防空警报
                        // 强制关闭风扇的警报模式，防止按键一启动风扇
                        Fan_Set_Alarm_Mode(0);
                    }
                    else
                    {
                        // 关闭LED和蜂鸣器
                        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);   // 关闭LED1
                        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);     // 关闭LED2
                        Buzzer_Off();  // 直接关闭蜂鸣器
                        Set_Alarm_Active(0);  // 清除警报激活状态
                        // 不清除key_override，保持手动模式
                    }
                }
            }
        }
    }
    else if (GPIO_Pin == KEY2_Pin)
    {
        // 处理引脚 KEY2 的中断 (下拉电阻)
        if(HAL_GPIO_ReadPin(KEY2_GPIO_Port, KEY2_Pin))
        {
            //上升沿(按下)
            if(key2_status == KEY_RELEASED)
            {
                if(HAL_GetTick() - key2_time > 10)
                {
                    key2_status = KEY_PRESSED;
                    key2_time = HAL_GetTick();
                    key2_state = KEY2_STATE_PRESSED;  // 进入按下状态
                }
            }
        }
        else
        {
            //下降沿(松开)
            if(key2_status == KEY_PRESSED)
            {
                if(HAL_GetTick() - key2_time > 10)
                {
                    key2_status = KEY_RELEASED;
                    key2_time = HAL_GetTick();
                    
                    // 如果是短按（没有进入长按状态），切换档位
                    if(key2_state == KEY2_STATE_PRESSED && !Fan_Get_Alarm_Mode())
                    {
                        Fan_Next_Level();  // 短按：切换档位
                    }
                    key2_state = KEY2_STATE_IDLE;  // 重置状态机
                }
            }
        }
    }
}

/**
 * @brief  KEY2长按检测（在主循环中调用）
 * @retval None
 */
void Key2_LongPress_Check(void)
{
    if(key2_state == KEY2_STATE_PRESSED)
    {
        uint32_t elapsed = HAL_GetTick() - key2_time;
        
        // 长按判定时间：500ms
        if(elapsed >= 500 && !Fan_Get_Alarm_Mode())
        {
            // 长按功能已移除（原切换风扇方向）
            key2_state = KEY2_STATE_LONG;  // 防止重复触发
        }
    }
}

/* USER CODE END 2 */

/* USER CODE BEGIN 3 */

/**
 * @brief  获取按键覆盖状态
 * @retval 1: 手动控制模式，0: 自动检测模式
 */
uint8_t Get_Key_Override(void)
{
    return key_override;
}

/**
 * @brief  设置按键覆盖状态（进入手动控制模式）
 * @retval None
 */
void Set_Key_Override(void)
{
    key_override = 1;
}

/**
 * @brief  清除按键覆盖状态（退出手动控制模式）
 * @retval None
 */
void Clear_Key_Override(void)
{
    key_override = 0;
}

/* USER CODE END 3 */
