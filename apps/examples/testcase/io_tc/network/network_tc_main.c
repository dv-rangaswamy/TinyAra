/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/// @file network_tc_main.c

/// @brief Main Function for Network TestCase Example
#include <tinyara/config.h>
#include <stdio.h>
#include <semaphore.h>
#include "tc_internal.h"

int nw_total_pass = 0;
int nw_total_fail = 0;

extern sem_t tc_sem;
extern int working_tc;

/****************************************************************************
 * Name: network_tc_main
 ****************************************************************************/
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int network_tc_main(int argc, char *argv[])
#endif
{
	sem_wait(&tc_sem);
	working_tc++;

	printf("=== TINYARA Network TC START! ===\n");

#ifdef CONFIG_TC_NET_SOCKET
	net_socket_main();
#endif
#ifdef CONFIG_TC_NET_SETSOCKOPT
	net_setsockopt_main();
#endif
#ifdef CONFIG_TC_NET_CONNECT
	net_connect_main();
#endif
#ifdef CONFIG_TC_NET_CLOSE
	net_close_main();
#endif
#ifdef CONFIG_TC_NET_BIND
	net_bind_main();
#endif
#ifdef CONFIG_TC_NET_LISTEN
	net_listen_main();
#endif
#ifdef CONFIG_TC_NET_GETSOCKNAME
	net_getsockname_main();
#endif
#ifdef CONFIG_TC_NET_GETSOCKOPT
	net_getsockopt_main();
#endif
#ifdef CONFIG_TC_NET_FCNTL
	net_fcntl_main();
#endif
#ifdef CONFIG_TC_NET_IOCTL
	net_ioctl_main();
#endif
#ifdef CONFIG_TC_NET_ACCEPT
	net_accept_main();
#endif
#ifdef CONFIG_TC_NET_SEND
	net_send_main();
#endif
#ifdef CONFIG_TC_NET_RECV
	net_recv_main();
#endif
#ifdef CONFIG_TC_NET_GETPEERNAME
	net_getpeername_main();
#endif
#ifdef CONFIG_TC_NET_SENDTO
	net_sendto_main();
#endif
#ifdef CONFIG_TC_NET_RECVFROM
	net_recvfrom_main();
#endif
#ifdef CONFIG_TC_NET_SHUTDOWN
	net_shutdown_main();
#endif
#ifdef CONFIG_TC_NET_DHCPC
	net_dhcpc_main();
#endif
#ifdef CONFIG_TC_NET_DHCPS
	net_dhcps_main();
#endif

	printf("\n=== TINYARA Network TC COMPLETE ===\n");
	printf("\t\tTotal pass : %d\n\t\tTotal fail : %d\n", nw_total_pass, nw_total_fail);

	working_tc--;
	sem_post(&tc_sem);

	return 0;
}
