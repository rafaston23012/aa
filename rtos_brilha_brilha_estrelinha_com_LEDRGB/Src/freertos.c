/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : freertos.c
  * @brief          : Reprodução de melodia com buzzer e controle de LED RGB via FreeRTOS
  * @author         : Pedro Henrique Guerra
  * @date           : 20/04/2026
  *
  * @details
  * Este firmware implementa a reprodução da música "Brilha Brilha Estrelinha"
  * utilizando um buzzer controlado por frequência e um LED RGB sincronizado
  * com cada nota musical.
  *
  * A arquitetura do sistema é baseada em FreeRTOS utilizando o modelo
  * Produtor–Consumidor:
  *
  * - MusicTask (Produtor):
  *      Percorre a sequência musical e envia notas para uma fila.
  *
  * - ControlTask (Consumidor):
  *      Recebe as notas da fila e aplica no hardware (buzzer + LED RGB).
  *
  * Comunicação entre tarefas:
  * - Utiliza uma fila (queue) para garantir sincronização e desacoplamento.
  *
  * Fluxo de execução:
  *
  * MusicTask → Queue → ControlTask → Hardware (Buzzer + LED)
  *
  * Características do sistema:
  * - Execução não bloqueante entre tarefas
  * - Separação clara entre lógica musical e controle físico
  * - Facilidade de expansão (novas músicas ou efeitos)
  *
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/**
 * @struct Nota
 * @brief Representa uma nota musical associada a uma cor RGB.
 *
 * @details
 * Esta estrutura encapsula todos os parâmetros necessários para
 * representar um evento musical completo:
 *
 * - freq: Frequência da nota (Hz) aplicada ao buzzer
 * - r,g,b: Intensidade dos canais RGB (0–255)
 * - dur: Duração relativa da nota (multiplicador de tempo base)
 *
 * Isso permite sincronizar áudio e efeito visual de forma simples.
 */
typedef struct {
    uint16_t freq;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t dur;
} Nota;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/**
 * @def C4
 * @brief nota C4 e sua respectiva frequência audível
 */
#define C4 261
/**
 * @def D4
 * @brief nota D4 e sua respectiva frequência audível
 */
#define D4 294
/**
 * @def E4
 * @brief nota E4 e sua respectiva frequência audível
 */
#define E4 329
/**
 * @def F4
 * @brief nota F4 e sua respectiva frequência audível
 */
#define F4 349
/**
 * @def G4
 * @brief nota G4 e sua respectiva frequência audível
 */
#define G4 392
/**
 * @def A4
 * @brief nota A4 e sua respectiva frequência audível
 */
#define A4 440
/**
 * @def B4
 * @brief nota B4 e sua respectiva frequência audível
 */
#define B4 494
/**
 * @def C5
 * @brief nota C5 e sua respectiva frequência audível
 */
#define C5 523
/**
 * @def PAUSA
 * @brief valor do intervalo entre cada nota
 */
#define PAUSA 0
/**
 * @def MUSICA_LEN
 * @brief tamanho da musica a ser tocada
 */
#define MUSICA_LEN (sizeof(musica)/sizeof(Nota))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/**
 * @brief Sequência de notas que compõem a música.
 *
 * @details
 * Cada elemento define:
 * - frequência da nota
 * - cor do LED RGB
 * - duração relativa da nota
 *
 * A música executada é uma variação de "Brilha brilha estrelinha".
 */
static const Nota musica[] = {
		{C4,255,0,0,1},{C4,255,0,0,1},{G4,0,0,255,1},{G4,0,0,255,1},
		{A4,75,0,130,1},{A4,75,0,130,1},{G4,0,0,255,2},

		{F4,0,255,0,1},{F4,0,255,0,1},{E4,255,255,0,1},{E4,255,255,0,1},
		{D4,255,127,0,1},{D4,255,127,0,1},{C4,255,0,0,2},

		{G4,0,0,255,1},{G4,0,0,255,1},{F4,0,255,0,1},{F4,0,255,0,1},
		{E4,255,255,0,1},{E4,255,255,0,1},{D4,255,127,0,2},

		{G4,0,0,255,1},{G4,0,0,255,1},{F4,0,255,0,1},{F4,0,255,0,1},
		{E4,255,255,0,1},{E4,255,255,0,1},{D4,255,127,0,2},

		{C4,255,0,0,1},{C4,255,0,0,1},{G4,0,0,255,1},{G4,0,0,255,1},
		{A4,75,0,130,1},{A4,75,0,130,1},{G4,0,0,255,2},

		{F4,0,255,0,1},{F4,0,255,0,1},{E4,255,255,0,1},{E4,255,255,0,1},
		{D4,255,127,0,1},{D4,255,127,0,1},{C4,255,0,0,2},
};
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for musicTask */
osThreadId_t musicTaskHandle;
const osThreadAttr_t musicTask_attributes = {
  .name = "musicTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for controlTask */
osThreadId_t controlTaskHandle;
const osThreadAttr_t controlTask_attributes = {
  .name = "controlTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for notaQueue */
/**
 * @brief Fila de comunicação entre MusicTask e ControlTask.
 *
 * @details
 * Esta fila transporta estruturas do tipo Nota entre tarefas:
 *
 * - MusicTask (produtor): envia notas da sequência musical
 * - ControlTask (consumidor): recebe e aplica no hardware
 *
 * @note
 * O uso de fila garante sincronização e desacoplamento entre as tarefas.
 */
osMessageQueueId_t notaQueueHandle;
const osMessageQueueAttr_t notaQueue_attributes = {
  .name = "notaQueue"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void MusicTask(void *argument);
void ControlTask(void *argument);

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
  /* creation of notaQueue */
  notaQueueHandle = osMessageQueueNew (5, sizeof(Nota), NULL);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of musicTask */
  musicTaskHandle = osThreadNew(MusicTask, NULL, &musicTask_attributes);

  /* creation of controlTask */
  controlTaskHandle = osThreadNew(ControlTask, NULL, &controlTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_MusicTask */
/**
 * @brief Tarefa responsável por executar a sequência musical.
 *
 * @details
 * Esta tarefa percorre o vetor de notas (musica[]) e envia cada nota
 * para a fila notaQueue.
 *
 * Para cada nota:
 * - Envia a nota para a fila
 * - Aguarda o tempo proporcional à duração
 * - Envia uma pausa (nota com frequência zero)
 *
 * Após completar a sequência, aguarda antes de reiniciar.
 *
 * @param argument Parâmetro não utilizado
 *
 * @note
 * Esta tarefa atua como produtora no modelo produtor-consumidor.
 */
/* USER CODE END Header_MusicTask */

void MusicTask(void *argument)
{
  /* USER CODE BEGIN MusicTask */
  /* Infinite loop */
    Nota pausa = {0,0,0,0,0};

    while(1)
    {
        for(uint32_t i = 0; i < MUSICA_LEN; i++)
        {
            osMessageQueuePut(notaQueueHandle, &musica[i], 0, osWaitForever);

            osDelay(400 * musica[i].dur);

            osMessageQueuePut(notaQueueHandle, &pausa, 0, osWaitForever);

            osDelay(70);
        }

        osDelay(5000);
    }
  /* USER CODE END MusicTask */
}

/* USER CODE BEGIN Header_ControlTask */
/**
 * @brief Tarefa responsável por aplicar as notas no hardware.
 *
 * @details
 * Esta tarefa aguarda por mensagens na fila notaQueue e,
 * ao receber uma nota:
 *
 * - Atualiza a frequência do buzzer
 * - Atualiza a cor do LED RGB
 *
 * @param argument Parâmetro não utilizado
 *
 * @note
 * Esta tarefa atua como consumidora no modelo produtor-consumidor.
 *
 * @warning
 * Esta tarefa deve ter prioridade maior para garantir resposta imediata.
 */
/* USER CODE END Header_ControlTask */
void ControlTask(void *argument)
{
  /* USER CODE BEGIN ControlTask */
  /* Infinite loop */
	Nota nota;

	    while(1)
	    {
	        if(osMessageQueueGet(notaQueueHandle, &nota, NULL, osWaitForever) == osOK)
	        {
	            buzzer_set_freq(nota.freq);
	            led_set_rgb(nota.r, nota.g, nota.b);
	        }
	    }
  /* USER CODE END ControlTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

