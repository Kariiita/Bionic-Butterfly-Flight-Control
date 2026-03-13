#include "stm32f10x.h"
#include "Delay.h"
#include "Servo.h"
#include "pwm_capture.h"
#include <math.h>

#define PI 3.1415926f
#define BASE_ANGLE 90.0f
#define MAX_FLAP_FREQ 4.5f   // 满油门时的最高扑打频率(Hz)

/* 控制映射配置 */
#define CH_STEERING PWM_CH_PB6 // 通道1 左右转
#define CH_PITCH    PWM_CH_PB8 // 通道2 俯仰
#define CH_THROTTLE PWM_CH_PA8 // 通道3 油门

// 飞行状态寄存器
typedef struct {
    float throttle; 
    float steering; 
    float pitch;    
    float phase;    
} FlightState_t;

FlightState_t butterfly = {0};

/* 方向摇杆解析：带死区并规整化到 -1.0 到 1.0 */
float ParseJoyStick(PWM_Channel ch) {
    if (!PWM_IsValid(ch)) return 0.0f; // 保护：掉线时归中
    int16_t pulse = PWM_GetPulseUs(ch);
    
    // 强制限幅
    if (pulse < 1000) pulse = 1000;
    if (pulse > 2000) pulse = 2000;
    
    // 中心 1500，转化为 -1.0 ~ 1.0
    float val = (float)(pulse - 1500) / 500.0f; 
    
    // 设置 6% 的死区，避免手抖的时候抽搐
    if (val > -0.06f && val < 0.06f) return 0.0f; 
    return val;
}

/* 油门解析：最低 1050 防止细微抖动起步，映射至 0.0 到 1.0 */
float ParseThrottle(PWM_Channel ch) {
    if (!PWM_IsValid(ch)) return 0.0f; // 保护：掉联时强制落回 0 点！阻止暴走！！
    int16_t pulse = PWM_GetPulseUs(ch);
    
    if (pulse < 1050) return 0.0f; // 油门拉到底时严格为 0 
    if (pulse > 1950) pulse = 1950;
    return (float)(pulse - 1050) / 900.0f; 
}

int main(void) {
    SystemInit();
    Servo_Init();         // 初始化 TIM2 (映射 PA0~PA3 控制舵机)
    PWM_Capture_InitAll(); // 初始化 TIM1, TIM4

    // 预留系统上电电压稳定时间
    Delay_ms(200);

    while (1) {
        /* ==== 1. 读取接收机输入 ==== */
        butterfly.steering = ParseJoyStick(CH_STEERING); 
        butterfly.pitch    = ParseJoyStick(CH_PITCH);    
        butterfly.throttle = ParseThrottle(CH_THROTTLE); 
        
        /* ==== 2. 运动学积分 (相位角推演) ==== */
        // 油门越大，翅膀的煽动节奏越快
        float current_freq = butterfly.throttle * MAX_FLAP_FREQ;
        // dt = 0.02s(20毫秒), 角速度 = 2 * PI * freq
        butterfly.phase += (2.0f * PI * current_freq) * 0.02f; 
        
        // 防止数据溢出
        if (butterfly.phase > 2.0f * PI) {
            butterfly.phase -= 2.0f * PI;
        }

        /* ==== 3. 计算空气动力学变形因子 ==== */
        // 核心振幅：由油门控制 (满油可打 45度，即 90 +/- 45 )
        float base_amp = butterfly.throttle * 45.0f; 

        // 俯仰计算 (前翅振幅加减，后翅做逆向补差)
        float front_amp = base_amp * (1.0f - butterfly.pitch * 0.4f);
        float rear_amp  = base_amp * (1.0f + butterfly.pitch * 0.4f) * 0.65f; // 后翅通常偏软偏小，缩放65%

        // 左右转向（改变一侧伺服臂悬挂基点）
        float steer_offset = butterfly.steering * 20.0f; 

        /* ==== 4. 四个舵机独立合成信号 ==== */
        float sin_val = sin(butterfly.phase);
        float sin_val_rear = sin(butterfly.phase - (PI / 2.5f)); // 后翼稍微滞后前翼，呈现波浪推进

        // 计算物理角度。注意：左与右是对称安装的，通常其中一边的 sin 计算要取负值！
        // 如果你的机架机械连杆是完全一致朝向的，就全部用减号。这里假设镜像安装。
        float angle_FR = BASE_ANGLE + steer_offset + (front_amp * sin_val);
        float angle_FL = BASE_ANGLE - steer_offset - (front_amp * sin_val);
        float angle_RR = BASE_ANGLE + steer_offset + (rear_amp * sin_val_rear);
        float angle_RL = BASE_ANGLE - steer_offset - (rear_amp * sin_val_rear);

        /* ==== 5. 最终物理执行 ==== */
        Servo_SetAngle(angle_FR, SERVO_CH_PA0);
        Servo_SetAngle(angle_FL, SERVO_CH_PA1);
        Servo_SetAngle(angle_RR, SERVO_CH_PA2);
        Servo_SetAngle(angle_RL, SERVO_CH_PA3);

        /* ==== 6. 严苛的时基 ==== */
        Delay_ms(20); 
    }
}