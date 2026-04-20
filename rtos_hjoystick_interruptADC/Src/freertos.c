/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : freertos.c
  * @brief          : Leitura de joystick via ADC com processamento em RTOS e saída UART
  * @author         : Pedro Henrique Guerra
  * @date           : 20/04/2026
  *
  * @details
  * Este módulo implementa a leitura de um joystick analógico (2 eixos)
  * utilizando o ADC do STM32 em modo interrupção, integrado ao FreeRTOS.
  *
  * A aquisição dos dados é feita de forma contínua e não bloqueante,
  * alternando dinamicamente entre dois canais ADC (X e Y).
  *
  * Os valores são processados em uma task dedicada e convertidos em
  * representação visual ASCII, transmitida via UART utilizando
  * comunicação não bloqueante (interrupt-driven).
  *
  * ---------------------------------------------------------------------------
  * Arquitetura do sistema:
  *
  * ADC (interrupção)
  *     ↓
  * Atualização de variáveis globais (adc_x_raw, adc_y_raw)
  *     ↓
  * Task (JoystickTask)
  *     ↓
  * Processamento → Geração de barra ASCII
  *     ↓
  * UART (IT)
  *     ↓
  * Callback UART → Liberação de semáforo
  *
  * ---------------------------------------------------------------------------
  * Fluxo RTOS:
  *
  * UART disponível → Semaphore liberado →
  * Task executa → monta buffer → transmite via UART →
  * Callback UART → libera semáforo novamente
  *
  * ---------------------------------------------------------------------------
  * Características:
  *
  * - Arquitetura não bloqueante
  * - Separação clara entre ISR e processamento
  * - Uso de semáforo para sincronização UART
  * - Baixo acoplamento entre aquisição e exibição
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

#include "adc.h"
#include "usart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
 * @def BAR_WIDTH
 * @brief Largura da barra ASCII utilizada para representar o valor do joystick.
 *
 * Define o número de posições possíveis exibidas para cada eixo.
 */

#define BAR_WIDTH   40

/**
 * @def ADC_MAX
 * @brief Valor máximo do conversor ADC de 12 bits.
 *
 * Usado para normalizar o valor lido e mapear para a posição da barra.
 */

#define ADC_MAX     4095
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
/**
 * @brief Valor bruto do eixo X do joystick.
 *
 * Atualizado dentro da interrupção de conversão do ADC.
 */
volatile uint16_t adc_x_raw;
/**
 * @brief Valor bruto do eixo Y do joystick.
 *
 * Atualizado dentro da interrupção de conversão do ADC.
 */
volatile uint16_t adc_y_raw;

/**
 * @brief Canal ADC atualmente configurado.
 *
 * Utilizado para alternar dinamicamente entre os canais
 * correspondentes aos eixos X e Y do joystick.
 */

volatile uint8_t  adc_channel = ADC_CHANNEL_9;

/**
 * @brief Buffer de transmissão UART.
 *
 * Armazena a string ASCII enviada ao terminal.
 */

uint8_t tx_buffer[100];

/* USER CODE END Variables */
/* Definitions for joystickTask */
osThreadId_t joystickTaskHandle;
const osThreadAttr_t joystickTask_attributes = {
  .name = "joystickTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for uartSemaphore */
osSemaphoreId_t uartSemaphoreHandle;
const osSemaphoreAttr_t uartSemaphore_attributes = {
  .name = "uartSemaphore"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
/* USER CODE END FunctionPrototypes */

void JoystickTask(void *argument);

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
  /* creation of uartSemaphore */
  uartSemaphoreHandle = osSemaphoreNew(1, 1, &uartSemaphore_attributes);

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
  /* creation of joystickTask */
  joystickTaskHandle = osThreadNew(JoystickTask, NULL, &joystickTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Create the event(s) */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_JoystickTask */
/**
  * @brief Task responsável pelo processamento e envio dos dados do joystick.
  *
  * @details
  * Esta task executa continuamente as seguintes etapas:
  *
  * 1. Aguarda disponibilidade da UART via semáforo
  * 2. Converte os valores ADC em posição na barra ASCII
  * 3. Constrói a string de saída
  * 4. Envia os dados via UART (modo não bloqueante)
  * 5. Aguarda próximo ciclo de execução
  *
  * Sincronização:
  * - O semáforo garante que apenas uma transmissão UART ocorra por vez
  * - A liberação do semáforo ocorre no callback da UART
  *
  * @param argument Parâmetro não utilizado
  */
/* USER CODE END Header_JoystickTask */
void JoystickTask(void *argument)
{
  /* USER CODE BEGIN JoystickTask */
  HAL_ADC_Start_IT(&hadc1);
  strcpy((char *)tx_buffer, "\rX: [");
  /* Infinite loop */
	while(1)
	    {
	        osSemaphoreAcquire(uartSemaphoreHandle, osWaitForever);

	        uint8_t bar_x_pos = (adc_x_raw * BAR_WIDTH) / ADC_MAX;
	        uint8_t bar_y_pos = (adc_y_raw * BAR_WIDTH) / ADC_MAX;

	        uint8_t i;

	        for (i = 0; i < BAR_WIDTH; i++)
	            tx_buffer[5+i] = (i == bar_x_pos ? 'o' : ' ');

	        tx_buffer[5+i] = '\0';
	        strcat((char *)tx_buffer, "]  Y: [");

	        for (i = 0; i < BAR_WIDTH; i++)
	            tx_buffer[52+i] = (i == bar_y_pos ? 'o' : ' ');

	        tx_buffer[52+i] = '\0';
	        strcat((char *)tx_buffer, "]");

	        HAL_UART_Transmit_IT(&huart1, tx_buffer, strlen((char *)tx_buffer)+1);

	        osDelay(50); // taxa de atualização
	    }
  /* USER CODE END JoystickTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/**
 * @brief Callback executado ao término da transmissão UART.
 *
 * @param huart Ponteiro para a UART que gerou a interrupção.
 *
 * @details
 * Esta função é chamada automaticamente pela HAL ao final da transmissão
 * em modo não bloqueante (HAL_UART_Transmit_IT).
 *
 * Responsabilidades:
 * - Reinicializar o buffer de transmissão
 * - Liberar o semáforo para permitir nova transmissão
 *
 * @note Executa em contexto de interrupção (ISR)
 */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        strcpy((char *)tx_buffer, "\rX: [");

        osSemaphoreRelease(uartSemaphoreHandle);
    }
}

/**
 * @brief Callback executado ao término de uma conversão ADC.
 *
 * @param hadc Ponteiro para o ADC que gerou a interrupção.
 *
 * @details
 * Implementa multiplexação manual de canais ADC para leitura de dois eixos.
 *
 * Funcionamento:
 *
 * - Se canal atual for X:
 *      → Armazena valor em adc_x_raw
 *      → Alterna para canal Y
 *
 * - Se canal atual for Y:
 *      → Armazena valor em adc_y_raw
 *      → Alterna para canal X
 *
 * Após alternância:
 * - Reconfigura o ADC
 * - Reinicia conversão em modo interrupção
 *
 * @note Executa em contexto de interrupção (ISR)
 * @warning Deve ser leve e rápida — evitar processamento pesado aqui
 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    uint16_t value = HAL_ADC_GetValue(hadc);

    ADC_ChannelConfTypeDef sConfig = {0};

    if (adc_channel == ADC_CHANNEL_9)
    {
        adc_x_raw = value;

        sConfig.Channel = ADC_CHANNEL_6;
        adc_channel = ADC_CHANNEL_6;
    }
    else
    {
        adc_y_raw = value;

        sConfig.Channel = ADC_CHANNEL_9;
        adc_channel = ADC_CHANNEL_9;
    }

    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_112CYCLES;

    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start_IT(&hadc1);
}
/* USER CODE END Application */

