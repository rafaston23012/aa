/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    freertos.c
  * @brief   FreeRTOS application and NeoPixel control task
  * @author Wu Shin Ting
  * @date   21/03/2026
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
  * Este arquivo contém a inicialização do FreeRTOS e a implementação da tarefa
  * responsável pelo controle de uma cadeia de LEDs NeoPixel (WS2812).
  *
  * O sistema foi projetado para operar com uma única tarefa RTOS dedicada ao
  * controle dos LEDs, garantindo simplicidade e previsibilidade temporal.
  *
  * A aplicação demonstra:
  * - Inicialização de periféricos para controle de WS2812 via PWM + DMA
  * - Manipulação de cores e padrões em LEDs
  * - Uso básico de tarefas no FreeRTOS
  *
  * Dois padrões principais são exibidos:
  * - Sequência inicial com múltiplas configurações de LEDs
  * - Alternância contínua entre dois estados nos LEDs 0 e 1
  *
  * @section user_code Código Implementado Manualmente
  *
  * O código dentro das regiões USER CODE inclui:
  * - Inicialização do driver WS2812:
  *   - ws2812_color_init()
  *   - ws2812_pwm_init()
  *
  * - Configuração de brilho global
  *
  * - Construção de padrões visuais:
  *   - Preenchimento total (fill)
  *   - Gradiente linear entre LEDs
  *   - Definição de pixels individuais e intervalos
  *
  * - Atualização dos LEDs via:
  *   - ws2812_pwm_show()
  *
  * - Lógica de alternância (toggle) entre dois estados:
  *   - LED 0 e LED 1 trocam suas cores periodicamente
  *
  * - Uso de osDelay() para controle temporal não bloqueante
  *
  * @section mental_model Modelo Mental do Sistema
  *
  * O sistema segue o modelo de:
  *
  *   "Single Owner Resource"
  *
  * onde
  * - A tarefa NeopixelTask é a única responsável pelo acesso ao hardware WS2812
  * - Não há concorrência sobre o periférico
  *
  * Fluxo simplificado:
  *
  *   [Init] -> [Configuração inicial LEDs] -> [Loop infinito]
  *                                              |
  *                                              v
  *                                     [Alterna estado A/B]
  *                                              |
  *                                              v
  *                                      [Atualiza LEDs]
  *                                              |
  *                                              v
  *                                          [Delay]
  *
  * Estados:
  * - Estado A:
  *     LED0 = (255,100,0)
  *     LED1 = (255,0,0)
  *
  * - Estado B:
  *     LED0 = (255,0,0)
  *     LED1 = (255,100,0)
  *
  * A alternância é determinística e sequencial, não dependendo de sincronização
  * entre múltiplas tarefas.
  *
  * @section cube_config Configuração no CubeMX
  *
  * Este projeto foi configurado utilizando o STM32CubeMX com as seguintes decisões:
  *
  * - FreeRTOS:
  *   - Uma única tarefa (neopixelTask)
  *   - Prioridade: Normal
  *   - Stack: 512 bytes
  *
  * - Timers:
  *   - TIM1 configurado para geração de PWM
  *   - Canal utilizado: TIM1 Channel 3
  *   - Evento de update habilitado para DMA
  *
  * - DMA:
  *   - Canal associado ao TIM1 Update
  *   - Utilizado para alimentar o CCR do timer (geração do sinal WS2812)
  *
  * - GPIO:
  *   - Pino configurado como saída do canal PWM
  *
  * - Estratégia adotada:
  *   - Uso de PWM + DMA para atender aos requisitos de temporização do WS2812
  *   - Evitar bit-banging por CPU
  *
  * - Arquitetura RTOS:
  *   - Apenas uma tarefa controla o periférico
  *   - Não há uso de mutex, semáforos ou filas neste exemplo
  *   - Justificativa: os estados são previsíveis e sequenciais
  *
  * @note
  * Este projeto prioriza simplicidade e clareza didática.
  * Em aplicações mais complexas, recomenda-se:
  * - Uso de filas para comunicação entre tarefas
  * - Separação entre lógica de aplicação e driver de hardware
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
#include "ws2812_pwm.h"
#include "ws2812_color.h"
#include "tim.h"
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
/* Definitions for neopixelTask */
osThreadId_t neopixelTaskHandle;
const osThreadAttr_t neopixelTask_attributes = {
  .name = "neopixelTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void NeopixelTask(void *argument);

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
  /* creation of neopixelTask */
  neopixelTaskHandle = osThreadNew(NeopixelTask, NULL, &neopixelTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_NeopixelTask */
/**
  * @brief  RTOS task responsible for driving NeoPixel updates.
  *
  * @param  argument: Not used.
  * @retval None
  *
  * @details
  * Esta tarefa executa duas fases distintas:
  *
  * 1. Sequência inicial de configuração visual:
  *    - Inicializa os módulos de controle de cor e PWM
  *    - Define brilho global
  *    - Preenche todos os LEDs com uma cor base
  *    - Aplica padrões adicionais:
  *        - Gradiente linear em um intervalo de LEDs
  *        - Segmento fixo com cor sólida
  *        - Pixel individual com cor específica
  *    - Cada etapa é seguida de atualização física dos LEDs e um pequeno atraso
  *
  * 2. Loop contínuo de alternância:
  *    - Alterna ciclicamente o estado de dois LEDs específicos (0 e 1)
  *    - A alternância é controlada por uma variável local (toggle)
  *    - A cada iteração:
  *        - Atualiza os valores no buffer
  *        - Envia os dados para os LEDs
  *        - Aguarda um intervalo fixo
  *
  * @note
  * - Todas as atualizações visuais só são efetivadas após chamada de ws2812_pwm_show().
  * - O uso de osDelay() permite escalonamento cooperativo com outras tarefas.
  * - A variável 'toggle' implementa um flip-flop simples para alternância de estados.
  *
  * @warning
  * Esta tarefa assume acesso exclusivo ao periférico WS2812.
  * Não é seguro acessar o driver de LEDs a partir de múltiplas tarefas sem sincronização.
  *
  * @see ws2812_pwm_show
  * @see ws2812_set_pixel
  * @see ws2812_set_pixel_line_gradient
  */
/* USER CODE END Header_NeopixelTask */
void NeopixelTask(void *argument)
{
  /* USER CODE BEGIN NeopixelTask */
	/* Inicializar com a quantidade máxima, 25 LEDs */
	ws2812_color_init(25);
	ws2812_pwm_init();

	ws2812_set_brightness(50);   /* intensidade da cor */

	ws2812_fill_slots ();     /* preencher os slots que marcam fim da cadeia de LEDs */

	ws2812_fill(255, 50, 0);  /* preencher o buffer dos LEDs com "orange" */

	ws2812_pwm_show();			  /* mostrar nos LEDs */

	osDelay(100);           /* tempo de espera */

	/* setar no buffer o gradiente linear aritmético entre verde (LED 3) a azul (LED 13) */
	ws2812_set_pixel_line_gradient(3, 13, 0, 255, 0, 0, 0, 255);

	ws2812_pwm_show();   /* mostra nos LEDs */

	osDelay (100);            /* tempo de espera */

	ws2812_set_pixel_line(14, 16, 255, 0, 0); /* setar no buffer a cor vermelha do LED 14 a 16 */

	ws2812_pwm_show();

	osDelay (100);            /* tempo de espera */

	ws2812_set_pixel(20, 255, 255, 255);

	ws2812_pwm_show();

	osDelay (100);            /* tempo de espera */

	uint8_t toggle = 0;
	/* Infinite loop */
	for(;;)
	{
		if (toggle)
		{
			ws2812_set_pixel(0, 255, 100, 0); /* yellow */
			ws2812_set_pixel(1, 255, 0, 0);   /* red */
		}
		else
		{
			ws2812_set_pixel(0, 255, 0, 0);
			ws2812_set_pixel(1, 255, 100, 0);
		}
		toggle ^= 1;

		ws2812_pwm_show();

		osDelay (100);            /* tempo de espera */

	}
  /* USER CODE END NeopixelTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

