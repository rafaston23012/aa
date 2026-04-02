/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         : freertos.c
  * @brief  	  : Migração do projeto FreeRTOS (Pico - https://gitlab.unicamp.br/rtos_pico/freertos_signal)
  * 		 para a plataforma BlackPill, demonstrando o uso de Grupo de Eventos para condicionar a mudança
  * 		 de estado do Led RGB em função do bip do buzzer.
  * @author		  : Wu Shin Ting
  * @date		  : 01/04/2026
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
  *
  * @section intro Introdução
  *
  * Este projeto demonstra a sincronização entre tarefas utilizando
  * **Event Flags (grupo de eventos)** do CMSIS-RTOS v2. O sistema é composto por
  * tarefas independentes que se comunicam por meio de eventos, evitando polling e
  * melhorando a eficiência do uso da CPU.
  *
  * @section eventflags Princípio de Operação – Event Flags
  *
  * Event Flags são um mecanismo de sincronização baseado em bits, onde:
  *
  * - cada bit representa um evento independente
  * - tarefas podem sinalizar eventos com:
  *      osEventFlagsSet()
  * - tarefas podem aguardar eventos com:
  *      osEventFlagsWait()
  *
  * Neste projeto, o evento @ref BUZZER_EVENT indica que o buzzer foi acionado
  * no seguinte fluxo
  * - BuzzerTask sinaliza o evento
  * - LedTask aguarda o evento (bloqueada)
  * - Ao receber, executa sua ação (troca de LED)
  *
  * Vantagens:
  * - Sincronização eficiente
  * - Baixo uso de CPU
  * - Comunicação simples entre tarefas
  *
  * @section config Configuração do Sistema
  *
  * @subsection tasks Tarefas
  * - defaultTask (osPriorityNormal)
  *   - Tarefa base do sistema com espera não-bloqueante
  *
  * - BuzzerTask (osPriorityLow)
  *   - Gera sinal PWM (200 Hz)
  *   - Ativo por 100 ms a cada 1 segundo
  *   - Sinaliza evento para sincronização
  *
  * - LedTask (osPriorityLow)
  *   - Aguarda evento do buzzer
  *   - Alterna LEDs (B → R → G)
  *
  * @subsection sync Sincronização
  *
  * - Mecanismo: Event Flags
  * - Objeto: buzzerSignalEventHandle
  * - Evento utilizado:
  *      - @ref BUZZER_EVENT
  *
  * @subsection hardware Hardware
  *
  * - TIM5 CH2 → geração de PWM (buzzer)
  * - GPIO → controle dos LEDs (Azul, Vermelho, Verde)
  *
  * @section flow Fluxo de Execução
  *
  * BuzzerTask                LedTask
  * -----------              -------------------
  * Liga buzzer
  * Aguarda 100 ms
  * Desliga buzzer
  * Sinaliza evento  ───────▶  Aguarda evento (bloqueada)
  * Aguarda 900 ms            Recebe evento
  *                           Alterna LED
  * @section notes Observações
  *
  * - A LedTask permanece bloqueada até receber evento (sem polling)
  * - O sistema é orientado a eventos (event-driven)
  * - Não há necessidade de Mutex neste caso
  *
  * - O uso de Event Flags pode ser expandido:
  *      - (1 << 1), (1 << 2), etc.
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
#include "main.h"
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
  * @def    BUZZER_EVENT
  * @brief  Flag de evento utilizada para sinalizar a ativação do buzzer.
  *
  * @details
  * Este bit é utilizado com o mecanismo de Event Flags do CMSIS-RTOS
  * para comunicação entre tarefas.
  *
  * - Sinalizado pela BuzzerTask
  * - Aguardado pela LedTask
  *
  * Quando ativado, indica que o buzzer foi ligado e que a LedTask
  * deve alternar o estado do LED.
  */
#define BUZZER_EVENT   (1 << 0)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for ledTask */
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for buzzerTask */
osThreadId_t buzzerTaskHandle;
const osThreadAttr_t buzzerTask_attributes = {
  .name = "buzzerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for buzzerSignalEvent */
osEventFlagsId_t buzzerSignalEventHandle;
const osEventFlagsAttr_t buzzerSignalEvent_attributes = {
  .name = "buzzerSignalEvent"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void Buzzer_On(void);
void Buzzer_Off(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void LedTask(void *argument);
void BuzzerTask(void *argument);

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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of ledTask */
  ledTaskHandle = osThreadNew(LedTask, NULL, &ledTask_attributes);

  /* creation of buzzerTask */
  buzzerTaskHandle = osThreadNew(BuzzerTask, NULL, &buzzerTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of buzzerSignalEvent */
  buzzerSignalEventHandle = osEventFlagsNew(&buzzerSignalEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Tarefa padrão gerada automaticamente pelo CubeMX.
  *
  * @param  argument Ponteiro genérico (não utilizado).
  * @retval None
  *
  * @details
  * Esta tarefa é criada automaticamente pelo CubeMX com prioridade
  * padrão mais alta (osPriorityNormal). No contexto deste projeto, ela não possui
  * função lógica relevante, sendo mantida apenas para compatibilidade com a estrutura
  * gerada automaticamente.
  *
  * Para evitar interferência no comportamento das demais tarefas:
  * - Foi incluída uma chamada a osDelay()
  * - Isso faz com que a tarefa permaneça a maior parte do tempo bloqueada
  * - Reduz o impacto no escalonamento do sistema
  *
  * Dessa forma, permite que as tarefas de interesse (LedTask e BuzzerTask),
  * com mesma prioridade, definam o comportamento concorrente do sistema.
  *
  * @note Sem o osDelay(), esta tarefa poderia monopolizar a CPU devido à
  *       sua prioridade mais alta.
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Loop infinito com espera não bloqueante */
  for(;;)
  {
    osDelay(10000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_LedTask */
/**
* @brief Tarefa que alterna as cores do LED (B, R, G) ao receber evento do buzzer.
*
* @details
* Esta tarefa permanece bloqueada aguardando um evento sinalizado pela
* BuzzerTask. A cada evento recebido, alterna ciclicamente entre os LEDs:
* Azul → Vermelho → Verde → Azul...
*/
/* USER CODE END Header_LedTask */
void LedTask(void *argument)
{
  /* USER CODE BEGIN LedTask */

  uint8_t state = 0;

  /* Estado inicial: LED Azul ligado */
  HAL_GPIO_WritePin(LedB_GPIO_Port, LedB_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LedR_GPIO_Port, LedR_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LedG_GPIO_Port, LedG_Pin, GPIO_PIN_RESET);

  for(;;)
  {
    /* Aguarda evento do buzzer */
    osEventFlagsWait(buzzerSignalEventHandle,
                     BUZZER_EVENT,
                     osFlagsWaitAny,
                     osWaitForever);

    /* Alterna estado */
    state = (state + 1) % 3;

    /* Atualiza LEDs */
    switch(state)
    {
      case 0: // Azul
        HAL_GPIO_WritePin(LedB_GPIO_Port, LedB_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LedR_GPIO_Port, LedR_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LedG_GPIO_Port, LedG_Pin, GPIO_PIN_RESET);
        break;

      case 1: // Vermelho
        HAL_GPIO_WritePin(LedB_GPIO_Port, LedB_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LedR_GPIO_Port, LedR_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LedG_GPIO_Port, LedG_Pin, GPIO_PIN_RESET);
        break;

      case 2: // Verde
        HAL_GPIO_WritePin(LedB_GPIO_Port, LedB_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LedR_GPIO_Port, LedR_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LedG_GPIO_Port, LedG_Pin, GPIO_PIN_SET);
        break;
    }
  }

  /* USER CODE END LedTask */
}

/**
  * @brief  Loop principal da tarefa do buzzer.
  *
  * @details
  * Este loop executa continuamente a geração de um sinal sonoro periódico
  * e sincroniza esse evento com outras tarefas do sistema.
  *
  * Funcionamento:
  * - Liga o buzzer (inicia PWM)
  * - Sinaliza um evento (BUZZER_EVENT) para notificar outras tarefas
  *   (ex: LedTask)
  * - Mantém o buzzer ativo por 100 ms
  * - Desliga o buzzer
  * - Aguarda 900 ms antes de reiniciar o ciclo
  *
  * O período total do ciclo é de aproximadamente 1 segundo.
  *
  * @note A sinalização via osEventFlagsSet permite sincronização eficiente
  *       entre tarefas sem necessidade de polling.
  */
/* USER CODE END Header_BuzzerTask */
void BuzzerTask(void *argument)
{
  /* USER CODE BEGIN BuzzerTask */
  /* Infinite loop */
  for(;;)
  {
	    Buzzer_On();
	    osDelay(100);

	    Buzzer_Off();

	    /* Sinaliza evento para o LED */
	    osEventFlagsSet(buzzerSignalEventHandle, BUZZER_EVENT);

	    osDelay(900);
  }
  /* USER CODE END BuzzerTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
  * @brief  Liga o buzzer iniciando o PWM no TIM5_CH2.
  *
  * @details
  * Esta função inicia a geração de sinal PWM no canal 2 do TIM5,
  * produzindo o som configurado (ex: 200 Hz).
  *
  * É necessário que o timer já esteja previamente configurado
  * (Prescaler, ARR e duty cycle).
  *
  * @note O duty cycle define o volume percebido do buzzer.
  */
void Buzzer_On(void)
{
  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_2);
}

/**
  * @brief  Desliga o buzzer interrompendo o PWM no TIM5_CH2.
  *
  * @details
  * Esta função para a geração de PWM no canal 2 do TIM5,
  * cessando a emissão de som.
  */
void Buzzer_Off(void)
{
  HAL_TIM_PWM_Stop(&htim5, TIM_CHANNEL_2);
}

/* USER CODE END Application */
