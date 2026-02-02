/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
/**
 * @brief Estrutura de dados que representa um evento associado a um botão.
 *
 * @details
 * Esta estrutura é usada para encapsular informações mínimas sobre um evento
 * gerado por um botão e transmiti-lo entre diferentes partes do sistema,
 * tipicamente via uma fila (queue) do FreeRTOS/CMSIS-RTOS2.
 *
 * Um evento de botão geralmente é produzido em contexto de interrupção (ISR),
 * por exemplo no callback EXTI, e consumido por uma task responsável por
 * processar o evento (debounce, controle de LEDs, etc.).
 *
 * Campos:
 * - @p button_id: identificador do botão que gerou o evento (por exemplo, o número
 *   do pino GPIO associado ao botão).
 * - @p rising_edge: indica se o evento corresponde a uma borda de subida
 *   (1 = rising edge, 0 = falling edge). A interpretação física depende da
 *   configuração do hardware (pull-up/pull-down) e do EXTI.
 */
typedef struct
{
  uint8_t button_id;    /**< Identificador do botão que gerou o evento. */
  uint8_t rising_edge;  /**< Flag indicando borda de subida (1) ou descida (0). */
} ButtonEvent_t;

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define LED_R_Pin GPIO_PIN_2
#define LED_R_GPIO_Port GPIOA
#define LED_B_Pin GPIO_PIN_5
#define LED_B_GPIO_Port GPIOA
#define LED_G_Pin GPIO_PIN_7
#define LED_G_GPIO_Port GPIOA
#define Botao_A_Pin GPIO_PIN_12
#define Botao_A_GPIO_Port GPIOB
#define Botao_A_EXTI_IRQn EXTI15_10_IRQn
#define Botao_B_Pin GPIO_PIN_15
#define Botao_B_GPIO_Port GPIOA
#define Botao_B_EXTI_IRQn EXTI15_10_IRQn
#define Botao_C_Pin GPIO_PIN_5
#define Botao_C_GPIO_Port GPIOB
#define Botao_C_EXTI_IRQn EXTI9_5_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
