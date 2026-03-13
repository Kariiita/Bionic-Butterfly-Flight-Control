#include "stm32f10x.h"

// 静态变量，用于标记 TIM3 是否已初始化
static uint8_t TIM3_Initialized = 0;

/**
  * @brief  初始化 TIM3 为 1MHz 计数时钟，连续计数模式
  * @param  无
  * @retval 无
  */
static void TIM3_Init(void)
{
    // 使能 TIM3 时钟（TIM3 位于 APB1 总线上）
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    // 配置预分频器，使计数频率为 1MHz
    // 假设系统时钟 72MHz，APB1 预分频为 2，则 TIM3 时钟 = 72MHz
    // 若 APB1 预分频不为 2，需根据实际时钟调整预分频值
    TIM_InitStructure.TIM_Prescaler = 71;          // 72MHz / (71+1) = 1MHz
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStructure.TIM_Period = 0xFFFF;         // 最大计数值 65535
    TIM_InitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_InitStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_InitStructure);
    
    // 使能 TIM3 计数器
    TIM_Cmd(TIM3, ENABLE);
    
    TIM3_Initialized = 1;
}

/**
  * @brief  微秒级延时（基于 TIM3）
  * @param  xus 延时时长，单位微秒。支持 0~任意值（自动处理 16 位计数器限制）
  * @retval 无
  */
void Delay_us(uint32_t xus)
{
    // 如果 TIM3 未初始化，先初始化
    if (!TIM3_Initialized)
    {
        TIM3_Init();
    }
    
    // TIM3 是 16 位计数器，最大单次延时 65535 微秒，循环处理
    while (xus > 65535)
    {
        TIM_SetCounter(TIM3, 0);
        while (TIM_GetCounter(TIM3) < 65535);
        xus -= 65535;
    }
    
    if (xus > 0)
    {
        TIM_SetCounter(TIM3, 0);
        while (TIM_GetCounter(TIM3) < xus);
    }
}

/**
  * @brief  毫秒级延时（基于 TIM3）
  * @param  xms 延时时长，单位毫秒
  * @retval 无
  */
void Delay_ms(uint32_t xms)
{
    while (xms--)
    {
        Delay_us(1000);
    }
}

/**
  * @brief  秒级延时（基于 TIM3）
  * @param  xs 延时时长，单位秒
  * @retval 无
  */
void Delay_s(uint32_t xs)
{
    while (xs--)
    {
        Delay_ms(1000);
    }
}
