#include "gpio.h"
#include "delay.h"
#include "dht11.h"



// DHT11 初始化
void DHT11_Init(void)
{
	__HAL_RCC_GPIOC_CLK_ENABLE();
}



/*

@Brief 通过 DHT11 传感器采集一次温湿度数据

@Params
	temp 输出参数，接收温度数据，如果对温度数据不感兴趣，本参数可以传 NULL
	humi 输出参数，接收湿度数据，如果对湿度数据不感兴趣，本参数可以传 NULL

@Return
	采集成功返回 0，失败返回非零错误码，具体如下：
		1 表示 DHT11 传感器已损坏或连接异常；
		2 表示校验失败，通信可能受到干扰；

@Author
	itmojun

@Email
	dj@itmojun.com
	
@Date
	2026-3-17 15:14
	
@Remark
	本函数依赖微秒级延时函数 Delay_US，移植时需要自己实现

@Example

float t, h;

if(DHT11_Get(&t, &h) == 0)
{
	// 成功处理
	// ...
}
else
{
	// 失败处理
	// ...
}

*/

uint8_t DHT11_Get(float* temp, uint8_t* humi)
{
	GPIO_InitTypeDef conf = {0};
	uint8_t data[5];  // 用于存放温度和湿度数据
	uint8_t i, j;
	
	conf.Pin = DHT11_PIN;
	conf.Speed = GPIO_SPEED_FREQ_LOW;
	
	// 将 PC10 设置为开漏输出
	conf.Mode = GPIO_MODE_AF_OD;
	HAL_GPIO_Init(DHT11_GPIO_PORT, &conf);
	
	// MCU 发出开始信号
	HAL_GPIO_WritePin(DHT11_GPIO_PORT, DHT11_PIN, GPIO_PIN_RESET);
	HAL_Delay(20);
	
	// 切换为输入模式，即释放通信总线，此时在上拉电阻的作用下，总线上的电平状态为高电平
	conf.Mode = GPIO_MODE_INPUT;
	HAL_GPIO_Init(DHT11_GPIO_PORT, &conf);
	
	Delay_US(53);
	
	// 读取 DHT11 响应信号，如果是高电平说明 DHT11 传感器发生故障或连接异常，如果是低电平就说明 DHT11 正常
	if(HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN))
	{
		// 如果 DHT11 已损坏或连接异常
		return 1;
	}

	// 如果 DHT11 传感器响应正常
	
	// 等待 DHT11 响应信号（低电平）结束
	while(!HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));
	
	Delay_US(120);
	
	
	// 开始逐位接收温度和湿度数据（共 40bit，高位先出）
	for(i = 0; i < 5; i++)
	{
		data[i] = 0;
		
		for(j = 0; j < 8; j++)
		{
			// 接收 1bit 数据 
			// 等待前导低电平信号结束
			while(!HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN));
			
			Delay_US(50);
			
			data[i] <<= 1;
			
			// 判断是 1 还是 0
			if(HAL_GPIO_ReadPin(DHT11_GPIO_PORT, DHT11_PIN))
			{
				// 1
				data[i] += 1;
				Delay_US(50);
			}
		}
	}
	
	// 校验数据是否有效（通过比对校验和实现）
	if(data[0] + data[1] + data[2] + data[3] != data[4])
	{
		// 校验失败，通信受到干扰，数据无效
		return 2;
	}

	// 校验成功，返回数据
	if(temp != NULL) *temp = data[2] + data[3] / 10.0;
	
	if(humi != NULL) *humi = data[0];
	
	return 0; // 采集数据成功
}

