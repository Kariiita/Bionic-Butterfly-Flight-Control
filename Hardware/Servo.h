#ifndef __SERVO_H
#define __SERVO_H

// 定义可用的PWM通道
typedef enum {
    SERVO_CH_PA0,   // TIM2_CH1_PA0
    SERVO_CH_PA1,   // TIM2_CH2_PA1
    SERVO_CH_PA2,   // TIM2_CH3_PA2
    SERVO_CH_PA3,   // TIM2_CH4_PA3
    SERVO_CH_NUM
} SERVO_Channel;


void Servo_Init(void);
void Servo_SetAngle1(float Angle);
void Servo_SetAngle2(float Angle);
void Servo_SetAngle3(float Angle);
void Servo_SetAngle4(float Angle);

typedef void (*ServoSetAngleType)(float);
extern ServoSetAngleType Servo_list[SERVO_CH_NUM];

/* 设置指定舵机的角度 */
void Servo_SetAngle(float Angle, SERVO_Channel ch);

#endif
