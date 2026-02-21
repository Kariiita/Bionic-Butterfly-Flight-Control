#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Servo.h"
#include "ibus.h"

#define T 			((uint32_t)800)			//扑动的周期，单位ms
#define GAP			((uint32_t)200)			//前后的相位差，单位ms（应小于T/2）
#define AMPLITUDE	((float)120)			//振幅

extern iBus_t iBus;

typedef struct{		//存储蝴蝶的状态
	uint16_t throttle;		//油门
	uint16_t yaw;			//偏航
	uint16_t pitch;			//俯仰
	float Amplitude;		//实时振幅
	float Amplitude_R;		//右前翅振幅
	float Amplitude_L;		//左前翅振幅
}States;

States states = {1000, 1500, 1500, AMPLITUDE, AMPLITUDE, AMPLITUDE};


int flag1 = 1;	//表示翅膀上下的标志位
int flag2 = 1;
typedef void (*Servo_SetAngle)(float);
void flutter(float angle_R,  
			 float angle_L, 		//左右的振幅
			 Servo_SetAngle Servo_SetAngle_Right, 	//控制右翅角度的函数
			 Servo_SetAngle Servo_SetAngle_Left,	//控制左翅角度的函数
			 int *flag
			)
{	/* 	控制 **一对** 翅膀扑动的函数
		在主循环中调用两次配合delay实现以一定相位差扑动
		后两个参数分别传入左右两边的角度控制函数，对应关系详见Servo.c	*/
	float Angle_R = (180-angle_R)/2;
	float Angle_L = (180-angle_L)/2;
	if (*flag != 1)
	{
		Angle_R = 180 - Angle_R;
		Angle_L = 180 - Angle_L;
	}
	Servo_SetAngle_Right(Angle_R);
	Servo_SetAngle_Left(180 - Angle_L);
	*flag = !*flag;
}


void StatesUpdate()
{
	states.throttle = IBUS_GetChannel(&iBus, 2);  				// 油门，通道3，左摇杆上下
	states.yaw = IBUS_GetChannel(&iBus, 3);  					// 偏航，通道4，左摇杆左右
	states.pitch = IBUS_GetChannel(&iBus, 1);  					// 俯仰，通道2，右摇杆上下
	
	states.Amplitude = AMPLITUDE*(states.throttle-1000)/1000;	// 油门值映射到实时振幅
	if (states.yaw>=1500)										// 根据偏航值增加左或右的幅度
	{
		states.Amplitude_R = states.Amplitude + (180-states.Amplitude)*(states.yaw-1500)/500;
		states.Amplitude_L = states.Amplitude;
	}
	else
	{
		states.Amplitude_L = states.Amplitude + (180-states.Amplitude)*(1500-states.yaw)/500;
		states.Amplitude_R = states.Amplitude;
	}
	
	if (states.pitch>=1500)										// 根据俯仰值增加左右的幅度
	{
		states.Amplitude_R += (180-states.Amplitude_R)*(states.pitch-1500)/500;
		states.Amplitude_L += (180-states.Amplitude_L)*(states.pitch-1500)/500;
	}
	else
	{
		states.Amplitude_R += (180-states.Amplitude_R)*(1500-states.pitch)/500;
		states.Amplitude_L += (180-states.Amplitude_L)*(1500-states.pitch)/500;
	}
	
}


int main(void)
{
	SystemInit();
	SysTick_Config(SystemCoreClock / 1000);   // 1ms 中断一次
	IBUS_Init();
	Servo_Init();
//	Servo_SetAngle1(30);	Servo_SetAngle2(150);		//复位
	
	while (1)
	{
		StatesUpdate();
		
		
		flutter(states.Amplitude_R, states.Amplitude_L, Servo_SetAngle1, Servo_SetAngle2, &flag1);
		Delay_ms(GAP);
		flutter(states.Amplitude, states.Amplitude, Servo_SetAngle3, Servo_SetAngle4, &flag2);
		Delay_ms(T/2-GAP);
	}
}


void SysTick_Handler(void)
{
    static uint32_t tick = 0;
    tick++;
    if (tick >= 2)      // 每 2ms 调用一次 IBUS_Read
    {
        tick = 0;
        IBUS_Read(&iBus);   // 注意：此函数非重入，但 SysTick 优先级应低于 UART 中断
    }
}
