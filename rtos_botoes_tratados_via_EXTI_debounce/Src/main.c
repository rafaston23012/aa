/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Este projeto realiza as mesmas tarefas do projeto "botoes_tratados_via_EXTI_debounce"
  * 				com DEBOUNCER.
  * 				Os códigos foram desenvolvidos em cima dos códigos gerados pelo STM32CubeMX.
  * @author			: Wu Shin Ting
  * @date			: 26/01/2026
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
#include "cmsis_os.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief Ponto de entrada da aplicação (função principal).
  *
  * @details
  * Esta função é gerada pelo STM32CubeMX/STM32CubeIDE e representa o fluxo
  * padrão de inicialização de um firmware STM32 usando HAL + FreeRTOS.
  *
  * Para quem está tendo o primeiro contato com FreeRTOS, é importante entender
  * que o programa possui duas fases bem definidas:
  *
  * 1) Fase "Bare-metal" (antes do FreeRTOS iniciar)
  * Nesta fase, ainda NÃO existe multitarefa. O código roda como em um firmware
  * tradicional: uma instrução após a outra.
  *
  * Aqui acontecem:
  * - inicialização do microcontrolador (HAL_Init)
  * - configuração do clock (SystemClock_Config)
  * - inicialização dos periféricos (MX_GPIO_Init, MX_USART_Init, etc.)
  *
  * - Nessa etapa você ainda pode usar delays do HAL (ex.: HAL_Delay).
  * - Ainda não existem tasks, escalonador, semáforos, filas, etc.
  *
  * 2) Fase "RTOS" (após o scheduler iniciar)
  * Depois que o FreeRTOS é inicializado e o scheduler é iniciado, o controle do
  * programa passa a ser do sistema operacional de tempo real.
  *
  * A partir desse ponto:
  * - o FreeRTOS passa a decidir qual task executa
  * - as tasks podem ser preemptadas (interrompidas) para outra task rodar
  * - delays e temporizações devem ser feitos com funções do RTOS
  *   (ex.: osDelay / vTaskDelay), e não com loops de espera
  *
  * Importante:
  * Após chamar osKernelStart(), a função main() não deve mais executar lógica
  * da aplicação. Toda a lógica do programa deve estar dentro das tasks criadas
  * em MX_FREERTOS_Init() (arquivo freertos.c).
  *
  * - Se houver inicializações extras (antes do RTOS iniciar), coloque o seu sódigo em
  *     `USER CODE BEGIN 2`
  * - Lógica de aplicação: dentro das tasks do FreeRTOS (em freertos.c)
  *
  * @retval int Código de retorno (não utilizado em aplicações embarcadas).
  */

/* USER CODE END 0 */

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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief Callback de interrupção externa (EXTI) para tratamento de eventos em GPIO.
 *
 * Esta função é chamada automaticamente pela HAL quando ocorre uma interrupção EXTI
 * em um pino GPIO configurado para gerar interrupção. O parâmetro @p GPIO_Pin
 * identifica qual pino gerou o evento.
 *
 * Neste projeto, a rotina implementa o seguinte mapeamento:
 * - Botao_A_Pin (PB12) alterna o estado do LED vermelho (LED_R_Pin)
 * - Botao_B_Pin (PA15) alterna o estado do LED azul    (LED_B_Pin)
 * - Botao_C_Pin (PB5)  alterna o estado do LED verde   (LED_G_Pin)
 *
 * @param GPIO_Pin Identificador do pino que gerou a interrupção EXTI.
 * @return void
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  switch (GPIO_Pin)
  {
    case Botao_A_Pin:   /* PB12 → LED_R */
      HAL_GPIO_TogglePin(GPIOA, LED_R_Pin);
      break;

    case Botao_B_Pin:   /* PA15 → LED_B */
      HAL_GPIO_TogglePin(GPIOA, LED_B_Pin);
      break;

    case Botao_C_Pin:    /* PB5 → LED_G */
      HAL_GPIO_TogglePin(GPIOA, LED_G_Pin);
      break;

    default:
      break;
  }
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
