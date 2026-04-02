/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file         : freertos.c
  * @brief        : Code for freertos applications
  * @author 	  : Wu Shin Ting
  * @date		  : 02/04/2026
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
  * Este arquivo contém a configuração e implementação de um sistema embarcado
  * baseado em FreeRTOS que utiliza o padrão de projeto Producer–Consumer para
  * controle de uma cadeia de LEDs endereçáveis (NeoPixel WS2812).
  *
  * Arquitetura:
  * - Uma fila RTOS (@ref neopixelQueueHandle) é utilizada para comunicação
  *   entre tarefas.
  * - A tarefa @ref ProducerTask gera comandos pseudoaleatórios contendo:
  *      - índice do LED
  *      - cor a ser exibida
  * - A tarefa @ref ConsumerTask consome esses comandos e atualiza os LEDs.
  *
  * Comunicação e sincronização:
  * - A troca de dados entre tarefas é feita via @ref osMessageQueue
  * - A atualização dos LEDs utiliza SPI com DMA para eficiência
  * - A sincronização da conclusão do DMA é realizada via
  *   @ref osEventFlags (spi_dmaEventHandle), sinalizada em interrupção
  *
  * Fluxo de execução:
  * @code
  * ProducerTask → Queue → ConsumerTask → SPI/DMA → ISR → EventFlag → ConsumerTask
  * @endcode
  *
  * Características principais:
  * - Uso de comunicação assíncrona entre tarefas (desacoplamento)
  * - Atualização eficiente de LEDs via DMA (sem bloqueio ativo)
  * - Sincronização segura entre contexto de interrupção e tarefas
  *
  * Dependências:
  * - CMSIS-RTOS v2 (FreeRTOS)
  * - HAL STM32 (SPI, DMA)
  * - Biblioteca WS2812 customizada
  *
  * @note
  * - O sistema assume acesso exclusivo ao barramento SPI durante a transmissão
  * - O buffer de LEDs não deve ser modificado durante operação de DMA
  *
  * @warning
  * - rand() é utilizado para geração de dados e não é thread-safe em geral
  * - Em aplicações críticas, recomenda-se uso de RNG por hardware
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
#include <stdlib.h>  // rand, srand

#include "spi.h"
#include "ws2812_spi.h"
#include "ws2812_color.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
    int ledn;   // número do LED (0–24)
    int ledc;   // cor ou código (1–7)
} LedMsg_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define NUM_LEDS 25   // qtde de LEDs no NeoPixel
/**
 * @brief Largura do pulso que representa '0'
 */
#define WS2812_0  0b11000000

/**
 * @brief Largura do pulso que representa '1'
 */
#define WS2812_1  0b11111100

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
 LedMsg_t msg; // cria a struct para os dados da fila
/* USER CODE END Variables */
/* Definitions for producerTask */
osThreadId_t producerTaskHandle;
const osThreadAttr_t producerTask_attributes = {
  .name = "producerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for consumerTask */
osThreadId_t consumerTaskHandle;
const osThreadAttr_t consumerTask_attributes = {
  .name = "consumerTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for neopixelQueue */
osMessageQueueId_t neopixelQueueHandle;
const osMessageQueueAttr_t neopixelQueue_attributes = {
  .name = "neopixelQueue"
};
/* Definitions for spi_dmaEvent */
osEventFlagsId_t spi_dmaEventHandle;
const osEventFlagsAttr_t spi_dmaEvent_attributes = {
  .name = "spi_dmaEvent"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
uint8_t pwm_1(void);  /*< Retorna o padrão PWM correspondente ao bit lógico '1' do WS2812 */
uint8_t pwm_0(void); /*< Retorna o padrão PWM correspondente ao bit lógico '0' do WS2812 */

ws2812_status_t ws2812_spi5_dma_show(uint8_t *dma_buffer, uint32_t DataLength); /*< Inicia transmissão SPI via DMA (não bloqueante) */

ws2812_status_t ws2812_spi_show (void); /*< Atualização sequencial: inicia DMA e aguarda término (bloqueante via event flag) */
ws2812_status_t ws2812_spi_show_compartilhado (void); /*< Atualização concorrente: inicia DMA e retorna imediatamente (sincronização externa) */
osEventFlagsId_t getSpi_dmaEventHandle (); /*< Retorna o handle da event flag usada para sincronização ISR ↔ tarefa */
/* USER CODE END FunctionPrototypes */

void ProducerTask(void *argument);
void ConsumerTask(void *argument);

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
  /* creation of neopixelQueue */
  neopixelQueueHandle = osMessageQueueNew (10, sizeof(LedMsg_t), &neopixelQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of producerTask */
  producerTaskHandle = osThreadNew(ProducerTask, NULL, &producerTask_attributes);

  /* creation of consumerTask */
  consumerTaskHandle = osThreadNew(ConsumerTask, NULL, &consumerTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */
  /* creation of spi_dmaEvent */
  spi_dmaEventHandle = osEventFlagsNew(&spi_dmaEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_ProducerTask */
/**
  * @brief  Tarefa produtora responsável por gerar comandos para LEDs.
  *
  * @details
  * Esta tarefa implementa o papel de *Producer* no padrão clássico
  * Producer–Consumer utilizando uma fila do FreeRTOS.
  *
  * Funcionamento:
  * - Inicializa o gerador de números pseudoaleatórios utilizando
  *   o tempo de sistema (HAL_GetTick).
  * - A cada iteração:
  *    - Gera um índice de LED (0 a 24)
  *    - Gera uma cor (1 a 7)
  *    - Envia a estrutura @ref LedMsg_t para a fila @ref neopixelQueueHandle
  *
  * Comunicação:
  * - Utiliza @ref osMessageQueuePut para envio de mensagens
  * - Timeout de 100 ms caso a fila esteja cheia
  *
  * Temporização:
  * - Executa periodicamente a cada 1 segundo
  *
  * @param  argument Ponteiro genérico (não utilizado)
  * @retval None
  *
  * @note
  * - Não há garantia de envio caso a fila esteja cheia
  * - Pode ser estendida para tratamento de erro (ex: debug via UART)
  *
  * @warning
  * - rand() não é thread-safe em todos os ambientes
  * - Em aplicações críticas, considerar RNG por hardware
  */
/* USER CODE END Header_ProducerTask */
void ProducerTask(void *argument)
{
  /* USER CODE BEGIN ProducerTask */

	  // Inicializa o gerador randômico (seed)
	  srand(HAL_GetTick());

  /* Infinite loop */
  for(;;)
  {
      msg.ledn = rand() % 25;      // valor 0–24
      msg.ledc = (rand() % 7) + 1; // valor 1–7

      // Envia para a fila (timeout de 100 ms)
      osMessageQueuePut(neopixelQueueHandle, &msg, 0, 100);

      osDelay(1000); // envia a cada 1s
  }
  /* USER CODE END ProducerTask */
}

/* USER CODE BEGIN Header_ConsumerTask */
/**
  * @brief  Tarefa consumidora responsável por atualizar LEDs WS2812.
  *
  * @details
  * Esta tarefa implementa o papel de *Consumer* no padrão Producer–Consumer,
  * recebendo mensagens de uma fila e aplicando
  * as configurações nos LEDs endereçáveis (WS2812).
  *
  * Inicialização:
  * - Configura a quantidade de LEDs
  * - Define o brilho global
  * - Prepara os slots de reset da cadeia WS2812
  *
  * Funcionamento:
  * - Aguarda indefinidamente por mensagens da fila
  *   @ref neopixelQueueHandle (modo bloqueante)
  * - Ao receber uma mensagem:
  *    - Percorre todos os LEDs
  *    - Acende apenas o LED indicado em @ref LedMsg_t.ledn
  *    - Define a cor conforme @ref LedMsg_t.ledc
  *    - Apaga os demais LEDs
  *
  * Cores suportadas:
  * - 1: Vermelho
  * - 2: Verde
  * - 3: Azul
  * - 4: Amarelo
  * - 5: Ciano
  * - 6: Magenta
  * - 7: Branco
  *
  * Atualização:
  * - Envia os dados via SPI utilizando DMA
  *   (@ref ws2812_spi_show_compartilhado)
  *
  * Sincronização:
  * - Aguarda término da transmissão via
  *   @ref osEventFlagsWait
  * - Evento sinalizado no callback de DMA
  *
  * @param  argument Ponteiro genérico (não utilizado)
  * @retval None
  *
  * @note
  * - A tarefa é bloqueante enquanto aguarda dados da fila
  * - Garante atualização consistente dos LEDs sem busy-wait
  *
  * @warning
  * - O buffer de LEDs não deve ser modificado durante o DMA
  * - Em cenários com múltiplos produtores, considerar uso de mutex
  */
/* USER CODE END Header_ConsumerTask */
void ConsumerTask(void *argument)
{
  /* USER CODE BEGIN ConsumerTask */
	ws2812_color_init(25);

	ws2812_set_brightness(50);   /* intensidade da cor */

	ws2812_fill_slots ();     /* preencher os slots que marcam fim da cadeia de LEDs */
	/* Infinite loop */
	for(;;)
	{
		// Aguarda receber mensagem da fila (bloqueante)
		if (osMessageQueueGet(neopixelQueueHandle, &msg, NULL, osWaitForever) == osOK)
		{
			for (int i = 0; i < NUM_LEDS; i++) {
				uint8_t R = 0, G = 0, B = 0;

				if (i == msg.ledn) {
					switch(msg.ledc) {
						case 1: R=255; G=0;   B=0;   break; // vermelho
						case 2: R=0;   G=255; B=0;   break; // verde
						case 3: R=0;   G=0;   B=255; break; // azul
						case 4: R=255; G=255; B=0;   break; // amarelo
						case 5: R=0;   G=255; B=255; break; // ciano
						case 6: R=255; G=0;   B=255; break; // magenta
						case 7: R=255; G=255; B=255; break; // branco
						default: R=0; G=0; B=0; break;
					}
				}

				// Atualiza pixel (outros LEDs ficam apagados)
				ws2812_set_pixel(i, R, G, B);
			}

			/* Iniciar transmissão DMA */
			ws2812_spi_show_compartilhado();

			/* Esperar DMA terminar */
			osEventFlagsWait(
				spi_dmaEventHandle,
				1U,
				osFlagsWaitAny,
				osWaitForever
			);
		}
	}
  /* USER CODE END ConsumerTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief Retorna o valor de PWM correspondente ao bit lógico '1' do protocolo WS2812.
 *
 * Este valor define o duty-cycle necessário para representar um bit '1'
 * na temporização do WS2812, conforme especificação do dispositivo
 * (https://www.superlightingled.com/PDF/WS2818.pdf).
 *
 * @return Valor de comparação (CCR) para PWM representando bit '1'.
 */
uint8_t pwm_1(void)
{
	return WS2812_1;
}

/**
 * @brief Retorna o valor de PWM correspondente ao bit lógico '0' do protocolo WS2812.
 *
 * Este valor define o duty-cycle necessário para representar um bit '0'
 * na temporização do WS2812, conforme especificação do dispositivo
 * (https://www.superlightingled.com/PDF/WS2818.pdf).
 *
 * @return Valor de comparação (CCR) para PWM representando bit '0'.
 */
uint8_t pwm_0(void)
{
	return WS2812_0;
}

/**
 * @brief Inicia a transmissão de dados para NeoPixel via SPI utilizando DMA.
 *
 * @param dma_buffer Ponteiro para o buffer contendo os dados codificados
 *        no formato compatível com o protocolo WS2812.
 * @param DataLength Número de elementos a serem transmitidos.
 *
 * @retval ws2812_status_t
 * - @c WS2812_OK em caso de sucesso
 * - @c WS2812_ERRO em caso de falha na inicialização da transmissão
 *
 * @details
 * Esta função:
 * - marca o periférico como ocupado (@c spi_dma_busy = 1)
 * - inicia uma transmissão SPI em modo DMA
 * - retorna imediatamente (operação não bloqueante)
 *
 * A conclusão da transmissão é sinalizada posteriormente pelo callback
 * @ref HAL_SPI_TxCpltCallback.
 *
 * Fluxo:
 * @code
 * chamada → inicia DMA → retorno imediato → ISR → callback → libera flag
 * @endcode
 *
 * @note
 * - A função não aguarda o término da transmissão.
 * - O buffer deve permanecer válido até o fim da operação.
 *
 * @note
 * - A biblioteca HAL não cobre explicitamente todas as possíveis combinações
 *   de transferência via DMA entre periféricos e memória. Em muitos casos,
 *   funções de alto nível (como @c HAL_SPI_Transmit_DMA) encapsulam apenas
 *   os cenários mais comuns.
 * - Para aplicações mais avançadas ou não convencionais, pode ser necessário
 *   configurar e iniciar o DMA diretamente (ex: @c HAL_DMA_Start_IT) e
 *   gerenciar manualmente os callbacks e a integração com o periférico.
 *
 * @warning
 * - Não iniciar nova transmissão enquanto @c spi_dma_busy estiver ativo.
 */
ws2812_status_t ws2812_spi5_dma_show(uint8_t *dma_buffer, uint32_t DataLength)
{

	HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA (
			&hspi5,
			dma_buffer,
			DataLength
	);

	if (status != HAL_OK) return WS2812_ERRO;

	return WS2812_OK;
}

/**
  * @brief Atualiza a cadeia de LEDs WS2812 via SPI + DMA (modo sequencial).
  *
  * @retval ws2812_status_t
  * - @c WS2812_OK em caso de sucesso
  * - Código de erro em caso de falha na inicialização do DMA
  *
  * @details
  * Esta função implementa um modelo de atualização **bloqueante**:
  *
  * 1. Obtém o endereço do buffer DMA contendo os dados codificados
  * 2. Calcula o tamanho total da transmissão:
  *    - 24 bits por LED
  *    - Slots adicionais de reset (@c WS2812_RESET_SLOTS)
  * 3. Inicia a transmissão via SPI utilizando DMA
  * 4. Aguarda o término da transmissão utilizando @c osEventFlagsWait
  *
  * Fluxo:
  * @code
  * start DMA → wait event → return
  * @endcode
  *
  * A sincronização com o término da transmissão é feita por meio de uma
  * event flag (@c spi_dmaEventHandle), sinalizada no callback de interrupção
  * @c HAL_SPI_TxCpltCallback.
  *
  * @note
  * - Esta função é bloqueante: a execução só retorna após o fim do DMA.
  * - Não requer controle externo de sincronização.
  * - Adequada para inicialização ou uso sem concorrência.
  *
  * @warning
  * - Em sistemas com múltiplos acessos ao buffer, deve-se garantir proteção
  *   externa (ex: mutex) para evitar condições de corrida.
  *
  * @see ws2812_spi_show_compartilhado
  * @see ws2812_spi5_dma_show
  */
ws2812_status_t ws2812_spi_show (void)
{
	uint8_t * address = ws2812_get_dma_buffer();
	uint32_t led_count = ws2812_get_led_count();

	ws2812_status_t  estado = ws2812_spi5_dma_show(
			address,
			led_count * 24 + WS2812_RESET_SLOTS
	);

	if (estado != WS2812_OK) return estado;

	osEventFlagsWait(
			spi_dmaEventHandle,
			1U,
			osFlagsWaitAny,
			osWaitForever
	);

	return WS2812_OK;
}

/**
  * @brief Inicia a atualização dos LEDs WS2812 via SPI + DMA (modo compartilhado).
  *
  * @retval ws2812_status_t
  * - @c WS2812_OK em caso de sucesso
  * - Código de erro em caso de falha na inicialização do DMA
  *
  * @details
  * Esta função NÃO é bloqueante por si só. Ela
  *
  * 1. obtém o buffer DMA com os dados dos LEDs
  * 2. calcula o tamanho total da transmissão
  * 3. inicia o DMA
  * 4. retorna imediatamente
  *
  * Fluxo:
  * @code
  * start DMA → return
  * @endcode
  *
  * A responsabilidade de sincronização é delegada ao chamador, que deve:
  *
  * @code
  * osMutexAcquire()
  * start DMA
  * osEventFlagsWait()
  * osMutexRelease()
  * @endcode
  *
  * Esse modelo permite:
  * - controle explícito da seção crítica
  * - integração com mutex para proteção do buffer
  * - evitar bloqueios internos na função
  *
  * @note
  * - Não aguarda o término da transmissão.
  * - Deve ser usada em conjunto com mecanismos de sincronização (event flags).
  *
  * @warning
  * - O buffer não deve ser modificado até o término do DMA.
  * - Falha em sincronizar corretamente pode causar corrupção de dados.
  *
  * @see ws2812_spi_show
  * @see ws2812_spi5_dma_show
  */
ws2812_status_t ws2812_spi_show_compartilhado (void)
{
	uint8_t * address = ws2812_get_dma_buffer();
	uint32_t led_count = ws2812_get_led_count();

	return ws2812_spi5_dma_show(
			address,
			led_count * 24 + WS2812_RESET_SLOTS
	);
}

/**
  * @brief Retorna o handle da event flag usada para sincronização SPI/DMA.
  *
  * @retval osEventFlagsId_t Handle da event flag (@c spi_dmaEventHandle)
  *
  * @details
  * Esta função fornece acesso ao handle da event flag utilizada para sinalizar
  * a conclusão de uma transmissão SPI via DMA (callback @c HAL_SPI_TxCpltCallback).
  *
  * A event flag é utilizada para sincronizar a tarefa que controla os LEDs
  * (`LedTask`) com a interrupção do DMA:
  * - A ISR sinaliza o evento quando a transmissão termina
  * - A tarefa bloqueada em `osEventFlagsWait()` é liberada
  *
  * @note
  * - O handle retornado deve ser utilizado em funções de wait/set de event flags
  *   dentro do contexto da tarefa.
  *
  * @see LedTask
  * @see HAL_SPI_TxCpltCallback
  */
osEventFlagsId_t getSpi_dmaEventHandle(void) {
	return spi_dmaEventHandle;
}

/* USER CODE END Application */

