#ifndef __IBUS_H
#define __IBUS_H

#include "stm32f10x.h"

// iBus协议常量定义
#define IBUS_FRAME_LENGTH       32    // 帧长度(字节)
#define IBUS_CHANNELS           14    // 通道数
#define IBUS_BAUDRATE          115200 // 波特率
#define IBUS_BUFFER_SIZE        64    // 环形缓冲区大小

// iBus帧头
#define IBUS_HEADER1           0x20
#define IBUS_HEADER2           0x40

// 通道值范围
#define IBUS_CHANNEL_MIN       1000   // 最小通道值
#define IBUS_CHANNEL_MAX       2000   // 最大通道值

// 错误代码
#define IBUS_OK                 0
#define IBUS_ERROR_CHECKSUM     -1
#define IBUS_ERROR_FRAME        -2
#define IBUS_ERROR_NO_DATA      -3

// 环形缓冲区结构
typedef struct {
    uint8_t buffer[IBUS_BUFFER_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} RingBuffer_t;

// iBus处理结构
typedef struct {
    uint16_t channels[IBUS_CHANNELS];  // 14个通道的值
    uint32_t last_update;               // 最后更新时间
    uint8_t frame_complete;             // 帧完成标志
    uint8_t buffer[IBUS_FRAME_LENGTH];  // 帧缓冲区
    uint8_t buffer_index;               // 缓冲区索引
    RingBuffer_t rx_buffer;             // 接收环形缓冲区
} iBus_t;

// 函数声明
void IBUS_Init(void);
void IBUS_UART_Init(uint32_t baudrate);
void IBUS_UART_RxCpltCallback(void);
int8_t IBUS_Read(iBus_t *ibus);
uint16_t IBUS_GetChannel(iBus_t *ibus, uint8_t channel);
uint8_t IBUS_IsActive(iBus_t *ibus, uint32_t timeout_ms);

#endif /* __IBUS_H */
