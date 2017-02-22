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
/****************************************************************************
 * examples/mdns_test/mdns_main.c
 *
 *   Copyright (C) 2008, 2011-2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/// @file mdns_main.c
/// @brief the program for testing mdns

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <stdio.h>

#include <apps/netutils/mdnsd.h>

#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
#define MDNS_HOST_NAME          "tinyara.local"
#define MDNS_NETIF_NAME         "en1"

/****************************************************************************
 * Enumeration
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * function prototype
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * static function
 ****************************************************************************/
static void show_usage(FAR const char *progname)
{
	printf("\nUsage: %s <command>\n", progname);
	printf("\nWhere:\n");
#if defined(CONFIG_NETUTILS_MDNS_RESPONDER_SUPPORT)
	printf(" <command>   command string (start | stop | hostname | resolve | discover) [2nd param] [3rd param]\n");
	printf("              - start    : start mdns daemon. 2nd param should be hostname. \n");
	printf("              - stop     : terminate mdns daemon \n");
	printf("              - hostname : get current host name as mdns style \n");
#else
	printf(" <command>   command string (resolve | discover) [2nd param] [3rd param]\n");
#endif
	printf("              - resolve  : resolve hostname to ipaddr. 2nd param should be hostname. \n");
	printf("              - discover : discover service. 2nd param should be service type string. 3rd param is discovery time in ms. (default=3000ms) \n");
	printf("\n");
}

/****************************************************************************
 * mdns_main
 ****************************************************************************/

#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int mdns_main(int argc, char *argv[])
#endif
{
	int cmdtype = 0;			/* 1: start, 2: stop */
	char *hostname = NULL;
	int ret = 0;
	int ipaddr = 0;
	struct mdns_service_info *sd_result = NULL;
	char *service_type = NULL;
	int num_of_result = 0;
	int discovery_time_ms = 3000;	/* default time is 3 secs */
#if defined(CONFIG_NETUTILS_MDNS_RESPONDER_SUPPORT)
	char hostname_result[32];
#endif

	if (argc == 2) {
#if defined(CONFIG_NETUTILS_MDNS_RESPONDER_SUPPORT)
		if (strcmp(argv[1], "stop") == 0) {
			cmdtype = 2;
		} else if (strcmp(argv[1], "hostname") == 0) {
			cmdtype = 3;
		} else {
			show_usage(argv[0]);
		}
#else
		show_usage(argv[0]);
#endif
	} else if ((3 <= argc) && (argc <= 4)) {
		switch (argc) {
		case 4:
			if (strcmp(argv[1], "discover") == 0) {
				discovery_time_ms = atoi(argv[3]);
			} else {
				show_usage(argv[0]);
				break;
			}
		/* Fall Through */

		case 3:
			if (strcmp(argv[1], "resolve") == 0) {
				cmdtype = 4;
				hostname = argv[2];
			} else if (strcmp(argv[1], "discover") == 0) {
				cmdtype = 5;
				service_type = argv[2];
#if defined(CONFIG_NETUTILS_MDNS_RESPONDER_SUPPORT)
			} else if (strcmp(argv[1], "start") == 0) {
				cmdtype = 1;
				hostname = argv[2];
#endif
			} else {
				show_usage(argv[0]);
			}
		}
	} else {
		show_usage(argv[0]);
	}

	switch (cmdtype) {
#if defined(CONFIG_NETUTILS_MDNS_RESPONDER_SUPPORT)
	case 1:					/* start */
		ret = mdnsd_start(hostname, MDNS_NETIF_NAME);
		if (ret != 0) {
			fprintf(stderr, "ERROR: |%s| fail to execute mdnsd_start() \n", __FUNCTION__);
			return -1;
		}
		printf("mdnsd_start() OK. \n");
		break;

	case 2:					/* stop */
		ret = mdnsd_stop();
		if (ret == 0) {
			printf("mdnsd_stop() OK. \n");
		} else {
			fprintf(stderr, "ERROR: |%s| fail to execute mdnsd_stop() \n", __FUNCTION__);
			return -1;
		}
		break;

	case 3:					/* hostname */
		ret = mdnsd_get_hostname(hostname_result);
		if (ret == 0) {
			printf("mdns hostname : %s \n", hostname_result);
		} else {
			fprintf(stderr, "ERROR: |%s| fail to execute mdnsd_get_hostname() \n", __FUNCTION__);
			return -1;
		}
		break;
#endif							/* CONFIG_NETUTILS_MDNS_RESPONDER_SUPPORT */

	case 4:					/* resolve */
		if (mdnsd_resolve_hostname(hostname, &ipaddr) == 0) {
			printf("%s : %d.%d.%d.%d \n",
				   hostname, (ipaddr >> 0) & 0xFF,
				   (ipaddr >> 8) & 0xFF,
				   (ipaddr >> 16) & 0xFF,
				   (ipaddr >> 24) & 0xFF);
		} else {
			printf("%s is not found \n", hostname);
		}
		break;

	case 5:					/* discover service */
		ret = mdnsd_discover_service(service_type, discovery_time_ms, &sd_result, &num_of_result);
		if (ret == 0) {
			int i;
			printf(" service discovery : %s \n", service_type);
			printf("-------------------------------------------------------------------------------- \n");
			printf(" %-4s %-25s %-25s %8s \n", "no", "hostname", "service name", "ip:port");
			printf("-------------------------------------------------------------------------------- \n");
			for (i = 0; i < num_of_result; i++) {
				printf(" %-4d %-25s %-25s %d.%d.%d.%d:%d \n",
					   i + 1,
					   sd_result[i].hostname ? sd_result[i].hostname : "(null)",
					   sd_result[i].instance_name ? sd_result[i].instance_name : "(null)",
					   (sd_result[i].ipaddr >> 0) & 0xFF,
					   (sd_result[i].ipaddr >> 8) & 0xFF,
					   (sd_result[i].ipaddr >> 16) & 0xFF,
					   (sd_result[i].ipaddr >> 24) & 0xFF,
					   sd_result[i].port);
			}
			printf("-------------------------------------------------------------------------------- \n");
		} else {
			printf("%s is not found \n", service_type);
		}
		break;
	}

	return 0;
}
