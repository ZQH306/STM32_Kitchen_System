#ifndef _DHT11_H_
#define _DHT11_H_


#include <stdint.h>

// 在不同 STM32 板子上移植只需要修改如下两个宏
#define DHT11_GPIO_PORT  GPIOC
#define DHT11_PIN        GPIO_PIN_13



void DHT11_Init(void);
uint8_t DHT11_Get(float* temp, uint8_t* humi);



#endif
