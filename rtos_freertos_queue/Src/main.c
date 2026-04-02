/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body. Migração de uma implementação Pico (https://gitlab.unicamp.br/rtos_pico/freertos_queue.git)
  * 				  para BlackPill.
  * @author         : Wu Shin Ting
  * @date           : 02/04/2026
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
  * @section Descricao_Geral
  *
  * Projeto FreeRTOS que implementa comunicação entre tarefas utilizando Queue,
  * baseado no padrão de concorrência Producer–Consumer.
  *
  * O sistema controla uma matriz de LEDs WS2812 (NeoPixel), onde comandos de
  * atualização são gerados por uma tarefa produtora e consumidos por uma tarefa
  * consumidora.
  *
  * ---------------------------------------------------------------------------
  * @section Arquitetura
  *
  * O sistema é composto por:
  *
  * - Duas tarefas:
  *    • ProducerTask (Produtora)
  *    • ConsumerTask (Consumidora)
  *
  * - Um mecanismo de comunicação:
  *    • Queue (neopixelQueueHandle)
  *
  * - Um mecanismo de sincronização:
  *    • Event Flags (spi_dmaEventHandle)
  *
  * @section Modelo_Producer_Consumer
  *
  * O projeto utiliza o padrão Producer–Consumer para desacoplar a geração
  * de dados do seu processamento:
  *
  * - ProducerTask:
  *    • Gera dados (LED + cor aleatório)
  *    • Envia para a fila
  *
  * - ConsumerTask:
  *    • Aguarda dados da fila (bloqueante)
  *    • Processa e aplica nos LEDs
  *
  * Esse modelo:
  * - Evita acesso concorrente direto
  * - Permite escalabilidade
  * - Melhora organização do código
  *
  * @section Geracao_Aleatoria
  *
  * A tarefa produtora utiliza a função rand() para gerar valores pseudoaleatórios
  * que definem:
  * - O índice do LED (0–24)
  * - A cor a ser exibida (1–7)
  *
  * Para isso, é necessário incluir:
  * @code
  * #include <stdlib.h>
  * @endcode
  *
  * Inicialização do gerador:
  * - É utilizada a função srand() para definir a semente (seed)
  * - A semente é baseada no tempo de execução do sistema:
  *
  * @code
  * srand(HAL_GetTick());
  * @endcode
  *
  * Tipicamente, usa-se a função time() em time.h. Em sistemas embarcados, a função time() geralmente não
  * está disponível ou depende da implementação de syscalls (ex: _gettimeofday). O uso de
  * @code
  * HAL_GetTick()
  * @endcode
  * garante uma semente variável baseada no uptime do sistema, sem dependência de sistema operacional completo.
  *
  * @note
  * - rand() gera números pseudoaleatórios (determinísticos)
  * - Para aplicações críticas, recomenda-se uso do periférico RNG (Random Number Generator) para geração por hardware
  *
  * @warning
  * - rand() pode não ser thread-safe em ambientes concorrentes
  * - A qualidade da aleatoriedade depende da semente utilizada
  *
  * @section Fila_RTOS
  *
  * A fila (Queue) é responsável por:
  *
  * - Armazenar mensagens do tipo LedMsg_t
  * - Garantir ordem FIFO (First-In, First-Out)
  * - Permitir sincronização entre tarefas
  *
  * Operações utilizadas:
  * - osMessageQueuePut  → envio (Producer)
  * - osMessageQueueGet  → recepção (Consumer)
  *
  * Características:
  * - Comunicação segura entre tarefas
  * - Suporte a bloqueio e timeout
  * - Desacoplamento temporal entre produtor e consumidor
  *
  * @section Fluxo_de_Execucao
  *
  * Fluxo lógico do sistema:
  *
  * @code
  * ProducerTask
  *     ↓ (envio)
  * Queue (neopixelQueue)
  *     ↓ (recepção)
  * ConsumerTask
  *     ↓
  * Atualização dos LEDs
  * @endcode
  *
  * @section Fluxo_SPI_DMA
  *
  * A atualização dos LEDs utiliza SPI com DMA para maior eficiência:
  *
  * @code
  * ConsumerTask
  *     ↓
  * ws2812_spi_show_compartilhado()
  *     ↓
  * SPI + DMA (transmissão não bloqueante)
  *     ↓
  * Interrupção (HAL_SPI_TxCpltCallback)
  *     ↓
  * osEventFlagsSet()
  *     ↓
  * ConsumerTask (retoma execução)
  * @endcode
  *
  * Benefícios:
  * - Redução de uso da CPU
  * - Execução assíncrona
  * - Sincronização eficiente via Event Flags
  *
  * @section Funcionamento
  *
  * 1. A cada 1 segundo, a ProducerTask:
  *    - Gera aleatoriamente um LED (0–24)
  *    - Gera aleatoriamente uma cor (1–7)
  *    - Envia para a fila
  *
  * 2. A ConsumerTask:
  *    - Aguarda mensagem da fila
  *    - Atualiza os LEDs:
  *         • Apaga todos
  *         • Acende apenas o LED selecionado
  *
  * @section Cores
  *
  * - 1: Vermelho
  * - 2: Verde
  * - 3: Azul
  * - 4: Amarelo
  * - 5: Ciano
  * - 6: Magenta
  * - 7: Branco
  *
  * @section Observacoes
  *
  * - Após osKernelStart(), o controle é assumido pelo FreeRTOS
  * - O sistema não utiliza busy-wait
  * - A sincronização é feita via Queue e Event Flags
  *
  * @section Limitacoes
  *
  * - rand() não é adequado para aplicações críticas
  * - Não há tratamento de erro para fila cheia
  * - Buffer de LEDs não deve ser alterado durante DMA
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
osEventFlagsId_t getSpi_dmaEventHandle (); /*< Retorna o handle da event flag usada para sincronização ISR ↔ tarefa */
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
