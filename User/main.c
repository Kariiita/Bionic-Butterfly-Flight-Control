#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Servo.h"

float Angle;
int flag = 1;

int main(void)
{
	SystemInit();
	Servo_Init();
	Servo_SetAngle1(30);	Servo_SetAngle2(150);		//复位
	Delay_ms(2500);
	while (1)
	{
		if (flag == 1)
		{
			Angle = 30;
		}
		else
		{
			Angle = 150;
		}
		Servo_SetAngle1(Angle);
		Servo_SetAngle2(180 - Angle);
		flag = !flag;
		Delay_ms(400);
	}
}
