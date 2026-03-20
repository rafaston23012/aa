/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Ponto de entrada da aplicação com FreeRTOS e display OLED. É
  * 					a versão do BitDog_no_centro_do_OLED em RTOS
  * @author 		: Wu Shin Ting
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
  * Este projeto demonstra o uso de um sistema embarcado baseado em FreeRTOS
  * para exibição de uma imagem monocromática em um display OLED controlado
  * pelo driver SSD1306 via interface I2C.
  *
  * A aplicação segue uma arquitetura baseada em tarefas, onde a lógica de
  * exibição é delegada a uma thread específica (OledTask), permitindo
  * escalabilidade futura para inclusão de novos módulos (sensores, comunicação,
  * interface de usuário, etc.).
  *
  * O fluxo principal do sistema é:
  * - Inicialização de hardware (HAL + periféricos)
  * - Inicialização do display OLED
  * - Inicialização do kernel do FreeRTOS
  * - Criação das tarefas
  * - Execução do scheduler
  *
  * Após o início do scheduler, o controle do sistema deixa de ser sequencial
  * e passa a ser gerenciado pelo kernel do RTOS.
  *
  * @section user_code Código Implementado Manualmente
  *
  * As principais partes implementadas manualmente neste arquivo são:
  *
  * - Inclusão do driver do display que está no projeto hal_BDL_Library (veja
  *  a configuração em Properties > C/C++ Build > Settings):
  *   @code
  *   #include "ssd1306.h"
  *   @endcode
  *
  * - Inicialização do display OLED:
  *   @code
  *   ssd1306_Init();
  *   @endcode
  *
  * - Integração com o FreeRTOS:
  *   @code
  *   osKernelInitialize();
  *   MX_FREERTOS_Init();
  *   osKernelStart();
  *   @endcode
  *
  * Observação:
  * A lógica de renderização da imagem não está neste arquivo, mas sim
  * encapsulada na tarefa OledTask definida em freertos.c.
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * Uma forma simples de entender o funcionamento do sistema é:
  *
  * @code
  * [main()]
  *    |
  *    |--> Inicializa hardware (HAL, GPIO, I2C)
  *    |--> Inicializa display (SSD1306)
  *    |--> Inicializa RTOS
  *            |
  *            |--> Cria OledTask
  *            |
  *            --> Scheduler assume o controle
  *                         |
  *                         --> Executa OledTask em loop
  *                                |
  *                                --> Atualiza display OLED
  * @endcode
  *
  * Ou seja, após o scheduler iniciar:
  * - O main() deixa de executar lógica ativa
  * - Toda a aplicação passa a rodar dentro das tarefas do RTOS
  *
  * @section cube_config Configuração no CubeMX
  *
  * A configuração do projeto foi realizada utilizando o STM32CubeMX, com os
  * seguintes pontos principais:
  *
  * - Middleware:
  *   - FreeRTOS habilitado (CMSIS-RTOS v2)
  *
  * - Periféricos:
  *   - I2C3 configurado para comunicação com o display SSD1306
  *   - GPIO inicializado conforme necessário
  *
  * - Clock:
  *   - Configurado via SystemClock_Config()
  *
  * - Gerenciamento de código:
  *   - Separação entre código gerado automaticamente e USER CODE
  *   - Arquivo freertos.c responsável pelas tarefas
  *
  * @note
  * Qualquer alteração estrutural (periféricos, RTOS, clock) deve ser feita
  * preferencialmente via CubeMX para manter a consistência do projeto.
  *
  * @warning
  * Este projeto utiliza o FreeRTOS de forma propositalmente simplificada e
  * NÃO representa uma boa prática de uso do RTOS em sistemas embarcados reais.
  *
  * A tarefa criada (OledTask) executa em loop infinito redesenhando uma imagem
  * estática no display OLED, o que resulta em:
  * - uso desnecessário de CPU
  * - atualizações redundantes do display
  * - ausência de modelo orientado a eventos (event-driven)
  *
  * Em um projeto bem estruturado com RTOS, o display deveria ser atualizado:
  * - apenas quando houver mudança de estado
  * - a partir de eventos (filas, semáforos ou notificações)
  * - com possível compartilhamento seguro de recursos (ex: barramento I2C)
  *
  * Além disso, a renderização pixel a pixel não é eficiente e poderia ser
  * substituída por operações em bloco no framebuffer.
  *
  * Portanto, este projeto tem como objetivo apenas demonstrar:
  * - a migração de um projeto baseado em HAL para uma arquitetura com RTOS
  * - a criação e execução de tarefas no FreeRTOS
  *
  * Não deve ser utilizado como referência de arquitetura para aplicações finais.
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* Protótipos das funções relacionadas ao driver SSD1306 (OLED) */
#include "ssd1306.h"
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
  MX_I2C3_Init();
  /* USER CODE BEGIN 2 */
  /* Inicialiar o driver ssd1306 */
  ssd1306_Init();

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
