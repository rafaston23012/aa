/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    freertos.c
  * @brief   Migração do projeto FreeRTOS (Pico - https://gitlab.unicamp.br/rtos_pico/freertos_mutex)
  * 		 para a plataforma BlackPill, demonstrando o uso de Mutex para controle de acesso à UART.
  * @author  Wu Shin Ting
  * @date    01/04/2026
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file is provided, this software is distributed AS-IS.
  *
  ******************************************************************************
  * @details
  * Este projeto demonstra o uso de mecanismos de sincronização no FreeRTOS,
  * utilizando um semáforo binário configurado como Mutex para proteger o acesso
  * a um recurso compartilhado (UART).
  *
  * Para execução adequada:
  * - Conectar a interface UART-USB (ST-LINK) aos pinos RX/TX da USART
  * - Abrir um terminal serial (ex: PuTTY, Tera Term) com a seguinte configuração:
  *      - Baud rate: 9600
  *      - Data bits: 8
  *      - Stop bits: 1
  *      - Paridade: nenhuma
  *      - Controle de fluxo: desabilitado
  *
  * @section tasks Descrição das tarefas
  *
  * - PrintTask1 e PrintTask2:
  *   - Enviam dados para a UART (huart1)
  *   - Compartilham o mesmo recurso, estando sujeitas a condições de corrida
  *
  * - PrintTask0 (padrão, mas comentada manualmente em cada regeração de códigos):
  *   - Pode ser configurada com prioridade padrão (osPriorityNormal)
  *   - Pode influenciar o escalonamento e mascarar problemas de concorrência
  *
  * PrintTask1 e PrintTask2 possuem a mesma prioridade (osPriorityLow),
  * o que pode resultar em interleaving na execução, especialmente quando
  * pequenos atrasos (osDelay) são utilizados sem proteção adequada.
  *
  * @section mutex Uso do Mutex
  *
  * O projeto utiliza um semáforo binário como Mutex para garantir exclusão mútua:
  *
  * @code
  * xMutexHandle = osSemaphoreNew(1, 1, &xMutex_attributes);
  * @endcode
  *
  * Esse mecanismo impede acesso concorrente à UART, evitando corrupção de dados.
  *
  * @section config Configuração
  *
  * O comportamento do sistema é controlado pela macro:
  *
  * @code
  * #define USE_MUTEX 0
  * @endcode
  *
  * - USE_MUTEX = 0:
  *      - Mutex desabilitado
  *      - Possível saída embaralhada no terminal
  *
  * - USE_MUTEX = 1:
  *      - Mutex habilitado
  *      - Acesso exclusivo à UART
  *      - Saída organizada
  *
  * @section experiment Experimentos sugeridos
  *
  * 1. Compilar com USE_MUTEX = 0
  *    - Observar possíveis falhas de sincronização
  *    - Saída inconsistente ou intercalada
  *
  * 2. Compilar com USE_MUTEX = 1
  *    - Verificar a correta serialização das mensagens
  *    - Execução previsível e organizada
  *
  * @section notes Observações
  *
  * - A função HAL_UART_Transmit é bloqueante:
  *      - Reduz a probabilidade de conflitos
  *      - NÃO substitui mecanismos de sincronização
  *
  * - Em aplicações reais, recomenda-se:
  *      - Uso de Mutex (osMutex)
  *      - Uso de filas (Queue) com UART via DMA ou interrupção (IT)
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "usart.h"
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
  * @def USE_MUTEX
  * @brief Controla o uso de Mutex para acesso à UART.
  *
  * @details
  * Define se as tarefas utilizarão exclusão mútua ao acessar o recurso
  * compartilhado (UART).
  *
  * - 0: Não utiliza Mutex (pode ocorrer corrupção de dados)
  * - 1: Utiliza Mutex (acesso exclusivo à UART)
  */
#define USE_MUTEX 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for printTask0 */
osThreadId_t printTask0Handle;
const osThreadAttr_t printTask0_attributes = {
  .name = "printTask0",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for printTask2 */
osThreadId_t printTask2Handle;
const osThreadAttr_t printTask2_attributes = {
  .name = "printTask2",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for printTask1 */
osThreadId_t printTask1Handle;
const osThreadAttr_t printTask1_attributes = {
  .name = "printTask1",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for xMutex */
osSemaphoreId_t xMutexHandle;
const osSemaphoreAttr_t xMutex_attributes = {
  .name = "xMutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void PrintTask0(void *argument);
void PrintTask2(void *argument);
void PrintTask1(void *argument);

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

  /* Create the semaphores(s) */
  /* creation of xMutex */
  xMutexHandle = osSemaphoreNew(1, 1, &xMutex_attributes);

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
  /* creation of printTask0 */
//  printTask0Handle = osThreadNew(PrintTask0, NULL, &printTask0_attributes);

  /* creation of printTask2 */
  printTask2Handle = osThreadNew(PrintTask2, NULL, &printTask2_attributes);

  /* creation of printTask1 */
  printTask1Handle = osThreadNew(PrintTask1, NULL, &printTask1_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_PrintTask0 */
/* USER CODE BEGIN Header_PrintTask0 */
/**
  * @brief  Tarefa padrão gerada pelo CubeMX (não utilizada neste projeto).
  *
  * @param  argument Ponteiro genérico (não utilizado).
  * @retval None
  *
  * @details
  * Esta tarefa é criada automaticamente pelo CubeMX com prioridade padrão
  * mais alta (osPriorityNormal).
  *
  * No contexto deste projeto, seu uso é evitado pois:
  * - sua prioridade superior pode impor um sequenciamento natural na execução
  *   das demais tarefas
  * - isso reduz ou elimina a concorrência entre tarefas de menor prioridade
  * - pode mascarar problemas de sincronização no acesso à UART
  *
  * Para fins didáticos, deseja-se que PrintTask1 e PrintTask2 executem com
  * mesma prioridade (osPriorityLow), evidenciando condições de corrida quando
  * o Mutex não é utilizado.
  *
  * Portanto, esta tarefa é mantida apenas como referência, mas não é utilizada.
  */
/* USER CODE END Header_PrintTask0 */
void PrintTask0(void *argument)
{
  /* USER CODE BEGIN PrintTask0 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END PrintTask0 */
}

/* USER CODE BEGIN Header_PrintTask2 */
/**
  * @brief  Tarefa responsável por enviar dados pela UART (identificador "2").
  *
  * @param  argument Ponteiro genérico (não utilizado).
  * @retval None
  *
  * @details
  * Esta tarefa transmite repetidamente a string "2" pela interface UART
  * (huart1), seguida de uma quebra de linha. O envio é realizado de forma
  * bloqueante utilizando HAL_UART_Transmit.
  *
  * Quando a macro USE_MUTEX está habilitada, o acesso à UART é protegido
  * por um semáforo binário (Mutex), garantindo exclusão mútua entre tarefas.
  *
  * Caso o Mutex não seja utilizado:
  * - Pode ocorrer concorrência com outras tarefas (ex: PrintTask1)
  * - A saída no terminal pode apresentar interleaving (dados embaralhados)
  *
  * Com o Mutex habilitado:
  * - O acesso à UART é serializado
  * - A integridade da saída é preservada
  *
  * @note A função HAL_UART_Transmit é bloqueante, mas não garante proteção
  *       contra acesso concorrente sem o uso de Mutex.
  */
/* USER CODE END Header_PrintTask2 */
void PrintTask2(void *argument)
{
  /* USER CODE BEGIN PrintTask2 */
	const char *msg = "2";

	/* Infinite loop */
	for(;;)
	{
#if USE_MUTEX
		if (osSemaphoreAcquire(xMutexHandle, osWaitForever) == osOK)
		{
#endif
			for (int i = 0; i < 20; i++) {
				/*
				 * Envia de forma bloqueante
				 */
				HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			}
			HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", strlen("\r\n"), HAL_MAX_DELAY);
#if USE_MUTEX
			osSemaphoreRelease(xMutexHandle);
		}
#endif

		osDelay(1);
	}
  /* USER CODE END PrintTask2 */
}

/* USER CODE BEGIN Header_PrintTask1 */
/**
  * @brief  Tarefa responsável por enviar dados pela UART (identificador "1").
  *
  * @param  argument Ponteiro genérico (não utilizado).
  * @retval None
  *
  * @details
  * Esta tarefa transmite continuamente a string "1" pela interface UART
  * (huart1), repetindo o envio múltiplas vezes e finalizando com quebra de linha.
  *
  * O envio é realizado de forma bloqueante utilizando HAL_UART_Transmit.
  *
  * Quando USE_MUTEX está habilitado:
  * - O acesso à UART é protegido por um semáforo binário (Mutex)
  * - Garante exclusão mútua entre tarefas concorrentes
  *
  * Quando USE_MUTEX está desabilitado:
  * - Pode ocorrer concorrência com outras tarefas (ex: PrintTask2)
  * - A saída pode apresentar interleaving (dados misturados)
  *
  * A presença de laço infinito aninhado não altera o comportamento prático,
  * pois ambos mantêm a execução contínua da tarefa.
  *
  * @note HAL_UART_Transmit é bloqueante, mas não substitui o uso de Mutex
  *       para proteção de recursos compartilhados.
  */
/* USER CODE END Header_PrintTask1 */
void PrintTask1(void *argument)
{
  /* USER CODE BEGIN PrintTask1 */
	const char *msg = "1";

	/* Infinite loop */
	for(;;)
	{
		for(;;)
		{
#if USE_MUTEX
			if (osSemaphoreAcquire(xMutexHandle, osWaitForever) == osOK)
			{
#endif
				for (int i = 0; i < 20; i++) {
					/*
					 * Envia de forma bloqueante
					 */
					HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
				}
				HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", strlen("\r\n"), HAL_MAX_DELAY);
#if USE_MUTEX
				osSemaphoreRelease(xMutexHandle);
			}
#endif
			osDelay(1);
		}
	}
  /* USER CODE END PrintTask1 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

