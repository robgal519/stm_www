#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__

#include "lwip/netif.h"

err_t ethernetif_init(struct netif *netif);
void  ethernetif_poll(struct netif *netif);
void  ethernetif_check_link (struct netif *netif);


/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Time
*/
u32_t sys_jiffies(void);
/**
* @brief  Returns the current time in milliseconds
*         when LWIP_TIMERS == 1 and NO_SYS == 1
* @param  None
* @retval Time
*/
u32_t sys_now(void);

#endif