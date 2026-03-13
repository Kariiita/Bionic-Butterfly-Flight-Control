#include "stm32f10x.h"
#include "Delay.h"
#include "Servo.h"
#include "pwm_capture.h"

// 记录四个通道的平滑角度状态，防止互相干扰
typedef struct {
    float last_angle;
    float current_servo_angle;
} Filter_Obj;

Filter_Obj f_list[4] = {{90.0f, 90.0f}, {90.0f, 90.0f}, {90.0f, 90.0f}, {90.0f, 90.0f}};

void Process_Servo(PWM_Channel pwm_ch, SERVO_Channel servo_ch, Filter_Obj* f) {
    if (PWM_IsValid(pwm_ch)) {
        uint16_t pulse = PWM_GetPulseUs(pwm_ch);
        
        // 1. 严格限幅 (根据你的接收机而定，典型值1000-2000)
        if (pulse < 1000) pulse = 1000;
        if (pulse > 2000) pulse = 2000;

        // 2. 转换为目标角度
        float target = (float)(pulse - 1000) / 1000.0f * 180.0f;

        // 3. 一阶低通滤波 (系数 0.2 意味着 20% 新数据 + 80% 旧数据)
        // 这个系数越小，舵机越稳，但响应会变肉
        f->last_angle = f->last_angle + 0.2f * (target - f->last_angle);

        // 4. 死区判断 (变化小于 1.5 度不动作，彻底消除静态抖动)
        if (f->last_angle - f->current_servo_angle > 1.5f || 
            f->current_servo_angle - f->last_angle > 1.5f) {
            f->current_servo_angle = f->last_angle;
            Servo_SetAngle(f->current_servo_angle, servo_ch);
        }
    }
}

int main(void) {
    SystemInit();
    // 延迟确保电压稳定
    Delay_ms(200);

    Servo_Init();
    PWM_Capture_InitAll();

    while (1) {
        // 轮询处理通道
        Process_Servo(PWM_CH_PB6, SERVO_CH_PA0, &f_list[0]);
        Process_Servo(PWM_CH_PB8, SERVO_CH_PA1, &f_list[1]);
		Process_Servo(PWM_CH_PA8, SERVO_CH_PA2, &f_list[2]);
		Process_Servo(PWM_CH_PA10, SERVO_CH_PA3, &f_list[3]);
        
        // 关键：延时控制。接收机 50Hz，我们延时 20ms 最匹配。
        Delay_ms(20);
    }
}
