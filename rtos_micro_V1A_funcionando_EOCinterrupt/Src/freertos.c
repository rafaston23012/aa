/* USER CODE BEGIN Header */
/**
    ******************************************************************************
  * @file    :freertos.c
  * @brief   :Implementação do VU Meter utilizando FreeRTOS e WS2812
  * @author  :Pedro Henrique Guerra
  * @date    : 20/4/2026
  * @details
  * Este módulo implementa um sistema de medição de nível de sinal (VU Meter)
  * utilizando uma matriz de LEDs endereçáveis WS2812
  * baseado em FreeRTOS.
  *
  * Arquitetura do sistema:
  *
  * - Task principal (VuMeterTask)
  * - ADC operando em modo interrupção
  * - Sincronização via semáforo
  * - Processamento logarítmico do sinal
  * - Atualização de matriz WS2812 via PWM + DMA
  *
  * Fluxo de execução:
  *
  * ADC -> Interrupção -> Semáforo -> Task -> Processamento -> Atualização LED
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
#include "adc.h"
#include <math.h>
#include "dma.h"
#include "tim.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
 * @def ADC_OFFSET
 * @brief Offset aplicado ao sinal analógico.
 *
 * Utilizado para remover o valor DC do sinal de entrada.
 * Em aplicações de áudio, o sinal costuma estar centrado em Vref/2.
 */

#define ADC_OFFSET 2048.0f

/**
 * @def NEOPIXEL_COLS
 * @brief Número de colunas da matriz de LEDs.
 */

#define NEOPIXEL_COLS 5

/**
 * @def NEOPIXEL_ROWS
 * @brief Número de linhas da matriz de LEDs.
 */
#define NEOPIXEL_ROWS 5

/**
 * @def NEOPIXEL_LEDS
 * @brief Número total de LEDs da matriz.
 */
#define NEOPIXEL_LEDS (NEOPIXEL_COLS * NEOPIXEL_ROWS)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/**
 * @brief Último valor convertido pelo ADC.
 */

volatile uint16_t adc_value;
/**
* @brief last_value Último número de LEDs acesos.
*/

uint8_t last_value = 0;

/* USER CODE END Variables */
/* Definitions for vuMeterTask */
osThreadId_t vuMeterTaskHandle;
const osThreadAttr_t vuMeterTask_attributes = {
  .name = "vuMeterTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for adcSemaphore */
/**
 * @brief Semáforo de sincronização ADC → Task
 *
 * @details
 * Liberado na ISR do ADC e consumido pela VuMeterTask.
 */
osSemaphoreId_t adcSemaphoreHandle;
const osSemaphoreAttr_t adcSemaphore_attributes = {
  .name = "adcSemaphore"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
uint8_t vu_meter(uint16_t adc_value, uint8_t last_value);
void color_gradient(float percentage,uint8_t *r, uint8_t *g, uint8_t *b);
void set_pixel(uint8_t x, uint8_t y, uint8_t r, uint8_t g, uint8_t b);
void update_display(void);
/* USER CODE END FunctionPrototypes */

void VuMeterTask(void *argument);

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

  /* Create the semaphores(s) */
  /* creation of adcSemaphore */
  adcSemaphoreHandle = osSemaphoreNew(1, 0, &adcSemaphore_attributes);

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
  /* creation of vuMeterTask */
  vuMeterTaskHandle = osThreadNew(VuMeterTask, NULL, &vuMeterTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_VuMeterTask */
/**
  * @brief Task responsável pelo processamento do VU Meter.
  *
  * @details
  * Fluxo da task:
  *
  * 1. Inicia conversão ADC
  * 2. Aguarda sinalização via semáforo
  * 3. Processa valor do ADC
  * 4. Atualiza matriz de LEDs
  * 5. Aguarda pequeno intervalo
  *
  * Sincronização:
  * - Utiliza semáforo liberado pela ISR do ADC
  *
  * @param argument Parâmetro não utilizado
  */
/* USER CODE END Header_VuMeterTask */
void VuMeterTask(void *argument)
{
  /* USER CODE BEGIN VuMeterTask */
  /* Infinite loop */

    while(1)
    {
    	HAL_ADC_Start_IT(&hadc1);

        osSemaphoreAcquire(adcSemaphoreHandle, osWaitForever);

        last_value = vu_meter(adc_value, last_value);

        osDelay(10);

    }
  /* USER CODE END VuMeterTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
/**
 * @brief Callback de término de conversão do ADC.
 *
 * @param hadc Ponteiro para o handle do ADC.
 *
 * @details
 * Executado em contexto de interrupção.
 *
 * Responsabilidades:
 * - Capturar valor convertido
 * - Liberar semáforo para a task
 *
 * @note
 * NÃO realizar processamento pesado nesta função.
 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if(hadc->Instance == ADC1)
    {
        adc_value = HAL_ADC_GetValue(&hadc1);

        osSemaphoreRelease(adcSemaphoreHandle);
    }
}

/**
 * @brief Calcula o nível do VU Meter e atualiza a matriz de LEDs.
 *
 * @param adc_value Valor bruto lido do ADC.
 * @param last_value Último número de LEDs acesos.
 *
 * @return Número atual de LEDs acesos.
 *
 * @details
 * Esta função executa o processamento principal do firmware:
 *
 * 1. Remove o offset do sinal
 * 2. Amplifica o sinal para melhorar sensibilidade
 * 3. Normaliza o valor para a faixa 0–1
 * 4. Aplica uma escala logarítmica para simular resposta de VU Meter
 * 5. Calcula quantos LEDs devem ser acesos
 * 6. Atualiza a matriz com gradiente de cores
 *
 * A escala logarítmica é utilizada para melhorar a percepção visual
 * de sinais de baixa amplitude.
 */

uint8_t vu_meter(uint16_t adc_value, uint8_t last_value)
{
    float volume;
    float volume_ratio;
    uint8_t num_leds_lit;

    volume = (float)adc_value - ADC_OFFSET;

    if (volume < 0)
        volume = 0;

    volume *= 3.0f;

    volume_ratio = volume / 4096.0f;

    num_leds_lit = (uint8_t)(
        NEOPIXEL_LEDS * log10f(1.0f + 9.0f * volume_ratio)
    );

    if(num_leds_lit != last_value)
    {

        for(uint8_t i=0;i<NEOPIXEL_LEDS;i++)
            ws2812_set_pixel(i,0,0,0);

        for(uint8_t i=0;i<num_leds_lit;i++)
        {

            uint8_t x = i % NEOPIXEL_COLS;
            uint8_t y = i / NEOPIXEL_COLS;

            uint8_t r,g,b;

            color_gradient((float)i/(NEOPIXEL_LEDS-1),&r,&g,&b);

            set_pixel(x,y,r,g,b);
        }

        update_display();
    }

    return num_leds_lit;
}
    /**
     * @brief Calcula a cor correspondente a uma posição no gradiente.
     *
     * @param percentage Valor normalizado entre 0 e 1.
     * @param r Ponteiro para o componente vermelho.
     * @param g Ponteiro para o componente verde.
     * @param b Ponteiro para o componente azul.
     *
     * @details
     * Gera um gradiente de cor utilizado no VU Meter:
     *
     * azul → verde → vermelho
     *
     * O gradiente permite identificar visualmente diferentes
     * níveis de intensidade do sinal.
     */

    void color_gradient(float percentage,
                               uint8_t *r,
                               uint8_t *g,
                               uint8_t *b)
    {
        if (percentage < 0.5f) {

            *r = 0;
            *g = (uint8_t)(510.0f * percentage);
            *b = 255 - *g;

        } else {

            *g = 255 - (uint8_t)(510.0f * (percentage - 0.5f));
            *r = 255 - *g;
            *b = 0;

        }
    }

    /**
     * @brief Define a cor de um LED na matriz.
     *
     * @param x Coordenada horizontal do LED
     * @param y Coordenada vertical do LED
     * @param r Intensidade do componente vermelho
     * @param g Intensidade do componente verde
     * @param b Intensidade do componente azul
     *
     * @details
     * Converte as coordenadas (x,y) em um índice linear utilizado
     * pela biblioteca WS2812.
     *
     * O cálculo considera a orientação da matriz de LEDs.
     */

    void set_pixel(uint8_t x, uint8_t y,
                          uint8_t r, uint8_t g, uint8_t b)
    {
        uint16_t index = (NEOPIXEL_ROWS - 1 - y) * NEOPIXEL_COLS + x;

        ws2812_set_pixel(index, r, g, b);
    }

    /**
     * @brief Atualiza o estado da matriz de LEDs.
     *
     * @details
     * Envia os dados armazenados no buffer de LEDs para a cadeia
     * WS2812 utilizando PWM + DMA.
     */

    void update_display(void)
    {
        ws2812_pwm_show();
    }
/* USER CODE END Application */

