/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "app_fatfs.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "app_fatfs.h"
#include "st7735.h"
#include <string.h>
#include <math.h>
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
/* ตัวแปรเช็ก SD Card */
FATFS fs;
FIL fil;
FRESULT fres_mount = 99;
FRESULT fres_open  = 99;
UINT bytes_read    = 0;
char sd_buffer[64];

/* ตัวแปรเช็กหน้าจอ 2 จอ */
volatile uint8_t lcd_status = 0;
/* ตารางเก็บคลื่น Sine Wave 32 จุด (ที่ 16kHz จะได้เสียงความถี่ 500 Hz) */
uint16_t sine_table[32];
volatile uint8_t sine_idx = 0;

/* ตัวแปรสำหรับดูค่าเสียงจากไมโครโฟนใน Live Expressions */
volatile uint16_t mic_raw = 0;      // ค่าดิบจาก ADC (0 - 4095) เงียบๆ จะอยู่แถว ~2048
volatile uint16_t mic_peak = 0;     // ความดังสูงสุดตอนพูดใส่ไมค์ (0 - 2048)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* ฟังก์ชันนี้จะถูกเรียกอัตโนมัติทุกๆ 1/16000 วินาที เพื่อส่งคลื่น Sine ออกขา PA6 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6)
    {
        HAL_DAC_SetValue(&hdac2, DAC_CHANNEL_1, DAC_ALIGN_12B_R, sine_table[sine_idx]);
        sine_idx++;
        if (sine_idx >= 32) sine_idx = 0;
    }
}
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC2_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  MX_TIM1_Init();
  MX_TIM4_Init();
  MX_USART1_UART_Init();
  MX_TIM8_Init();
  if (MX_FATFS_Init() != APP_OK) {
    Error_Handler();
  }
  MX_TIM6_Init();
  MX_ADC5_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(500); // รอไฟเลี้ยงอุปกรณ์ทุกชิ้นนิ่ง

  /* --- 1. ทดสอบอ่านไฟล์ test.txt จาก SD Card (SPI2) --- */
  memset(sd_buffer, 0, sizeof(sd_buffer));
  fres_mount = f_mount(&fs, "", 1);
  if (fres_mount == FR_OK) {
      fres_open = f_open(&fil, "test.txt", FA_READ);
      if (fres_open == FR_OK) {
          f_read(&fil, sd_buffer, sizeof(sd_buffer) - 1, &bytes_read);
          f_close(&fil);
      }
  }

  /* --- 2. ทดสอบแสดงผลบนหน้าจอคู่ ST7735 ทั้ง 2 จอ (SPI3) --- */
  lcd_status = 1; // เริ่มต้น Init จอ
  ST7735_Init(1); // จอซ้าย
  ST7735_Init(2); // จอขวา

  lcd_status = 2; // เคลียร์พื้นหลังสีดำทั้ง 2 จอ
  ST7735_FillScreen(1, ST7735_BLACK);
  ST7735_FillScreen(2, ST7735_BLACK);

  /* วาดตาเพนกวิน: จอซ้ายสีฟ้า / จอขวาสีเหลือง */
  ST7735_FillRoundRect(1, 34, 24, 60, 80, 18, ST7735_CYAN);
  ST7735_FillRoundRect(2, 34, 24, 60, 80, 18, ST7735_YELLOW);

  /* จุดสีเขียวมุมซ้ายบนของจอที่ 1 = อ่าน SD Card สำเร็จ / จุดสีแดง = อ่านไม่ได้ */
  if (fres_mount == FR_OK) {
      ST7735_FillCircle(1, 12, 12, 6, ST7735_GREEN);
  } else {
      ST7735_FillCircle(1, 12, 12, 6, ST7735_RED);
  }

  lcd_status = 3; // ทำงานครบทุกขั้นตอนสมบูรณ์
  // สร้างตารางคลื่น Sine 12-bit (ค่าแกว่งระหว่าง 0 ถึง 4095 กึ่งกลางที่ 2048)
  for (int i = 0; i < 32; i++) {
      sine_table[i] = (uint16_t)(2048.0f + 1800.0f * sinf(2.0f * 3.1415926f * i / 32.0f));
  }

  // เริ่มปล่อยสัญญาณเสียง Sine Wave 500 Hz ออกขา PA6 (ลำโพง)
  HAL_DAC_Start(&hdac2, DAC_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim6);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // อ่านค่าแรงดันเสียงจากไมโครโฟน (ขา PA0 / ADC1)
	  HAL_ADC_Start(&hadc1);
	  if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
	      mic_raw = HAL_ADC_GetValue(&hadc1);

	      // คำนวณความแรงเสียงเทียบกับจุดกึ่งกลาง (2048)
	      uint16_t amp = (mic_raw > 2048) ? (mic_raw - 2048) : (2048 - mic_raw);
	      if (amp > mic_peak) mic_peak = amp; // เก็บค่าสูงสุดที่พูดดังสุด
	  }
	  HAL_ADC_Stop(&hadc1);
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
