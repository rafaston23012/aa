/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file freertos.c
  * @brief Implementação da lógica RTOS com aquisição ADC e transmissão UART.
  * @author Wu Shin Ting
  * @date  20/03/2026
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
  * Este arquivo implementa a arquitetura baseada em RTOS do sistema, utilizando CMSIS-RTOS v2
  * sobre FreeRTOS para organizar a execução concorrente e a comunicação entre os módulos.
  * O sistema segue o padrão produtor-consumidor, no qual a aquisição de dados e a transmissão
  * são desacopladas por meio de mecanismos de sincronização e filas.
  *
  * A tarefa @c AdcTask atua como produtora de dados. Ela é responsável por inicializar o ADC
  * em modo interrupção e iniciar o temporizador que gera o trigger de conversão. Após isso,
  * permanece bloqueada aguardando a sinalização de término de conversão através de uma event flag.
  * Quando liberada, a task lê a última amostra consolidada (par de canais) e a envia para uma
  * fila RTOS, permitindo que o processamento subsequente ocorra de forma assíncrona.
  *
  * A tarefa @c UsartTask atua como consumidora. Ela permanece bloqueada aguardando dados na fila.
  * Ao receber uma nova amostra, converte os valores digitais do ADC em uma representação visual
  * baseada em barras ASCII, onde a posição de um marcador indica a magnitude do sinal. Em seguida,
  * realiza a transmissão dos dados via UART utilizando modo não bloqueante com interrupção.
  * Após iniciar a transmissão, a task aguarda a confirmação de término por meio de uma event flag,
  * garantindo que não haja concorrência no uso do periférico de comunicação.
  *
  * O sistema utiliza callbacks de interrupção da HAL para integrar eventos de hardware ao fluxo
  * do RTOS. O callback de conversão do ADC sinaliza a conclusão de uma conversão e aciona uma
  * event flag que libera a AdcTask. Já o callback de término de transmissão UART sinaliza que o
  * envio foi concluído, liberando a UsartTask para uma nova operação. Esses callbacks executam
  * em contexto de interrupção e são mantidos curtos, delegando o processamento mais complexo
  * para as tasks.
  *
  * A comunicação entre tarefas é realizada por meio de uma fila RTOS, que permite transferir
  * estruturas contendo pares de amostras ADC de forma segura e desacoplada. Já a sincronização
  * entre interrupções e tarefas é feita por meio de event flags, evitando o uso de polling e
  * garantindo eficiência no uso da CPU. O projeto não utiliza mutexes explicitamente, pois os
  * recursos compartilhados são acessados de forma controlada por contexto único ou por meio de
  * sincronização implícita do fluxo.
  *
  * O fluxo de dados do sistema inicia no disparo periódico do ADC por hardware, seguido pelas
  * interrupções de conversão que alimentam o pipeline de processamento. As tasks então assumem
  * o controle no contexto do RTOS, realizando o transporte e a apresentação dos dados. Essa
  * abordagem resulta em um sistema reativo, não bloqueante e eficiente, com clara separação
  * entre aquisição, processamento e comunicação.
  *
  * @note
  * O sistema é orientado a eventos e mantém as tasks bloqueadas sempre que possível,
  * reduzindo o uso de CPU.
  *
  * @warning
  * A integridade dos dados depende da correta sincronização entre callbacks e tarefas;
  * modificações no fluxo devem preservar essa relação.
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
#include "tim.h"
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
void le_amostra (SamplePair_t *a);

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
 * @brief Task responsável pela aquisição e envio de amostras ADC.
 *
 * @details
 * Esta task atua como produtora no modelo produtor-consumidor, sendo responsável
 * por intermediar a aquisição de dados via ADC (realizada em contexto de interrupção)
 * e o envio dessas amostras para processamento posterior por outras tasks.
 *
 * Na inicialização, a task:
 * - Inicia o ADC em modo interrupção com @c HAL_ADC_Start_IT()
 * - Inicia o temporizador (@c TIM3), responsável por gerar o trigger periódico
 *   de conversão do ADC via hardware
 *
 * Após a inicialização, a task entra em um loop infinito onde:
 *
 * 1. Permanece bloqueada aguardando a sinalização de término de conversão ADC,
 *    através da event flag @c adcEventHandle.
 *
 * 2. Ao ser liberada (evento gerado pela ISR @c HAL_ADC_ConvCpltCallback):
 *    - Lê a última amostra disponível (par de canais) através da função @c le_amostra()
 *    - Envia a amostra para a fila RTOS (@c sampleQueueHandle)
 *
 * A comunicação com a fila é feita de forma não bloqueante, permitindo que a task
 * retorne rapidamente ao estado de espera.
 *
 * Fluxo resumido:
 * @code
 * ADC ISR (HAL_ADC_ConvCpltCallback)
 *     └── setaAdcEventFlag()
 *             └── AdcTask desbloqueada
 *                     ├── le_amostra()
 *                     └── osMessageQueuePut()
 * @endcode
 *
 * @param argument Parâmetro da task (não utilizado)
 * @retval None
 *
 * @note
 * - A task permanece bloqueada a maior parte do tempo, garantindo eficiência
 * - O disparo do ADC é controlado por hardware (TIM3), não por software
 * - A função @c le_amostra() realiza cópia da amostra global
 *
 * @warning
 * - A leitura da amostra não é protegida contra condição de corrida com a ISR
 * - Caso a fila esteja cheia, a amostra pode ser descartada (envio não bloqueante)
 *
 * @see HAL_ADC_ConvCpltCallback
 * @see le_amostra
 * @see osMessageQueuePut
 */
/* USER CODE END Header_AdcTask */
void AdcTask(void *argument)
{
  /* USER CODE BEGIN AdcTask */
  /**< Reinicia a conversão em modo interrupção */
  HAL_ADC_Start_IT(&hadc1);

  HAL_TIM_Base_Start(&htim3);

  /* Infinite loop */
  for(;;)
  {
	  osEventFlagsWait(adcEventHandle, 0x01, osFlagsWaitAny, osWaitForever);

      /* Envio não bloqueante para fila tratada por uma tarefa */
	  SamplePair_t amostra;

	  le_amostra (&amostra);

      (void)osMessageQueuePut(sampleQueueHandle, &amostra, 0U, 0U);

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
  * - Depois do envio, a task permanece bloqueada em @c osEventFlagsWait, garantindo
  *   que não haja uso concorrente do canal TX.
  *
  * @warning
  * - O uso de @c strlen((char *)tx_buffer)+1 assume que o buffer está completamente
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

