#include "stepper_motor.h"

// 引入 CubeMX 生成的定时器句柄
extern TIM_HandleTypeDef htim3;

// 内部变量

static MotorDirection motorDirection = CLOCKWISE;
static uint8_t currentStep = 0;
static volatile uint8_t isRunning = 0; 
static uint16_t stepsRemaining = 0;    

// 步进序列 (全步进)
static const uint8_t fullStepSequence[4] = {
    0x01,  // IN1 (0001)
    0x02,  // IN2 (0010)
    0x04,  // IN3 (0100)
    0x08   // IN4 (1000)
};

// 初始化函数 (硬件初始化已由CubeMX完成，这里主要是复位状态)
void StepperMotor_Init(void) {
    StepperMotor_Stop();
}

// 设置速度 (单位us)
void StepperMotor_SetSpeed(uint16_t speed_us) {
    if(speed_us < MIN_SPEED) speed_us = MIN_SPEED;
    if(speed_us > MAX_SPEED) speed_us = MAX_SPEED;
    
    
    
    // 动态更新定时器周期
    __HAL_TIM_SET_AUTORELOAD(&htim3, speed_us - 1);
}

// 设置方向
void StepperMotor_SetDirection(MotorDirection direction) {
    motorDirection = direction;
}

// 移动指定步数
void StepperMotor_Step(uint16_t steps) {
    stepsRemaining = steps;
    isRunning = 1;
    HAL_TIM_Base_Start_IT(&htim3);  // 启动定时器中断
}

// 连续旋转
void StepperMotor_RotateContinuous(void) {
    stepsRemaining = 0;  // 0表示无限旋转
    isRunning = 1;
    HAL_TIM_Base_Start_IT(&htim3);  // 启动定时器中断
}

// 停止电机
void StepperMotor_Stop(void) {
    isRunning = 0;
    HAL_TIM_Base_Stop_IT(&htim3);   // 停止定时器中断
    
    // 将所有引脚拉低，防止电机发热
    HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN1 | MOTOR_PIN2 | MOTOR_PIN3 | MOTOR_PIN4, GPIO_PIN_RESET);
}

// 检查是否运行
uint8_t StepperMotor_IsRunning(void) {
    return isRunning;
}

// HAL库的定时器中断回调函数 (替代原来的 TIM3_IRQHandler)
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if(htim->Instance == TIM3) {
        if(isRunning) {
            // 1. 计算下一步的索引
            if(motorDirection == CLOCKWISE) {
                currentStep = (currentStep + 1) % 4;
            } else {
                currentStep = (currentStep == 0) ? 3 : (currentStep - 1);
            }
            
            // 2. 更新线圈状态 (解析序列并写入GPIO)
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN1, (fullStepSequence[currentStep] & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN2, (fullStepSequence[currentStep] & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN3, (fullStepSequence[currentStep] & 0x04) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            HAL_GPIO_WritePin(MOTOR_PORT, MOTOR_PIN4, (fullStepSequence[currentStep] & 0x08) ? GPIO_PIN_SET : GPIO_PIN_RESET);
            
            // 3. 步数控制
            if(stepsRemaining > 0) {
                stepsRemaining--;
                if(stepsRemaining == 0) {
                    StepperMotor_Stop();
                }
            }
        }
    }
}
