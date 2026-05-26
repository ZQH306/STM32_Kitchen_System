#ifndef __OLED_H_
#define __OLED_H_
/*
用I2C方式驱动OLED显示屏
移植性最佳，只需修改下面的硬件配置宏定义
oled.c 和 oled.h 是用来操作开发板上有关OLED的实验
*/
#include "stm32f1xx_hal.h"

// 硬件配置 - 移植时只需修改这些宏定义
#define OLED_I2C_HANDLE           hi2c1   // I2C句柄
#define OLED_I2C_ADDRESS          0x78    // OLED设备地址

typedef unsigned char u8;//通过typedef定义u8类型
typedef unsigned int u32;//通过typedef定义u32类型
     
#define OLED_CMD  0	//写命令
#define OLED_DATA 1	//写数据


//OLED控制用函数
extern void OLED_WR_Byte(u8 data, u8 cmd);//IIC为CMD写入一个字节的数据

extern void OLED_Display_On(void);//OLED开启显示

extern void OLED_Display_Off(void);//OLED关闭显示		   		    

extern void OLED_Init(void);//初始化OLED

extern void OLED_Clear(void);//OLED清屏

extern void OLED_DrawPoint(u8 x, u8 y, u8 t);//画一个点

extern void OLED_Fill(u8 x1, u8 y1, u8 x2, u8 y2, u8 dot);//画一个矩形

extern void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size);//显示一个字符

extern void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size);//显示一个整数

extern void OLED_ShowString(u8 x, u8 y, u8 *p, u8 Char_Size);//显示一个字符串

extern void OLED_Set_Pos(u8 x, u8 y);//设置像素点的坐标

extern void OLED_ShowTEmp(u8 x, u8 y);//显示汉字温度

extern void OLED_ShowHUm(u8 x, u8 y);//显示汉字湿度

extern void OLED_ShowTIttle(u8 x, u8 y);//显示摄氏度的符号

extern void OLED_DrawBMP(u8 x0, u8 y0, u8 x1, u8 y1, u8 BMP[]);//显示图片

extern void fill_picture(u8 fill_Data);//填充图片

extern void picture_1(void);

extern void OLED_ShowLight(u8 x, u8 y);//显示汉字光照
extern void OLED_ShowNaturalGas(u8 x, u8 y);//显示汉字天然气
extern void OLED_ShowH2(u8 x, u8 y);//显示汉字氢气
extern void OLED_ShowFire(u8 x, u8 y);//显示汉字火焰
extern void OLED_ShowLED(u8 x, u8 y);//显示汉字警报灯
extern void OLED_ShowBuzzer(u8 x, u8 y);//显示汉字警报器
extern void OLED_ShowFan(u8 x, u8 y);//显示汉字风扇
extern void OLED_ShowOpen(void);//显示开机画面
extern void Classic_Startup_Music(void);//小星星开机音乐

// 应用层显示函数
extern void Display_Page1(float temp, uint8_t humi, float mq2_ppm);
extern void Display_Page2(uint8_t fire, uint8_t led_state, uint8_t buzzer_state, uint8_t fan_level);
extern void Display_Page3(uint32_t light);
extern void Display_Sensors(float temp, uint8_t humi, float mq2_ppm,
                          uint32_t light, uint8_t fire, uint8_t led_state, uint8_t buzzer_state, uint8_t fan_level);

#endif  
	 



