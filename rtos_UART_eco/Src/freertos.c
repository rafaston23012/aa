/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    freertos.c
  * @brief   Implementação das tarefas e mecanismos RTOS para eco UART.
  * @author Wu Shin Ting
  * @date 21/03/2026
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
  * Este arquivo contém a configuração e implementação de uma aplicação baseada
  * em FreeRTOS que realiza o eco de caracteres recebidos via UART. A arquitetura
  * foi organizada em duas tarefas principais: uma responsável pela recepção
  * (RxTask) e outra pela transmissão (TxTask), comunicando-se por meio de uma
  * fila RTOS.
  *
  * A recepção ocorre através de leitura bloqueante da UART, enquanto a transmissão
  * envia os dados recebidos de volta ao terminal, caracterizando o comportamento
  * de eco.
  *
  * @section user_code Código Implementado Manualmente
  *
  * Os principais elementos implementados manualmente neste arquivo são:
  *
  * - Criação de uma fila RTOS (@c txQueueHandle) para comunicação entre tarefas
  * - Implementação da @c RxTask:
  *   - Responsável por receber dados da UART por polling
  *   - Envia os dados recebidos para a fila
  *
  * - Implementação da @c TxTask:
  *   - Responsável por consumir dados da fila
  *   - Realiza a transmissão via UART, também por polling
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O sistema pode ser compreendido como um pipeline simples de processamento:
  *
  * @code
  * Terminal → UART RX → RxTask → Fila → TxTask → UART TX → Terminal
  * @endcode
  *
  * - A @c RxTask atua como produtora de dados
  * - A @c TxTask atua como consumidora
  * - A fila desacopla temporalmente as duas tarefas
  *
  * Embora o uso de RTOS introduza maior modularidade e separação de responsabilidades,
  * neste caso específico ele também aumenta a complexidade em comparação com uma
  * implementação HAL simples (@c hal_UART_eco).
  *
  * @section cube_config Configuração no CubeMX
  *
  * Neste projeto, o CubeMX foi utilizado para configurar:
  * - A UART em modo de transmissão e recepção por polling
  * - O sistema operacional FreeRTOS (CMSIS-OS v2)
  * - Criação de tarefas e fila
  *
  * A recepção e a transmissão UART foram configuradas utilizando polling do estado
  * dos canais (flags internas), por meio das funções bloqueantes da HAL:
  * - @c HAL_UART_Receive()
  * - @c HAL_UART_Transmit()
  *
  * É importante destacar que esta abordagem é, em geral, inadequada quando utilizada
  * sobre uma camada RTOS, pois o polling caracteriza um mecanismo bloqueante que
  * pode comprometer o escalonamento e a eficiência do sistema.
  *
  * No entanto, por motivos didáticos, optou-se por manter a mesma arquitetura do
  * exemplo clássico de eco UART em HAL (sem RTOS), com o objetivo de demonstrar
  * como a introdução de tarefas, filas e mecanismos de sincronização pode tornar
  * o código mais estruturado, porém também mais complexo.
  *
  * Dessa forma, evidencia-se que o uso de RTOS não é sempre a solução mais simples
  * ou mais eficiente, cabendo ao projetista avaliar cuidadosamente os trade-offs
  * entre:
  * - Simplicidade vs. modularidade
  * - Desempenho vs. escalabilidade
  * - Código bare-metal vs. arquitetura baseada em RTOS
  *
  * A escolha final deve considerar os requisitos específicos da aplicação.
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
#include "usart.h"
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
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for txTask */
osThreadId_t txTaskHandle;
const osThreadAttr_t txTask_attributes = {
  .name = "txTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for rxTask */
osThreadId_t rxTaskHandle;
const osThreadAttr_t rxTask_attributes = {
  .name = "rxTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for txQueue */
osMessageQueueId_t txQueueHandle;
const osMessageQueueAttr_t txQueue_attributes = {
  .name = "txQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void TxTask(void *argument);
void RxTask(void *argument);

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

  /* Create the queue(s) */
  /* creation of txQueue */
  txQueueHandle = osMessageQueueNew (16, sizeof(uint8_t), &txQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of txTask */
  txTaskHandle = osThreadNew(TxTask, NULL, &txTask_attributes);

  /* creation of rxTask */
  rxTaskHandle = osThreadNew(RxTask, NULL, &rxTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_TxTask */
/**
  * @brief Task responsável pela transmissão de dados via UART (eco).
  *
  * @details
  * Esta tarefa implementa o papel de consumidora no modelo produtor-consumidor,
  * recebendo bytes de uma fila RTOS (@c txQueueHandle) e transmitindo-os via UART.
  *
  * O fluxo de execução é o seguinte:
  * - A task permanece bloqueada aguardando dados na fila utilizando
  *   @c osMessageQueueGet(), evitando consumo desnecessário de CPU.
  * - Quando um byte é recebido da fila:
  *   - A função @c HAL_UART_Transmit() é chamada para enviar o dado.
  *   - A transmissão é realizada em modo bloqueante, aguardando o término completo
  *     da transmissão (flag TC - Transmission Complete).
  *
  * Este comportamento resulta no eco do caractere previamente recebido pela
  * @c RxTask e encaminhado via fila.
  *
  * @param argument Parâmetro da task (não utilizado)
  * @retval None
  *
  * @note
  * - A função @c HAL_UART_Transmit() é bloqueante, o que significa que a task
  *   permanece ocupada até a conclusão da transmissão.
  * - O uso de polling/bloqueio na camada HAL não é a abordagem mais eficiente
  *   em sistemas com RTOS, sendo mais indicado o uso de interrupções ou DMA.
  * - Entretanto, esta implementação foi mantida por fins didáticos.
  *
  * @warning
  * - O uso de timeout igual a 0 pode causar falha imediata na transmissão caso
  *   o periférico não esteja pronto, retornando erro silencioso se não tratado.
  * - Em aplicações reais, recomenda-se validar o valor de retorno da função.
  *
  * @see RxTask
  * @see osMessageQueueGet
  * @see HAL_UART_Transmit
  */
/* USER CODE END Header_TxTask */
void TxTask(void *argument)
{
  /* USER CODE BEGIN TxTask */
  uint8_t tx_data;

  /* Infinite loop */
  for(;;)
  {
	  if (osMessageQueueGet(txQueueHandle, &tx_data, NULL, osWaitForever) == osOK)
	  {
		  /**
		   * Transmite o byte recebido (eco). Função bloqueante da HAL, aguardando TC.
		   */
		  HAL_UART_Transmit(&huart1, &tx_data, 1, 0);
	  }
  }
  /* USER CODE END TxTask */
}

/* USER CODE BEGIN Header_RxTask */
/**
  * @brief Task responsável pela recepção de dados via UART.
  *
  * @details
  * Esta tarefa implementa o papel de produtora no modelo produtor-consumidor,
  * sendo responsável por receber caracteres da interface UART e encaminhá-los
  * para uma fila RTOS (@c txQueueHandle), onde serão posteriormente consumidos
  * pela @c TxTask.
  *
  * O funcionamento ocorre da seguinte forma:
  * - A task executa um loop infinito.
  * - Em cada iteração, chama @c HAL_UART_Receive() para ler 1 byte da UART.
  * - A função é chamada com @c HAL_MAX_DELAY, tornando a operação bloqueante:
  *   a task permanece suspensa até que um caractere seja recebido.
  * - Após a recepção bem-sucedida (@c HAL_OK):
  *   - O byte é enviado para a fila RTOS usando @c osMessageQueuePut().
  *
  * Esse mecanismo permite desacoplar a recepção da transmissão, promovendo
  * melhor organização do código e separação de responsabilidades entre tarefas.
  *
  * @param argument Parâmetro da task (não utilizado)
  * @retval None
  *
  * @note
  * - O uso de @c HAL_UART_Receive() em modo bloqueante realiza polling ativo
  *   da flag RXNE, mantendo a CPU ocupada até a recepção do dado ou timeout.
  * - Em sistemas com RTOS, isso pode reduzir a responsividade, pois a tarefa
  *   não entra em estado bloqueado do escalonador.
  * - A fila RTOS realiza uma cópia do dado, garantindo segurança no
  *   compartilhamento entre tarefas.
  *
  * @warning
  * - Apesar de não realizar polling explícito, a função ainda é bloqueante
  *   e depende de implementação interna da HAL.
  * - Para aplicações mais eficientes em RTOS, recomenda-se o uso de
  *   recepção por interrupção (@c HAL_UART_Receive_IT) ou DMA.
  *
  * @see TxTask
  * @see osMessageQueuePut
  * @see HAL_UART_Receive
*/
/* USER CODE END Header_RxTask */
void RxTask(void *argument)
{
  /* USER CODE BEGIN RxTask */
	uint8_t rx_data;
	/* Infinite loop */
	for(;;)
	{
		if (HAL_UART_Receive(&huart1, &rx_data, 1, HAL_MAX_DELAY) == HAL_OK) {
			(void)osMessageQueuePut(txQueueHandle, &rx_data, 0U, 0U);
		}
	}
  /* USER CODE END RxTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

