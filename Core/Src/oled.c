#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"
#include <stdio.h>  // 用于sprintf函数

// I2C句柄声明 - 需要在i2c.c中定义
extern I2C_HandleTypeDef OLED_I2C_HANDLE;


/*
一般来说OLED显示屏厂家会有齐全的手册和案例,直接移植即可
*/
//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127



/**********************************************
// IIC Write Command
**********************************************/
void Write_IIC_Command(u8 IIC_Command)
{
	uint8_t data[2] = {0x00, IIC_Command};
	HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDRESS, data, sizeof(data), 1000);
}

/**********************************************
// IIC Write Data
**********************************************/
void Write_IIC_Data(u8 IIC_Data)
{
	uint8_t data[2] = {0x40, IIC_Data};
	HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDRESS, data, sizeof(data), 1000);
}

/**********************************************
// IIC writes a byte of data for a cmd
**********************************************/
void OLED_WR_Byte(u8 data, u8 cmd)
{
	if(cmd)
		Write_IIC_Data(data);
	else
		Write_IIC_Command(data);
}



/********************************************
// Oled Setting coordinates
********************************************/
void OLED_Set_Pos(u8 x, u8 y) 
{ 	
	OLED_WR_Byte(0xb0 + y, OLED_CMD);
	OLED_WR_Byte(((x & 0xf0) >> 4) | 0x10, OLED_CMD);
	OLED_WR_Byte((x & 0x0f), OLED_CMD); 
}

/********************************************
// Oled Open Display
********************************************/
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D, OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14, OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF, OLED_CMD);  //DISPLAY ON
}

/********************************************
// Oled Close Display
********************************************/
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D, OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10, OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE, OLED_CMD);  //DISPLAY OFF
}

/********************************************
// Oled Clear Screen(All points do not show color)
********************************************/
void OLED_Clear(void)  
{  
	u8 i, n;
	for(i = 0; i < 8; i++)  
	{  
		OLED_WR_Byte(0xb0 + i, OLED_CMD);	//设置页地址（0~7）
		OLED_WR_Byte(0x00, OLED_CMD);			//设置显示位置—列低地址
		OLED_WR_Byte(0x10, OLED_CMD);			//设置显示位置—列高地址   
		for(n = 0; n < 128; n++)
			OLED_WR_Byte(0, OLED_DATA); 
	} //更新显示
}

/********************************************
// Oled Clear Screen(All Points Display Color)
********************************************/
void OLED_On(void)  
{  
	u8 i,n;		    
	for(i = 0; i < 8; i++)  
	{  
		OLED_WR_Byte(0xb0 + i, OLED_CMD);	//设置页地址（0~7）
		OLED_WR_Byte(0x00, OLED_CMD);			//设置显示位置—列低地址
		OLED_WR_Byte(0x10, OLED_CMD);			//设置显示位置—列高地址   
		for(n = 0; n < 128; n++)
			OLED_WR_Byte(1, OLED_DATA); 
	} //更新显示
}

/********************************************
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//chr:所需要显示的字符
//size:选择字体 16/12
********************************************/ 
void OLED_ShowChar(u8 x, u8 y, u8 chr, u8 Char_Size)
{      	
	u8 c = 0, i = 0;
	
	c = chr - ' ';//得到偏移后的值			
	if(x > 128 - 1)
	{
		x = 0;
		y += 2;
	}
	if(Char_Size == 16)
	{
		OLED_Set_Pos(x, y);	
		for(i = 0; i < 8; i++)
			OLED_WR_Byte(F8X16[c * 16 + i], OLED_DATA);
		OLED_Set_Pos(x, y + 1);
		for(i = 0; i < 8; i++)
			OLED_WR_Byte(F8X16[c * 16 + i + 8], OLED_DATA);
	}
	else
	{	
		OLED_Set_Pos(x, y);
		for(i = 0; i < 6; i++)
			OLED_WR_Byte(F6x8[c][i], OLED_DATA);
	}
}

/********************************************
//求m的n次方的函数
********************************************/
u32 oled_pow(u8 m, u8 n)
{
	u32 result = 1;	 
	while(n--)result *= m;    
	return result;
}
/********************************************
//显示整数数字
//x,y :起点坐标(x:0~127 y:0~63)
//num:数值(0~4294967295);
//len :数字的位数
//size:字体大小(16/12)
********************************************/	 		  
void OLED_ShowNum(u8 x, u8 y, u32 num, u8 len, u8 size2)
{         	
	u8 t, temp;
	u8 enshow=0;
	
	for(t = 0; t < len; t++)
	{
		temp = (num / oled_pow(10, len - t - 1)) % 10;
		if(enshow == 0 && t < (len - 1))
		{
			if(temp == 0)
			{
				OLED_ShowChar(x + (size2 / 2) * t, y, ' ', size2);
				continue;
			}else 
				enshow = 1;
		}
	 	OLED_ShowChar(x + (size2 / 2) * t, y , temp + '0', size2); 
	}
}

/********************************************
//显示一个字符号串
//x,y :起点坐标(x:0~127 y:0~63)
//chr :显示的字符串
//size:字体大小(16/12)
********************************************/
void OLED_ShowString(u8 x, u8 y, u8 *chr, u8 Char_Size)
{
	u8 j = 0;
	
	while(chr[j])
	{
		OLED_ShowChar(x, y, chr[j], Char_Size);
		x += 8;
		if(x > 120)
		{
			x = 0;
			y += 2;
		}
		j++;
	}
}

/********************************************
//显示汉字温度(汉字已做好在oledfont.h文件中)
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowTEmp(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;
	
	for(i = 0; i < 4; i++)
	{
		if(i == 2) flag = 16;
		OLED_Set_Pos(x + flag, y + (i % 2));	
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(Temp[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字湿度(汉字已做好在oledfont.h文件中)
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowHUm(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;
	
	for(i = 0; i < 4; i++)
	{
		if(i == 2) flag = 16;
		OLED_Set_Pos(x + flag, y + (i % 2));	
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(Hum[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示摄氏度的符号(摄氏度的符号已经做好在oledfont.h文件中)
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowTIttle(u8 x, u8 y)
{
	u8 t, i, adder = 0;
	
	for(i = 0; i < 2; i++)
	{
		OLED_Set_Pos(x, y + (i % 2));	
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(T[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示其他汉字(需要做字模修改程序)
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowCHinese(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 8; i++)
	{
		if(i % 2 == 0) flag += 16;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(chan[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字光照
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowLight(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 4; i++)
	{
		if(i == 2) flag = 16;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(L[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字天然气
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowNaturalGas(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 6; i++)
	{
		if(i == 2) flag = 16;
		if(i == 4) flag = 32;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(NaturalGas[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字氢气
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowH2(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 4; i++)
	{
		if(i == 2) flag = 16;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(H2[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字火焰
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowFire(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 4; i++)
	{
		if(i == 2) flag = 16;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(FIRE[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字警报灯
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowLED(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 6; i++)
	{
		if(i == 2) flag = 16;
		if(i == 4) flag = 32;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(LED[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字警报器
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowBuzzer(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 6; i++)
	{
		if(i == 2) flag = 16;
		if(i == 4) flag = 32;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(BUZZER[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示汉字风扇
//x,y :起点坐标(x:0~127 y:0~63)
********************************************/
void OLED_ShowFan(u8 x, u8 y)
{
	u8 t, i, adder = 0, flag = 0;

	for(i = 0; i < 4; i++)
	{
		if(i == 2) flag = 16;
		OLED_Set_Pos(x + flag, y + (i % 2));
		for(t = 0; t < 16; t++)
		{
			OLED_WR_Byte(FAN_WORD[i][t], OLED_DATA);
			adder += 1;
		}
	}
}

/********************************************
//显示开机画面
********************************************/
void OLED_ShowOpen(void)
{
	unsigned char x, y;
	unsigned int i = 0;

	for(y = 0; y < 8; y++)
	{
		OLED_WR_Byte(0xb0 + y, 0);
		OLED_WR_Byte(0x00, 0);
		OLED_WR_Byte(0x10, 0);
		for(x = 0; x < 128; x++)
		{
			OLED_WR_Byte(OPEN[i++], 1);
		}
	}
}



//初始化SSD1306  
void OLED_Init(void)
{ 	
	HAL_Delay(800);

	OLED_WR_Byte(0xAE, OLED_CMD);//--display off
	OLED_WR_Byte(0x00, OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10, OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40, OLED_CMD);//--set start line address  
	OLED_WR_Byte(0xB0, OLED_CMD);//--set page address
	OLED_WR_Byte(0x81, OLED_CMD); // contract control
	OLED_WR_Byte(0xFF, OLED_CMD);//--128   
	OLED_WR_Byte(0xA1, OLED_CMD);//set segment remap 
	OLED_WR_Byte(0xA6, OLED_CMD);//--normal / reverse
	OLED_WR_Byte(0xA8, OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3F, OLED_CMD);//--1/32 duty
	OLED_WR_Byte(0xC8, OLED_CMD);//Com scan direction
	OLED_WR_Byte(0xD3, OLED_CMD);//-set display offset
	OLED_WR_Byte(0x00, OLED_CMD);//
	
	OLED_WR_Byte(0xD5, OLED_CMD);//set osc division
	OLED_WR_Byte(0x80, OLED_CMD);//
	
	OLED_WR_Byte(0xD8, OLED_CMD);//set area color mode off
	OLED_WR_Byte(0x05, OLED_CMD);//
	
	OLED_WR_Byte(0xD9, OLED_CMD);//Set Pre-Charge Period
	OLED_WR_Byte(0xF1, OLED_CMD);//
	
	OLED_WR_Byte(0xDA, OLED_CMD);//set com pin configuartion
	OLED_WR_Byte(0x12, OLED_CMD);//
	
	OLED_WR_Byte(0xDB, OLED_CMD);//set Vcomh
	OLED_WR_Byte(0x30, OLED_CMD);//
	
	OLED_WR_Byte(0x8D, OLED_CMD);//set charge pump enable
	OLED_WR_Byte(0x14, OLED_CMD);//
	
	OLED_WR_Byte(0xAF, OLED_CMD);//--turn on oled panel
}

/* USER CODE BEGIN 1 */
/**
 * @brief 小星星
 * @note   较长的旋律
 *          节奏：每拍约200-600ms，旋律较完整
 */
void Classic_Startup_Music(void)
{
    extern TIM_HandleTypeDef htim1;

    // 《小星星》：一闪一闪亮晶晶，满天都是小星星
    uint16_t melody[] = {
        523, 523, 784, 784, 880, 880, 784,  // 1 1 5 5 6 6 5
        698, 698, 659, 659, 587, 587, 523   // 4 4 3 3 2 2 1
    };

    // 每个音的持续时间 (ms)
    uint16_t durations[] = {
        200, 200, 200, 200, 200, 200, 400,
        200, 200, 200, 200, 200, 200, 600
    };

    uint8_t note_count = sizeof(melody) / sizeof(melody[0]);

    for(uint8_t i = 0; i < note_count; i++)
    {
        // 设置当前频率
        uint16_t period = 1000000 / melody[i] - 1;
        uint16_t compare = period / 2;

        __HAL_TIM_SetAutoreload(&htim1, period);
        __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, compare);
        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);

        // 持续音效时间
        HAL_Delay(durations[i]);

        // 关闭蜂鸣器间隔
        HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
        HAL_Delay(50);
    }
}

/* USER CODE END 1 */
/* USER CODE BEGIN 1 */
/**
 * @brief 显示页面1: 温湿度和气体传感器
 */
void Display_Page1(float temp, uint8_t humi, float mq2_ppm)
{
    char display_str[32];

    // 第一行：温度
    OLED_ShowTEmp(0, 0);
    sprintf(display_str, ": %.1f", temp);
    OLED_ShowString(32, 0, (uint8_t*)display_str, 16);
    OLED_ShowTIttle(80, 0);

    // 第二行：湿度
    OLED_ShowHUm(0, 2);
    sprintf(display_str, ": %u%%", humi);
    OLED_ShowString(32, 2, (uint8_t*)display_str, 16);

    // 第三行：甲烷
    OLED_ShowNaturalGas(0, 4);
    sprintf(display_str, ": %.1fPPM", mq2_ppm);
    OLED_ShowString(48, 4, (uint8_t*)display_str, 16);
}

/**
 * @brief 显示页面2: 警报状态
 */
void Display_Page2(uint8_t fire, uint8_t led_state, uint8_t buzzer_state, uint8_t fan_level)
{
    char display_str[32];

    // 第一行：标题
    OLED_ShowString(0, 0, (uint8_t*)"ALARM STATUS", 16);

    // 第二行：LED状态
    OLED_ShowLED(0, 2);
    sprintf(display_str, ": %s", led_state ? "ON" : "OFF");
    OLED_ShowString(48, 2, (uint8_t*)display_str, 16);

    // 第三行：风扇档位
    OLED_ShowFan(0, 4);
    sprintf(display_str, ": %s", fan_level == 0 ? "OFF" : (fan_level == 1 ? "L1" : (fan_level == 2 ? "L2" : "L3")));
    OLED_ShowString(32, 4, (uint8_t*)display_str, 16);

    // 第四行：警报器状态
    OLED_ShowBuzzer(0, 6);
    sprintf(display_str, ": %s", buzzer_state ? "ON" : "OFF");
    OLED_ShowString(48, 6, (uint8_t*)display_str, 16);
}

/**
 * @brief 显示页面3: 光照强度
 */
void Display_Page3(uint32_t light)
{
    char display_str[32];

    // 第一行：标题
    OLED_ShowString(0, 0, (uint8_t*)"ENV STATUS", 16);

    // 第二行：光照强度
    OLED_ShowLight(0, 2);
    sprintf(display_str, ": %u Lux", light);
    OLED_ShowString(32, 2, (uint8_t*)display_str, 16);
}

/**
 * @brief 自动滑动显示系统 - 在OLED上循环显示所有传感器数据
 */
void Display_Sensors(float temp, uint8_t humi, float mq2_ppm,
                      uint32_t light, uint8_t fire, uint8_t led_state, uint8_t buzzer_state, uint8_t fan_level)
{
    static uint8_t current_page = 0;
    static uint8_t page_counter = 0; // 页面计数器，控制切换频率
    static uint8_t need_clear = 1; // 需要清屏标志

    // 需要清屏时执行清屏
    if(need_clear)
    {
        OLED_Clear();
        need_clear = 0;
    }

    // 当前页面显示时间后切换到下一页
    switch(current_page)
    {
        case 0: // 页面1: 温湿度和气体传感器
            Display_Page1(temp, humi, mq2_ppm);
            break;
        case 1: // 页面2: 警报状态
            Display_Page2(fire, led_state, buzzer_state, fan_level);
            break;
        case 2: // 页面3: 光照强度
            Display_Page3(light);
            break;
    }

    // 页面计数，每个页面显示3次（6秒）后切换
    page_counter++;
    if(page_counter >= 3)
    {
        page_counter = 0;
        need_clear = 1; // 下一轮循环需要清屏

        // 切换到下一页
        current_page++;
        if(current_page > 2) current_page = 0;
    }
}
/* USER CODE END 1 */



