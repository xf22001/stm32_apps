

/*================================================================
 *
 *
 *   文件名称：net_callback.c
 *   创 建 者：肖飞
 *   创建日期：2019年11月13日 星期三 15时48分08秒
 *   修改日期：2019年11月19日 星期二 12时38分26秒
 *   描    述：
 *
 *================================================================*/
#include "app_platform.h"
#include "cmsis_os.h"
#include "lwip.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"

extern struct netif gnetif;

void ethernetif_notify_conn_changed(struct netif *netif)
{
	/* NOTE : This is function could be implemented in user file
	          when the callback is needed,
	*/
	if(netif_is_link_up(netif)) {
		netif_set_up(&gnetif);
		dhcp_release(&gnetif);
		dhcp_start(&gnetif);
	} else {
		netif_set_down(&gnetif);
	}

}
