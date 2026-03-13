#ifndef __PWM_CAPTURE_H
#define __PWM_CAPTURE_H

#include "stm32f10x.h"

// 定义可用的PWM通道
typedef enum {
    PWM_CH_PB6,   // TIM4_CH1 (上升沿) + TIM4_CH2 (下降沿)
    PWM_CH_PB8,   // TIM4_CH3 (上升沿) + TIM4_CH4 (下降沿)
    PWM_CH_PA8,   // TIM1_CH1 (上升沿) + TIM1_CH2 (下降沿)
    PWM_CH_PA10,  // TIM1_CH3 (上升沿) + TIM1_CH4 (下降沿)
    PWM_CH_NUM
} PWM_Channel;

// 初始化所有PWM捕获通道
void PWM_Capture_InitAll(void);

// 检查指定通道是否有有效信号
uint8_t PWM_IsValid(PWM_Channel ch);

// 获取高电平脉宽（微秒）
uint16_t PWM_GetPulseUs(PWM_Channel ch);

// 获取占空比（百分比，0~100）
float PWM_GetDuty(PWM_Channel ch);

#endif
