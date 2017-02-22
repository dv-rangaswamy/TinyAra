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

#include <tinyara/config.h>

#ifdef CONFIG_NET
#include <apps/shell/tash.h>

#include <sys/stat.h>			/* Needed for open */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>
#include <fcntl.h>				/* Needed for open */
#include <libgen.h>				/* Needed for basename */
#include <errno.h>
#include <debug.h>

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ether.h>

#include <tinyara/clock.h>
#include <tinyara/net/net.h>
#include <tinyara/fs/fs_utils.h>
#ifdef CONFIG_NET_LWIP
#include <net/lwip/netif.h>
#include <net/lwip/dhcp.h>
#include <net/lwip/stats.h>
#endif
#include <tinyara/net/ip.h>
#include <apps/netutils/dhcpc.h>

#ifdef CONFIG_NETUTILS_NETLIB
#include <apps/netutils/netlib.h>
#endif

#if defined(CONFIG_NET_ICMP) && defined(CONFIG_NET_ICMP_PING) && !defined(CONFIG_DISABLE_SIGNALS)
#include <apps/netutils/netlib.h>
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
#include <apps/netutils/netlib.h>
#include <apps/netutils/tftp.h>
#endif

#if defined(CONFIG_NET_TCP) && CONFIG_NFILE_DESCRIPTORS > 0
#include <apps/netutils/netlib.h>
#include <apps/netutils/webclient.h>
#endif

#if defined(CONFIG_NETUTILS_DHCPC) || defined(CONFIG_SYSTEM_NETDB)
#ifdef CONFIG_HAVE_GETHOSTBYNAME
#include <netdb.h>
#endif
#include <apps/netutils/dhcpc.h>
#ifndef DNS_DEFAULT_PORT
#define DNS_DEFAULT_PORT   53
#endif
#endif

#include "netcmd.h"
#include "netcmd_ping.h"

#undef HAVE_PING
#undef HAVE_PING6

/* Size of the ECHO data */

#define DEFAULT_PING_DATALEN 56



#ifdef CONFIG_EXAMPLES_WAKAAMA_CLIENT
extern int wakaamaclient_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_DTLS_CLIENT
extern int dtls_client_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_DTLS_SERVER
extern int dtls_server_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_FTPD
extern int ftpd_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_IOTIVITY
extern int iotivity_main(int argc, char *argv[]);
#ifdef CONFIG_EXAMPLES_IOTIVITY_SECURED
extern int iotivity_sec_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_IOTIVITY_CLOUD
extern int iotivity_cloud_main(int argc, char *argv[]);
#endif
#endif
#ifdef CONFIG_EXAMPLES_IPERF
extern int iperf_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_LWIPDHCPC
extern int lwipdhcpc_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_LWIPDHCPS
extern int lwipdhcps_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_LWIPDNS
extern int lwipdns_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_MDNS_TEST
extern int mdns_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_SYSTEM_NETDB
extern int netdb_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_NETSTRESSTEST
extern int netstresstest_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_NETTEST
extern int nettest_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_NTPCLIENT_TEST
extern int ntpclient_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_SELECT_TEST
extern int select_test_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_TELNETC
extern int telnetc_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_TELNETD
extern int telnetd_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_TLS_CLIENT
extern int tls_client_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_TLS_SELFTEST
extern int tls_selftest_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_TLS_SERVER
extern int tls_server_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_WEBCLIENT
extern int webclient_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_WEBSERVER
extern int webserver_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_WICEDWIFI
extern int wicedwifi_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_WEBSOCKET
extern int websocket_main(int argc, char *argv[]);
#endif
#ifdef CONFIG_EXAMPLES_ARTIK_DEMO
extern int artik_demo_main(int argc, char *argv[]);
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
struct tftpc_args_s {
	bool binary;				/* true:binary ("octet") false:text ("netascii") */
	bool allocated;				/* true: destpath is allocated */
	char *destpath;				/* Path at destination */
	const char *srcpath;		/* Path at src */
	in_addr_t ipaddr;			/* Host IP address */
};
#endif


#if defined(CONFIG_NET_STATISTICS)
static inline void net_statistics()
{
	/* Headings */

	printf("           ");
#ifdef CONFIG_NET_IPv4
	printf("  IPv4");
#endif
#ifdef CONFIG_NET_IPv6
	printf("  IPv6");
#endif
#ifdef CONFIG_NET_TCP
	printf("   TCP");
#endif
#ifdef CONFIG_NET_UDP
	printf("   UDP");
#endif
#ifdef CONFIG_NET_ICMP
	printf("  ICMP");
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  ICMPv6");
#endif
	printf("\n");

	/* Received packets */

	printf("Received   ");
#ifdef CONFIG_NET_IPv4
	printf("  %04x", g_netstats.ipv4.recv);
#endif
#ifdef CONFIG_NET_IPv6
	printf("  %04x", g_netstats.ipv6.recv);
#endif
#ifdef CONFIG_NET_TCP
	printf("  %04x", g_netstats.tcp.recv);
#endif
#ifdef CONFIG_NET_UDP
	printf("  %04x", g_netstats.udp.recv);
#endif
#ifdef CONFIG_NET_ICMP
	printf("  %04x", g_netstats.icmp.recv);
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  %04x", g_netstats.icmpv6.recv);
#endif
	printf("\n");

	/* Dropped packets */

	printf("Dropped    ");
#ifdef CONFIG_NET_IPv4
	printf("  %04x", g_netstats.ipv4.drop);
#endif
#ifdef CONFIG_NET_IPv6
	printf("  %04x", g_netstats.ipv6.drop);
#endif
#ifdef CONFIG_NET_TCP
	printf("  %04x", g_netstats.tcp.drop);
#endif
#ifdef CONFIG_NET_UDP
	printf("  %04x", g_netstats.udp.drop);
#endif
#ifdef CONFIG_NET_ICMP
	printf("  %04x", g_netstats.icmp.drop);
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  %04x", g_netstats.icmpv6.drop);
#endif
	printf("\n");

	/* Dropped IP packets */

#ifdef CONFIG_NET_IPv4
	printf("  IPv4        VHL: %04x   Frg: %04x\n", g_netstats.ipv4.vhlerr, g_netstats.ipv4.fragerr);
#endif
#ifdef CONFIG_NET_IPv6
	printf("  IPv6        VHL: %04x\n", g_netstats.ipv6.vhlerr);
#endif

	/* Checksum errors */

	printf("  Checksum ");
#ifdef CONFIG_NET_IPv4
	printf("  %04x", g_netstats.ipv4.chkerr);
#endif
#ifdef CONFIG_NET_IPv6
	printf("  ----");
#endif
#ifdef CONFIG_NET_TCP
	printf("  %04x", g_netstats.tcp.chkerr);
#endif
#ifdef CONFIG_NET_UDP
	printf("  %04x", g_netstats.udp.chkerr);
#endif
#ifdef CONFIG_NET_ICMP
	printf("  ----");
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  ----");
#endif
	printf("\n");

#ifdef CONFIG_NET_TCP
	printf("  TCP         ACK: %04x   SYN: %04x\n", g_netstats.tcp.ackerr, g_netstats.tcp.syndrop);
	printf("              RST: %04x  %04x\n", g_netstats.tcp.rst, g_netstats.tcp.synrst);
#endif

	/* Prototype errors */

	printf("  Type     ");
#ifdef CONFIG_NET_IPv4
	printf("  %04x", g_netstats.ipv4.protoerr);
#endif
#ifdef CONFIG_NET_IPv6
	printf("  %04x", g_netstats.ipv6.protoerr);
#endif
#ifdef CONFIG_NET_TCP
	printf("  ----");
#endif
#ifdef CONFIG_NET_UDP
	printf("  ----");
#endif
#ifdef CONFIG_NET_ICMP
	printf("  %04x", g_netstats.icmp.typeerr);
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  %04x", g_netstats.icmpv6.typeerr);
#endif
	printf("\n");

	/* Sent packets */

	printf("Sent       ");
#ifdef CONFIG_NET_IPv4
	printf("  %04x", g_netstats.ipv4.sent);
#endif
#ifdef CONFIG_NET_IPv6
	printf("  %04x", g_netstats.ipv6.sent);
#endif
#ifdef CONFIG_NET_TCP
	printf("  %04x", g_netstats.tcp.sent);
#endif
#ifdef CONFIG_NET_UDP
	printf("  %04x", g_netstats.udp.sent);
#endif
#ifdef CONFIG_NET_ICMP
	printf("  %04x", g_netstats.icmp.sent);
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  %04x", g_netstats.icmpv6.sent);
#endif
	printf("\n");

	/* TCP retransmissions */

#ifdef CONFIG_NET_TCP
	printf("  Rexmit   ");
#ifdef CONFIG_NET_IPv4
	printf("  ----");
#endif
#ifdef CONFIG_NET_IPv6
	printf("  ----");
#endif
	printf("  %04x", g_netstats.tcp.rexmit);
#ifdef CONFIG_NET_UDP
	printf("  ----");
#endif
#ifdef CONFIG_NET_ICMP
	printf("  ----");
#endif
#ifdef CONFIG_NET_ICMPv6
	printf("  ----");
#endif
	printf("\n");
#endif							/* CONFIG_NET_TCP */
}
#else
#define net_statistics()
#endif

static void
nic_display_state(void)
{
	struct ifreq *ifr;
	struct sockaddr_in *sin;
	struct sockaddr *sa;
	struct ifconf ifcfg;
	int fd;
	int numreqs = 3;
	int num_nic = 0;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	memset(&ifcfg, 0, sizeof(ifcfg));
	ifcfg.ifc_buf = NULL;
	ifcfg.ifc_len = sizeof(struct ifreq) * numreqs;
	ifcfg.ifc_buf = malloc(ifcfg.ifc_len);
	if (ioctl(fd, SIOCGIFCONF, (void *)&ifcfg) < 0) {
		perror("SIOCGIFCONF ");
		goto DONE;
	}
	num_nic = ifcfg.ifc_len / sizeof(struct ifreq);
	ifr = ifcfg.ifc_req;
	int i = 0;
	for (; i < num_nic; ifr++, i++) {
		printf("%s\t", ifr->ifr_name);
		sin = (struct sockaddr_in *)&ifr->ifr_addr;
		if ((sin->sin_addr.s_addr) == INADDR_LOOPBACK) {
			printf("Loop Back\t");
		} else {
			struct ifreq tmp;
			strncpy(tmp.ifr_name, ifr->ifr_name, IF_NAMESIZE);
			ioctl(fd, SIOCGIFHWADDR, (char *)&tmp);
			sa = &tmp.ifr_hwaddr;
			printf("Link encap: %s\t", ether_ntoa((struct ether_addr *)sa->sa_data));

			ioctl(fd, SIOCGIFFLAGS, (void *)ifr);
			printf("RUNNING: %s\n", (ifr->ifr_flags & IFF_UP) ? "UP" : "DOWN");
		}
		printf("\tinet addr: %s\t", inet_ntoa(sin->sin_addr));
		ioctl(fd,  SIOCGIFBRDADDR, (char *)ifr);
		sin = (struct sockaddr_in *)&ifr->ifr_broadaddr;
		printf("Bcast: %s\t", inet_ntoa(sin->sin_addr));
		ioctl(fd, SIOCGIFNETMASK, (char *)ifr);
		sin = (struct sockaddr_in *)&ifr->ifr_addr;
		printf("Mask: %s\t", inet_ntoa(sin->sin_addr));
		ioctl(fd, SIOCGIFMTU, (char *)ifr);
		printf("MTU: %d\n", ifr->ifr_mtu);
		printf("\n");
	}
DONE:
	free(ifcfg.ifc_buf);
	close(fd);
}


#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0
int tftpc_parseargs(int argc, char **argv, struct tftpc_args_s *args)
{
	FAR const char *fmt = fmtarginvalid;
	bool badarg = false;
	int option;

	/* Get the ping options */

	memset(args, 0, sizeof(struct tftpc_args_s));
	optind = -1;
	while ((option = getopt(argc, argv, ":bnf:h:")) != ERROR) {
		switch (option) {
		case 'b':
			args->binary = true;
			break;

		case 'n':
			args->binary = false;
			break;

		case 'f':
			args->destpath = optarg;
			break;

		case 'h':
			if (!netlib_ipaddrconv(optarg, (FAR unsigned char *)&args->ipaddr)) {
				printf(fmtarginvalid, argv[0]);
				badarg = true;
			}
			break;

		case ':':
			printf(fmtargrequired, argv[0]);
			badarg = true;
			break;

		case '?':
		default:
			printf(fmtarginvalid, argv[0]);
			badarg = true;
			break;
		}
	}

	/* If a bad argument was encountered, then return without processing the command */

	if (badarg) {
		return ERROR;
	}

	/* There should be exactly one parameter left on the command-line */

	if (optind == argc - 1) {
		args->srcpath = argv[optind];
	}

	/* optind == argc means that there is nothing left on the command-line */

	else if (optind >= argc) {
		fmt = fmtargrequired;
		goto errout;
	}

	/* optind < argc-1 means that there are too many arguments on the
	 * command-line
	 */

	else {
		fmt = fmttoomanyargs;
		goto errout;
	}

	/* The HOST IP address is also required */

	if (!args->ipaddr) {
		fmt = fmtargrequired;
		goto errout;
	}

	/* If the destpath was not provided, then we have do a little work. */

	if (!args->destpath) {
		char *tmp1;
		char *tmp2;

		/* Copy the srcpath... baseanme might modify it */

		fmt = fmtcmdoutofmemory;
		tmp1 = strdup(args->srcpath);
		if (!tmp1) {
			goto errout;
		}

		/* Get the basename of the srcpath */

		tmp2 = basename(tmp1);
		if (!tmp2) {
			free(tmp1);
			goto errout;
		}

		/* Use that basename as the destpath */

		args->destpath = strdup(tmp2);
		free(tmp1);
		if (!args->destpath) {
			goto errout;
		}

		args->allocated = true;
	}

	return OK;

 errout:
	printf(fmt, argv[0]);
	return ERROR;
}
#endif

#if defined(CONFIG_NET_UDP) && CONFIG_NFILE_DESCRIPTORS > 0 && CONFIG_NETUTILS_TFTPC
int cmd_get(int argc, char **argv)
{
	struct tftpc_args_s args;
	char *fullpath;

//for fs
	int i = 0;
	int fd;
	int ret = -1;
	char write_buffer[300];
	char read_buffer[301];
	char filename[30]
	char newfilename[30];
	char seek_wbuffer[100];
	char seek_rbuffer[101];
//~for fs

	/* Parse the input parameter list */

	if (tftpc_parseargs(argc, argv, &args) != OK) {
		return ERROR;
	}

	/* Get the full path to the local file */

	fullpath = get_fullpath(args.destpath);

	printf("Doing FS setting...\n");
	ret = fs_erase("/dev/smart1");
	if (ret != OK) {
		printf("FS erase error\n");
		return;
	}
	//printf("FS erase done\n");

	ret = fs_initiate("/dev/smart1", "smartfs");
	if (ret != OK) {
		printf("FS initiate error");
		return;
	} else {
		printf("FS initiate done\n");
	}

	/* Then perform the TFTP get operation */

	printf("src: %s full: %s addr: %d bin: %d\n", args.srcpath, fullpath, args.ipaddr, args.binary);

//if (tftpget(args.srcpath, fullpath, 0xC0A80022, args.binary) != OK)
	if (tftpget(args.srcpath, fullpath, args.ipaddr, args.binary) != OK) {
		printf(fmtcmdfailed, argv[0], "tftpget");
	}

	fd = open("/mnt/test.txt", O_RDONLY);
	if (fd < 0) {
		printf("Open failed: %d\n", errno);
	}
	printf("Opened\n");

	ret = read(fd, seek_rbuffer, 100);
	if (ret < 0) {
		printf("Seek read failed %d\n", ret);
		return;
	} else {
		printf("Read done\n");
	}

	for (i = 0; i < 101; i++) {
		printf("%c", seek_rbuffer[i]);
	}

	/* Release any allocated memory */

	if (args.allocated) {
		free(args.destpath);
	}

	free(fullpath);
	return OK;
}
#endif

int cmd_ifup(int argc, char **argv)
{
	FAR char *intf = NULL;
	int ret;

	if (argc != 2) {
		printf("Please select nic_name:\n");
		nic_display_state();
		return OK;
	}

	intf = argv[1];
	ret = netlib_ifup(intf);
	printf("ifup %s...%s\n", intf, (ret == OK) ? "OK" : "Failed");
	return ret;
}

int cmd_ifdown(int argc, char **argv)
{
	FAR char *intf = NULL;
	int ret;

	if (argc != 2) {
		printf("Please select nic_name:\n");
		nic_display_state();
		return OK;
	}

	intf = argv[1];
	ret = netlib_ifdown(intf);
	printf("ifdown %s...%s\n", intf, (ret == OK) ? "OK" : "Failed");
	return ret;
}

int cmd_ifconfig(int argc, char **argv)
{
#ifdef CONFIG_NET_IPv4
	struct in_addr addr;
#endif
#ifdef CONFIG_NET_IPv6
	struct in6_addr addr6;
#endif
	in_addr_t gip;
	int i;
	FAR char *intf = NULL;
	FAR char *hostip = NULL;
	FAR char *gwip = NULL;
	FAR char *mask = NULL;
	FAR char *tmp = NULL;
#ifdef CONFIG_NET_ETHERNET
	FAR char *hw = NULL;
#endif
#if defined(CONFIG_NETUTILS_DHCPC) || defined(CONFIG_SYSTEM_NETDB)
	FAR char *dns = NULL;
#endif
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
	bool inet6 = false;
#endif
	bool badarg = false;
	uint8_t mac[IFHWADDRLEN];

#ifdef CONFIG_NET_LWIP
	struct netif *netif;
#endif

	/* With one or no arguments, ifconfig simply shows the status of Ethernet
	 * device:
	 *
	 *   ifconfig
	 *   ifconfig [nic_name]
	 */

	if (argc <= 2) {
		nic_display_state();
		net_statistics();
		return OK;
	}

	/* If both the network interface name and an IP address are supplied as
	 * arguments, then ifconfig will set the address of the Ethernet device:
	 *
	 *    ifconfig nic_name ip_address
	 */

	if (argc > 2) {
		for (i = 0; i < argc; i++) {
			if (i == 1) {
				intf = argv[i];
			} else if (i == 2) {
				hostip = argv[i];
			} else {
				tmp = argv[i];
				if (!strcmp(tmp, "dr") || !strcmp(tmp, "gw") || !strcmp(tmp, "gateway")) {
					if (argc - 1 >= i + 1) {
						gwip = argv[i + 1];
						i++;
					} else {
						badarg = true;
					}
				} else if (!strcmp(tmp, "netmask")) {
					if (argc - 1 >= i + 1) {
						mask = argv[i + 1];
						i++;
					} else {
						badarg = true;
					}
				} else if (!strcmp(tmp, "inet")) {
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
					inet6 = false;
#elif !defined(CONFIG_NET_IPv4)
					badarg = true;
#endif
				} else if (!strcmp(tmp, "inet6")) {
#if defined(CONFIG_NET_IPv4) && defined(CONFIG_NET_IPv6)
					inet6 = true;
#elif !defined(CONFIG_NET_IPv6)
					badarg = true;
#endif
				}
#ifdef CONFIG_NET_ETHERNET
				/* REVISIT: How will we handle Ethernet and SLIP networks together? */

				else if (!strcmp(tmp, "hw")) {
					if (argc - 1 >= i + 1) {
						hw = argv[i + 1];
						i++;
						badarg = !netlib_hwmacconv(hw, mac);
					} else {
						badarg = true;
					}
				}
#endif

#if defined(CONFIG_NETUTILS_DHCPC) || defined(CONFIG_SYSTEM_NETDB)
				else if (!strcmp(tmp, "dns")) {
					if (argc - 1 >= i + 1) {
						dns = argv[i + 1];
						i++;
					} else {
						badarg = true;
					}
				}
#endif
			}
#ifdef CONFIG_NET_LWIP
			netif = netif_find(intf);
#endif
		}
	}

	if (badarg) {
		printf(fmtargrequired, argv[0]);
		return ERROR;
	}
#ifdef CONFIG_NET_ETHERNET
	/* Set Hardware Ethernet MAC address */
	/* REVISIT: How will we handle Ethernet and SLIP networks together? */

	if (hw) {
		ndbg("HW MAC: %s\n", hw);
		netlib_setmacaddr(intf, mac);
	}
#endif

	/* Set IP address */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
	if (inet6)
#endif
	{
		UNUSED(addr6);
		UNUSED(gip);
		UNUSED(hostip);
	}
#endif							/* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
	else
#endif
	{
		if (hostip != NULL) {

#if defined(CONFIG_NETUTILS_DHCPC) || defined(CONFIG_NET_LWIP)
			if (!strcmp(hostip, "dhcp")) {
				/* Set DHCP addr */

				ndbg("DHCPC Mode\n");
				gip = addr.s_addr = 0;
			} else
#endif
			{
				/* Set host IP address */

				ndbg("Host IP: %s\n", hostip);
				gip = addr.s_addr = inet_addr(hostip);
			}

			netlib_set_ipv4addr(intf, &addr);
		} else {
			printf("hostip is not provided\n");
			return ERROR;
		}
	}
#endif							/* CONFIG_NET_IPv4 */

#ifdef CONFIG_NETUTILS_DHCPC
	/* Get the MAC address of the NIC */

	if (!gip) {
		FAR void *handle;
		netlib_getmacaddr(intf, mac);

		/* Set up the DHCPC modules */

		handle = dhcpc_open(intf);

		/* Get an IP address.  Note that there is no logic for renewing the IP
		 * address in this example.  The address should be renewed in
		 * ds.lease_time/2 seconds.
		 */

		if (!handle) {
			return ERROR;
		}
		struct dhcpc_state ds;

		int ret = dhcpc_request(handle, &ds);
		if (ret < 0) {
			dhcpc_close(handle);
			return ERROR;
		}

		netlib_set_ipv4addr(intf, &ds.ipaddr);

		if (ds.netmask.s_addr != 0) {
			netlib_set_ipv4netmask(intf, &ds.netmask);
		}

		if (ds.default_router.s_addr != 0) {
			netlib_set_dripv4addr(intf, &ds.default_router);
		}
		printf("IP address %s\n", inet_ntoa(ds.ipaddr));
		printf("Netmask %s\n", inet_ntoa(ds.netmask));
		printf("Gateway %s\n", inet_ntoa(ds.default_router));

#ifdef CONFIG_NETDB_DNSCLIENT
		if (ds.dnsaddr.s_addr != 0) {
			struct sockaddr_in sock_dns;
			sock_dns.sin_family = AF_INET;
			sock_dns.sin_port = htons(DNS_DEFAULT_PORT);
			sock_dns.sin_addr.s_addr = htonl(ds.dnsaddr.s_addr);
			dns_add_nameserver((FAR struct sockaddr *)&sock_dns, sizeof(struct sockaddr_in));
		}
#endif
		dhcpc_close(handle);

		return OK;
	}
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
	if (inet6)
#endif
	{
		UNUSED(gwip);
	}
#endif							/* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
	else
#endif
	{
		/* Set gateway */
		if (gwip) {
			ndbg("Gateway: %s\n", gwip);
			gip = addr.s_addr = inet_addr(gwip);
		} else {
			if (gip) {
				ndbg("Gateway: default\n");
				gip = NTOHL(gip);
				gip &= ~0x000000ff;
				gip |= 0x00000001;
				gip = HTONL(gip);
			}

			addr.s_addr = gip;
		}
		netlib_set_dripv4addr(intf, &addr);
	}
#endif							/* CONFIG_NET_IPv4 */

	/* Set network mask */

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
	if (inet6)
#endif
	{
		UNUSED(mask);
	}
#endif							/* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
	else
#endif
	{
		if (mask) {
			ndbg("Netmask: %s\n", mask);
			addr.s_addr = inet_addr(mask);
		} else {
			ndbg("Netmask: Default\n");
			addr.s_addr = inet_addr("255.255.255.0");
		}
		netlib_set_ipv4netmask(intf, &addr);
	}
#endif							/* CONFIG_NET_IPv4 */

#if defined(CONFIG_NETUTILS_DHCPC) || defined(CONFIG_SYSTEM_NETDB)
#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
	if (inet6)
#endif
	{
		UNUSED(dns);
	}
#endif							/* CONFIG_NET_IPv6 */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
	else
#endif
	{
		if (dns) {
			ndbg("DNS: %s\n", dns);
			addr.s_addr = inet_addr(dns);
		} else {
			ndbg("DNS: Default\n");
			addr.s_addr = gip;
		}

		//   dns_setserver(&addr);
	}
#endif							/* CONFIG_NET_IPv4 */
#endif

	return OK;
}



const static tash_cmdlist_t net_utilcmds[] = {
#ifdef CONFIG_NETUTILS_TFTPC
	{"ftp_get", cmd_get, TASH_EXECMD_SYNC},
#endif
	{"ifconfig", cmd_ifconfig, TASH_EXECMD_SYNC},
	{"ifdown", cmd_ifdown, TASH_EXECMD_SYNC},
	{"ifup", cmd_ifup, TASH_EXECMD_SYNC},
#ifdef CONFIG_NET_LWIP
	{"lwip_stats", stats_display, TASH_EXECMD_ASYNC},
#endif
	{"ping", cmd_ping, TASH_EXECMD_SYNC},
#if defined(CONFIG_NET_ICMPv6) && defined(CONFIG_NET_ICMPv6_PING) && !defined(CONFIG_DISABLE_SIGNALS)
	{"ping6", cmd_ping6, TASH_EXECMD_SYNC},
#endif
	{NULL, NULL, 0}
};

const static tash_cmdlist_t net_appcmds[] = {
#ifdef CONFIG_EXAMPLES_ARTIK_DEMO
	{"artik_demo", artik_demo_main, TASH_EXECMD_SYNC},
#endif
#ifdef CONFIG_EXAMPLES_LWIPDHCPC
	{"dhcpc", lwipdhcpc_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_LWIPDHCPS
	{"dhcps", lwipdhcps_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_LWIPDNS
	{"dns", lwipdns_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_DTLS_CLIENT
	{"dtlsc", dtls_client_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_DTLS_SERVER
	{"dtlss", dtls_server_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_FTPD
	{"ftpd", ftpd_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_IOTIVITY
	{"iotivity", iotivity_main, TASH_EXECMD_ASYNC},
#ifdef CONFIG_EXAMPLES_IOTIVITY_CLOUD
	{"iotivity_cloud", iotivity_cloud_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_IOTIVITY_SECURED
	{"iotivity_sec", iotivity_sec_main, TASH_EXECMD_ASYNC},
#endif
#endif							/* CONFIG_EXAMPLES_IOTIVITY */
#ifdef CONFIG_EXAMPLES_IPERF
	{"iperf", iperf_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_MDNS_TEST
	{"mdns", mdns_main, TASH_EXECMD_SYNC},
#endif
#ifdef CONFIG_SYSTEM_NETDB
	{"netdb", netdb_main, TASH_EXECMD_SYNC},
#endif
#ifdef CONFIG_EXAMPLES_NETSTRESSTEST
	/* Sample App to stress test Transport Layer (TCP / UDP) / IP Multicast Functionality */
	{"netstresstest", netstresstest_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_NETTEST
	/* Sample App to test Transport Layer (TCP / UDP) / IP Multicast Functionality */
	{"nettest", nettest_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_NTPCLIENT_TEST
	{"ntpclient", ntpclient_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_SELECT_TEST
	{"select_test", select_test_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_TELNETC
	{"telnetc", telnetc_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_TELNETD
	{"telnetd", telnetd_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_TLS_CLIENT
	{"tlsc", tls_client_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_TLS_SERVER
	{"tlss", tls_server_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_TLS_SELFTEST
	{"tlsself", tls_selftest_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_WEBSOCKET
	{"websocket", websocket_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_WAKAAMA_CLIENT
	{"wakaamaclient", wakaamaclient_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_WEBCLIENT
	{"webclient", webclient_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_WEBSERVER
	{"webserver", webserver_main, TASH_EXECMD_ASYNC},
#endif
#ifdef CONFIG_EXAMPLES_WICEDWIFI
	{"wicedwifi", wicedwifi_main, TASH_EXECMD_ASYNC},
#endif
	{NULL, NULL, 0}
};

void net_register_utilcmds(void)
{
	tash_cmdlist_install(net_utilcmds);
}

void net_register_appcmds(void)
{
	tash_cmdlist_install(net_appcmds);
}
#endif
