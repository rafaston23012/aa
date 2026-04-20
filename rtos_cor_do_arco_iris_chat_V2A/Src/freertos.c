/* USER CODE BEGIN Header */
/**
   ******************************************************************************
  * @file           : freertos.c
  * @brief          : Controle de LED RGB com transições suaves via PWM (FreeRTOS)
  * @author         : Pedro Henrique Guerra
  * @date           : 20/04/2026
  *
  * @details
  * Este módulo implementa a geração de cores dinâmicas em um LED RGB
  * utilizando modulação por largura de pulso (PWM) controlada por timers.
  *
  * O sistema percorre uma sequência de cores predefinidas (arco-íris),
  * aplicando transições suaves entre elas por meio de interpolação linear.
  *
  * A execução é realizada dentro de uma task do FreeRTOS, garantindo
  * comportamento não bloqueante para o restante do sistema (com exceção
  * da função de transição, que utiliza delay interno controlado).
  *
  * ---------------------------------------------------------------------------
  * Arquitetura:
  *
  * Task (RainbowTask)
  *     ↓
  * Seleção de cor da tabela
  *     ↓
  * Aplicação direta (set_rgb)
  *     ↓
  * Transição suave (interpolação)
  *     ↓
  * Atualização PWM (TIM CCR)
  *
  * ---------------------------------------------------------------------------
  * Características:
  *
  * - Controle direto de hardware via registradores CCR
  * - Transições suaves com interpolação linear
  * - Sequência cíclica de cores (efeito arco-íris)
  * - Baixo custo computacional
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
#include "tim.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
 * @def PWM_STEPS
 * @brief Número de passos utilizados para interpolação entre duas cores.
 */
#define PWM_STEPS      100
/**
 * @def COLOR_HOLD_MS
 * @brief Tempo em milissegundos que cada cor permanece estática.
 */
#define COLOR_HOLD_MS  3000
/**
 * @def TRANSITION_MS
 * @brief Tempo total da transição entre duas cores consecutivas.
 */
#define TRANSITION_MS  2000
/**
 * @def RAINBOW_SIZE
 * @brief Número total de cores presentes na tabela de cores do arco-íris.
 */
#define RAINBOW_SIZE (sizeof(rainbow) / sizeof(rainbow[0]))
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/**
 * @brief Tabela de cores do espectro arco-íris.
 *
 * Cada entrada representa uma cor no formato:
 * {R, G, B}
 *
 * Os valores correspondem ao duty cycle aplicado ao PWM
 * de cada canal RGB.
 */
static const uint16_t rainbow[][3] =
{
    {1023,   0,   0},   /**< Vermelho */
    {1023, 165,   0},   /**< Laranja */
    {1023,1023,   0},   /**< Amarelo */
    {   0,1023,   0},   /**< Verde */
    {   0,   0,1023},   /**< Azul */
    {  75,   0, 130},   /**< Anil */
    { 238, 130, 238}    /**< Violeta */
};
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for rainbowTask */
osThreadId_t rainbowTaskHandle;
const osThreadAttr_t rainbowTask_attributes = {
  .name = "rainbowTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void transition(const uint16_t *from, const uint16_t *to);
static void set_rgb(uint16_t r, uint16_t g, uint16_t b);
/* USER CODE END FunctionPrototypes */

void RainbowTask(void *argument);

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
  /* creation of rainbowTask */
  rainbowTaskHandle = osThreadNew(RainbowTask, NULL, &rainbowTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_RainbowTask */
/**
  * @brief Task responsável pelo controle dinâmico das cores do LED RGB.
  *
  * @details
  * Esta task executa continuamente um ciclo de cores baseado
  * na tabela @ref rainbow.
  *
  * Para cada cor:
  * 1. Aplica a cor diretamente ao LED
  * 2. Aguarda um período fixo (@ref COLOR_HOLD_MS)
  * 3. Realiza transição suave para a próxima cor
  *
  * Ao final da sequência:
  * - Desliga o LED
  * - Aguarda antes de reiniciar o ciclo
  *
  * @note A transição utiliza delay interno, portanto a task
  *       ocupa CPU durante esse período.
  *
  * @param argument Parâmetro não utilizado
  */
/* USER CODE END Header_RainbowTask */
void RainbowTask(void *argument)
{
  /* USER CODE BEGIN RainbowTask */
  /* Infinite loop */
	while(1)
	    {
	        for (uint32_t i = 0; i < RAINBOW_SIZE; i++)
	        {
	            set_rgb(rainbow[i][0], rainbow[i][1], rainbow[i][2]);

	            osDelay(COLOR_HOLD_MS);

	            if (i < RAINBOW_SIZE - 1)
	            {
	                transition(rainbow[i], rainbow[i+1]);
	            }
	        }

	        set_rgb(0,0,0);

	        osDelay(2000); // pausa antes de reiniciar
	    }
  /* USER CODE END RainbowTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief Atualiza os duty cycles PWM dos canais RGB.
 *
 * @param r Valor PWM do canal vermelho
 * @param g Valor PWM do canal verde
 * @param b Valor PWM do canal azul
 *
 * @details
 * Cada valor é aplicado diretamente ao registrador CCR
 * correspondente do timer responsável pelo canal PWM.
 */

static void set_rgb(uint16_t r, uint16_t g, uint16_t b)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, r);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, g);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, b);
}

/**
 * @brief Realiza transição suave entre duas cores.
 *
 * @param from Ponteiro para a cor inicial no formato RGB.
 * @param to Ponteiro para a cor final no formato RGB.
 *
 * @details
 * A transição é feita através de interpolação linear em
 * @ref PWM_STEPS passos, atualizando gradualmente os valores
 * PWM de cada canal.
 */

static void transition(const uint16_t *from, const uint16_t *to)
{
    for (uint32_t i = 0; i < PWM_STEPS; i++)
    {
        uint16_t r = from[0] + (i * (to[0] - from[0])) / PWM_STEPS;
        uint16_t g = from[1] + (i * (to[1] - from[1])) / PWM_STEPS;
        uint16_t b = from[2] + (i * (to[2] - from[2])) / PWM_STEPS;

        set_rgb(r,g,b);

        HAL_Delay(TRANSITION_MS / PWM_STEPS);
    }
}
/* USER CODE END Application */

