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
#include "button_queue.h"

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
 * @brief Callback chamada pelo HAL quando ocorre interrupção em pinos EXTI (botões).
 *
 * Esta função é chamada automaticamente pelo HAL dentro do handler da interrupção
 * do pino GPIO configurado para EXTI. Ou seja, ela é executada no contexto de uma ISR.
 *
 * @param[in] GPIO_Pin Identificador do pino que gerou a interrupção. Ex.: GPIO_PIN_12, GPIO_PIN_15, GPIO_PIN_5.
 *
 * @details
 * - O objetivo desta função é registrar eventos de botão sem executar código pesado dentro da ISR.
 * - Em sistemas RTOS, é importante que a ISR seja curta e rápida, para que outras interrupções
 *   e tasks possam ser atendidas sem atraso.
 *
 * Passos realizados:
 * 1. Identifica qual botão gerou o evento, com base no pino que gerou a interrupção.
 * 2. Lê o estado atual do pino (SET ou RESET) para determinar se ocorreu uma borda de subida
 *    (pressionamento/soltura do botão, dependendo da configuração do circuito e pull-up/pull-down).
 * 3. Cria uma estrutura `ButtonEvent_t` contendo:
 *    - `button_id`: identificador do botão
 *    - `rising_edge`: flag indicando borda de subida (1) ou descida (0)
 * 4. Envia a estrutura para a fila do FreeRTOS (`buttonQueueHandle`) usando
 *    `osMessageQueuePut()` com timeout 0, ou seja, não bloqueante.
 *
 * @note Nunca use funções bloqueantes dentro de ISR, como HAL_Delay() ou loops longos,
 *       pois isso pode travar o sistema ou atrasar outras interrupções.
 *
 * @note Se a fila estiver cheia, o evento será descartado. Em aplicações críticas,
 *       pode-se contar os eventos perdidos ou aumentar a profundidade da fila.
 *
 * @note O tratamento dos LEDs, debounce e lógica de controle são realizados em uma task separada,
 *       que retira os eventos da fila e processa de forma segura e eficiente.
 *
 * @warning Esta função não deve realizar delays nem operações demoradas, apenas empacotar
 *          o evento e enviar para a fila.
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

    /* Envia evento para a fila (não bloqueante dentro de ISR) */
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
