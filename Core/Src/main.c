/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "dht11.h"
#include "light.h"
#include "mq2.h"
#include "oled.h"
#include "led_buzzer.h"
#include "gpio.h"
#include "fan.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define DEVICE_ID "huahua"        // 设备唯一ID
#define GAS_THRESHOLD_MQ2   50.0f    // MQ2甲烷浓度报警阈值(PPM)
#define LOOP_DELAY_MS      2000      // 主循环延时时间(ms)
#define UART_TIMEOUT_DATA  1000      // 数据传输超时时间(ms)
#define UART_TIMEOUT_STATE 100       // 状态传输超时时间(ms)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();  // 初始化TIM4（用于风扇PWM）
  /* USER CODE BEGIN 2 */
    HAL_ADCEx_Calibration_Start(&hadc1);
    ADC_DMA_Init(); // 初始化ADC DMA
    DHT11_Init();

    // 初始化气体传感器
    MQ2_Init();
    Light_Init();
    Fan_Init();  // 初始化风扇驱动
    OLED_Init();
    OLED_Clear();

    // 关闭LED（确保开机时不亮）
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

    // 显示开机画面10秒
//    OLED_ShowOpen();
//    Classic_Startup_Music();  // 播放小星星开机音乐
//    HAL_Delay(10000);
//    OLED_Clear();


    // 串口接收中断在usart.c中初始化
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

    float temp = 0;
    uint8_t humi = 0;
    uint32_t light = 0;
    char upload_data[100];
    float mq2_ppm = 0.0f;
    uint8_t fire_state = 0;
    uint8_t buzzer_state = 0;
    uint8_t led_state = 0;
    // static uint8_t last_fire_state = 0;  // 记录上一次的火灾状态（暂时禁用）

  while (1)
  {
        uint16_t len;  // 串口传输长度

        // 采集温湿度（最多重试3次）
        uint8_t dht11_retry = 0;
        uint8_t dht11_result = 1;
        for(dht11_retry = 0; dht11_retry < 3; dht11_retry++)
        {
            dht11_result = DHT11_Get(&temp, &humi);
            if(dht11_result == 0) break;
            HAL_Delay(100);
        }
        if(dht11_result == 0)
        {
            len = sprintf(upload_data, "%s/sensor/dht11 %.1f_%u\n", DEVICE_ID, temp, humi);
            HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);
        }
        else
        {
            len = sprintf(upload_data, "%s/sensor/dht11 ERR_ERR\n", DEVICE_ID);
            HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);
        }

        // 采集光照强度,上传云端
        light = Light_Get_Lux();
        len = sprintf(upload_data, "%s/sensor/light %u\n", DEVICE_ID, light);
        HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);

        // 调试：发送光照原始ADC值
        uint16_t light_adc = Light_Get();
        len = sprintf(upload_data, "%s/debug/light_adc %u\n", DEVICE_ID, light_adc);
        HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);

        // 采集mq2,上传云端
        mq2_ppm = MQ2_ReadPPM();
        len = sprintf(upload_data, "%s/sensor/mq2 %.2f\n", DEVICE_ID, mq2_ppm);
        HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);

        // 调试：发送MQ2原始ADC值
        uint16_t mq2_adc = MQ2_ReadRawADC();
        len = sprintf(upload_data, "%s/debug/mq2_adc %u\n", DEVICE_ID, mq2_adc);
        HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);

        // 上传风扇状态到云端
        uint8_t fan_level = Fan_Get_Level();
        len = sprintf(upload_data, "%s/state/fan %u\n", DEVICE_ID, fan_level);
        HAL_UART_Transmit(&huart2, (uint8_t*)upload_data, len, UART_TIMEOUT_DATA);

        // 获取火焰状态
        fire_state = !HAL_GPIO_ReadPin(FIRE_GPIO_Port, FIRE_Pin);

        // 警报检测和控制逻辑：有火焰或气体超标时触发警报
        uint8_t alarm_trigger = fire_state || (mq2_ppm > GAS_THRESHOLD_MQ2);
        uint8_t manual_mode = Get_Key_Override();  // 获取手动控制模式状态

        if(alarm_trigger)
        {
            // 检测到火灾或气体超标，强制触发警报（安全优先）
            // 退出手动控制模式，进入自动模式
            if(manual_mode)
            {
                Clear_Key_Override();
            }
            Trigger_Alarm();
            // 启动风扇的警报模式，自动满速
            Fan_Set_Alarm_Mode(1);
        }
        else
        {
            // 没有危险情况
            if(!manual_mode)  // 如果不是手动模式，自动关闭警报
            {
                Stop_Alarm();
                // 关闭风扇的警报模式
                Fan_Set_Alarm_Mode(0);
            }
            // 如果是手动模式，保持按键或网页控制的状态，不做任何操作
        }

        // 更新警报音频（如果警报激活）
        Update_Alarm_Sound();

        // 更新风扇状态（检查警报状态，自动切换风扇模式）
        Fan_Update();

        // KEY2长按检测
        Key2_LongPress_Check();

        // 获取LED状态用于上传和显示
        led_state = HAL_GPIO_ReadPin(LED1_GPIO_Port, LED1_Pin);
        buzzer_state = Buzzer_Is_Active();  // 获取实际蜂鸣器状态

        // 在OLED上显示传感器数据（自动滑动显示3个页面）
        Display_Sensors(temp, humi, mq2_ppm, light, fire_state, led_state, buzzer_state, fan_level);

        // 上传各种设备工作的状态
        len = sprintf(upload_data,"%s/state/buzzer %u\n",DEVICE_ID,buzzer_state);
        HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,len,UART_TIMEOUT_STATE);

        len = sprintf(upload_data,"%s/state/led %u\n",DEVICE_ID,led_state);
        HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,len,UART_TIMEOUT_STATE);

        len = sprintf(upload_data,"%s/state/fire %u\n",DEVICE_ID,fire_state);
        HAL_UART_Transmit(&huart2,(uint8_t*)upload_data,len,UART_TIMEOUT_STATE);

        // 更新火焰状态记录（暂时禁用）
        // last_fire_state = fire_state;

        HAL_Delay(LOOP_DELAY_MS);  // 主循环延时

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
