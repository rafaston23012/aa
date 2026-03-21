/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file freertos.c
  * @brief Implementação das tarefas RTOS para leitura do RTC e exibição em display OLED.
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
  * Este arquivo contém a implementação da lógica concorrente do sistema utilizando
  * CMSIS-RTOS v2 sobre FreeRTOS. O objetivo principal é ler periodicamente o tempo
  * do RTC (Real-Time Clock) e exibi-lo em um display OLED baseado no controlador SSD1306.
  *
  * A arquitetura é baseada em tarefas independentes que se comunicam por meio de
  * uma fila RTOS, promovendo desacoplamento entre aquisição de dados (RTC) e
  * apresentação (display).
  *
  * @section user_code Código Implementado Manualmente
  *
  * Os principais elementos implementados manualmente neste arquivo incluem:
  *
  * - Criação e configuração das tarefas:
  *   - @c RtcTask: responsável pela leitura periódica do RTC
  *   - @c OledTask: responsável pela atualização do display OLED
  *
  * - Criação da fila RTOS:
  *   - @c oledQueueHandle: utilizada para transferir estruturas de tempo (@c RTC_TimeTypeDef)
  *     entre as tarefas
  *
  * - Implementação da lógica das tasks:
  *   - Inicialização do RTC (tempo inicial)
  *   - Leitura contínua do tempo
  *   - Formatação da string de horário
  *   - Atualização do display via driver SSD1306
  *
  * - Integração com drivers:
  *   - RTC (HAL)
  *   - I2C
  *   - Display OLED (SSD1306)
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O sistema pode ser interpretado como um pipeline produtor-consumidor:
  *
  * @code
  *        RtcTask (produtor)
  *              │
  *              ├── HAL_RTC_GetTime()
  *              ├── HAL_RTC_GetDate()
  *              ▼
  *         oledQueue (fila RTOS)
  *              ▼
  *        OledTask (consumidor)
  *              ├── formatação HH:MM:SS
  *              ├── renderização gráfica
  *              ▼
  *        Display OLED (SSD1306 via I2C)
  * @endcode
  *
  * Fluxo de execução:
  * - A RtcTask lê o tempo atual a cada 500 ms
  * - Envia os dados para a fila
  * - A OledTask bloqueia aguardando novos dados
  * - Ao receber, atualiza completamente o display
  *
  * Características do modelo:
  * - Tasks bloqueantes (sem polling)
  * - Comunicação assíncrona via fila
  * - Separação clara entre aquisição e interface gráfica
  *
  *
  * @section cube_config Configuração no CubeMX
  *
  * A configuração de hardware foi realizada no STM32CubeMX com os seguintes pontos:
  *
  * - RTC:
  *   - Utiliza fonte de clock LSE (Low-Speed External), garantindo maior precisão
  *     para contagem de tempo
  *
  * - Sistema:
  *   - Utiliza HSE (High-Speed External) como fonte principal de clock para o restante
  *     dos periféricos e do núcleo
  *
  * - I2C:
  *   - Configurado para comunicação com o display OLED SSD1306
  *
  * - FreeRTOS:
  *   - Duas tarefas configuradas (RTC e OLED)
  *   - Uma fila para comunicação entre tasks
  *
  * @note
  * Neste projeto, o arquivo @c main.c gerado pelo CubeMX não foi modificado.
  * Toda a lógica adicional foi implementada exclusivamente neste arquivo (@c freertos.c),
  * mantendo a separação entre código gerado automaticamente e código de aplicação.
  *
  * @warning
  * - A função @c HAL_RTC_GetDate() deve ser chamada após @c HAL_RTC_GetTime()
  *   para garantir leitura correta no STM32F4
  * - A atualização completa do display a cada ciclo pode impactar desempenho
  *   em aplicações mais complexas
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c.h"
#include "rtc.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "conversions.h"
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
/* Definitions for rtcTask */
osThreadId_t rtcTaskHandle;
const osThreadAttr_t rtcTask_attributes = {
  .name = "rtcTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for oledTask */
osThreadId_t oledTaskHandle;
const osThreadAttr_t oledTask_attributes = {
  .name = "oledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for oledQueue */
osMessageQueueId_t oledQueueHandle;
const osMessageQueueAttr_t oledQueue_attributes = {
  .name = "oledQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void RtcTask(void *argument);
void OledTask(void *argument);

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
  /* creation of oledQueue */
  oledQueueHandle = osMessageQueueNew (16, sizeof(RTC_TimeTypeDef), &oledQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of rtcTask */
  rtcTaskHandle = osThreadNew(RtcTask, NULL, &rtcTask_attributes);

  /* creation of oledTask */
  oledTaskHandle = osThreadNew(OledTask, NULL, &oledTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_RtcTask */
/**
 * @brief Task responsável pela leitura periódica do RTC (Real-Time Clock).
 *
 * @details
 * Esta função implementa uma task do FreeRTOS, ou seja, uma "função que roda
 * em paralelo" com outras partes do sistema. No contexto de RTOS, cada task
 * executa de forma independente e pode ser escalonada pelo sistema operacional.
 *
 * O objetivo desta task é:
 * - Configurar um horário inicial no RTC
 * - Ler continuamente o horário atual
 * - Enviar esse horário para outra task (OledTask) através de uma fila
 *
 * Funcionamento passo a passo:
 *
 * 1. Inicialização do tempo:
 *    - Define um horário inicial (17:58:55)
 *    - Configura o RTC com esse valor usando @c HAL_RTC_SetTime()
 *
 * 2. Loop infinito:
 *    - Toda task RTOS normalmente possui um loop infinito (@c for(;;))
 *    - Dentro desse loop, a task executa sua função continuamente
 *
 * 3. Leitura do RTC:
 *    - @c HAL_RTC_GetTime(): lê o horário atual
 *    - @c HAL_RTC_GetDate(): necessário para "validar" a leitura no STM32F4
 *
 * 4. Comunicação entre tasks:
 *    - O horário lido é enviado para a fila @c oledQueueHandle
 *    - Outra task (OledTask) irá receber esses dados e atualizar o display
 *
 * 5. Delay controlado:
 *    - @c osDelay(500) faz a task "dormir" por 500 ms
 *    - Isso evita uso excessivo da CPU e define a taxa de atualização
 *
 * Conceitos importantes:
 *
 * - Task:
 *   Uma função executada pelo RTOS de forma concorrente com outras tasks.
 *
 * - Fila (Queue):
 *   Mecanismo seguro para troca de dados entre tasks.
 *
 * - Bloqueio (Delay):
 *   Ao usar @c osDelay(), a task libera a CPU para outras tasks.
 *
 * @param argument Parâmetro da task (não utilizado)
 * @retval None
 *
 * @note
 * - A chamada de @c HAL_RTC_GetDate() após @c HAL_RTC_GetTime() é obrigatória
 *   em microcontroladores STM32F4 para garantir leitura correta
 * - O envio para a fila é não bloqueante (timeout = 0)
 *
 * @warning
 * - Se a fila estiver cheia, o dado pode ser perdido (não há tratamento de erro)
 * - O tempo é reinicializado sempre que o sistema inicia
 *
 * @see OledTask
 * @see osMessageQueuePut
 */
/* USER CODE END Header_RtcTask */
void RtcTask(void *argument)
{
	/* USER CODE BEGIN RtcTask */
	/* Configurar o tempo inicial */
	RTC_TimeTypeDef sTime = {0};
	RTC_DateTypeDef sDate = {0}; // Necessário para a leitura correta no F4

	/* Definir o tempo inicial: 17:58:55 */
	sTime.Hours = 17;
	sTime.Minutes = 58;
	sTime.Seconds = 55;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;

	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	/* Infinite loop */
	for(;;)
	{
		/*Obter o tempo e data correntes  */
		HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
		HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN); // Important: Must call GetDate after GetTime

		(void)osMessageQueuePut(oledQueueHandle, &sTime, 0U, 0U);

		osDelay(500);
	}
	/* USER CODE END RtcTask */
}

/* USER CODE BEGIN Header_OledTask */
/**
 * @brief Task responsável por exibir o horário no display OLED.
 *
 * @details
 * Esta função implementa uma task do FreeRTOS que atua como consumidora
 * no modelo produtor-consumidor. Sua função é receber o horário atual
 * (enviado pela @c RtcTask) e exibi-lo em um display OLED.
 *
 * Funcionamento passo a passo:
 *
 * 1. Inicialização do display:
 *    - A função @c ssd1306_Init() configura o display OLED
 *    - Essa inicialização ocorre apenas uma vez, antes do loop infinito
 *
 * 2. Loop infinito:
 *    - Como toda task RTOS, utiliza @c for(;;) para execução contínua
 *
 * 3. Recebimento de dados:
 *    - @c osMessageQueueGet() aguarda dados da fila @c oledQueueHandle
 *    - A task fica BLOQUEADA até que novos dados estejam disponíveis
 *    - Isso evita uso desnecessário da CPU (sem polling)
 *
 * 4. Processamento dos dados:
 *    - Converte os valores de hora, minuto e segundo para string
 *      no formato "HH:MM:SS" usando @c format_time_hhmmss()
 *
 * 5. Atualização do display:
 *    - Limpa a tela (@c ssd1306_Fill)
 *    - Define a posição do cursor (@c ssd1306_SetCursor)
 *    - Escreve a string formatada (@c ssd1306_WriteString)
 *    - Atualiza fisicamente o display (@c ssd1306_UpdateScreen)
 *
 * Conceitos importantes para iniciantes:
 *
 * - Task consumidora:
 *   Recebe dados produzidos por outra task (RtcTask).
 *
 * - Fila (Queue):
 *   Utilizada para comunicação segura entre tasks.
 *
 * - Bloqueio eficiente:
 *   O uso de @c osWaitForever faz com que a task só execute quando necessário.
 *
 * - Atualização de display:
 *   A escrita no SSD1306 ocorre em memória interna e só aparece na tela após
 *   @c ssd1306_UpdateScreen().
 *
 * @param argument Parâmetro da task (não utilizado)
 * @retval None
 *
 * @note
 * - O buffer @c time_str deve ter tamanho suficiente para "HH:MM:SS" + terminador nulo
 * - A task só executa quando há novos dados na fila
 *
 * @warning
 * - Atualizações frequentes do display podem impactar desempenho em sistemas maiores
 * - Caso múltiplas tasks acessem o display, será necessário mecanismo de sincronização
 *
 * @see RtcTask
 * @see osMessageQueueGet
 * @see ssd1306_UpdateScreen
 */
/* USER CODE END Header_OledTask */
void OledTask(void *argument)
{
	RTC_TimeTypeDef sTime;
	char time_str[9]; /* "HH:MM:SS" */

	/* USER CODE BEGIN OledTask */
	/* Inicializa o driver SSD1306 para OLED */
	ssd1306_Init();

	/* Infinite loop */
	for(;;)
	{
		if (osMessageQueueGet(oledQueueHandle, &sTime, NULL, osWaitForever) == osOK) {
			/* Formatar os valores em HH:MM:SS  */
			format_time_hhmmss(sTime.Hours, sTime.Minutes, sTime.Seconds, time_str);

			/* Preecher a tela com a cor branca */
			ssd1306_Fill(White);

			/* Centralizar text (128x64) */
			ssd1306_SetCursor(32, 24);

			/* Escrever o texto em fonte 7x10 e cor preta */
			ssd1306_WriteString(time_str, Font_7x10, Black);

			/* Atualizar a tela */
			ssd1306_UpdateScreen();

		}
	}
	/* USER CODE END OledTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

