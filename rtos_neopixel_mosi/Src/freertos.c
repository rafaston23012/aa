/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file    freertos.c
 * @brief   Implementação do modelo de concorrência utilizando FreeRTOS (CMSIS-RTOS v2).
 * @author Wu Shin Ting
 * @date   22/03/2026
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
 * Este arquivo define a estrutura de execução concorrente do sistema, baseada
 * em uma única tarefa e na interação com uma rotina de interrupção (ISR) do SPI.
 *
 * O sistema é composto pela tarefa @c LedTask, responsável por gerar padrões de
 * LEDs e transmiti-los para uma cadeia de dispositivos WS2812 utilizando SPI
 * com suporte a DMA. Embora exista apenas uma tarefa, há concorrência entre o
 * contexto da tarefa (CPU) e o contexto de interrupção (DMA/SPI), já que ambos
 * acessam o mesmo recurso compartilhado: o @c dma_buffer.
 *
 * O @c dma_buffer é utilizado simultaneamente pela CPU (para escrita dos dados
 * dos LEDs) e pelo periférico SPI com DMA (para leitura durante a transmissão).
 * Para garantir integridade dos dados e evitar condições de corrida, o acesso
 * a esse buffer é protegido por um mutex (@c dmaBufferMutex).
 *
 * O fluxo de uso do buffer segue o padrão:
 *
 * @code
 * osMutexAcquire()
 *   → escrita/renderização no buffer
 *   → início da transmissão DMA
 *   → espera pela conclusão
 * osMutexRelease()
 * @endcode
 *
 * Um ponto crítico do projeto é que a finalização da transmissão ocorre em
 * contexto de interrupção, através do callback @c HAL_SPI_TxCpltCallback.
 * Como não é permitido liberar mutex dentro de uma ISR, foi adotado um
 * mecanismo intermediário de sincronização utilizando event flags
 * (@c osEventFlags).
 *
 * Quando o DMA termina a transmissão:
 * - A ISR executa @c osEventFlagsSet()
 * - A tarefa, que está bloqueada em @c osEventFlagsWait(), é liberada
 * - O mutex é então liberado em contexto seguro (thread)
 *
 * Esse modelo desacopla a ISR da lógica de gerenciamento de recursos,
 * garantindo segurança e previsibilidade.
 *
 * O sistema implementa dois modos distintos de atualização dos LEDs:
 *
 * - Atualização sequencial (bloqueante), via @c ws2812_spi_show:
 *   A função inicia a transmissão DMA e aguarda sua conclusão internamente,
 *   utilizando event flags. Esse modelo é simples, porém bloqueia a execução
 *   até o fim da transmissão e não é adequado para cenários com acesso
 *   compartilhado ao buffer.
 *
 * - Atualização compartilhada (não bloqueante), via
 *   @c ws2812_spi_show_compartilhado:
 *   A função apenas inicia a transmissão DMA e retorna imediatamente.
 *   A responsabilidade de sincronização (esperar o término e liberar o mutex)
 *   é delegada à tarefa chamadora. Esse modelo permite controle explícito da
 *   seção crítica e é adequado para uso em sistemas com RTOS.
 *
 * Em resumo, o modelo de concorrência adotado combina:
 *
 * - Exclusão mútua via mutex para proteção do buffer
 * - Sincronização ISR → tarefa via event flags
 * - Separação clara entre contexto de interrupção e contexto de tarefa
 *
 * Fluxo completo:
 *
 * @code
 * [Task]
 *   Lock → Render → Start DMA → Wait Event → Unlock
 *
 * [ISR]
 *   DMA Complete → Set Event Flag
 * @endcode
 *
 * Essa abordagem garante:
 * - integridade do buffer durante a transmissão
 * - ausência de race conditions
 * - uso correto das primitivas de sincronização do RTOS
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
#include "spi.h"
#include "ws2812_spi.h"
#include "ws2812_color.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
	IMG_A = 0,
	IMG_B
} image_id_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
uint8_t *dma_buffer;
volatile image_id_t current_image = IMG_A;
/* USER CODE END Variables */
/* Definitions for ledTask */
osThreadId_t ledTaskHandle;
const osThreadAttr_t ledTask_attributes = {
  .name = "ledTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for dmaBufferMutex */
osMutexId_t dmaBufferMutexHandle;
const osMutexAttr_t dmaBufferMutex_attributes = {
  .name = "dmaBufferMutex"
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

void LedTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of dmaBufferMutex */
  dmaBufferMutexHandle = osMutexNew(&dmaBufferMutex_attributes);

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
  /* creation of ledTask */
  ledTaskHandle = osThreadNew(LedTask, NULL, &ledTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
	/* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* creation of spi_dmaEvent */
  spi_dmaEventHandle = osEventFlagsNew(&spi_dmaEvent_attributes);

  /* USER CODE BEGIN RTOS_EVENTS */
	/* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_LedTask */
/**
 * @brief Tarefa principal responsável pelo controle dos LEDs WS2812.
 *
 * @param argument Parâmetro da tarefa (não utilizado).
 *
 * @retval None
 *
 * @details
 * Esta tarefa implementa o fluxo completo de:
 * - inicialização visual dos LEDs
 * - geração de padrões
 * - sincronização com transmissão SPI via DMA
 *
 * A execução é dividida em duas fases:
 *
 * 1. Fase de inicialização (sequencial):
 *    - Configura parâmetros globais (cor, brilho)
 *    - Preenche o buffer com diferentes padrões de teste
 *    - Utiliza @c ws2812_spi_show (modo bloqueante)
 *
 *    Essa fase não utiliza mutex, pois não há concorrência ativa.
 *
 * 2. Loop infinito (modo concorrente):
 *
 *    A tarefa passa a operar em um modelo sincronizado com DMA:
 *
 *    @code
 *    Lock → Render → Start DMA → Wait Event → Unlock → Delay
 *    @endcode
 *
 *    Etapas detalhadas:
 *
 *    - Aquisição do mutex (@c osMutexAcquire):
 *      Garante acesso exclusivo ao @c dma_buffer, evitando concorrência
 *      com o periférico SPI/DMA.
 *
 *    - Renderização da imagem:
 *      Alterna entre dois padrões simples (IMG_A e IMG_B),
 *      demonstrando atualização dinâmica do buffer.
 *
 *    - Início da transmissão:
 *      Chamada de @c ws2812_spi_show_compartilhado(), que inicia o DMA
 *      de forma não bloqueante.
 *
 *    - Sincronização:
 *      A tarefa bloqueia em @c osEventFlagsWait aguardando o término da
 *      transmissão, sinalizado pela ISR (@c HAL_SPI_TxCpltCallback).
 *
 *    - Liberação do mutex:
 *      O mutex é liberado somente após a confirmação de término do DMA,
 *      garantindo que o buffer não seja modificado durante a transmissão.
 *
 *    - Alternância de estado:
 *      A variável @c current_image alterna entre IMG_A e IMG_B.
 *
 *    - Controle de taxa:
 *      Um @c osDelay() define a frequência de atualização dos LEDs.
 *
 * @note
 * - O uso de event flags evita busy-wait e melhora eficiência do sistema.
 * - O mutex nunca é liberado em ISR; a ISR apenas sinaliza o evento.
 * - A integridade do buffer é garantida durante toda a transmissão DMA.
 *
 * @warning
 * - É essencial limpar a event flag antes de iniciar nova transmissão,
 *   para evitar retornos imediatos de @c osEventFlagsWait().
 *
 * @see ws2812_spi_show
 * @see ws2812_spi_show_compartilhado
 * @see HAL_SPI_TxCpltCallback
 */
/* USER CODE END Header_LedTask */
void LedTask(void *argument)
{
	/* USER CODE BEGIN LedTask */
	ws2812_color_init(25);

	ws2812_set_brightness(50);   /* intensidade da cor */

	ws2812_fill_slots ();     /* preencher os slots que marcam fim da cadeia de LEDs */

	ws2812_fill(255, 50, 0);  /* preencher o buffer dos LEDs com "orange" */

	ws2812_spi_show();			  /* mostrar nos LEDs */

	osDelay (100);            /* tempo de espera */

	/* setar no buffer o gradiente linear aritmético entre verde (LED 3) a azul (LED 13) */
	ws2812_set_pixel_line_gradient(3, 13, 0, 255, 0, 0, 0, 255);

	ws2812_spi_show();   /* mostra nos LEDs */

	osDelay (100);            /* tempo de espera */

	ws2812_set_pixel_line(14, 16, 255, 0, 0); /* setar no buffer a cor vermelha do LED 14 a 16 */

	ws2812_spi_show();

	osDelay (100);            /* tempo de espera */

	ws2812_set_pixel(20, 255, 255, 255);

	ws2812_spi_show();

	osDelay (100);            /* tempo de espera */

	/* Infinite loop */
	for(;;)
	{
		/* Proteger dma_buffer */
		osMutexAcquire(dmaBufferMutexHandle, osWaitForever);

		/* Desenhar imagem */
		if (current_image == IMG_A)
		{
			ws2812_set_pixel(0, 255, 100, 0); /* yellow */
			ws2812_set_pixel(1, 255, 0, 0);   /* red */
		}
		else
		{
			ws2812_set_pixel(0, 255, 0, 0);   /* red */
			ws2812_set_pixel(1, 255, 100, 0); /* yellow */
		}

		/* Iniciar transmissão DMA */
		if (ws2812_spi_show_compartilhado () != WS2812_OK)
		{
			osMutexRelease(dmaBufferMutexHandle);
			continue;
		}

		/* Esperar DMA terminar (SEM busy wait!) */
		osEventFlagsWait(
				spi_dmaEventHandle,
				1U,
				osFlagsWaitAny,
				osWaitForever
		);

		/* Liberar recurso */
		osMutexRelease(dmaBufferMutexHandle);

		/* Alternar imagem */
		current_image = (current_image == IMG_A) ? IMG_B : IMG_A;

		/* Controla taxa de atualização */
		osDelay(250);
	}
	/* USER CODE END LedTask */
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

