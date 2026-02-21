#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Servo.h"

#define T 			((uint32_t)800)			//扑动的周期，单位ms
#define GAP			((uint32_t)200)			//前后的相位差，单位ms（应小于T/2）
#define Amplitude	((float)120)			//振幅

int flag1 = 1;
int flag2 = 1;	//表示翅膀上下的标志位


typedef void (*Servo_SetAngle)(float);
 
void flutter(float angle, 							//角度，准确来说是振幅
			 Servo_SetAngle Servo_SetAngle_Right, 	//控制右翅角度的函数
			 Servo_SetAngle Servo_SetAngle_Left, 	//控制左翅角度的函数
			 int *flag
			)
{	/* 	
		控制 **一对** 翅膀扑动的函数
		在主循环中调用两次配合delay实现差动
		分别传入左右两边的角度控制函数，对应关系详见Servo.c
	*/
	float Angle = (180-angle)/2;
	if (*flag != 1)
	{
		Angle = 180 - Angle;
	}
	Servo_SetAngle_Right(Angle);
	Servo_SetAngle_Left(180 - Angle);
	*flag = !*flag;
}



int main(void)
{
	SystemInit();
	Servo_Init();
	Servo_SetAngle1(30);	Servo_SetAngle2(150);		//复位
	Delay_ms(2500);
	while (1)
	{
		flutter(Amplitude, Servo_SetAngle1, Servo_SetAngle2, &flag1);
		Delay_ms(GAP);
		flutter(Amplitude, Servo_SetAngle3, Servo_SetAngle4, &flag2);
		Delay_ms(T/2-GAP);
	}
}
