#include "mq2.h"
#include "adc.h"
#include <math.h>

/**
将ADC值转换为电压值
adc_value: ADC原始读数值
电压值 (V)
*/
static float MQ2_GetVoltage(uint16_t adc_value)
{
    return (float)adc_value * (MQ2_ADC_REF_VOLTAGE / MQ2_ADC_RESOLUTION);
}

/**
计算传感器电阻 Rs
根据分压原则：RS = (Vcc - Vrl) * RL / Vrl
voltage: 传感器输出电压 (V)
传感器电阻值 (KΩ)
*/
static float MQ2_GetResistance(float voltage)
{
    // 检查除零保护
    if(voltage <= 0.001f)
    {
        return 999.0f;  // 返回一个很大的值表示短路或异常
    }
    return (MQ2_VCC - voltage) * MQ2_RL / voltage;
}

void MQ2_Init(void)
{
    // ADC已经在系统中初始化，这里只需确保MQ2的ADC通道已配置
    // MQ2传感器需要预热1-2分钟才能稳定，这里进行充分预热
    for(int i = 0; i < 60; i++)
    {
        MQ2_ReadRawADC();
        HAL_Delay(100);  // 6秒预热足够，1000ms太长
    }
}

/**
从MQ2传感器读取原始ADC值
ADC原始读数值 (0-4095)
*/
uint16_t MQ2_ReadRawADC(void)
{
    return (uint16_t)ADC_ReadChannel(MQ2_ADC_CHANNEL);
}

/**
将ADC值转换为甲烷浓度 (PPM)
使用MQ2数据手册的标准公式
公式: ppm = A * (Rs/R0)^(-B)
其中Rs为当前传感器电阻，R0为洁净空气中的基准电阻

adc_value: ADC原始读数值
甲烷浓度值 (PPM)
*/
float MQ2_ConvertToPPM(uint16_t adc_value)
{
    float voltage;      // ADC读取的电压值
    float PPM;          // 浓度值
    float RS;           // 传感器当前电阻
    float R0;           // 传感器基准电阻

    // 转换为电压值
    voltage = MQ2_GetVoltage(adc_value);

    // 检查电压有效性
    if(voltage <= 0.1f || voltage >= 4.9f)
    {
        return 0.0f;  // 电压异常，返回0
    }

    // 计算传感器电阻 Rs = (Vcc - Vrl) * RL / Vrl
    RS = (MQ2_VCC - voltage) * MQ2_RL / voltage;

    // 计算基准电阻 R0
    // 根据MQ2数据手册，洁净空气中 Rs/R0 ≈ 9.8
    // 所以 R0 = Rs / 9.8
    R0 = RS / MQ2_RO_RATIO;

    // 限制电阻范围，避免计算异常
    if(RS < 1.0f)
    {
        RS = 1.0f;
    }
    if(RS > 100.0f)
    {
        RS = 100.0f;
    }

    // 使用MQ2数据手册公式计算甲烷浓度
    // 公式: ppm = A * (Rs/R0)^(-B)
    float ratio = RS / R0;
    PPM = MQ2_CURVE_A * powf(ratio, -MQ2_CURVE_B);

    // 确保PPM值在合理范围内
    if(PPM < 0.0f || PPM > 10000.0f)
    {
        PPM = 0.0f;
    }

    return PPM;
}

/**
读取MQ2传感器甲烷浓度
天然气主要成分是甲烷(85-95%)，使用甲烷拟合参数
甲烷浓度值 (PPM)
*/
float MQ2_ReadPPM(void)
{
    uint16_t adc_value = MQ2_ReadRawADC();
    float ppm = MQ2_ConvertToPPM(adc_value);

    return ppm;
}
