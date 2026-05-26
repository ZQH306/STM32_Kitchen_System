/**
  ******************************************************************************
  * @file    led_buzzer.c
  * @brief   LED和蜂鸣器控制函数
  ******************************************************************************
  */

#include "led_buzzer.h"

// TIM1句柄声明 - 需要在tim.c中定义
extern TIM_HandleTypeDef htim1;

// 警报音频播放状态
static uint8_t alarm_active = 0;
// 防空警报频率范围：从500Hz到2000Hz
#define ALARM_FREQ_MIN 500
#define ALARM_FREQ_MAX 2000
#define ALARM_FREQ_STEP 50  // 每次频率变化步长（加快节奏）

static uint16_t current_freq = ALARM_FREQ_MIN;  // 当前频率
static int8_t freq_direction = 1;  // 频率变化方向：1=上升，-1=下降
static uint32_t last_buzzer_time = 0;
static uint8_t buzzer_state = 0;  // 0: 关闭, 1: 开启

/**
 * @brief 开启蜂鸣器（PWM模式）
 */
void Buzzer_On(void)
{
    // 设置蜂鸣器频率为1000Hz
    __HAL_TIM_SetAutoreload(&htim1, 1000000 / 1000 - 1);
    __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, (1000000 / 1000 - 1) / 2);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
}

/**
 * @brief 关闭蜂鸣器
 */
void Buzzer_Off(void)
{
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
}

/**
 * @brief 检查蜂鸣器是否正在工作
 * @retval 1: 工作中, 0: 已停止
 */
uint8_t Buzzer_Is_Active(void)
{
    return (htim1.Instance->CR1 & TIM_CR1_CEN) ? 1 : 0;
}

/**
 * @brief 触发警报：点亮LED并启动警报音频播放
 */
void Trigger_Alarm(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);     // LED1高电平驱动
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);   // LED2低电平驱动
    alarm_active = 1;  // 启动警报状态

    // 重置防空警报参数
    current_freq = ALARM_FREQ_MIN;
    freq_direction = 1;
    buzzer_state = 0;
    last_buzzer_time = 0;
}

/**
 * @brief 停止警报：关闭LED和蜂鸣器
 */
void Stop_Alarm(void)
{
    alarm_active = 0;  // 停止警报状态
    buzzer_state = 0;  // 重置蜂鸣器状态
    Buzzer_Off();  // 关闭蜂鸣器
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);   // 关闭LED1
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);     // 关闭LED2（低电平驱动）
}

/**
 * @brief 设置警报激活状态
 * @param  state: 1=激活，0=停止
 */
void Set_Alarm_Active(uint8_t state)
{
    alarm_active = state;
}

/**
 * @brief 获取警报激活状态
 * @retval 1=激活，0=停止
 */
uint8_t Get_Alarm_Active(void)
{
    return alarm_active;
}

/**
 * @brief 更新警报音频（在主循环中调用）
 * @note   连续变化的防空警报音效
 */
void Update_Alarm_Sound(void)
{
    if(!alarm_active)
    {
        // 警报未激活，关闭蜂鸣器
        if(buzzer_state)
        {
            Buzzer_Off();
            buzzer_state = 0;
        }
        return;
    }

    uint32_t current_time = HAL_GetTick();

    // 每10ms更新一次频率，实现快速变化的防空警报效果
    if((current_time - last_buzzer_time) >= 10)
    {
        // 更新频率
        current_freq += freq_direction * ALARM_FREQ_STEP;

        // 到达边界时反转方向
        if(current_freq >= ALARM_FREQ_MAX)
        {
            current_freq = ALARM_FREQ_MAX;
            freq_direction = -1;  // 开始下降
        }
        else if(current_freq <= ALARM_FREQ_MIN)
        {
            current_freq = ALARM_FREQ_MIN;
            freq_direction = 1;  // 开始上升
        }

        // 设置新的频率
        uint16_t period = 1000000 / current_freq - 1;
        uint16_t compare = period / 2;

        __HAL_TIM_SetAutoreload(&htim1, period);
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, compare);

        // 确保蜂鸣器是开启的
        if(!buzzer_state)
        {
            HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
            buzzer_state = 1;
        }

        last_buzzer_time = current_time;
    }
}

/**
 * @brief 点亮LED1
 */
void LED1_On(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}

/**
 * @brief 关闭LED1
 */
void LED1_Off(void)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 点亮LED2
 */
void LED2_On(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);  // LED2低电平驱动
}

/**
 * @brief 关闭LED2
 */
void LED2_Off(void)
{
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);   // 关闭LED2（低电平驱动）
}

/**
 * @brief 切换LED1状态
 */
void LED1_Toggle(void)
{
    HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
}

/**
 * @brief 切换LED2状态
 */
void LED2_Toggle(void)
{
    HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
}
