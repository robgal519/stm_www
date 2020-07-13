/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
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
#include "httpserver-netconn.h"
#include "lwip.h"
#include "bosh_BME.h"
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
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .priority = (osPriority_t) osPriorityIdle,
  .stack_size = 512
};



/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t BMETaskHandle;
const osThreadAttr_t bme_thread_attributes = {
  .name = "bme_thread",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 512
};
void BME_task();
static volatile application_state state;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

extern void MX_LWIP_Init(void);
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
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  BMETaskHandle = osThreadNew(BME_task, NULL, &bme_thread_attributes);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */

#define HTTP_OK "HTTP/1.1 200 OK\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\n"

#define CONTENT_TYPE_JSON "Content-Type: application/json; charset=utf-8\n"
#define CONTENT_TYPE_TEXT "Content-Type: text/html; charset=utf-8\n"

#define END_OF_HEADER "\r\n"
#include "../webpages/index.h"
#include <string.h>
err_t index_html_handler(struct netconn *connection_context) {
  err_t ret = netconn_write(
      connection_context, HTTP_OK CONTENT_TYPE_TEXT "\r\n",
      strlen(HTTP_OK) + strlen(CONTENT_TYPE_TEXT) + 2, NETCONN_NOCOPY);

  ret = netconn_write(connection_context, (const unsigned char *)index_html,
                      index_html_len, NETCONN_NOCOPY);
  return ret;
}



static void i2C_event(uint32_t event) {
BME_i2c_event_register(event); 
}
void BME_task(){
  log_message("start BME task\n");
   extern ARM_DRIVER_I2C Driver_I2C1;

  Driver_I2C1.Initialize(i2C_event);
  Driver_I2C1.PowerControl(ARM_POWER_FULL);
  Driver_I2C1.Control(ARM_I2C_BUS_SPEED, ARM_I2C_BUS_SPEED_FAST);
  Driver_I2C1.Control(ARM_I2C_BUS_CLEAR, 0);
  init_BME(&Driver_I2C1);
  log_message("BME initialized\n");
  const TickType_t xDelay = 500;
  while(true){
    BME_set_enable();
    run_BME(&state);
    osDelay(xDelay);
  }
}

err_t check_memeory(struct netconn *connection_context){
  netconn_write(connection_context, HTTP_OK CONTENT_TYPE_JSON END_OF_HEADER,
                strlen(HTTP_OK) + strlen(CONTENT_TYPE_JSON) +
                    strlen(END_OF_HEADER),
                NETCONN_NOCOPY);
  static char value[255];
  uint32_t size_of_val = snprintf(value,255,
  "{\n"\
  "\"total_mem\":%d\n"\
  "\"free_mem\":%d\n"\
"\"historic_min_free\":%d\n"\
"\"used_mem\":%d\n"\
  "}"
  ,configTOTAL_HEAP_SIZE,xPortGetFreeHeapSize(),xPortGetMinimumEverFreeHeapSize(),configTOTAL_HEAP_SIZE-xPortGetFreeHeapSize());
  netconn_write(connection_context, value, size_of_val, NETCONN_NOCOPY);
  return ERR_OK;
}

err_t get_BME_temperature(struct netconn *connection_context) {
  netconn_write(connection_context, HTTP_OK CONTENT_TYPE_JSON END_OF_HEADER,
                strlen(HTTP_OK) + strlen(CONTENT_TYPE_JSON) +
                    strlen(END_OF_HEADER),
                NETCONN_NOCOPY);
  static char value[255];
  uint32_t size_of_val = snprintf(value,255,
  "{"\
  "\"temperature\":%d.%02d,"\
  "\"unit\":\"°C\""\
  "}"
  ,(uint32_t)state.temp, ((uint32_t)(state.temp*100))%100);
  netconn_write(connection_context, value, size_of_val, NETCONN_NOCOPY);
  return ERR_OK;
}
err_t get_BME_pressure(struct netconn *connection_context) {
  netconn_write(connection_context, HTTP_OK CONTENT_TYPE_JSON END_OF_HEADER,
                strlen(HTTP_OK) + strlen(CONTENT_TYPE_JSON) +
                    strlen(END_OF_HEADER),
                NETCONN_NOCOPY);
  static char value[255];
  uint32_t size_of_val = snprintf(value,255,
  "{"\
  "\"pressure\":%d.%04d,"\
  "\"unit\":\"hPa\""\
  "}"
  ,(uint32_t)state.pressure/100, ((uint32_t)(state.pressure*100))%10000);
  netconn_write(connection_context, value, size_of_val, NETCONN_NOCOPY);
  return ERR_OK;
}
err_t get_BME_humidity(struct netconn *connection_context) {
  netconn_write(connection_context, HTTP_OK CONTENT_TYPE_JSON END_OF_HEADER,
                strlen(HTTP_OK) + strlen(CONTENT_TYPE_JSON) +
                    strlen(END_OF_HEADER),
                NETCONN_NOCOPY);
  static char value[255];
  uint32_t size_of_val = snprintf(value,255,
  "{"\
  "\"humidity\":%d.%02d,"\
  "\"unit\":\"%%\""\
  "}"
  ,(uint32_t)state.humidity, ((uint32_t)(state.humidity*100))%100);
  netconn_write(connection_context, value, size_of_val, NETCONN_NOCOPY);
  return ERR_OK;
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument) {
  log_message("start default task\n");

  // sys_thread_new("BME280", BME_task, NULL,
  //                DEFAULT_THREAD_STACKSIZE, osPriorityNormal);
  // /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  register_endpoint(GET, "/alive", alive_handler);
  register_endpoint(GET, "/index.html", index_html_handler);
  register_endpoint(GET, "/", index_html_handler);
  register_endpoint(GET, "/temperature", get_BME_temperature);
  register_endpoint(GET, "/pressure", get_BME_pressure);
  register_endpoint(GET, "/humidity", get_BME_humidity);
  register_endpoint(GET, "/memory", check_memeory);
  http_server_netconn_init();
  //


  /* Infinite loop */
  for (;;) {
    poll();
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
