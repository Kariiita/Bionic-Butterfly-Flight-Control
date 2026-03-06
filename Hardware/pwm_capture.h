#ifndef __PWM_CAPTURE_H
#define __PWM_CAPTURE_H

#include "stm32f10x.h"

/**
 * @brief  初始化PB6(TIM4_CH1)为PWM输入捕获模式
 * @note   使用TIM4的PWM Input模式，自动捕获周期和占空比
 */
void PWM_Capture_Init(void);

/**
 * @brief  获取捕获到的PWM占空比
 * @return 占空比值，范围 0.0 ~ 100.0（百分比）
 *         如果没有信号输入，返回 0.0
 */
float PWM_Capture_GetDuty(void);

/**
 * @brief  获取捕获到的PWM频率
 * @return 频率值，单位 Hz
 *         如果没有信号输入，返回 0.0
 */
uint16_t PWM_Capture_GetPulseUs(void);

/**
 * @brief  检查是否有有效的PWM信号输入
 * @return 1: 有信号  0: 无信号
 */
uint8_t PWM_Capture_IsValid(void);

#endif /* __PWM_CAPTURE_H */