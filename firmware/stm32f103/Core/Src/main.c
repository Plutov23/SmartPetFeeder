/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "OLED_Menu.h"
#include "aht20.h"
#include "ds1302.h"
#include <stdio.h>
#include "buzzer.h"
#include "stepper_motor.h"
#include "HX711.h"
#include <string.h>
#include "ESP01S.h"
#include "SystemLog.h"
#include "schedule.h"
#include "maixcam.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// ==========================================================
// 【串口1】：电脑调试专用接收缓存
// ==========================================================
uint8_t debug_rx_char;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// 声明一个函数，用于处理从云平台收到的指令
extern void ESP_Handle_Recv_Data(char *recv_str);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// ==========================================================
// 【传感器全局变量】
// ==========================================================
float temp = 0.0f;
float humi = 0.0f;
long weight = 0;

// ==========================================================
// 【串口3】：ESP01S WiFi 专用接收缓冲
// ==========================================================
uint8_t wifi_rx_char;
char wifi_rx_buffer[256];
uint16_t wifi_rx_index = 0;
uint8_t wifi_rx_flag = 0;

// ==========================================================
// 【时间调度变量】
// ==========================================================
uint32_t last_mid_task = 0;
uint32_t last_upload_time = 0;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_TIM3_Init();
  MX_I2C1_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  APP_LOG("SYS", "Board power on...");

  // 初始化外设
  OLED_Menu_Init();
  AHT20_Init();
  DS1302_Init();
  //DS1302_SetTime(26, 4, 11, 17, 30, 10); 
  Schedule_Init();
  Buzzer_Init();
  StepperMotor_Init();

  // 开启串口1 (调试) 和 串口3 (WiFi) 的初始接收中断
  HAL_UART_Receive_IT(&huart1, &debug_rx_char, 1);
  HAL_UART_Receive_IT(&huart3, &wifi_rx_char, 1);

  APP_LOG("WEIGHT", "HX711 peeling start...");
  HX711_Init();

  APP_LOG("WIFI", "Connecting to OneNET...");
  ESP_Init();
  HAL_Delay(3000);

  // 开启串口2 (摄像头) 的初始接收中断
  APP_LOG("SYS", "Starting MaixCam UART...");
  // 开启前，先暴力清空一下刚才死等期间摄像头强塞进来的垃圾数据
  __HAL_UART_CLEAR_OREFLAG(&huart2);
  // 开启串口2 (摄像头)
  Maixcam_Init();

  APP_LOG("SYS", "--- Ready ---");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // 运行 MaixCam 视觉底层流转
    Maixcam_Run();

    uint32_t now = HAL_GetTick();

    // --------------------------------------------------------
    // 【1秒任务】：刷新屏幕、读时间、称重、检查喂食计划
    // --------------------------------------------------------
    if (now - last_mid_task >= 1000)
    {
      last_mid_task = now;

      DS1302_ReadAndPrintTime();
      weight = HX711_Get_Weight();

      uint8_t current_h = DS1302_GetHour();
      uint8_t current_m = DS1302_GetMinute();
      uint8_t current_s = DS1302_GetSecond();

      Schedule_CheckAndFeed(current_h, current_m);
      OLED_Show_Home(current_h, current_m, current_s, weight);
    }

    // --------------------------------------------------------
    // 【5秒任务】：读取温湿度、上传心跳数据 (防卡死隔离)
    // --------------------------------------------------------
    if (now - last_upload_time >= 5000)
    {
      last_upload_time = now;
      AHT20_Read(&temp, &humi);
      ESP_Publish_AllData(temp, humi, weight);
    }

    // --------------------------------------------------------
    // 【网络命令处理】：解析 WiFi 传来的完整数据包
    // --------------------------------------------------------
    if (wifi_rx_flag == 1)
    {
      ESP_Handle_Recv_Data(wifi_rx_buffer);
      // 处理完毕后，清空 WiFi 缓冲区
      memset(wifi_rx_buffer, 0, sizeof(wifi_rx_buffer));
      wifi_rx_index = 0;
      wifi_rx_flag = 0;
    }

    // ==========================================================
    // 🛡️ 【状态巡逻兵】：终极防死锁，保证摄像头数据永远能收进来
    // ==========================================================
    if (huart2.RxState != HAL_UART_STATE_READY && huart2.RxState != HAL_UART_STATE_BUSY_RX)
    {
      __HAL_UNLOCK(&huart2);
      huart2.RxState = HAL_UART_STATE_READY;
      HAL_UART_Receive_IT(Maixcam_GetUARTHandle(), Maixcam_GetRxCharPtr(), 1);
      APP_LOG("WARN", "USART2 RxState unlocked!");
    }

    // 加一点点呼吸延时，防止死循环跑满 CPU 导致中断进不去
    HAL_Delay(10);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// ==========================================================
// 【正常接收数据】：绝对不在这里处理错误！
// ==========================================================
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  // 1. 调试串口 (USART1)
  if (huart->Instance == USART1)
  {
    HAL_UART_Receive_IT(&huart1, &debug_rx_char, 1);
  }

  // 2. WiFi 串口 (USART3)
  else if (huart->Instance == USART3)
  {
    if (wifi_rx_char == '\r' || wifi_rx_char == '\n')
    {
      wifi_rx_buffer[wifi_rx_index] = '\0';
      wifi_rx_flag = 1; // 标记一句话接收完成
    }
    else
    {
      if (wifi_rx_index < 250)
      { // 加上边界保护，防越界崩溃
        wifi_rx_buffer[wifi_rx_index] = wifi_rx_char;
        wifi_rx_index++;
      }
    }
    HAL_UART_Receive_IT(&huart3, &wifi_rx_char, 1);
  }

  // 3. 摄像头串口 (USART2)
  else if (huart->Instance == USART2)
  {
    // 摄像头的正常接收逻辑：只管接，只管塞给解析器，然后极速重启接收
    uint8_t recv_ch = *Maixcam_GetRxCharPtr();
    Maixcam_UART_IRQHandler(recv_ch);
    HAL_UART_Receive_IT(Maixcam_GetUARTHandle(), Maixcam_GetRxCharPtr(), 1);
  }
}

// ==========================================================
// 【错误抢救中心】：只有发生溢出锁死时，才会进这里
// ==========================================================
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    // 暴力清空错误
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);

    // 强行重启接收中断
    HAL_UART_Receive_IT(Maixcam_GetUARTHandle(), Maixcam_GetRxCharPtr(), 1);

    // 打印 WARN 提示
    APP_LOG("WARN", "USART2 ORE Auto-Recovered!");
  }
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
