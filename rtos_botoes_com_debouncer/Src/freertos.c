/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file       : freertos.c
  * @brief      : Controle de LED RGB por Botões (versão RTOS de botoes_com_debouncer)
  * @author 	: Wu Shin Ting
  * @date		: 19/03/2026
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
  * Este arquivo contém a configuração e implementação de uma aplicação simples
  * utilizando FreeRTOS em um microcontrolador STM32.
  *
  * A maior parte da estrutura (inicialização do kernel, criação de tarefas, etc.)
  * foi gerada automaticamente pelo STM32CubeMX.
  *
  * @section user_code Código Implementado Manualmente
  *
  * As seguintes funções foram implementadas manualmente:
  * - AllLedsOff()
  * - SetLedOnly()
  * - ButtonTask()
  *
  * Todas as demais estruturas e funções foram geradas automaticamente pelo CubeMX.
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * Apesar de utilizar FreeRTOS, o sistema foi modelado de forma **não concorrente**,
  * com apenas uma única tarefa responsável por toda a lógica da aplicação.
  *
  * A tarefa ButtonTask executa continuamente um loop infinito realizando:
  *
  * 1. Leitura sequencial (polling) dos três botões
  * 2. Decisão de qual LED deve ser acionado
  * 3. Atualização do LED RGB
  *
  * Características importantes do modelo:
  *
  * - Não há concorrência entre tarefas
  * - Não há uso de filas, semáforos ou mutexes
  * - A lógica é determinística e sequencial
  * - O sistema se comporta como um "superloop", porém encapsulado em uma tarefa RTOS
  *
  * @section cube_config Configuração no CubeMX
  *
  * No STM32CubeMX, o sistema foi definido como:
  *
  * - Uma única tarefa: "buttonTask"
  * - Prioridade: Normal
  * - Stack: 512 bytes
  *
  * Essa abordagem é útil para introdução ao FreeRTOS, permitindo focar:
  *
  * - No conceito de tarefas
  * - Na estrutura básica do sistema operacional
  *
  * antes de evoluir para sistemas concorrentes mais complexos.
  *
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
 * @brief Tempo de debounce (em milissegundos) para filtragem do botão.
 */
#define DEBOUNCE_MS  20

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for buttonTask */
osThreadId_t buttonTaskHandle;
const osThreadAttr_t buttonTask_attributes = {
  .name = "buttonTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void ButtonTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of buttonTask */
  buttonTaskHandle = osThreadNew(ButtonTask, NULL, &buttonTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_ButtonTask */
/* ===================== FUNÇÕES AUXILIARES ===================== */
/**
 * @brief Desliga todos os LEDs do LED RGB.
 *
 * @note Função implementada manualmente pelo usuário.
 */
static void AllLedsOff(void)
{
    HAL_GPIO_WritePin(GPIOA, LED_R_Pin | LED_B_Pin | LED_G_Pin, GPIO_PIN_RESET);
}

/**
 * @brief Liga apenas um LED específico, desligando os demais.
 *
 * @param led_pin Pino correspondente ao LED a ser ligado
 *
 * @note Função implementada manualmente pelo usuário.
 */
static void SetLedOnly(uint16_t led_pin)
{
    AllLedsOff();
    HAL_GPIO_WritePin(GPIOA, led_pin, GPIO_PIN_SET);
}

/**
  * @brief Tarefa responsável pela leitura dos botões e controle do LED RGB.
  *
  * Esta tarefa executa continuamente um loop infinito realizando polling
  * dos botões e atualizando o LED correspondente.
  *
  * Lógica de funcionamento:
  * - Botão A pressionado → LED vermelho
  * - Botão B pressionado → LED azul
  * - Botão C pressionado → LED verde
  * - Nenhum botão → todos LEDs apagados
  *
  * @param argument Parâmetro não utilizado
  *
  * @note Função implementada manualmente pelo usuário.
  */
/* USER CODE END Header_ButtonTask */
void ButtonTask(void *argument)
{
  /* USER CODE BEGIN ButtonTask */
	AllLedsOff();

	/* Infinite loop */
	for(;;)
	{
//        HAL_GPIO_TogglePin(GPIOA, LED_R_Pin);
//        osDelay(500);

		if (!HAL_GPIO_ReadPin(Botao_A_GPIO_Port, Botao_A_Pin))
		{
			SetLedOnly(LED_R_Pin);
		}
		else if (!HAL_GPIO_ReadPin(Botao_B_GPIO_Port, Botao_B_Pin))
		{
			SetLedOnly(LED_B_Pin);
		}
		else if (!HAL_GPIO_ReadPin(Botao_C_GPIO_Port, Botao_C_Pin))
		{
			SetLedOnly(LED_G_Pin);
		}
		else
		{
			AllLedsOff();
		}

//		osDelay(1);
	}
  /* USER CODE END ButtonTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

