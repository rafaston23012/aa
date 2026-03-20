/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file       freertos.c
  * @brief      Code for freertos applications (fila/queue foi aplicada para coordenar
  *             os eventos entre interrupção e tarefa)
  * @author     Wu Shin-Ting
  * @date       26/01/2026
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
  * Em FreeRTOS usando a camada CMSIS-RTOS2, podemos enviar dados para uma tarefa
  * por meio de filas (queues), permitindo que esses dados sejam processados
  * posteriormente em contexto de tarefa. Essas filas são representadas por um identificador do tipo ::osMessageQueueId_t,
  * também chamado de "handle".
  *
  * Um "handle" representa a fila e contém as informações necessárias para que
  * diferentes partes do sistema possam interagir com ela, por exemplo:
  * - enviar eventos de botão para a fila (ex.: a partir de uma ISR/EXTI callback)
  * - receber eventos na task responsável por processar os botões
  *
  * Importante:
  * - A fila em si não processa os dados; ela apenas armazena e organiza os eventos.
  * - O processamento é realizado por uma tarefa que consome os dados da fila.
  *
  * Por padrão, o CubeMX gera o handle da fila como uma variável global no
  * arquivo `freertos.c`:
  * @code
  * osMessageQueueId_t buttonQueueHandle;
  * @endcode
  *
  * Entretanto, boas práticas de engenharia de software recomendam evitar
  * expor variáveis globais diretamente (via `extern`) sempre que possível.
  *
  * Vantagens:
  * - encapsula o acesso ao recurso
  * - evita espalhar `extern` pelo projeto
  * - facilita refatorações futuras (ex.: mudar o nome da fila ou criar múltiplas filas)
  *
  * Atenção:
  * - A fila precisa ter sido criada antes de ser utilizada.
  *   Em geral, isso ocorre dentro de ::MX_FREERTOS_Init().
  * - Se utilizada antes da criação, o retorno do handle pode ser `NULL`.
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
#include "queue.h"
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
/**
 * @brief Identificador (handle) para a fila de mensagens de eventos do botão.
 * * Este objeto é utilizado pelas funções do CMSIS-RTOS para gerenciar a
 * comunicação entre a tarefa de varredura (polling/interrupt) e a tarefa
 * de processamento de lógica.
 */
osMessageQueueId_t buttonQueueHandle;
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
  buttonQueueHandle = osMessageQueueNew(
		                      8,                       // profundidade
		                      sizeof(ButtonEvent_t),   // tamanho do item
		                      NULL);
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
/**
 * @brief Tarefa (thread/task) do FreeRTOS.
 *
 * @details
 * Em um sistema baseado em RTOS, cada task funciona como um "programa" independente
 * que o escalonador executa em fatias de tempo, alternando entre várias tasks.
 *
 * Diferença fundamental para o "superloop" (while(1) do bare-metal):
 * - Em bare-metal, o while(1) é o fluxo principal do programa.
 * - Em RTOS, o while(1) existe dentro de cada task e o escalonador decide
 *   quando cada task roda.
 *
 * Nesta aplicação, a task:
 * 1) aguarda eventos de botão vindos de uma fila (queue)
 * 2) faz o *debounce* fora da interrupção (ISR)
 * 3) interpreta qual botão foi pressionado
 * 4) atualiza os LEDs de acordo com o botão
 *
 * A comunicação entre interrupção e task é feita pela fila:
 * - a ISR (EXTI callback) detecta o botão e envia um evento para a fila
 * - esta task bloqueia esperando esse evento
 *
 * Esse padrão é considerado uma boa prática em sistemas embarcados com RTOS:
 * - ISR curta e rápida
 * - processamento (debounce + lógica) em task
 *
 * @param argument Ponteiro genérico (não utilizado aqui). Pode ser usado para
 *        passar parâmetros para a task.
 */
/* USER CODE END Header_ButtonTask */
void ButtonTask(void *argument)
{
	/* USER CODE BEGIN ButtonTask */

	/**
	 * @brief Variável local para armazenar o evento recebido da fila.
	 *
	 * ButtonEvent_t é uma estrutura criada pelo usuário para representar um evento
	 * gerado por um botão. Ela normalmente contém informações como:
	 * - qual botão gerou a interrupção (ID ou pino)
	 * - se foi borda de subida/descida (pressionou/soltou)
	 * - timestamp/tick
	 *
	 * Aqui, a variável evt será preenchida a cada evento recebido via queue.
	 */

	ButtonEvent_t evt;

	/**
	 * @brief Loop infinito da task.
	 *
	 * Toda task do FreeRTOS normalmente possui um loop infinito. Porém, diferente
	 * do bare-metal, este loop não "prende" a CPU o tempo todo, porque:
	 * - a task pode BLOQUEAR (ex.: esperando fila, semáforo, delay)
	 * - quando bloqueada, o FreeRTOS executa outras tasks
	 *
	 * O segredo para um RTOS eficiente é:
	 * - evitar busy-wait (laços vazios)
	 * - usar bloqueios (queue/semaphore) e osDelay/vTaskDelay
	 */

	/* Infinite loop */
	for(;;)
	{
		/**
		 * @brief Espera por um evento na fila buttonQueueHandle.
		 *
		 * osMessageQueueGet:
		 * - copia um item da fila para a variável evt
		 * - retorna osOK se conseguiu receber
		 *
		 * Parâmetros:
		 * - buttonQueueHandle: identificador da fila (criada em MX_FREERTOS_Init)
		 * - &evt: ponteiro para onde o item recebido será copiado
		 * - NULL: prioridade do item (não utilizada aqui)
		 * - osWaitForever: a task fica BLOQUEADA até chegar um evento
		 *
		 * Ponto importante para RTOS:
		 * - Enquanto espera (bloqueada), esta task NÃO consome CPU.
		 * - Assim que a ISR enviar um evento para a fila, a task acorda.
		 */
		if (osMessageQueueGet(buttonQueueHandle, &evt, NULL, osWaitForever) == osOK)
		{
			/**
			 * @brief Debounce fora da ISR (boa prática).
			 *
			 * Botões mecânicos geram "ruído" (bouncing), ou seja, várias transições
			 * rápidas ao apertar/soltar.
			 *
			 * Em vez de fazer debounce dentro da interrupção (o que deixaria a ISR lenta),
			 * fazemos aqui na task usando osDelay.
			 *
			 * osDelay(20):
			 * - bloqueia esta task por ~20 ms
			 * - permite que outras tasks rodem nesse tempo
			 *
			 * Não use HAL_Delay dentro de tasks (em geral).
			 * Prefira osDelay/vTaskDelay, pois o RTOS gerencia melhor.
			 */
			/* Debounce fora da ISR */
			osDelay(20);

			/**
			 * @brief Tratamento para borda de subida (rising edge).
			 *
			 * Dependendo da configuração do EXTI, um evento pode representar:
			 * - pressionar o botão
			 * - soltar o botão
			 *
			 * Aqui o código assume que evt.rising_edge indica uma borda de subida
			 * (por exemplo, botão foi solto, dependendo do pull-up/pull-down).
			 *
			 * Quando ocorre rising edge, desligamos todos os LEDs e voltamos ao loop.
			 */

			if (evt.rising_edge)
			{
				HAL_GPIO_WritePin(GPIOA,
						LED_R_Pin | LED_B_Pin | LED_G_Pin,
						GPIO_PIN_RESET);
				continue;
			}

			/**
			 * @brief Decide a ação com base no botão pressionado.
			 *
			 * evt.button_id armazena o identificador do botão.
			 * Aqui foram usados valores numéricos (12, 15, 5), que correspondem aos
			 * pinos GPIO associados aos botões.
			 *
			 * Sugestão didática:
			 * Em versões futuras, pode-se trocar esses números por um enum:
			 *   typedef enum { BTN_A, BTN_B, BTN_C } ButtonId_t;
			 * Isso torna o código mais legível.
			 */
			switch (evt.button_id)
			{
			case 12: /* Botão A */
				/* Liga LED vermelho e desliga os demais */
				HAL_GPIO_WritePin(GPIOA, LED_R_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, LED_B_Pin | LED_G_Pin, GPIO_PIN_RESET);
				break;

			case 15: /* Botão B */
				/* Liga LED azul e desliga os demais */
				HAL_GPIO_WritePin(GPIOA, LED_B_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, LED_R_Pin | LED_G_Pin, GPIO_PIN_RESET);
				break;

			case 5:  /* Botão C */
				/* Liga LED verde e desliga os demais */
				HAL_GPIO_WritePin(GPIOA, LED_G_Pin, GPIO_PIN_SET);
				HAL_GPIO_WritePin(GPIOA, LED_R_Pin | LED_B_Pin, GPIO_PIN_RESET);
				break;

			default:
				/* Caso chegue um evento inesperado, nenhuma ação é tomada */
				break;
			}
		}
	}
	/* USER CODE END ButtonTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief Retorna o identificador (handle) da fila de mensagens dos botões.
 *
 * @details Esta função é utilizada como uma "interface" para acessar o handle
 *          da fila criada pelo RTOS
 *
 * @return ::osMessageQueueId_t Handle/ID da fila de eventos de botões.
 */
osMessageQueueId_t get_buttonQueueHandle(void) {
	return buttonQueueHandle;
}
/* USER CODE END Application */

