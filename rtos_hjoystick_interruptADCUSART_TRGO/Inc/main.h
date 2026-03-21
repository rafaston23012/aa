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
 * @brief Estrutura que representa um par de amostras do ADC.
 *
 * @details
 * Esta estrutura agrupa as leituras de dois canais analógicos do ADC
 * (por exemplo, IN6 e IN7), formando uma única unidade lógica de dados.
 *
 * Cada instância de ::SamplePair_t corresponde a um ciclo completo de aquisição,
 * no qual ambos os canais são amostrados sequencialmente.
 *
 * O uso de uma estrutura para armazenar o par de amostras garante:
 * - consistência dos dados (os valores pertencem ao mesmo instante lógico)
 * - simplicidade no envio através de filas do RTOS
 * - melhor organização e legibilidade do código
 *
 * No contexto do sistema:
 * - A tarefa de aquisição (ADCTask) preenche esta estrutura
 * - A estrutura é enviada para uma fila (queue)
 * - A tarefa de comunicação (USARTTask) consome e transmite os dados
 *
 * @note Os valores são do tipo uint16_t, compatíveis com a resolução do ADC.
 */
typedef struct {
    uint16_t ch6; /**< Valor amostrado do canal ADC IN6 */
    uint16_t ch9; /**< Valor amostrado do canal ADC IN9 */
} SamplePair_t;

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
#define EIXO_Y_Pin GPIO_PIN_6
#define EIXO_Y_GPIO_Port GPIOA
#define EIXO_X_Pin GPIO_PIN_1
#define EIXO_X_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
