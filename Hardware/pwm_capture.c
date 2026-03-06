#include "pwm_capture.h"

/* 中断中写入的原始数据（可能随时被改） */
static volatile uint16_t raw_period   = 0;
static volatile uint16_t raw_hightime = 0;
static volatile uint8_t  raw_update   = 0;  /* 中断中置1，表示有新数据 */

/* 主循环使用的安全副本（关中断后拷贝过来的） */
static uint16_t safe_period   = 0;
static uint16_t safe_hightime = 0;
static uint8_t  data_valid    = 0;

void PWM_Capture_Init(void)
{
    GPIO_InitTypeDef        GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_ICInitTypeDef       TIM_ICInitStruct;
    NVIC_InitTypeDef        NVIC_InitStruct;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB6 浮空输入 */
    GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* 时基：1MHz计数 */
    TIM_TimeBaseStruct.TIM_Prescaler         = 72 - 1;
    TIM_TimeBaseStruct.TIM_CounterMode       = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period            = 65535;
    TIM_TimeBaseStruct.TIM_ClockDivision     = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStruct);

    /* PWM输入模式 */
    TIM_ICInitStruct.TIM_Channel     = TIM_Channel_1;
    TIM_ICInitStruct.TIM_ICPolarity  = TIM_ICPolarity_Rising;
    TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStruct.TIM_ICFilter    = 0x0F;  /* 最大滤波，抗干扰 */
    TIM_PWMIConfig(TIM4, &TIM_ICInitStruct);

    TIM_SelectInputTrigger(TIM4, TIM_TS_TI1FP1);
    TIM_SelectSlaveMode(TIM4, TIM_SlaveMode_Reset);
    TIM_SelectMasterSlaveMode(TIM4, TIM_MasterSlaveMode_Enable);

    NVIC_InitStruct.NVIC_IRQChannel                   = TIM4_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    TIM_ITConfig(TIM4, TIM_IT_CC1, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}

/**
 * @brief  TIM4中断 - 只做最简单的事：存数据、置标志
 */
void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM4, TIM_IT_CC1) != RESET)
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);

        raw_period   = TIM_GetCapture1(TIM4);
        raw_hightime = TIM_GetCapture2(TIM4);
        raw_update   = 1;
    }
}

/**
 * @brief  安全地将中断数据拷贝到主循环（关中断保护）
 *         在读取数据前调用一次即可
 */
static void PWM_Capture_Update(void)
{
    uint16_t p, h;

    /* 关中断，原子地读取两个值，防止读到一半被中断改掉 */
    __disable_irq();
    if (raw_update)
    {
        p = raw_period;
        h = raw_hightime;
        raw_update = 0;
    }
    else
    {
        __enable_irq();
        return;  /* 没有新数据，不更新 */
    }
    __enable_irq();

    /* 合法性检查：接收机50Hz信号，周期约20000us，脉宽800~2200us */
    if (p > 10000 && p < 30000 && h >= 800 && h <= 2200)
    {
        safe_period   = p;
        safe_hightime = h;
        data_valid    = 1;
    }
    /* 不合法的数据直接丢弃，保留上一次的有效值 */
}

uint8_t PWM_Capture_IsValid(void)
{
    PWM_Capture_Update();  /* 每次查询时自动更新安全副本 */	
    return data_valid;
}

/**
 * @brief  获取高电平脉宽（微秒）
 * @return 典型值 1000~2000，无效返回 0
 */
uint16_t PWM_Capture_GetPulseUs(void)
{
    if (!data_valid)
        return 0;
    return safe_hightime;
}

float PWM_Capture_GetDuty(void)
{
    if (!data_valid || safe_period == 0)
        return -1.0f;
    return ((float)safe_hightime / (float)safe_period) * 100.0f;
}




