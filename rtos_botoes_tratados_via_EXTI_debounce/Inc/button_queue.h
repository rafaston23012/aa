/*
 * @file button_queue.h
 * @brief private functions in freertos.c
 * @date 2/2/2026
 * @author Wu Shin Ting
  */

#ifndef BUTTON_QUEUE_H_
#define BUTTON_QUEUE_H_
//#include "cmsis_os2.h"
#include "queue.h"

osMessageQueueId_t get_buttonQueueHandle(void);

#endif /* BUTTON_QUEUE_H_ */
