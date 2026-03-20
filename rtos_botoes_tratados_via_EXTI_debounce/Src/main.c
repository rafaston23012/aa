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
  *
  * @section overview Visão Geral
  *
  * Esta aplicação demonstra a integração entre:
  *
  * - HAL (Hardware Abstraction Layer) → responsável por lidar com o hardware e interrupções
  * - FreeRTOS (via CMSIS-RTOS2) → responsável por organizar e coordenar a execução
  *
  * O sistema reage a eventos de três botões físicos utilizando interrupções (EXTI).
  * Cada evento é capturado rapidamente e enviado para uma fila do RTOS, onde será
  * processado posteriormente por uma tarefa.
  *
  * Esse modelo é conhecido como "Deferred Interrupt Processing (Processamento diferido de
  * interrupções)", ou seja:
  * - A interrupção apenas registra o evento
  * - A lógica pesada roda em contexto de tarefa
  *
  *
  * @section user_code Código Implementado Manualmente
  *
  * As seguintes partes foram implementadas manualmente pelo usuário:
  *
  * - HAL_GPIO_EXTI_Callback()
  *   → captura eventos de interrupção dos botões
  *   → empacota os dados em uma estrutura
  *   → envia para a fila do FreeRTOS
  *
  * - Estrutura ButtonEvent_t (definida em outro arquivo)
  *   → representa um evento de botão (qual botão + tipo de borda)
  *
  * - Uso da fila (buttonQueueHandle) para comunicação entre ISR e task
  *
  * Todo o restante (inicialização do HAL, clock, setup básico do RTOS)
  * foi gerado automaticamente pelo STM32CubeMX.
  *
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O sistema pode ser entendido como um pipeline de eventos:
  *
  * @code
  * [Botão físico]
  *        ↓
  * [Interrupção EXTI (hardware)]
  *        ↓
  * [HAL_GPIO_EXTI_Callback()  ← contexto de ISR]
  *        ↓
  * [Fila do FreeRTOS (queue)]
  *        ↓
  * [ButtonTask ← contexto de tarefa]
  *        ↓
  * [Atualização do LED RGB]
  * @endcode
  *
  * Características importantes:
  *
  * - Eventos podem ocorrer de forma concorrente (interrupções)
  * - A fila garante:
  *   - ordem de processamento
  *   - desacoplamento entre ISR e lógica
  *
  * - Apenas a task manipula o LED → evita condições de corrida
  * - ISR é rápida e não bloqueante
  *
  * Esse modelo é amplamente utilizado em sistemas embarcados reais.
  *
  *
  * @section cube_config Configuração no CubeMX
  *
  * No STM32CubeMX, foram configurados:
  *
  * GPIO:
  * - 3 pinos como entrada com interrupção (EXTI)
  * - Trigger por borda (subida, descida ou ambos)
  *
  * FreeRTOS:
  * - 1 fila (queue) para eventos de botão
  *   - profundidade: 8
  *   - tipo: ButtonEvent_t
  *
  * - 1 tarefa:
  *   - Nome: buttonTask
  *   - Prioridade: Normal
  *
  * NVIC:
  * - Interrupções EXTI habilitadas para os pinos dos botões
  *
  *
  * Observação didática:
  * Este projeto mostra uma evolução em relação ao polling:
  *
  * - rtos_botoes_com_debouncer: leitura contínua dos botões (polling)
  * - Agora: eventos assíncronos + RTOS
  *
  * Isso torna o sistema mais eficiente e escalável.
  *
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
 * @brief Callback de interrupção para pinos GPIO (EXTI).
 *
 * @param[in] GPIO_Pin Pino que gerou a interrupção.
 *
 * @details
 * Esta função é chamada automaticamente pelo HAL sempre que ocorre
 * uma interrupção em um pino configurado como EXTI.
 *
 * @attention Esta função executa em contexto de interrupção (ISR).
 *
 * Responsabilidades:
 * - Identificar qual botão gerou o evento
 * - Determinar o tipo de borda (subida/descida)
 * - Criar um evento (ButtonEvent_t)
 * - Enviar o evento para a fila do RTOS
 *
 * @warning O que NÃO fazer aqui:
 * - Não usar delays
 * - Não executar lógica complexa. Delegue-a para tarefas
 * - Não acessar recursos compartilhados diretamente
 * - Código bloqueante
 *
 * Integração HAL + RTOS:
 * - HAL detecta a interrupção
 * - RTOS organiza o processamento via fila
 *
 * Isso garante:
 * - segurança (sem race conditions)
 * - escalabilidade
 * - melhor desempenho
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    ButtonEvent_t evt;

    /* Identifica o botão que gerou o evento */
    switch (GPIO_Pin)
    {
        case GPIO_PIN_12:  /* Botão A */
            evt.button_id = 12;
            evt.rising_edge = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12) == GPIO_PIN_SET);
            break;

        case GPIO_PIN_15:  /* Botão B */
            evt.button_id = 15;
            evt.rising_edge = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_SET);
            break;

        case GPIO_PIN_5:   /* Botão C */
            evt.button_id = 5;
            evt.rising_edge = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET);
            break;

        default:
            return; /* pino desconhecido: ignora */
    }

    /* Envio não bloqueante para fila tratada por uma tarefa */
    (void)osMessageQueuePut(get_buttonQueueHandle(), &evt, 0U, 0U);
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
