/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Este projeto realiza as mesmas tarefas do projeto "hjoystick_interruptADCUSART"
  * 				  Os códigos foram desenvolvidos em cima dos códigos gerados pelo STM32CubeMX.
  * @author			: Wu Shin Ting
  * @date			: 19/03/2026
  *
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
  * Este projeto implementa um sistema embarcado baseado em RTOS no qual os
  * periféricos ADC e USART são modelados segundo o padrão produtor-consumidor.
  *
  * - O ADC atua como produtor de dados, realizando conversões em dois canais:
  *   - IN6 (eixo Y)
  *   - IN9 (eixo X)
  *
  * - A USART atua como consumidora, sendo responsável por transmitir os dados
  *   adquiridos para um Terminal Serial.
  *
  * A comunicação entre produtor e consumidor é realizada por meio de uma fila
  * (queue), garantindo desacoplamento temporal e estrutural entre aquisição e
  * transmissão.
  *
  * Para preservar o disparo por software das conversões ADC em dois canais
  * distintos, utiliza-se um mecanismo de sincronização baseado em flags de
  * evento. Esse mecanismo coordena:
  * - O callback @ref HAL_ADC_ConvCpltCallback, que indica o término de uma
  *   conversão individual;
  * - A task responsável pelo ADC (AdcTask), que dispara a aquisição de um novo
  *   par de amostras (eixo X, eixo Y).
  *
  * Dessa forma, garante-se que cada ciclo completo de aquisição (par de canais)
  * seja tratado como uma unidade lógica antes de iniciar um novo ciclo.
  *
  * @section user_code Código Implementado Manualmente
  *
  * Os principais elementos implementados manualmente são:
  * - Estrutura @c SamplePair_t para armazenar pares de amostras (IN6, IN9)
  * - Callback @ref HAL_ADC_ConvCpltCallback para:
  *   - alternar dinamicamente o canal ADC
  *   - montar pares de amostras
  *   - enviar dados para a fila
  * - Uso de:
  *   - @c osMessageQueuePut para comunicação com tasks
  *   - função @c setaAdcEventFlag para sincronização com a AdcTask
  *
  * O sistema evita processamento pesado em interrupções, delegando o fluxo
  * principal de controle para tarefas RTOS.
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O comportamento do sistema pode ser descrito pelo seguinte fluxo:
  *
  * @code
  * [AdcTask]
  *     ↓ (disparo de conversão)
  * [ADC]
  *     ↓ (interrupção)
  * [HAL_ADC_ConvCpltCallback]
  *     ↓
  * [montagem parcial ou completa do par]
  *     ↓
  * [fila RTOS]
  *     ↓
  * [Task consumidora (ex: USART)]
  * @endcode
  *
  * E para sincronização do ciclo completo:
  *
  * @code
  * Callback → seta flag → AdcTask libera novo ciclo
  * @endcode
  *
  * Esse modelo garante:
  * - desacoplamento entre aquisição e processamento
  * - previsibilidade temporal
  * - ausência de polling
  *
  * @section cube_config Configuração no CubeMX
  *
  * As principais configurações realizadas no CubeMX incluem:
  *
  * - ADC:
  *   - Modo com interrupção (IT)
  *   - Um canal ativo por vez (reconfigurado dinamicamente)
  *
  * - USART:
  *   - Configurada para transmissão de dados
  *
  * - FreeRTOS (CMSIS-v2):
  *   - Uso de filas (message queues)
  *   - Uso de event flags para sincronização
  *
  * - Clock:
  *   - Fonte externa (HSE) com PLL habilitado
  *
  * - GPIO:
  *   - Configuração dos pinos analógicos e UART
  *
  * A separação entre código gerado e código do usuário é mantida por meio
  * das regiões @c USER CODE, garantindo compatibilidade com regeneração automática.
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
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
SamplePair_t amostra;
volatile uint8_t adc_channel = 9;  /**< Canal ADC atualmente em conversão (começa com PB1) */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void setaAdcEventFlag ();
void setaUsartEventFlag ();
void le_amostras();
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
		uint16_t value = HAL_ADC_GetValue(hadc);  /**< Lê o valor convertido */

		/* Alterna entre os canais 6 (EIXO_Y) e 9 (EIXO_X) */
		if(adc_channel == 6)
		{
			amostra.ch6 = value;

			/**
			 * @brief Configuração do próximo canal: ADC_CHANNEL_9
			 *
			 * IMPORTANTE:
			 * A estrutura é inicializada com zero para evitar lixo de memória.
			 * Em seguida, TODOS os campos relevantes são explicitamente definidos.
			 */
			ADC_ChannelConfTypeDef sConfig = {0};

			sConfig.Channel      = ADC_CHANNEL_9;               /**< Seleção do canal físico */
			sConfig.Rank         = 1;                           /**< Rank 1 na sequência regular */
			sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;    /**< Tempo de amostragem */
			sConfig.Offset       = 0;                           /**< Offset digital */

			HAL_ADC_ConfigChannel(hadc, &sConfig);

			setaAdcEventFlag();

			adc_channel = 9;
		}
		else
		{
			amostra.ch9 = value;

			/**
			 * @brief Configuração do próximo canal: ADC_CHANNEL_6
			 *
			 * Todos os campos da estrutura são explicitamente configurados
			 * para garantir previsibilidade do hardware.
			 */
			ADC_ChannelConfTypeDef sConfig = {0};

			sConfig.Channel      = ADC_CHANNEL_6;
			sConfig.Rank         = 1;
			sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;
			sConfig.Offset       = 0;

			HAL_ADC_ConfigChannel(hadc, &sConfig);

			adc_channel = 6;

			/**< Reinicia a conversão em modo interrupção */
			HAL_ADC_Start_IT(hadc);
		}
	}
}

/**
  * @brief Callback de término de transmissão UART.
  *
  * Chamado quando uma transmissão via USART é concluída.
  *
  * Pode ser utilizado para:
  * - liberar buffers
  * - sinalizar tasks
  * - iniciar nova transmissão
  *
  * @param phuart Ponteiro para a UART
  * @retval None
  */
void HAL_UART_TxCpltCallback (UART_HandleTypeDef *phuart) {
    if (phuart->Instance == USART1)  // garante que é a UART correta
    {
        setaUsartEventFlag();
    }
}


void le_amostras (SamplePair_t *par) {
	par->ch6 = amostra.ch6;
	par->ch9 = amostra.ch9;
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
