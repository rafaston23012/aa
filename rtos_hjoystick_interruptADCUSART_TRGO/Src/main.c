/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Este projeto é uma versão RTOS do hjoystick_interruptADCUSART_TRGO.
  * 				Arquivo principal da aplicação embarcada baseada em FreeRTOS
  * 				com aquisição ADC e comunicação UART.
  * @author			: Wu Shin Ting
  * @date			: 20/03/2026
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
  * @section overview Visão Geral
  *
  * Este projeto implementa um sistema embarcado baseado em RTOS para aquisição periódica
  * de sinais analógicos utilizando o ADC, seguido de processamento e transmissão dos dados via USART.
  *
  * A arquitetura é baseada no modelo produtor-consumidor:
  * - O ADC (via interrupção) atua como produtor de dados
  * - Uma task RTOS processa e encaminha os dados para uma fila
  * - Outra task consome os dados e realiza a transmissão via UART
  *
  * O sistema utiliza interrupções e comunicação assíncrona para garantir eficiência,
  * evitando bloqueios e reduzindo uso de CPU.
  *
  * @section user_code Código Implementado Manualmente
  *
  * As principais partes implementadas manualmente incluem:
  *
  * - Callback de conversão ADC (@c HAL_ADC_ConvCpltCallback):
  *   - Alterna dinamicamente entre os canais 6 e 9
  *   - Armazena amostras
  *   - Sinaliza eventos para sincronização com tasks
  *
  * - Callback de transmissão UART (@c HAL_UART_TxCpltCallback):
  *   - Indica término de envio
  *   - Libera fluxo para nova transmissão
  *
  * - Tasks do FreeRTOS:
  *   - @c AdcTask:
  *     - Inicializa ADC e TIM3
  *     - Aguarda evento de conversão
  *     - Envia amostras para fila
  *
  *   - @c UsartTask:
  *     - Consome dados da fila
  *     - Converte valores em representação ASCII (barra)
  *     - Realiza transmissão não bloqueante via UART
  *
  * - Mecanismos de sincronização:
  *   - Event Flags para ADC e UART
  *   - Fila RTOS para comunicação entre tasks
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O sistema pode ser entendido como um pipeline reativo orientado a eventos:
  *
  * @code
  *        TIM3 (hardware trigger)
  *                 │
  *                 ▼
  *         ADC (scan 2 canais)
  *                 │
  *                 ▼
  *     Interrupt (EOC - cada conversão)
  *                 │
  *                 ▼
  *   HAL_ADC_ConvCpltCallback (ISR)
  *       ├── Alterna canal (6 ↔ 9)
  *       ├── Armazena amostra parcial
  *       └── Sinaliza evento (quando necessário)
  *                 │
  *                 ▼
  *            AdcTask
  *       ├── Lê amostra completa
  *       └── Envia para fila
  *                 │
  *                 ▼
  *           sampleQueue
  *                 │
  *                 ▼
  *           UsartTask
  *       ├── Formata dados (ASCII bar)
  *       └── Transmite via UART (IT)
  *                 │
  *                 ▼
  *   HAL_UART_TxCpltCallback (ISR)
  *       └── Libera próxima transmissão
  * @endcode
  *
  * Características importantes:
  * - Totalmente não bloqueante
  * - Orientado a interrupções
  * - Separação clara entre aquisição e comunicação
  *
  * @section cube_config Configuração no CubeMX
  *
  * A configuração do sistema foi realizada utilizando o STM32CubeMX com os seguintes pontos:
  *
  * - ADC:
  *   - Modo scan habilitado
  *   - Conversão de 2 canais:
  *     - IN6 (ex: eixo Y)
  *     - IN9 (ex: eixo X)
  *   - Interrupção de fim de conversão (EOC) habilitada para cada conversão individual
  *
  * - Trigger:
  *   - Disparo externo por hardware via TIM3 (TRGO)
  *   - Conversões ocorrem periodicamente sem intervenção da CPU
  *
  * - Inicialização:
  *   - O ADC é iniciado apenas uma vez com:
  *     @code
  *     HAL_ADC_Start_IT(&hadc1);
  *     @endcode
  *
  *   - Após isso, o TIM3 controla continuamente o início das conversões
  *
  * - Timer (TIM3):
  *   - Configurado como fonte de trigger do ADC
  *   - Define a frequência de amostragem do sistema
  *
  * - UART:
  *   - Operando em modo interrupção (@c HAL_UART_Transmit_IT)
  *
  *
  * @note
  * - O uso de interrupções e RTOS permite alta eficiência e escalabilidade
  * - O sistema evita polling e bloqueios ativos
  *
  * @warning
  * - Código em callbacks deve ser rápido (executa em contexto de interrupção)
  * - A sincronização entre tasks depende corretamente das event flags
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
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
SamplePair_t amostra;		/**< par de sinais (x,y) amostrados por ADC */
volatile uint8_t adc_channel = 9;  /**< Canal ADC atualmente em conversão (começa com PB1) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void setaAdcEventFlag ();   /**< seta flag de adcEvent */
void setaUsartEventFlag (); /**< seta flag de usartEvent */
void le_amostras();         /**< le par de sinais amostrados nos canais IN9 e IN6 */
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
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
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
  * @brief Callback de término de conversão ADC. Esta função é chamada automaticamente pela HAL
  * ao término de cada conversão ADC em modo interrupção.
  *
  * O comportamento implementado alterna dinamicamente entre os canais:
  * - ADC_CHANNEL_6 (eixo Y)
  * - ADC_CHANNEL_9 (eixo X)
  *
  * Funcionamento:
  * - Armazena a amostra atual
  * - Reconfigura o próximo canal
  * - Ao completar o par (ch6 + ch9):
  *   - envia os dados para a fila RTOS
  *   - sinaliza a task via event flag
  *
  * @param hadc Ponteiro para a estrutura do ADC
  * @retval None
  *
  * @note
  * - A função deve ser rápida (executa em contexto de interrupção)
  * - Não realiza bloqueios
  * - Utiliza comunicação assíncrona via fila
  */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        /* Alterna entre os canais 6 (EIXO_Y) e 9 (EIXO_X) */
        if(adc_channel == 6)
        {
            amostra.ch6 = HAL_ADC_GetValue(hadc);  /**< Lê o valor convertido */

            setaAdcEventFlag();

            adc_channel = 9;
        }
        else
        {
            amostra.ch9 = HAL_ADC_GetValue(hadc);  /**< Lê o valor convertido */

            adc_channel = 6;
        }
    }
}

/**
 * @brief Callback de término de transmissão UART (modo interrupção).
 *
 * Esta função é chamada automaticamente pela HAL ao final de uma transmissão
 * iniciada com @c HAL_UART_Transmit_IT().
 *
 * @details
 * O callback é utilizado para sinalizar que o periférico USART concluiu o envio
 * completo do buffer, permitindo que o sistema:
 * - Libere o canal de transmissão
 * - Sincronize tasks dependentes
 * - Inicie uma nova transmissão com segurança
 *
 * No contexto desta aplicação:
 * - Verifica se a interrupção pertence à USART1
 * - Sinaliza a conclusão da transmissão através de uma event flag RTOS
 *
 * Fluxo típico:
 * @code
 * UsartTask
 *    └── HAL_UART_Transmit_IT()
 *            └── (envio via ISR interna da HAL)
 *                    └── HAL_UART_TxCpltCallback()
 *                            └── setaUsartEventFlag()
 *                                    └── Libera próxima transmissão
 * @endcode
 *
 * @param phuart Ponteiro para a estrutura de controle da UART que gerou o evento
 * @retval None
 *
 * @note
 * - Executa em contexto de interrupção (ISR)
 * - Deve ser rápida e não bloqueante
 * - Não deve conter chamadas que possam bloquear (ex: delays, filas com espera)
 *
 * @warning
 * - Caso múltiplas UARTs sejam utilizadas, é essencial validar @c phuart->Instance
 *   para evitar interferência entre periféricos
 *
 * @see HAL_UART_Transmit_IT
 * @see setaUsartEventFlag
 */
void HAL_UART_TxCpltCallback (UART_HandleTypeDef *phuart)
{
    if (phuart->Instance == USART1)  // garante que é a UART correta
    {
        setaUsartEventFlag ();
    }
}

/**
 * @brief Copia a última amostra ADC adquirida para uma estrutura fornecida.
 *
 * @details
 * Esta função realiza a leitura da última amostra disponível, armazenada na
 * variável global @c amostra, e copia seus valores para a estrutura apontada
 * por @p a.
 *
 * A estrutura @c SamplePair_t contém os valores correspondentes aos canais:
 * - @c ch6 → Canal ADC IN6 (ex: eixo Y)
 * - @c ch9 → Canal ADC IN9 (ex: eixo X)
 *
 * Esta função é utilizada para desacoplar o acesso à variável global,
 * permitindo que outras partes do sistema obtenham os dados de forma
 * controlada e organizada.
 *
 * @param[out] a Ponteiro para a estrutura que receberá os valores da amostra
 * @retval None
 *
 * @note
 * - A função realiza apenas cópia de dados (sem bloqueio)
 * - Pode ser chamada em contexto de task
 *
 * @warning
 * - Não há proteção contra condição de corrida (race condition):
 *   a variável global @c amostra pode ser atualizada por uma ISR
 *   (@c HAL_ADC_ConvCpltCallback) durante a cópia
 * - Caso consistência total seja necessária, deve-se utilizar mecanismos
 *   de sincronização (ex: desabilitar interrupções temporariamente ou usar mutex)
 *
 * @see HAL_ADC_ConvCpltCallback
 * @see SamplePair_t
 */
void le_amostra (SamplePair_t *a)
{
    a->ch6 = amostra.ch6;
    a->ch9 = amostra.ch9;
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
