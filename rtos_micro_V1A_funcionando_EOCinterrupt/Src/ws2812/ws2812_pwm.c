/**
 ******************************************************************************
 * @file           : ws2812.c
 * @author		   : Wu Shin - Ting
 * @date 		   : 5/3/2026
 * @brief          : Definição das funções para controle de LEDs WS2812 via interface HAL
 ******************************************************************************
 * @note Este código foi desenvolvido para fins didáticos. Uma reimplementação do neopixel_V1A,
 * 		migrando da interface CMSIS para a HAL.
 *
 * @warning Certifique-se de configurar corretamente o clock do sistema.
 *
 ******************************************************************************
 */
#include <string.h>
#include <tim.h>
#include <dma.h>
#include "ws2812_pwm.h"
#include "ws2812_color.h"

/**
 * @brief Largura do pulso que representa '0'
 */
#define WS2812_0  ((__HAL_TIM_GET_AUTORELOAD(&htim1) * 34) / 100) //arredonda para 3

/**
 * @brief Largura do pulso que representa '1'
 */
#define WS2812_1  ((__HAL_TIM_GET_AUTORELOAD(&htim1) * 70) / 100) //arredonda para 6

/**
 * @brief Callback executado ao final da transferência DMA.
 *
 * Esta função é chamada quando a transferência de todos os dados
 * do @ref dma_buffer para o TIM1 é concluída.
 *
 * Responsabilidades:
 * - Desabilitar novas requisições DMA do timer.
 * - Forçar o duty-cycle do PWM para zero (linha de dados em nível baixo).
 * - Parar a contagem do TIM1.
 *
 * Isso garante que o sinal permaneça em nível baixo por tempo
 * suficiente para gerar o período de reset (latch) exigido pelos
 * LEDs WS2812.
 *
 * @param hdma Ponteiro para o handle do DMA que gerou o callback.
 */
void my_DMA_XferCpltCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_tim1_up)
    {
        /* Desabilitar requisição DMA do timer */
        __HAL_TIM_DISABLE_DMA(&htim1, TIM_DMA_UPDATE);

        /* Garantir linha baixa no pino de dados */
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);

        /* Parar a contagem do timer */
        HAL_TIM_Base_Stop(&htim1);
    }
}

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
 * @brief Inicializa o driver dos LEDs WS2812.
 *
 * Inicializa o hardware necessário para a comunicação.
 *
 * @param count Número total de LEDs WS2812.
 */
ws2812_status_t ws2812_pwm_init(void)
{

    /* Habilitar a interrupção transfer complete */
        __HAL_DMA_ENABLE_IT(&hdma_tim1_up, DMA_IT_TC);

    /* TIM1 é um timer avançado com bit de controle de saída MOE */
    __HAL_TIM_MOE_ENABLE(&htim1);

    /* Registrar callback para requisição DMA2 disparada por TIM1_UP
     * Este callback não é registrado automaticamente pela HAL
     * Deve ser feito manualmente
     */
    HAL_StatusTypeDef estado = HAL_DMA_RegisterCallback(&hdma_tim1_up,
                             HAL_DMA_XFER_CPLT_CB_ID,
                             my_DMA_XferCpltCallback);

    if (estado != HAL_OK) return WS2812_ERRO_HAL;

    return WS2812_OK;
}

/**
 * @brief Inicia a transmissão dos dados dos LEDs WS2812 usando TIM1 + DMA.
 *
 * Esta função inicia a transferência DMA que envia o conteúdo do
 * @ref dma_buffer para o registrador CCR3 do TIM1. Cada valor do buffer
 * define o duty-cycle do PWM responsável por gerar os pulsos do protocolo
 * WS2812.
 *
 * Fluxo da operação:
 * 1. Aborta qualquer transferência DMA anterior.
 * 2. Inicia nova transferência DMA com interrupção de conclusão.
 * 3. Habilita a requisição DMA gerada pelo evento Update do TIM1.
 * 4. Inicia o PWM no canal 3 do TIM1.
 *
 * O buffer @ref dma_buffer deve conter previamente:
 * - os bits codificados de todos os LEDs (24 bits por LED).
 * - slots adicionais de reset (linha baixa) definidos por
 *   @ref WS2812_RESET_SLOTS.
 *
 * A finalização da transmissão é tratada no callback
 * @ref my_DMA_XferCpltCallback().
 *
 * @note
 * A HAL não fornece uma função específica para a combinação do evento
 * de Update do timer (TIM1_UP) com um canal de PWM (TIM1_CH3) utilizando DMA.
 *
 * Em particular:
 * - não existe uma API do tipo @c HAL_TIM_UP_Start_DMA().
 * - as funções disponíveis (ex: @c HAL_TIM_PWM_Start_DMA) estão associadas
 *   ao evento de comparação (Capture/Compare), como o fim do pulso no canal,
 *   e utilizam o callback @c HAL_TIM_PWM_PulseFinishedCallback().
 *
 * Portanto, para aplicações como o controle de WS2812 — onde o sincronismo
 * preciso é baseado no evento de Update do timer — é necessário:
 * - inicializar o DMA manualmente (ex: @c HAL_DMA_Start_IT)
 * - habilitar explicitamente a requisição DMA via @c TIM_DMA_UPDATE
 * - implementar o callback de conclusão no nível do DMA
 *
 * Essa abordagem oferece maior flexibilidade e permite explorar combinações
 * de eventos não diretamente suportadas pela HAL, ao custo de maior controle
 * manual sobre o hardware.
 *
 * @retval WS2812_OK   Transmissão iniciada com sucesso.
 * @retval WS2812_ERRO Falha ao iniciar a transferência DMA.
 */
ws2812_status_t ws2812_tim1ch3_dma_show( uint8_t *dma_buffer, uint32_t DataLength)
{
	static uint16_t tmp_buffer[
		WS2812_MAX_LEDS * WS2812_BITS_LED + WS2812_RESET_SLOTS
	];

    for (uint32_t i=0; i<DataLength; i++) {
    	tmp_buffer[i] = dma_buffer[i];
    }

    HAL_StatusTypeDef status = HAL_DMA_Start_IT(
        &hdma_tim1_up,
		(uint32_t)tmp_buffer,
        (uint32_t)&TIM1->CCR3,
        DataLength
    );

    if (status != HAL_OK)
        return WS2812_ERRO;

    /* Habilitar requisição DMA do TIM1 */
    __HAL_TIM_ENABLE_DMA(&htim1, TIM_DMA_UPDATE);

    /* Habilitar PWM no canal 3 do TIM1 */
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);

    return WS2812_OK;
}

/**
 * @brief      Verifica se o módulo WS2812 está pronto para nova transmissão.
 *
 * @details    Esta função consulta o estado do DMA associado ao SPI5 utilizado para envio
 *             dos dados para os LEDs WS2812. Se o DMA ainda estiver transmitindo dados,
 *             a função retorna que o módulo está ocupado; caso contrário, indica que está liberado.
 *
 * @return     ws2812_status_t
 *             - WS2812_OCUPADO  : DMA em execução, o módulo não pode iniciar nova transferência.
 *             - WS2812_LIBERADO : DMA pronto, o módulo pode iniciar nova transferência.
 */
ws2812_status_t ws2812_pwm_estado () {

	if (HAL_DMA_GetState(&hdma_tim1_up) != HAL_DMA_STATE_READY)
		return WS2812_OCUPADO;
	else return WS2812_LIBERADO;
}

/**
 * @brief Envia os dados do buffer para a cadeia de LEDs WS2812.
 *
 * Esta função inicia a transmissão dos dados de cor armazenados em
 * `dma_buffer` para os LEDs WS2812, incluindo os slots de reset ao final
 * do frame.
 *
 * O método de transmissão é por hardware (TIM + DMA), para temporização precisa e sem jitter
 *
 * O número total de elementos transmitidos é:
 *   led_count * 24 + WS2812_RESET_SLOTS
 *
 * @note A transmissão por hardware é a forma recomendada para garantir
 *       conformidade total com o protocolo WS2812.
 */
ws2812_status_t ws2812_pwm_show(void)
{
	ws2812_status_t estado;

	uint8_t *buffer = ws2812_get_dma_buffer();

	uint32_t led_count = ws2812_get_led_count();

	estado = ws2812_tim1ch3_dma_show (buffer, led_count * 24 + WS2812_RESET_SLOTS);

	if (estado != WS2812_OK) return estado;

	while (ws2812_pwm_estado() != WS2812_LIBERADO);

	return WS2812_OK;
}


