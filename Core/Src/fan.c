/**
  ******************************************************************************
  * @file    fan.c
  * @brief   风扇驱动控制函数实现
   * @note    支持风扇档位控制以及警报模式自动控制
  ******************************************************************************
  */

#include "fan.h"
#include "led_buzzer.h"

/* TIM4句柄声明 - 需要在tim.c中定义，用于风扇PWM控制 */
extern TIM_HandleTypeDef htim4;

/* 风扇状态变量 */
static FanLevel_t fan_level = FAN_OFF;      /* 当前风扇档位 */
static uint8_t fan_alarm_mode = 0;          /* 警报模式标志：1=警报模式，0=正常模式 */
#define FAN_PWM_ARR 999
static const uint16_t fan_duty_cycles[] = {
    0,      /* FAN_OFF: 0% 占空比，风扇停止 */
    330,    /* FAN_LEVEL_1: 33% 占空比，低速档 */
    660,    /* FAN_LEVEL_2: 66% 占空比，中速档 */
    999     /* FAN_LEVEL_3: 100% 占空比，高速档（满速） */
};

/**
  * @brief  初始化风扇驱动
  * @retval None
  * @note   初始化风扇控制引脚和状态变量，确保风扇初始状态为关闭
  */
void Fan_Init(void)
{
    fan_level = FAN_OFF;           /* 初始化为关闭状态 */
    fan_alarm_mode = 0;            /* 初始化为正常模式 */
    
    /* 风扇方向固定为向外排风（逆时针），设置方向控制引脚 PA5 = 1 */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    
    /* 
     * 启动 PWM 设为 100% 占空比（H桥反转 = 0% 功率 = 风扇关闭）。
     * 绝不能 HAL_TIM_PWM_Stop —— PB8 LOW = 0% PWM → H桥反转 = 100% 功率 = 全速！
     */
    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, FAN_PWM_ARR);
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    
    fan_level = FAN_OFF;
}

/**
  * @brief  开启风扇（使用当前档位）
  * @retval None
  * @note   如果当前是关闭状态，默认开启为低速档
  */
void Fan_On(void)
{
    if(fan_level == FAN_OFF)
    {
        fan_level = FAN_LEVEL_1;  /* 默认开启为低速档 */
    }

  uint16_t duty = fan_duty_cycles[fan_level];
  /* PWM占空比需要反转（H桥驱动特性），与 Fan_Set_Level 保持一致 */
  duty = FAN_PWM_ARR - duty;
  __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, duty);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
}

/**
  * @brief  关闭风扇
  * @retval None
  * @note   在警报模式下不允许关闭风扇，确保安全优先
  */
void Fan_Off(void)
{
    /* 
     * 不能 HAL_TIM_PWM_Stop！PB8 停止输出后为 LOW，
     * LOW = 0% PWM → H桥反转 = 100% 功率 = 风扇全速！
     * 正确做法：保持 PWM 运行，设为 100% 占空比（H桥反转 = 0% 功率 = 关闭）
     */
    if(fan_alarm_mode)
    {
        fan_level = FAN_OFF;
        return;
    }

    __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, FAN_PWM_ARR);
    /* PWM 已经在运行，只需改占空比即可 */
    fan_level = FAN_OFF;
}

/**
  * @brief  设置风扇档位
  * @param  level: 风扇档位（FAN_OFF, FAN_LEVEL_1, FAN_LEVEL_2, FAN_LEVEL_3）
  * @retval None
  * @note   在警报模式下，强制设置为满速，忽略用户设置
  *         由于H桥驱动特性，PWM占空比需要反转（999 - duty）
  */
void Fan_Set_Level(FanLevel_t level)
{
    /* 在警报模式下，强制设置为满速，忽略按键控制 */
    if(fan_alarm_mode)
    {
        fan_level = FAN_LEVEL_3;
        uint16_t duty = FAN_PWM_ARR - fan_duty_cycles[FAN_LEVEL_3];
        __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, duty);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
        return;
    }

    fan_level = level;

    if(level == FAN_OFF)
    {
        Fan_Off();
    }
    else
    {
        uint16_t duty = fan_duty_cycles[level];
        /* PWM占空比需要反转（H桥驱动特性） */
        duty = FAN_PWM_ARR - duty;
        __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, duty);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
    }
}

/**
  * @brief  获取当前风扇档位
  * @retval 当前风扇档位（FanLevel_t）
  */
FanLevel_t Fan_Get_Level(void)
{
    return fan_level;
}

/**
  * @brief  切换到下一档（循环切换：OFF -> LEVEL1 -> LEVEL2 -> LEVEL3 -> OFF）
  * @retval None
  * @note   在警报模式下，强制保持满速，忽略切换操作
  */
void Fan_Next_Level(void)
{
    /* 在警报模式下，不允许切换档位，强制保持满速 */
    if(fan_alarm_mode)
    {
        fan_level = FAN_LEVEL_3;
        uint16_t duty = FAN_PWM_ARR - fan_duty_cycles[FAN_LEVEL_3];
        __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, duty);
        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
        return;
    }

    /* 循环切换档位 */
    if(fan_level >= FAN_LEVEL_3)
    {
        Fan_Off();
    }
    else
    {
        FanLevel_t next_level = (FanLevel_t)(fan_level + 1);
        Fan_Set_Level(next_level);
    }
}

/**
  * @brief  更新风扇状态（在主循环中调用）
  * @retval None
  * @note   此函数预留用于未来扩展，当前逻辑已在Fan_Set_Alarm_Mode中处理
  */
void Fan_Update(void)
{
    /* 当前警报逻辑已在Fan_Set_Alarm_Mode中处理 */
    /* 此函数预留用于未来的周期性任务扩展 */
}

/**
  * @brief  检查风扇是否开启
  * @retval 1: 风扇开启, 0: 风扇关闭
  */
uint8_t Fan_Is_On(void)
{
    return (fan_level != FAN_OFF);
}

/**
  * @brief  设置警报模式
  * @param  alarm_mode: 1=进入警报模式，0=退出警报模式
  * @retval None
  * @note   警报模式下风扇强制向外排风（逆时针）并满速运行
  *         退出警报模式时自动关闭风扇
  */
void Fan_Set_Alarm_Mode(uint8_t alarm_mode)
{
    /* 如果状态未变化，无需处理 */
    if(fan_alarm_mode == alarm_mode)
    {
        return;
    }

    fan_alarm_mode = alarm_mode;

    if(fan_alarm_mode)
    {
        /* 进入警报模式：强制向外排风并满速运行 */
        /* 火灾或甲烷超标时，向外排风可以将烟雾和有害气体排出室外 */
        Fan_Set_Level(FAN_LEVEL_3);      /* 设置为最高档位 */
    }
    else
    {
        /* 退出警报模式：关闭风扇，恢复正常控制 */
        Fan_Off();
    }
}

/**
  * @brief  获取警报模式状态
  * @retval 1: 警报模式，0: 正常模式
  */
uint8_t Fan_Get_Alarm_Mode(void)
{
    return fan_alarm_mode;
}
