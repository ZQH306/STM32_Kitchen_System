#include "adc.h"
#include "light.h"
#include <math.h>

// 校准标志
static uint8_t light_calibrated = 0;

// 校准参数存储
static float dark_adc_value = 0.0f;
static float light_adc_value = 0.0f;

// 滤波相关
static uint8_t filter_enabled = 1;
static float filter_buffer[LIGHT_FILTER_SIZE] = {0};
static uint8_t filter_index = 0;
static uint8_t filter_filled = 0;

/**
初始化光照传感器
*/
void Light_Init(void)
{
    // 预热 - 进行几次ADC读取使传感器稳定
    for(int i = 0; i < 5; i++)
    {
        Light_Get();
        HAL_Delay(10);
    }

    // 初始化滤波缓冲区
    for(int i = 0; i < LIGHT_FILTER_SIZE; i++)
    {
        filter_buffer[i] = 0.0f;
    }
}

/**
从光照传感器读取原始ADC值
返回: ADC原始读数值 (0-4095)
*/
uint32_t Light_Get(void)
{
    return (uint32_t)ADC_ReadChannel(LIGHT_ADC_CHANNEL);
}

/**
将ADC值转换为电压值 (mV)
adc_value: ADC原始读数值
返回: 电压值 (mV)
*/
static float Light_ADC_to_Voltage(float adc_value)
{
    return adc_value * (LIGHT_ADC_REF_VOLTAGE / LIGHT_ADC_RESOLUTION);
}

/**
将ADC值转换为lux值
adc_value: ADC原始读数值
返回: lux值

电路原理：
光敏电阻模块 - 光照强 → 阻值小 → ADC值低 → lux高
             光照弱 → 阻值大 → ADC值高 → lux低

标准光照参考：
- 黑夜：0.001-0.02 lux
- 月夜：0.02-0.3 lux
- 阴暗夜晚：0.003-0.007 lux
- 夜间路灯：0.1 lux
- 黄昏室内：10 lux
- 阴天室内：5-50 lux
- 阴天室外：50-500 lux
- 日出日落：300 lux
- 晴天室内：100-1000 lux
- 阴天：3000-10000 lux
- 晴天：30000-300000 lux
*/
float Light_ADC_to_Lux(float adc_value)
{
    float lux;

    // 检查ADC值范围
    if(adc_value <= 0.0f)
    {
        return 100000.0f;  // ADC为0，最强光（晴天）
    }
    if(adc_value >= LIGHT_ADC_RESOLUTION)
    {
        return 0.001f;  // ADC为4095，完全黑暗
    }

    // 如果已校准，使用校准后的范围进行映射
    float adc_ratio;
    if(light_calibrated && dark_adc_value > light_adc_value)
    {
        // 黑暗时ADC值高，强光时ADC值低
        adc_ratio = (dark_adc_value - adc_value) / (dark_adc_value - light_adc_value);
    }
    else
    {
        // 默认范围转换：ADC高→lux低，ADC低→lux高
        adc_ratio = 1.0f - (adc_value / LIGHT_ADC_RESOLUTION);
    }

    // 限制在0-1范围内
    if(adc_ratio < 0.0f) adc_ratio = 0.0f;
    if(adc_ratio > 1.0f) adc_ratio = 1.0f;

    // 分段映射，根据标准光照值
    if(adc_ratio < 0.1f)
    {
        // 0-10% 黑夜到月夜 (0-0.3 lux)
        lux = adc_ratio * 3.0f;
    }
    else if(adc_ratio < 0.3f)
    {
        // 10-30% 月夜到黄昏 (0.3-10 lux)
        lux = 0.3f + (adc_ratio - 0.1f) * 48.5f;
    }
    else if(adc_ratio < 0.5f)
    {
        // 30-50% 黄昏到阴天室内 (10-50 lux)
        lux = 10.0f + (adc_ratio - 0.3f) * 200.0f;
    }
    else if(adc_ratio < 0.7f)
    {
        // 50-70% 阴天室内到阴天室外 (50-500 lux)
        lux = 50.0f + (adc_ratio - 0.5f) * 2250.0f;
    }
    else if(adc_ratio < 0.85f)
    {
        // 70-85% 阴天室外到晴天室内 (500-1000 lux)
        lux = 500.0f + (adc_ratio - 0.7f) * 3333.33f;
    }
    else if(adc_ratio < 0.95f)
    {
        // 85-95% 晴天室内到阴天 (1000-10000 lux)
        lux = 1000.0f + (adc_ratio - 0.85f) * 90000.0f;
    }
    else
    {
        // 95-100% 阴天到晴天 (10000-100000 lux)
        lux = 10000.0f + (adc_ratio - 0.95f) * 1800000.0f;
    }

    // 限制在合理范围内
    if(lux < 0.001f)
    {
        lux = 0.001f;
    }
    else if(lux > 100000.0f)
    {
        lux = 100000.0f;
    }

    return lux;
}

/**
获取lux值 (uint32_t版本)
返回: lux值
*/
uint32_t Light_Get_Lux(void)
{
    uint32_t adc_value = Light_Get();
    float lux = Light_ADC_to_Lux((float)adc_value);
    return (uint32_t)lux;
}

/**
获取lux值 (float版本)
返回: lux值 (float)
*/
float Light_Get_Lux_Float(void)
{
    uint32_t adc_value = Light_Get();
    return Light_ADC_to_Lux((float)adc_value);
}

/*
检查是否已校准
返回: 1=已校准, 0=未校准
*/
uint8_t Light_Is_Calibrated(void)
{
    return light_calibrated;
}

/**
启用/禁用滤波
enable: 1=启用, 0=禁用
*/
void Light_Enable_Filter(uint8_t enable)
{
    filter_enabled = enable;

    // 如果禁用滤波，清空缓冲区
    if(!enable)
    {
        for(int i = 0; i < LIGHT_FILTER_SIZE; i++)
        {
            filter_buffer[i] = 0.0f;
        }
        filter_filled = 0;
        filter_index = 0;
    }
}

/**
获取滤波后的lux值
使用移动平均滤波
返回: 滤波后的lux值
*/
float Light_Get_Filtered_Lux(void)
{
    if(!filter_enabled)
    {
        return Light_Get_Lux_Float();
    }

    // 获取新的lux值
    float new_lux = Light_Get_Lux_Float();

    // 存入缓冲区
    filter_buffer[filter_index] = new_lux;
    filter_index = (filter_index + 1) % LIGHT_FILTER_SIZE;

    // 标记缓冲区已填满
    if(!filter_filled && filter_index == 0)
    {
        filter_filled = 1;
    }

    // 计算移动平均
    float sum = 0.0f;
    int count;

    if(filter_filled)
    {
        count = LIGHT_FILTER_SIZE;
    }
    else if(filter_index == 0)
    {
        count = 1;  // 只有一个样本
    }
    else
    {
        count = filter_index;  // 当前有filter_index个样本
    }

    if(count == 0)
    {
        return new_lux;
    }

    for(int i = 0; i < count; i++)
    {
        sum += filter_buffer[i];
    }

    return sum / count;
}

/**
校准黑暗环境
在完全黑暗的环境中调用此函数
黑暗时ADC值应该比较高
*/
void Light_Calibrate_Dark(void)
{
    // 简化版校准：在黑暗环境中记录ADC值作为基准
    float total_adc = 0.0f;
    int samples = 20;

    // 采集多次样本
    for(int i = 0; i < samples; i++)
    {
        uint32_t adc_value = Light_Get();
        total_adc += (float)adc_value;
        HAL_Delay(10);
    }

    // 黑暗时的平均ADC值
    dark_adc_value = total_adc / samples;
    light_calibrated = 1;
}

/**
校准光照环境
在已知强光照环境中调用此函数（如日光灯下）
强光时ADC值应该比较低
*/
void Light_Calibrate_Light(void)
{
    // 简化版校准：在光照环境中记录ADC值
    float total_adc = 0.0f;
    int samples = 20;

    // 采集多次样本
    for(int i = 0; i < samples; i++)
    {
        uint32_t adc_value = Light_Get();
        total_adc += (float)adc_value;
        HAL_Delay(10);
    }

    // 强光时的平均ADC值
    light_adc_value = total_adc / samples;
    light_calibrated = 1;
}
