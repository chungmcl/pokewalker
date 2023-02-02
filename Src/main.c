/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "dma.h"
#include "eth.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Override printf() to huart3
int _write(int fd, char* ptr, int len) {
  // HAL_UART_Transmit(&huart3, (uint8_t *) ptr, len, HAL_MAX_DELAY);
  HAL_UART_Transmit(&huart3, (uint8_t *) ptr, len, 2000);
  return len;
}

void toggleGreen()  { HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);   }
void toggleRed()    { HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);       }
void toggleYellow() { HAL_GPIO_TogglePin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin); }

// #define HUART2_BUF_SIZE 256
#define HUART2_BUF_SIZE 16
#define READ_BUF_SIZE 16384
uint8_t* huart2_buf;
uint8_t* read_buf;
// uint8_t* read_buf_start;
uint8_t read_buf_start[READ_BUF_SIZE];
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

volatile uint8_t init_byte_received = 0;

uint8_t const priv_key[4] = {
  0x00,
  0x00,
  0x00,
  0x00
};

uint8_t handshake[8] = {
  0x50,
  0xAB,
  0xF3,
  0xE5,
  0x00,
  0x00,
  0x00,
  0x00
};

uint8_t request[8] = {
  0x8A,
  0xAB,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

typedef enum _action {
  NONE,
  START,
  CHECK_DEFAULT_PARAM,
  INIT_SESH_ID,
  VALIDATE_SESH_ID,
} action;

uint8_t session_id[4] = { 0x0, 0x0, 0x0, 0x0 };

action next_action = NONE;
action prev_action = NONE;
uint32_t global_idx = 0;
uint32_t next_process = 0;

uint16_t calculate_checksum(uint8_t* bytes, uint32_t len) {
  for (int i = 0; i < 4; i += 1) {
    bytes[i+4] = session_id[i] ^ 0xAA;
  }
  uint32_t sum = 0;
  for (int i = 0; i < len; i++) {
    // bytes 2 and 3 are actually the checksums themselves -- don't
    // include in calculation.
		uint16_t byte = (i == 2 || i == 3) ? 0x00 : 0xAA ^ bytes[i];
		
		if (!(i & 1))
			byte <<= 8;
		sum += byte;
	}
  while (sum >> 16) {
    sum = (uint16_t)sum + (sum >> 16);
  }
  sum += 2;
  bytes[2] = 0xAA ^ (uint8_t)sum;
  bytes[3] = 0xAA ^ (sum >> 8);
  return (uint16_t)sum;
}

uint8_t process(uint8_t byte) {
  if (prev_action == NONE) {
    if (byte == 0xF8) {
      next_action = CHECK_DEFAULT_PARAM;
      prev_action = START;
      return 1;
    } else {
      return 0;
    }
  } 

  if (next_action == CHECK_DEFAULT_PARAM && byte == 0x02) {
    init_byte_received = 99;
    if (prev_action == START) {
      next_action = INIT_SESH_ID;
    } else {
      next_action = VALIDATE_SESH_ID;
    }
    return 6;
  }

  if (next_action == INIT_SESH_ID) {
    uint8_t i = 0;
    for (uint8_t* p = read_buf - 4; p < read_buf; p += 1) {
      session_id[i] = *p ^ handshake[i+4] ^ 0xAA;
      i += 1;
    }
    session_id[i-1] = byte ^ handshake[i-1+4] ^ 0xAA;
    calculate_checksum(request, 8);
    HAL_Delay(2);
    HAL_UART_Transmit_DMA(&huart2, request, 8);
    //HAL_Delay(500);
  }

  return 0;
}

void copy(uint8_t* dest, uint8_t* src, size_t size) {
  uint32_t i = 0;
  while (i < size) {
    uint8_t byte = src[i] ^ 0xAA;
    if (global_idx == next_process)
      next_process = global_idx + process(byte);
    dest[i] = byte;
    i += 1;
    global_idx += 1;
  }
}

volatile uint8_t copy_buf_fl = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (!init_byte_received) {
    if (huart2_buf[0] == 0x56) {
      init_byte_received = 1;
    }
  } 
  else {
    copy_buf_fl = 1;
  }
}

volatile uint8_t copy_buf_hf = 0;
void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart) {
  copy_buf_hf = 1;
}

volatile uint8_t should_print = 0;
// Interrupt handler for GPIO inputs
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == GPIO_PIN_13) { // If The INT Source Is EXTI Line9 (A9 Pin)
    should_print = 1;
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
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USART6_UART_Init();
  MX_USB_OTG_HS_USB_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  huart2_buf = malloc(HUART2_BUF_SIZE);
  read_buf = read_buf_start;
  // HAL_UART_Receive_DMA(&huart2, huart2_buf, HUART2_BUF_SIZE);
  HAL_UART_Receive_IT(&huart2, huart2_buf, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    if (copy_buf_fl) {
      read_buf += HUART2_BUF_SIZE / 2;
      copy(read_buf - HUART2_BUF_SIZE / 2, huart2_buf + HUART2_BUF_SIZE / 2, HUART2_BUF_SIZE / 2);
      copy_buf_fl = 0;
      HAL_UART_Receive_DMA(&huart2, huart2_buf, HUART2_BUF_SIZE);
    }

    if (copy_buf_hf) {
      read_buf += HUART2_BUF_SIZE / 2;
      copy(read_buf - HUART2_BUF_SIZE / 2, huart2_buf, HUART2_BUF_SIZE / 2);
      copy_buf_hf = 0;
    }

    if (init_byte_received == 1) {
      HAL_Delay(2);
      HAL_UART_Transmit_DMA(&huart2, handshake, sizeof(handshake));
      if (init_byte_received == 1) {
        HAL_UART_Receive_DMA(&huart2, huart2_buf, HUART2_BUF_SIZE);
      }
      init_byte_received += 1;
    }

    if (should_print) {
      for (int i = 0; i < 4; i += 1) {
        printf("id[%d]: %02X\n", i, session_id[i]);
      }
      for (uint8_t* p = read_buf_start; p < read_buf; p += 1) {
        printf("%02X  %02X\n", *p, *p ^ 0xAA);
      }
      should_print = 0;
    }
    // PC13 == Blue User Button
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 275;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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
