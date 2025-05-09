/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "lcd1602_i2c.h"
#include "lcd1602_text.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/dhcp.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c2;
extern struct netif gnetif;
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
osThreadId defaultTaskHandle;
osThreadId displayTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void DisplayTask(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

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
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of displayTask */
  osThreadDef(displayTask, DisplayTask, osPriorityNormal, 0, 512);
  displayTaskHandle = osThreadCreate(osThread(displayTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    sys_check_timeouts();  // This is critical for DHCP, TCP, and timers to work
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_DisplayTask */
/**
* @brief Function implementing the displayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_DisplayTask */
void DisplayTask(void const * argument)
{
  /* USER CODE BEGIN DisplayTask */
  char ip_str[16]; // Buffer to store the IP address as a string
  char prev_ip_str[16] = ""; // Previous IP address
  char prev_hostname[32] = ""; // Previous hostname
	uint8_t prev_dhcp_state = 0xFF;
  LCD1602_I2C_HandleTypeDef lcd; // LCD handle (initialize it properly before use)

  // Initialize the LCD
  lcd.hi2c = &hi2c2; // Replace with your I2C handle
  lcd.address = 0x7E; // Replace with your LCD I2C address
  lcd_init(&lcd);
  lcd_clear(&lcd); // Clear the LCD after initialization

  /* Infinite loop */
  for(;;)
  {
    // Get the IP address
    ip4_addr_t ip = gnetif.ip_addr;

    // Format the IP address as a string
    snprintf(ip_str, sizeof(ip_str), "%s", ip4addr_ntoa(&ip));

    // Get the hostname
    const char *hostname = netif_get_hostname(&gnetif);

    // Get DHCP state
    const struct dhcp *dhcp = netif_dhcp_data(&gnetif);
    uint8_t dhcp_state = dhcp ? dhcp->state : 255;

    // Check if the IP address or DHCP state or hostname has changed
    if (strcmp(ip_str, prev_ip_str) != 0 || dhcp_state != prev_dhcp_state || strcmp(hostname, prev_hostname) != 0)
    {
      // Update the previous values
      strncpy(prev_ip_str, ip_str, sizeof(prev_ip_str));
      prev_ip_str[sizeof(prev_ip_str) - 1] = '\0';

      strncpy(prev_hostname, hostname, sizeof(prev_hostname));
      prev_hostname[sizeof(prev_hostname) - 1] = '\0';

      prev_dhcp_state = dhcp_state;

      // Clear the LCD and print the DHCP state and IP address
      lcd_clear(&lcd);

      //lcd_gotoxy(&lcd, 0, 0); // Move to the first row
      //lcd_puts(&lcd, hostname); // Print hostname

      char tmp[16];
      snprintf(tmp, sizeof(tmp), "DHCP STATE: %u", dhcp_state);
      //lcd_puts(&lcd, tmp); // Print DHCP state number
      lcd_scroll_text(&lcd, tmp, 0, 300);
      lcd_scroll_text(&lcd, ip_str, 1, 300);

      //lcd_gotoxy(&lcd, 0, 1); // Move to the second row
      //lcd_puts(&lcd, ip_str); // Print IP address
    }

    osDelay(1000); // Check for updates every second
  }
  /* USER CODE END DisplayTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

