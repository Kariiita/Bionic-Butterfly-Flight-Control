#include "ibus.h"
#include <stdio.h>

// 全局iBus对象
iBus_t iBus;

// 环形缓冲区函数
static uint8_t RingBuffer_Write(RingBuffer_t *rb, uint8_t data)
{
    uint16_t next_head = (rb->head + 1) % IBUS_BUFFER_SIZE;
    
    if(next_head == rb->tail) {
        return 0; // 缓冲区满
    }
    
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return 1;
}

static uint8_t RingBuffer_Read(RingBuffer_t *rb, uint8_t *data)
{
    if(rb->tail == rb->head) {
        return 0; // 缓冲区空
    }
    
    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % IBUS_BUFFER_SIZE;
    return 1;
}

static uint16_t RingBuffer_Available(RingBuffer_t *rb)
{
    return (rb->head >= rb->tail) ? 
           (rb->head - rb->tail) : 
           (IBUS_BUFFER_SIZE - rb->tail + rb->head);
}

// 初始化iBus
void IBUS_Init(void)
{
    // 初始化iBus结构体
    for(int i = 0; i < IBUS_CHANNELS; i++) {
        iBus.channels[i] = 1500; // 中位值
    }
    
    iBus.last_update = 0;
    iBus.frame_complete = 0;
    iBus.buffer_index = 0;
    
    // 初始化环形缓冲区
    iBus.rx_buffer.head = 0;
    iBus.rx_buffer.tail = 0;
    
    // 初始化UART
    IBUS_UART_Init(IBUS_BAUDRATE);
    
    printf("iBus Initialized\r\n");
}

// 初始化UART (USART1, PA9-TX, PA10-RX)
void IBUS_UART_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
	
	// 确保 SysTick 优先级低于 UART 中断
	NVIC_SetPriority(USART1_IRQn, 1);      // 较高优先级（数值小）
	NVIC_SetPriority(SysTick_IRQn, 2);     // 较低优先级（数值大）
    
    // 使能时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
    
    // 配置GPIO
    // PA9 - USART1_TX (可选，实际只需要RX)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // PA10 - USART1_RX (浮空输入)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置USART
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);
    
    // 使能接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    
    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能USART
    USART_Cmd(USART1, ENABLE);
    
}

// USART1中断服务函数
void USART1_IRQHandler(void)
{
    if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        uint8_t data = USART_ReceiveData(USART1);
        RingBuffer_Write(&iBus.rx_buffer, data);
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

// 校验和计算
static uint16_t IBUS_CalculateChecksum(uint8_t *data, uint8_t len)
{
    uint16_t checksum = 0xFFFF;
    
    for(int i = 0; i < len; i++) {
        checksum -= data[i];
    }
    
    return checksum;
}

// 读取并解析iBus数据
int8_t IBUS_Read(iBus_t *ibus)
{
    uint8_t data;
    
    // 从环形缓冲区读取数据
    while(RingBuffer_Read(&ibus->rx_buffer, &data)) {
        // 状态机解析
        static enum {
            IBUS_STATE_WAIT_HEADER1,
            IBUS_STATE_WAIT_HEADER2,
            IBUS_STATE_RECEIVING_DATA
        } state = IBUS_STATE_WAIT_HEADER1;
        
        switch(state) {
            case IBUS_STATE_WAIT_HEADER1:
                if(data == IBUS_HEADER1) {
                    state = IBUS_STATE_WAIT_HEADER2;
                    ibus->buffer_index = 0;
                    ibus->buffer[ibus->buffer_index++] = data;
                }
                break;
                
            case IBUS_STATE_WAIT_HEADER2:
                if(data == IBUS_HEADER2) {
                    state = IBUS_STATE_RECEIVING_DATA;
                    ibus->buffer[ibus->buffer_index++] = data;
                } else {
                    state = IBUS_STATE_WAIT_HEADER1;
                }
                break;
                
            case IBUS_STATE_RECEIVING_DATA:
                ibus->buffer[ibus->buffer_index++] = data;
                
                // 检查是否收到完整帧
                if(ibus->buffer_index >= IBUS_FRAME_LENGTH) {
                    state = IBUS_STATE_WAIT_HEADER1;
                    
                    // 验证校验和
                    uint16_t received_checksum = (ibus->buffer[IBUS_FRAME_LENGTH - 1] << 8) | 
                                                  ibus->buffer[IBUS_FRAME_LENGTH - 2];
                    uint16_t calculated_checksum = IBUS_CalculateChecksum(
                        ibus->buffer, 
                        IBUS_FRAME_LENGTH - 2
                    );
                    
                    if(received_checksum == calculated_checksum) {
                        // 解析通道数据
                        for(int i = 0; i < IBUS_CHANNELS; i++) {
                            uint8_t low_byte = ibus->buffer[2 + i * 2];
                            uint8_t high_byte = ibus->buffer[2 + i * 2 + 1];
                            ibus->channels[i] = (high_byte << 8) | low_byte;
                        }
                        
                        ibus->last_update = SysTick->VAL; // 使用系统滴答计时器
                        ibus->frame_complete = 1;
                        
                        return IBUS_OK;
                    } else {
                        return IBUS_ERROR_CHECKSUM;
                    }
                }
                break;
        }
    }
    
    return IBUS_ERROR_NO_DATA;
}

// 获取指定通道的值
uint16_t IBUS_GetChannel(iBus_t *ibus, uint8_t channel)
{
    if(channel >= IBUS_CHANNELS) {
        return 0;
    }
    
    return ibus->channels[channel];
}

// 检查iBus是否活跃（接收数据）
uint8_t IBUS_IsActive(iBus_t *ibus, uint32_t timeout_ms)
{
    uint32_t current_time = SysTick->VAL;
    uint32_t elapsed_time;
    
    if(ibus->last_update == 0) {
        return 0;
    }
    
    // 计算经过的时间（简化计算）
    if(current_time > ibus->last_update) {
        elapsed_time = (current_time - ibus->last_update) / (SystemCoreClock / 1000);
    } else {
        elapsed_time = ((0xFFFFFFFF - ibus->last_update) + current_time) / (SystemCoreClock / 1000);
    }
    
    return (elapsed_time < timeout_ms);
}


