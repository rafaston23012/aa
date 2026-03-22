/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Ponto de entrada da aplicação com FreeRTOS e controle de LEDs WS2812 via SPI + DMA.
  * 					Versão RTOS do projeto neopixel_mosi
  * @author         : Wu Shin Ting
  * @date           : 22/03/2026
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
  *
  * @section overview Visão Geral
  *
  * Este projeto implementa o controle de LEDs endereçáveis (NeoPixel - WS2812)
  * utilizando o periférico SPI em conjunto com DMA, sob gerenciamento de tarefas
  * via FreeRTOS (CMSIS-RTOS v2).
  *
  * O sistema utiliza uma única tarefa responsável por alternar entre dois padrões
  * visuais distintos de LEDs, transmitindo os dados através de um buffer compartilhado.
  *
  * A transmissão é realizada de forma não bloqueante via DMA, e a sincronização
  * entre CPU e periférico é feita por meio de eventos do RTOS.
  *
  * @section user_code Código Implementado Manualmente
  *
  * As principais implementações manuais incluem:
  *
  * - Integração SPI + DMA para geração do protocolo WS2812
  * - Implementação de buffer codificado para os LEDs
  * - Uso de FreeRTOS (CMSIS-RTOS v2):
  *   - Mutex para proteção de recurso compartilhado (dma_buffer)
  *   - Event Flags para sincronização com ISR
  * - Callback HAL para sinalização de término de transmissão
  *
  * Destaque:
  * - O buffer de DMA é compartilhado entre diferentes padrões de imagem
  * - A tarefa LedTask controla completamente o ciclo de renderização e envio
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O sistema pode ser entendido como uma pipeline controlada por estados:
  *
  * 1. A tarefa LedTask adquire o mutex (acesso exclusivo ao dma_buffer)
  *
  * 2. Um padrão de imagem é renderizado no buffer:
  *    - IMG_A ou IMG_B
  *
  * 3. A transmissão SPI via DMA é iniciada (não bloqueante)
  *
  * 4. O periférico SPI + DMA passa a consumir o buffer
  *
  * 5. Ao término da transmissão:
  *    - A ISR (HAL_SPI_TxCpltCallback) é executada
  *    - Uma flag de evento é setada (osEventFlagsSet)
  *
  * 6. A tarefa LedTask:
  *    - Aguarda essa flag (osEventFlagsWait)
  *    - Após o sinal, sabe que o DMA terminou
  *
  * 7. O mutex é liberado com segurança
  *
  * 8. O próximo padrão é preparado
  *
  * Fluxo simplificado:
  *
  * @code
  * Task → Lock → Render → Start DMA → Wait Event → Unlock → Repeat
  * ISR  → (DMA complete) → Set Event Flag
  * @endcode
  *
  * @warning
  * - Não é permitido liberar mutex dentro de ISR
  * - Por isso, utiliza-se Event Flags como mecanismo intermediário
  *
  * Isso garante:
  * - Exclusão mútua correta
  * - Ausência de race conditions no buffer
  * - Sincronização segura entre contexto de interrupção e tarefa
  *
  * @section cube_config Configuração no CubeMX
  *
  * O projeto foi configurado no STM32CubeMX com os seguintes pontos principais:
  *
  * - SPI5:
  *   - Configurado para transmissão de dados para os LEDs
  *   - Operando com suporte a DMA
  *
  * - DMA:
  *   - Canal associado ao SPI5 TX
  *   - Modo normal (não circular)
  *
  * - FreeRTOS (CMSIS-RTOS v2):
  *   - 1 tarefa (LedTask)
  *   - 1 mutex (dmaBufferMutex)
  *   - 1 event flag (spi_dmaEvent)
  *
  * - GPIO:
  *   - Configurado conforme necessário para SPI
  *
  * Estratégia adotada:
  * - Um único dma_buffer compartilhado
  * - Controle de acesso via mutex
  * - Sincronização com DMA via event flags
  *
  * Decisão de projeto:
  * - Evitar busy-wait
  * - Evitar liberação de mutex em ISR
  * - Garantir integridade do buffer durante transmissão
  *
  ******************************************************************************
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "spi.h"
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
osEventFlagsId_t getSpi_dmaEventHandle ();
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_SPI5_Init();
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 128;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV4;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
  * @brief Callback de conclusão de transmissão SPI via DMA.
  *
  * Esta função é chamada automaticamente pela HAL ao término de uma
  * transferência SPI iniciada com @c HAL_SPI_Transmit_DMA().
  *
  * @param hspi Ponteiro para a estrutura SPI.
  *
  * @retval None
  *
  * @details
  * - Verifica se a interrupção corresponde ao periférico SPI5.
  * - Libera o estado de ocupação do DMA, sinalizando que uma nova
  *   transmissão pode ser iniciada.
  *
  * Este mecanismo implementa uma forma simples de sincronização entre
  * código principal e periférico, baseada em flag global.
  *
  * @note
  * - Executa em contexto de interrupção.
  * - Deve ser rápida e não bloqueante.
  * - Pode ser substituída por mecanismos RTOS (event flags) para
  *   maior robustez em sistemas concorrentes.
  */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi){
    if (hspi->Instance == SPI5)
    {
    	osEventFlagsSet(getSpi_dmaEventHandle(), 1U);
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
