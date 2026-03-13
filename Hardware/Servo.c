#include "stm32f10x.h"                  // Device header
#include "PWM.h"
#include "Servo.h"

ServoSetAngleType Servo_list[SERVO_CH_NUM] = {
	Servo_SetAngle1,
	Servo_SetAngle2,
	Servo_SetAngle3,
	Servo_SetAngle4
};

void Servo_Init(void)
{
	PWM_Init();
}

void Servo_SetAngle1(float Angle)	// A0引脚为右
{
	PWM_SetCompare1(Angle / 180 * 2000 + 500);
}

void Servo_SetAngle2(float Angle)	// A1引脚为左
{
	PWM_SetCompare2(Angle / 180 * 2000 + 500);
}

void Servo_SetAngle3(float Angle)	// A2引脚为右
{
	PWM_SetCompare3(Angle / 180 * 2000 + 500);
}

void Servo_SetAngle4(float Angle)	// A3引脚为左
{
	PWM_SetCompare4(Angle / 180 * 2000 + 500);
}


/* 设置指定舵机的角度 */
void Servo_SetAngle(float Angle, SERVO_Channel ch)
{
	Servo_list[ch](Angle);
}
