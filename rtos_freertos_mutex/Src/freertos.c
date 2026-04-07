/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    freertos.c
  * @brief   Migração do projeto FreeRTOS (Pico - https://gitlab.unicamp.br/rtos_pico/freertos_mutex)
  *          para a plataforma BlackPill, demonstrando o uso de Mutex para
  *          controle de acesso à UART.
  * @author  Wu Shin Ting
  * @date    01/04/2026
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file is provided, this software is distributed AS-IS.
  *
  ******************************************************************************
  * @details
  * Este projeto demonstra o uso de mecanismos de sincronização no FreeRTOS,
  * utilizando um semáforo binário como Mutex para proteger o acesso a um
  * recurso compartilhado (UART).
  *
  * Além disso, o projeto evidencia o comportamento de concorrência entre tarefas
  * de mesma prioridade e os efeitos do escalonamento cooperativo/preemptivo
  * quando pequenos atrasos (osDelay) são introduzidos.
  *
  * Para execução adequada:
  * - Conectar a interface UART-USB (ST-LINK) aos pinos RX/TX da USART1
  * - Abrir um terminal serial (ex: PuTTY, Tera Term) com a seguinte configuração:
  *      - Baud rate: 9600
  *      - Data bits: 8
  *      - Stop bits: 1
  *      - Paridade: nenhuma
  *      - Controle de fluxo: desabilitado
  *
  * @section tasks Descrição das tarefas
  *
  * - PrintTask1 e PrintTask2:
  *   - Executam a mesma função (PrintTask)
  *   - Recebem como argumento uma string constante ("1" ou "2")
  *   - Transmitem 20 caracteres via UART seguidos de nova linha
  *   - Inserem pequenos atrasos (osDelay(1)) entre transmissões para permitir
  *     alternância entre tarefas (time-slicing)
  *   - Compartilham o periférico UART, estando sujeitas a condições de corrida
  *     quando não há proteção
  *
  * - PrintTask0:
  *   - Tarefa padrão gerada pelo CubeMX com prioridade superior (osPriorityNormal)
  *   - Permanece bloqueada a maior parte do tempo (osDelay(10000))
  *   - Não interfere significativamente na execução das demais tarefas
  *   - Mantida apenas como referência estrutural
  *
  * PrintTask1 e PrintTask2 possuem a mesma prioridade (osPriorityLow),
  * permitindo a observação de concorrência e interleaving na saída serial.
  *
  * @section mutex Uso do Mutex
  *
  * O projeto utiliza um semáforo binário como mecanismo de exclusão mútua:
  *
  * @code
  * xMutexHandle = osSemaphoreNew(1, 1, &xMutex_attributes);
  * @endcode
  *
  * Quando habilitado, o acesso à UART é serializado, garantindo que apenas
  * uma tarefa transmita por vez.
  *
  * @section config Configuração
  *
  * O comportamento do sistema é controlado em tempo de compilação pela macro:
  *
  * @code
  * #define USE_MUTEX 0
  * @endcode
  *
  * - USE_MUTEX = 0:
  *      - Código sem proteção é compilado (#if USE_MUTEX)
  *      - Pode ocorrer interleaving (embaralhamento) de caracteres
  *      - Evidencia condição de corrida
  *
  * - USE_MUTEX = 1:
  *      - Código com exclusão mútua é compilado
  *      - Acesso à UART é protegido
  *      - Saída torna-se organizada e previsível
  *
  * @section experiment Experimentos sugeridos
  *
  * 1. Compilar com USE_MUTEX = 0
  *    - Observar interleaving entre "1" e "2"
  *    - Verificar comportamento não determinístico
  *
  * 2. Compilar com USE_MUTEX = 1
  *    - Verificar a serialização das mensagens
  *    - Comparar com o caso anterior
  *
  * 3. Alterar os valores de osDelay:
  *    - Remover ou reduzir delays → menor alternância entre tarefas
  *    - Aumentar delays → maior interleaving
  *
  * @section notes Observações
  *
  * - A função HAL_UART_Transmit é bloqueante:
  *      - Garante a conclusão da transmissão antes do retorno
  *      - NÃO garante exclusão mútua entre tarefas
  *
  * - O interleaving observado neste projeto é induzido pelos delays explícitos,
  *   e não por preempção durante a execução da função de transmissão
  *
  * - Em aplicações reais, recomenda-se:
  *      - Uso de Mutex nativo (osMutex) em vez de semáforo
  *      - Uso de filas (Queue) para desacoplamento entre tarefas
  *      - Uso de UART com DMA ou interrupções (IT)
  *
  * - O uso de argumentos (void *) permite reutilização da função de tarefa,
  *   garantindo modularidade e reentrância
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
#include <string.h>
#include "usart.h"
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/**
  * @def USE_MUTEX
  * @brief Controla o uso de Mutex para acesso à UART.
  *
  * @details
  * Define se as tarefas utilizarão exclusão mútua ao acessar o recurso
  * compartilhado (UART).
  *
  * - 0: Não utiliza Mutex (pode ocorrer corrupção de dados)
  * - 1: Utiliza Mutex (acesso exclusivo à UART)
  */
#define USE_MUTEX 0

/**
  * @def UART_DELAY
  * @brief Atraso entre iterações da tarefa de transmissão UART (em milissegundos).
  *
  * @details
  * Define o tempo de espera aplicado ao final de cada ciclo completo de envio
  * (20 caracteres + nova linha) na função PrintTask, por meio de osDelay().
  *
  * Esse atraso influencia diretamente o comportamento de escalonamento das tarefas:
  * - Valores menores:
  *      - Maior frequência de execução da tarefa
  *      - Maior competição pelo uso da CPU e da UART
  *
  * - Valores maiores:
  *      - Reduzem a frequência de execução
  *      - Facilitam a alternância entre tarefas concorrentes
  *
  * Em conjunto com os atrasos internos (osDelay(1)), este parâmetro ajuda a
  * evidenciar efeitos de concorrência e interleaving entre tarefas de mesma prioridade.
  *
  * @note O valor está em milissegundos e depende da configuração do tick do sistema (RTOS).
  */
#define UART_DELAY 10

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for printTask0 */
osThreadId_t printTask0Handle;
const osThreadAttr_t printTask0_attributes = {
  .name = "printTask0",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for printTask2 */
osThreadId_t printTask2Handle;
const osThreadAttr_t printTask2_attributes = {
  .name = "printTask2",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for printTask1 */
osThreadId_t printTask1Handle;
const osThreadAttr_t printTask1_attributes = {
  .name = "printTask1",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for xMutex */
osSemaphoreId_t xMutexHandle;
const osSemaphoreAttr_t xMutex_attributes = {
  .name = "xMutex"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void PrintTask0(void *argument);
void PrintTask(void *argument);

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
  /* creation of xMutex */
  xMutexHandle = osSemaphoreNew(1, 1, &xMutex_attributes);

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
  /* creation of printTask0 */
  printTask0Handle = osThreadNew(PrintTask0, NULL, &printTask0_attributes);

  /* creation of printTask2 */
  printTask2Handle = osThreadNew(PrintTask, (void *)"1", &printTask2_attributes);

  /* creation of printTask1 */
  printTask1Handle = osThreadNew(PrintTask, (void *)"2", &printTask1_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_PrintTask0 */
/**
  * @brief  Tarefa padrão gerada automaticamente pelo CubeMX.
  *
  * @param  argument Ponteiro genérico (não utilizado).
  * @retval None
  *
  * @details
  * Esta tarefa é criada automaticamente pelo CubeMX com prioridade padrão
  * (osPriorityNormal), que neste projeto é superior à das demais tarefas
  * (configuradas como osPriorityLow).
  *
  * No contexto deste projeto, seu uso efetivo é evitado pois:
  * - sua prioridade mais alta faz com que ela seja escalonada sempre que estiver pronta
  * - isso pode reduzir a frequência de execução das tarefas de menor prioridade
  * - pode interferir na observação de concorrência entre tarefas de mesma prioridade
  * - pode mascarar problemas de sincronização no acesso à UART
  *
  * Para fins didáticos, deseja-se que PrintTask1 e PrintTask2 executem com
  * mesma prioridade (osPriorityLow), permitindo observar condições de corrida
  * quando não há proteção por Mutex.
  *
  * Nesta implementação, a tarefa permanece bloqueada na maior parte do tempo
  * (osDelay(10000)), não competindo ativamente pela CPU, sendo mantida apenas
  * como referência estrutural do projeto gerado pelo CubeMX.
  */
/* USER CODE END Header_PrintTask0 */
void PrintTask0(void *argument)
{
  /* USER CODE BEGIN PrintTask0 */
  /* Infinite loop */
  for(;;)
  {
    osDelay(10000);
  }
  /* USER CODE END PrintTask0 */
}

/* USER CODE BEGIN Header_PrintTask */
/**
  * @brief  Tarefa responsável pela transmissão periódica de caracteres via UART.
  *
  * @param  argument Ponteiro genérico que deve conter uma string (ex: "1" ou "2")
  *         representando o caractere a ser transmitido por esta instância da tarefa.
  * @retval None
  *
  * @details
  * Esta tarefa implementa um loop infinito que transmite repetidamente uma
  * sequência de caracteres pela UART. O conteúdo transmitido é definido pelo
  * parâmetro @p argument, que deve ser um ponteiro válido para uma string
  * terminada em '\0'.
  *
  * A cada iteração do loop principal:
  * - São enviados 20 caracteres consecutivos via UART
  * - Um pequeno atraso (osDelay(1)) é inserido entre transmissões para permitir
  *   alternância entre tarefas de mesma prioridade (time-slicing)
  * - Ao final da sequência, é enviado um caractere de nova linha ("\r\n")
  * - Um atraso adicional (osDelay(UART_DELAY)) é aplicado antes da próxima repetição
  *
  * ## Sincronização (controle por macro USE_MUTEX)
  *
  * - Quando USE_MUTEX = 1:
  *   - A tarefa adquire um semáforo (xMutexHandle) antes de acessar a UART
  *   - Garante exclusão mútua no uso do periférico
  *   - Evita interleaving (embaralhamento) de caracteres entre tarefas concorrentes
  *
  * - Quando USE_MUTEX = 0:
  *   - Não há proteção de acesso à UART
  *   - Múltiplas tarefas podem transmitir simultaneamente
  *   - Pode ocorrer interleaving de caracteres, evidenciando condição de corrida
  *
  * ## Comportamento em tempo de execução
  *
  * - A função HAL_UART_Transmit é utilizada em modo bloqueante (HAL_MAX_DELAY),
  *   garantindo a conclusão da transmissão de cada caractere antes de prosseguir
  * - A presença de osDelay(1) dentro do loop permite que o escalonador do FreeRTOS
  *   alterne entre tarefas de mesma prioridade
  * - O atraso externo (osDelay(UART_DELAY)) evita sincronização perfeita entre tarefas,
  *   favorecendo a observação de concorrência
  *
  * ## Observações importantes
  *
  * - A UART não é intrinsecamente thread-safe; o uso de mutex é recomendado em aplicações reais
  * - Este exemplo possui caráter didático, visando demonstrar:
  *   - concorrência entre tarefas
  *   - condições de corrida
  *   - uso de mecanismos de sincronização (mutex)
  *
  * @note O parâmetro @p argument deve apontar para uma string válida durante toda
  *       a execução da tarefa (ex: string literal).
  */
/* USER CODE END Header_PrintTask */
void PrintTask(void *argument)
{
  /* USER CODE BEGIN PrintTask */
	const char *msg = (const char *)argument;  // define o caractere a ser transmitido nesta instância

  /* Infinite loop */
  for(;;)
  {
#if USE_MUTEX
		if (osSemaphoreAcquire(xMutexHandle, osWaitForever) == osOK)
		{
			for (int i = 0; i < 20; i++) {
				/*
				 * Envia de forma bloqueante
				 */
				HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
			    osDelay(1);
			}
			HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", strlen("\r\n"), HAL_MAX_DELAY);
			osSemaphoreRelease(xMutexHandle);
		}
#else // Caso sem Mutex, mesmo código porém sem aguardar, tomar e devolver o Mutex
		for (int i = 0; i < 20; i++) {
			/*
			 * Envia de forma bloqueante
			 */
			HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
		    osDelay(1);
		}
		HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", strlen("\r\n"), HAL_MAX_DELAY);
#endif
	    osDelay(UART_DELAY);
  }
  /* USER CODE END PrintTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

