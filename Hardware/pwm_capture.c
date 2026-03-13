#include "pwm_capture.h"

// ----------------- 通道内部数据结构 -----------------
typedef struct { 
    volatile uint16_t rise_time;    // 记录上升沿时刻
    volatile uint16_t period;       // 周期 (us)
    volatile uint16_t high_time;    // 高电平 (us)
    volatile uint8_t  state;        // 状态: 0-等待上升沿, 1-等待下降沿
    volatile uint8_t  updated;      // 新数据标志

    uint16_t safe_high;             // 主循环使用的安全脉宽
    uint16_t safe_period;
    uint8_t  data_valid;
} PWM_Channel_Obj;

static PWM_Channel_Obj g_pwm[PWM_CH_NUM];

// 核心处理函数：在中断中调用
// 参数：ch - 通道编号, capture - 当前捕获值, TIMx - 使用哪个定时器, TIM_Channel - 对应寄存器
static void PWM_Process(PWM_Channel ch, uint16_t capture, TIM_TypeDef* TIMx, uint16_t TIM_Channel) {
    PWM_Channel_Obj *p = &g_pwm[ch];
    
    if (p->state == 0) { // 刚才捕获的是【上升沿】
        p->rise_time = capture;
        p->state = 1;
        // 关键：立即切换为捕获【下降沿】
        if (TIM_Channel == TIM_Channel_1) TIM_OC1PolarityConfig(TIMx, TIM_ICPolarity_Falling);
        else if (TIM_Channel == TIM_Channel_3) TIM_OC3PolarityConfig(TIMx, TIM_ICPolarity_Falling);
    } 
    else { // 刚才捕获的是【下降沿】
        // 计算高电平时间 = 当前时刻 - 上升沿时刻（自动处理计数器回绕）
        p->high_time = (uint16_t)(capture - p->rise_time);
        p->updated = 1;
        p->state = 0;
        // 关键：立即切换回捕获【上升沿】
        if (TIM_Channel == TIM_Channel_1) TIM_OC1PolarityConfig(TIMx, TIM_ICPolarity_Rising);
        else if (TIM_Channel == TIM_Channel_3) TIM_OC3PolarityConfig(TIMx, TIM_ICPolarity_Rising);
    }
}

// ----------------- TIM4 中断 (PB6, PB8) -----------------
void TIM4_IRQHandler(void) {
    if (TIM_GetITStatus(TIM4, TIM_IT_CC1) != RESET) {
        PWM_Process(PWM_CH_PB6, TIM_GetCapture1(TIM4), TIM4, TIM_Channel_1);
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC1);
    }
    if (TIM_GetITStatus(TIM4, TIM_IT_CC3) != RESET) {
        PWM_Process(PWM_CH_PB8, TIM_GetCapture3(TIM4), TIM4, TIM_Channel_3);
        TIM_ClearITPendingBit(TIM4, TIM_IT_CC3);
    }
}

// ----------------- TIM1 中断 (PA8, PA10) -----------------
void TIM1_CC_IRQHandler(void) {
    if (TIM_GetITStatus(TIM1, TIM_IT_CC1) != RESET) {
        PWM_Process(PWM_CH_PA8, TIM_GetCapture1(TIM1), TIM1, TIM_Channel_1);
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC1);
    }
    if (TIM_GetITStatus(TIM1, TIM_IT_CC3) != RESET) {
        PWM_Process(PWM_CH_PA10, TIM_GetCapture3(TIM1), TIM1, TIM_Channel_3);
        TIM_ClearITPendingBit(TIM1, TIM_IT_CC3);
    }
}

// ----------------- 初始化逻辑 -----------------
void PWM_Capture_InitAll(void) {
    GPIO_InitTypeDef GPIO_InitStruct;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_ICInitTypeDef TIM_ICInitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    /* 1. 时钟使能 (TIM1在APB2, TIM4在APB1) */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);

    /* 2. GPIO 配置：PB6, PB8, PA8, PA10 */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_8;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_10;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 3. 定时器时基配置: 1MHz频率 (1us分辨率) */
    TIM_TimeBaseStruct.TIM_Prescaler = 72 - 1;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period = 0xFFFF;
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStruct);
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStruct);

    /* 4. 输入捕获初始化: 先设为上升沿 */
    TIM_ICInitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_ICInitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICInitStruct.TIM_ICFilter = 0x0F; // 最大滤波，防止信号毛刺

    // 配置四个通道
    TIM_ICInitStruct.TIM_Channel = TIM_Channel_1; TIM_ICInit(TIM4, &TIM_ICInitStruct);
    TIM_ICInitStruct.TIM_Channel = TIM_Channel_3; TIM_ICInit(TIM4, &TIM_ICInitStruct);
    TIM_ICInitStruct.TIM_Channel = TIM_Channel_1; TIM_ICInit(TIM1, &TIM_ICInitStruct);
    TIM_ICInitStruct.TIM_Channel = TIM_Channel_3; TIM_ICInit(TIM1, &TIM_ICInitStruct);

    /* 5. 中断使能与NVIC */
    TIM_ITConfig(TIM4, TIM_IT_CC1 | TIM_IT_CC3, ENABLE);
    TIM_ITConfig(TIM1, TIM_IT_CC1 | TIM_IT_CC3, ENABLE);

    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;

    NVIC_InitStruct.NVIC_IRQChannel = TIM4_IRQn; NVIC_Init(&NVIC_InitStruct);
    NVIC_InitStruct.NVIC_IRQChannel = TIM1_CC_IRQn; NVIC_Init(&NVIC_InitStruct);

    /* 6. 启动 */
    TIM_Cmd(TIM4, ENABLE);
    TIM_Cmd(TIM1, ENABLE);

    // 数据初始化
    for(int i=0; i<PWM_CH_NUM; i++) {
        g_pwm[i].state = 0;
        g_pwm[i].updated = 0;
    }
}

// ----------------- 安全读取接口 -----------------
static void SyncData(PWM_Channel ch) {
    __disable_irq();
    if (g_pwm[ch].updated) {
        // 合法性过滤 (针对标准舵机信号 500us~2500us)
        if (g_pwm[ch].high_time > 400 && g_pwm[ch].high_time < 3000) {
            g_pwm[ch].safe_high = g_pwm[ch].high_time;
            g_pwm[ch].data_valid = 1;
        } else {
            g_pwm[ch].data_valid = 0;
        }
        g_pwm[ch].updated = 0;
    }
    __enable_irq();
}

uint8_t PWM_IsValid(PWM_Channel ch) {
    SyncData(ch);
    return g_pwm[ch].data_valid;
}

uint16_t PWM_GetPulseUs(PWM_Channel ch) {
    SyncData(ch);
    return (g_pwm[ch].data_valid) ? g_pwm[ch].safe_high : 0;
}
