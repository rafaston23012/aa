/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @File       : freertos.c
  * @brief 		: Configuração e inicialização dos objetos do FreeRTOS (CMSIS-RTOS v2).
  * @author 	: Wu Shin Ting
  * @data		: 19/03/2026
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
  * @details
  * Este arquivo define e inicializa os principais elementos do sistema RTOS,
  * incluindo tarefas (threads), filas de mensagens (queues) e flags de eventos.
  *
  * O sistema implementa uma arquitetura baseada no padrão produtor-consumidor,
  * com separação clara entre aquisição de dados, processamento e transmissão.
  *
   * Duas tarefas principais (tasks/threads) são criadas:
  *
  * - @c AdcTask (prioridade normal):
  *   Responsável por controlar o ciclo de aquisição do ADC. Dispara conversões
  *   em modo interrupção e aguarda a conclusão de um par de amostras por meio
  *   de uma flag de evento.
  *
  * - @c UsartTask (prioridade baixa):
  *   Atua como consumidora, recebendo dados da fila e realizando a transmissão
  *   via UART de forma não bloqueante.
  *
  * A diferença de prioridade garante que a aquisição de dados tenha precedência
  * sobre a transmissão, mantendo a integridade temporal do sistema.
  *
  * Uma fila (queue) é criada.
  * - @c sampleQueue:
  *   Fila de mensagens utilizada para transferir pares de amostras do ADC
  *   (@c SamplePair_t) da camada de aquisição (ISR/callback) para a task
  *   de processamento/transmissão.
  *
  * Características:
  * - Capacidade: 16 elementos
  * - Tipo: estrutura contendo dois canais (IN6 e IN9)
  *
  * Papel:
  * - Desacoplar produtor (ADC) e consumidor (USART)
  * - Absorver variações de tempo entre aquisição e transmissão
  * - Evitar perda de dados em picos de processamento
  *
  *E duas flags de evento são definidas.
  * - @c adcEvent:
  *   Flag de evento utilizada para sincronizar a conclusão de um ciclo completo
  *   de aquisição ADC (par de canais).
  *
  * - @c usartEvent:
  *   Flag de evento utilizada para sincronizar a conclusão de uma transmissão completa
  *   de uma string.
  *
  * Funcionamento:
  * - O callback do ADC sinaliza a flag ao finalizar um par de conversões
  * - A @c AdcTask aguarda essa sinalização antes de iniciar um novo ciclo
  * - O callback do USART sinaliza a flag ao finalizar uma transmissão de string
  * - A @c UsartTask aguarda essa sinalização antes de iniciar uma nova transmissão
  *
  * Papel:
  * - Garantir controle determinístico do fluxo de aquisição
  * - Evitar sobreposição de ciclos de conversão/transmissão
  * - Manter o disparo do ADC sob controle explícito da AdcTask
  * - Manter o início de uma transmissão sob controle explícito da UsartTask
  * O RTOS atua como um orquestrador transparente do fluxo de execução:
  *
  * @code
  * [AdcTask] → dispara ADC
  *      ↓
  * [ISR / Callback ADC]
  *      ↓
  * [Fila (sampleQueue)]
  *      ↓
  * [UsartTask] → envia string
  *      ↓
  * [ISR / Callback USART]
  *      ↓
  * [Terminal Serial]
  * @endcode
  *
  * E para sincronização:
  *
  * @code
  * Callback ADC → seta flag → AdcTask libera novo ciclo
  * Callback USART → seta flag → usartTask libera nova transmissão
  * @endcode
  *
  * Essa organização permite que:
  * - O fluxo de controle seja reativo e orientado a eventos
  * - O código permaneça modular e escalável
  * - A complexidade de concorrência seja abstraída pelo RTOS
  *
  * Em outras palavras, o programador define *o que* deve acontecer, enquanto o RTOS gerencia
  * *quando* e *em que ordem* as tarefas são executadas.
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "adc.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BAR_WIDTH   40   /**< Largura da barra ASCII */
#define ADC_MAX     4095 /**< Valor máximo do ADC (12 bits) */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
uint8_t tx_buffer[100];  /**< buffer de transmissão */
SamplePair_t sample;

/* USER CODE END Variables */
/* Definitions for adcTask */
osThreadId_t adcTaskHandle;
const osThreadAttr_t adcTask_attributes = {
  .name = "adcTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for usartTask */
osThreadId_t usartTaskHandle;
const osThreadAttr_t usartTask_attributes = {
  .name = "usartTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for sampleQueue */
osMessageQueueId_t sampleQueueHandle;
const osMessageQueueAttr_t sampleQueue_attributes = {
  .name = "sampleQueue"
};
/* Definitions for adcEvent */
osEventFlagsId_t adcEventHandle;
const osEventFlagsAttr_t adcEvent_attributes = {
  .name = "adcEvent"
};
/* Definitions for usartEvent */
osEventFlagsId_t usartEventHandle;
const osEventFlagsAttr_t usartEvent_attributes = {
  .name = "usartEvent"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void setaUsartEventFlag ();
void setaAdcEventFlag ();
osMessageQueueId_t get_sampleQueueHandle(void);
void le_amostras();
/* USER CODE END FunctionPrototypes */

void AdcTask(void *argument);
void UsartTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	/**
	  * @brief Inicializa os objetos do FreeRTOS.
	  *
	  * Cria e configura todos os elementos necessários para execução do sistema:
	  * - Fila de mensagens (@c sampleQueue)
	  * - Tasks (@c AdcTask e @c UsartTask)
	  * - Event flags (@c adcEvent)
	  *
	  * @param None
	  * @retval None
	  *
	  * @note
	  * Esta função deve ser chamada após @c osKernelInitialize() e antes de
	  * @c osKernelStart().
	  */
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
  /* creation of sampleQueue */
  sampleQueueHandle = osMessageQueueNew (16, sizeof(SamplePair_t), &sampleQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of adcTask */
  adcTaskHandle = osThreadNew(AdcTask, NULL, &adcTask_attributes);

  /* creation of usartTask */
  usartTaskHandle = osThreadNew(UsartTask, NULL, &usartTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of adcEvent */
  adcEventHandle = osEventFlagsNew(&adcEvent_attributes);

  /* creation of usartEvent */
  usartEventHandle = osEventFlagsNew(&usartEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_AdcTask */
/**
  * @brief Task responsável pelo controle do ciclo de aquisição ADC.
  *
  * Esta task implementa o controle de alto nível das conversões ADC em modo
  * interrupção, atuando como o elemento coordenador do ciclo de aquisição
  * de pares de amostras (canais IN6 e IN9).
  *
  * @details
  * O funcionamento segue um modelo orientado a eventos:
  *
  * 1. A task inicia uma conversão ADC chamando @c HAL_ADC_Start_IT().
  * 2. O hardware ADC realiza a conversão e gera uma interrupção ao final.
  * 3. O callback @ref HAL_ADC_ConvCpltCallback trata cada conversão:
  *    - alterna entre os canais
  *    - monta um par de amostras (eixo X, eixo Y)
  *    - sinaliza a conclusão do par via event flag
  * 4. A task bloqueia em @c osEventFlagsWait(), aguardando essa sinalização.
  * 5. Ao receber o evento, reinicia o ciclo com uma nova conversão.
  *
  * Esse mecanismo garante que:
  * - as conversões sejam disparadas exclusivamente por software
  * - cada par de amostras seja tratado como uma unidade lógica
  * - não haja polling, mantendo eficiência no uso da CPU
  *
  * @param argument Parâmetro da task (não utilizado)
  * @retval None
  *
  * @note
  * - A task permanece bloqueada enquanto aguarda o término de um ciclo completo
  *   de aquisição, liberando a CPU para outras tarefas.
  * - A sincronização com o callback do ADC é feita por meio de event flags.
  * - O uso de @c HAL_ADC_Start_IT() mantém o processamento fora da task,
  *   delegando ao hardware e à ISR o controle fino da conversão.
  *
  * @warning
  * - Esta função nunca retorna (loop infinito).
  * - Não deve conter operações bloqueantes adicionais além da espera por evento,
  *   para não comprometer a previsibilidade do sistema.
  */
/* USER CODE END Header_AdcTask */
void AdcTask(void *argument)
{
  /* USER CODE BEGIN AdcTask */
	/* Infinite loop */
	for(;;)
	{
		HAL_ADC_Start_IT(&hadc1);

		osEventFlagsWait(adcEventHandle, 0x01, osFlagsWaitAny, osWaitForever);

        /* Envio não bloqueante para fila tratada por uma tarefa */
		SamplePair_t par;

		le_amostras (&par);

        (void)osMessageQueuePut(sampleQueueHandle, &par, 0U, 0U);

	}
  /* USER CODE END AdcTask */
}

/* USER CODE BEGIN Header_UsartTask */
/**
  * @brief Task responsável pelo consumo e transmissão de amostras via USART.
  *
  * Esta task implementa o papel de consumidora no padrão produtor-consumidor,
  * recebendo pares de amostras ADC a partir de uma fila RTOS e convertendo-os
  * em uma representação textual para transmissão via UART.
  *
  * @details
  * O funcionamento segue o fluxo:
  *
  * 1. A task bloqueia aguardando dados na fila @c sampleQueueHandle.
  * 2. Ao receber uma nova amostra (@c SamplePair_t):
  *    - Converte os valores ADC em posições relativas dentro de uma barra ASCII.
  *    - Constrói uma string no formato:
  *      @code
  *      X: [   o     ]  Y: [      o ]
  *      @endcode
  *    - Transmite o buffer via UART utilizando modo não bloqueante.
  *    - A task bloqueia aguardando o evento que indica a conclusão de transmissão
  *
  * A visualização utiliza uma barra de tamanho fixo (@c BAR_WIDTH), onde a
  * posição do caractere 'o' representa a magnitude do sinal em cada eixo:
  * - Canal IN9 → eixo X
  * - Canal IN6 → eixo Y
  *
  * @param argument Parâmetro da task (não utilizado)
  * @retval None
  *
  * @note
  * - A task permanece bloqueada em @c osMessageQueueGet(), garantindo uso
  *   eficiente da CPU (sem polling).
  * - A transmissão UART é realizada via @c HAL_UART_Transmit_IT(), evitando
  *   bloqueio da execução.
  * - O buffer @c tx_buffer deve possuir tamanho suficiente para acomodar
  *   toda a string formatada.
  *
  * @warning
  * - O uso de @c sizeof(tx_buffer)-1 assume que o buffer está completamente
  *   preenchido; inconsistências na construção da string podem gerar envio
  *   de dados inválidos.
  * - Não há proteção explícita contra acesso concorrente ao @c tx_buffer;
  *   esta task deve ser a única responsável por sua manipulação.
  *
  * @see HAL_UART_TxCpltCallback
  * @see osMessageQueueGet
  * @see HAL_ADC_ConvCpltCallback
  */
/* USER CODE END Header_UsartTask */
void UsartTask(void *argument)
{
  /* USER CODE BEGIN UsartTask */
	/* Infinite loop */

	for(;;)
	{
		if (osMessageQueueGet(sampleQueueHandle, &sample, NULL, osWaitForever) == osOK)
		{
			// agora 'sample' contém os dados retirados da fila
			/**
			 * @brief Inicializa prefixo do buffer de transmissão.
			 *
			 * Formato esperado:
			 *   "X: [........]  Y: [........]"
			 */
			strcpy ((char *)tx_buffer, "\rX: [");
			/**
			 * @brief Converte valores ADC em posição de barra.
			 *
			 * Escala o valor bruto do ADC para a largura da barra ASCII.
			 */
			uint8_t bar_x_pos = (sample.ch9 * BAR_WIDTH) / ADC_MAX;
			uint8_t bar_y_pos = (sample.ch6 * BAR_WIDTH) / ADC_MAX;

			uint8_t i;
			/**
			 * ================= Construção da Barra X =================
			 */
			for (i = 0; i < BAR_WIDTH; i++)
				tx_buffer[5+i] = (i == bar_x_pos ? 'o' : ' ');

			tx_buffer[5+i] = '\0';
			strcat ((char *)tx_buffer, "]  Y: [");

			/**
			 * ================= Construção da Barra Y =================
			 *
			 * Offset fixo baseado no layout da string.
			 */
			for (i = 0; i < BAR_WIDTH; i++)
				tx_buffer[52+i] = (i == bar_y_pos ? 'o' : ' ');

			tx_buffer[52+i] = '\0';
			strcat ((char *)tx_buffer, "]");

			/**
			 * @brief Transmissão não bloqueante via interrupção.
			 *
			 * Funcionamento interno da HAL_UART_Transmit_IT():
			 *
			 *  1. Habilita interrupção TXE (TXEIE no CR1).
			 *  2. Carrega primeiro byte no registrador DR.
			 *  3. A cada flag TXE, ISR envia próximo byte.
			 *  4. Ao finalizar buffer, desabilita TXEIE.
			 *  5. Quando TC é detectado, chama:
			 *       HAL_UART_TxCpltCallback()
			 *
			 * Vantagem:
			 *  - CPU permanece livre durante transmissão.
			 *  - Arquitetura totalmente não bloqueante.
			 */
			if (HAL_UART_Transmit_IT(&huart1, tx_buffer, strlen((char *)tx_buffer) + 1)) {
				osEventFlagsWait(usartEventHandle, 0x01, osFlagsWaitAny, osWaitForever);
			}
		}
	}
  /* USER CODE END UsartTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
  * @brief Retorna o handle da fila de amostras.
  *
  * Permite que módulos independentes (como callbacks) acessem a fila
  * responsável pela comunicação entre produtor (ADC) e consumidor.
  *
  * @retval osMessageQueueId_t Handle da fila de amostras do ADC
  */
osMessageQueueId_t get_sampleQueueHandle(void) {
	return sampleQueueHandle;
}

/**
  * @brief Sinaliza evento de término de aquisição ADC.
  *
  * Utilizada para sincronizar a conclusão de um par de conversões
  * com a task responsável por iniciar um novo ciclo.
  *
  * @retval None
  */
void setaAdcEventFlag () {
    osEventFlagsSet(adcEventHandle, 0x01);
}

/**
  * @brief Sinaliza evento de término de transmissão USART.
  *
  * Utilizada para sincronizar a conclusão de um par de conversões
  * com a task responsável por iniciar um novo ciclo.
  *
  * @retval None
  */
void setaUsartEventFlag () {
    osEventFlagsSet(usartEventHandle, 0x01);
}

/* USER CODE END Application */

