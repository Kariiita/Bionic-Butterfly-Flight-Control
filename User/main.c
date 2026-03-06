#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Servo.h"
#include "pwm_capture.h"

#define T 			((uint32_t)800)			//扑动的周期，单位ms
#define GAP			((uint32_t)200)			//前后的相位差，单位ms（应小于T/2）
#define AMPLITUDE	((float)120)			//振幅


/**
 *@brief	
 */
void SetAngleMapped(float last_angle, float servo_angle)
{
	if (PWM_Capture_IsValid())
	{
		uint16_t pulse = PWM_Capture_GetPulseUs();

		/* 限幅 */
		if (pulse < 1000) pulse = 1000;
		if (pulse > 2000) pulse = 2000;

		/* 脉宽映射到角度：1000us→0°  2000us→180° */
		float target_angle = (float)(pulse - 1000) / 1000.0f * 180.0f;

		/* ========== 一阶低通滤波（关键！消除抖动） ========== */
		/* 系数0.3：越小越平滑但响应越慢，越大响应越快但越抖 */
		/* 0.2~0.4 适合舵机控制 */
		float angle = last_angle + 0.3f * (target_angle - last_angle);
		last_angle = angle;

		/* 死区：角度变化太小就不更新，避免舵机反复微调抖动 */
		if (angle - servo_angle > 2.0f || servo_angle - angle > 2.0f)
		{
			servo_angle = angle;
			Servo_SetAngle1(servo_angle);
		}
	}
	/* 无信号时不动，保持上次位置 */
}


int main(void)
{
    float last_angle = 90.0f;  /* 上一次的角度，用于平滑 */
	static float servo_angle = 90.0f;
	
    SystemInit();
    Servo_Init();
    PWM_Capture_Init();

    /* 等待接收机信号稳定 */
    Delay_ms(500);
	
    while (1)
    {
        SetAngleMapped(last_angle, servo_angle);
        Delay_ms(20);
    }
}
