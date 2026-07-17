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
 * examples/nettest/nettest.c
 *
 *   Copyright (C) 2012 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * This is a leverage of similar logic from uIP:
 *
 *   Author: Adam Dunkels <adam@sics.se>
 *   Copyright (c) 2003, Adam Dunkels.
 *   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ****************************************************************************/

/**
 * @testcase		   :lwip_nettest_01
 * @brief		   :To establish a connection between tcp server and tcp client
 * @scenario		   :
 * @apicovered	   :
 * @precondition	   : TCP Server(board1)		        TCP Client(board2)
 *			   :1. Connect to Wi-Fi		      1. Connect to Wi-Fi
*			   :2. Get an IP address	      2. Get an IP address
*			   :3. Run TCP Server		      3. Run TCP Client
*			   :TASH> nettest <servip> 5001 1   TASH> nettest <servip> 5001 4
* @postcondition	   :
*/

/**
 * @testcase		   :lwip_nettest_02
 * @brief		   :To establish a connection between udp server and udp client
 * @scenario		   :
 * @apicovered	   :
 * @precondition	   : UDP Server(board1)		        UDP Client(board2)
 *			   :1. Connect to Wi-Fi		      1. Connect to Wi-Fi
*			   :2. Get an IP address	      2. Get an IP address
*			   :3. Run UDP Server		      3. Run UDP Client
*			   :TASH> nettest <servip> 5001 2   TASH> nettest <servip> 5001 5
* @postcondition	   :
*/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <sched.h>

#include <tinyara/progmem.h>
#include <tinyara/fs/smart.h>
#include <tinyara/fs/ioctl.h>
#include <tinyara/spi/spi.h>

#include <sys/stat.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netutils/netlib.h>
#include <sys/select.h>
#include <sys/socket.h>

#define AF_INETX AF_INET
#define PF_INETX PF_INET

/*
* TCP
*/
#define BUF_SIZE 512 // test client sends 504bytes max
#define CLIENT_MAX_PAYLOAD 500
/*
* UDP
*/
#define APP_MSG_SIZE 256

/*
* NETWORK
*/

#define MCAST_PORT 5555
#define MCAST_GROUP "225.1.1.1"

#define NUM_PACKETS 50
#define LOCAL_DEVICE "192.168.2.10"
#define NETTEST_MAX_PACKETS 20000

// Packet header for data verification
#define HDR_SIZE    4   // 4 bytes each: len
/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/
/* Default localIP, target IP and target port, will be set to appropriate target IP and target Port in netstresstest_main */
char *g_app_target_addr = "192.168.2.6";
int g_app_target_port = 5555;
uint32_t total_data;

#define NETTEST_SERVER_MODE 1
#define NETTEST_CLIENT_MODE 2
#define NETTEST_INTERNAL_MODE 3
#define NETTEST_PROTO_TCP "tcp"
#define NETTEST_PROTO_UDP "udp"
#define NETTEST_PROTO_BROADCAST "brc"
#define NETTEST_PROTO_MULTICAST "mtc"
#define NETTEST_PROTO_STRESS "str"

typedef enum {
	NT_NONE,
	NT_TCP,
	NT_UDP,
	NT_BROADCAST,
	NT_MULTICAST,
	NT_STRESS,
} nettest_proto_e;

/****************************************************************************
 * Private Functions
 ****************************************************************************/
static void show_usage(void)
{
	printf("Usage: \n\n");
	printf("\tnettest MODE PROTOCOL ADDRESS PORT PACKETS [INTERVAL] [INTERFACE]\n");

	printf("MODE:\n");
	printf("\t1: server\n");
	printf("\t2: client\n");
	printf("\t3: internal\n");

	printf("PROTOCOL\n");
	printf("\ttcp: TCP\n");
	printf("\tudp: UDP\n");
	printf("\tmtc: MULTICAST\n");
	printf("\tbrc: BROADCAST\n");
	printf("\tstr: STRESS TEST\n");

	printf("ADDRESS\n");
	printf("\tAddress to bind if mode is server\n");
	printf("\t\t(If you want to choose the default interface then insert 0)\n");
	printf("\tTarget address if mode is client\n");
	printf("\tGroup address to join if mode is multicast\n");

	printf("PORT\n");
	printf("\tPort to bind if mode is server\n");
	printf("\tPort to connect if mode is client\n");

	printf("PACKET\n");
	printf("\tThe number of packets to receive if the mode is server\n");
	printf("\t\t(If you want to receive continousely then insert 0)\n");
	printf("\tThe number of packets to send if the mode is client\n");
	printf("\t\t(If you want to send continousely then insert 0)\n");

	printf("INTERVAL\n");
	printf("\tSet the interval of the sent packet if mode is client(default 0)\n");
	printf("\t\t(Set 0 if you want send packet as many as possible)\n");
	printf("\t\t(You can't insert the interval if mode is server)\n");

	printf("INTERFACE\n");
	printf("\tThe interface to bind if mode is multicast\n");

	printf("Sample Command:\n");
	printf("\tRun TCP Server\n");
	printf("\t\tTASH>>nettest 1 tcp 0 9098 10\n");

	printf("\tRun UDP Client\n");
	printf("\t\tTASH>>nettest 2 udp 192.168.1.1 9098 10 3\n");

	printf("\tRun Multicast Receiver\n");
	printf("\t\tTASH>>nettest 1 mtc 224.1.1.1 5555 0 wl1\n");

	printf("\tRun Stress Test\n");
	printf("\t\tTASH>>nettest 2 str 192.168.1.226 5555\n");
	printf("\t\tNOTE: shutdown() is called at random time between 7 and 17 secs.\n");
	printf("\t\tand heapinfo is printed out for 10 shutdown() calls\n");
	printf("\n\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/
#define MIN_LENGTH 1
#define MAX_LENGTH 500

// Function to generate random length from MIN_LENGTH to MAX_LENGTH
size_t generateRandomLength(void) {
	// Ensure the randomness by seeding with current time
	srand(time(NULL));

	// Generate random length
	size_t length = (rand() % (MAX_LENGTH - MIN_LENGTH + 1)) + MIN_LENGTH;

	return length;
}

// Function to generate random string data
char* generateRandomString(size_t length) {
	unsigned int full_len = (length + HDR_SIZE);
	char *test_data = (char*)zalloc(full_len);
	if (!test_data) {
		printf("%s: Memory alloc fail\n",__func__);
		return NULL;
	}
	// Include length of payload in the header
	memcpy(test_data, &length, HDR_SIZE);
	const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	size_t charsetSize = sizeof(charset) - 1;

	for (size_t i = 0; i < length; i++) {
		int key = rand() % charsetSize;
		test_data[HDR_SIZE + i] = charset[key];
	}

	return test_data;
}

/* Read exactly 'n' bytes from a TCP socket. Returns 0 on success, -1 on connection close/error. */
static int recv_all(int sockfd, uint8_t *buf, int n)
{
	int offset = 0;
	while (offset < n) {
		int nread = recv(sockfd, buf + offset, n - offset, 0);
		if (nread <= 0) {
			printf("[TCPSERV] connection closed or error (got %d of %d bytes)\n", offset, n);
			return -1;
		}
		offset += nread;
	}
	return 0;
}

/* ------------------------------------------------------------ */
/*                                                              */
/* Send Broadcast example.                                      */
/*                                                              */
/* ------------------------------------------------------------ */
void broadcast_send(uint32_t num_packets, uint32_t sleep_time)
{
	int sock;
	int is_broadcast = 1;
	int res = -1;
	struct sockaddr_in dest_addr;
	const char databuf[] = "broadcast msg from TizenRT";

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == -1) {
		printf("[BRCLIENT] socket create fail err(%d)\n", errno);
		return;
	}

	res = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &is_broadcast, sizeof(is_broadcast));
	if (res == -1) {
		printf("[BRCLIENT] set socket option fail err(%d)\n", errno);
		goto return_with_close;
	}

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(g_app_target_port);
	dest_addr.sin_addr.s_addr = inet_addr(g_app_target_addr);
	int i = 1;
	for (; i <= num_packets; i++) {
		res = sendto(sock, databuf, sizeof(databuf), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
		if (res < 0) {
			printf("[BRCLIENT] [ERR] sending datagram message (%d)", errno);
			goto return_with_close;
		}
		printf("[BRCLIENT] sending broadcast message length (%d) number (%d)\n", res, i);
		if (sleep_time != 0) {
			usleep(sleep_time);
		}
	}
	printf("[BRCLIENT] Terminate broadcast sender after sending sufficient messages (%d)\n", num_packets);

return_with_close:
	close(sock);
}

/* ------------------------------------------------------------ */
/*                                                              */
/* Receive Broadcast example.                                   */
/*                                                              */
/* ------------------------------------------------------------ */
void broadcast_receive(uint32_t num_packets)
{
	int sockfd;
	int is_broadcast = 1;
	int count;
	int nbytes;
	char databuf[256];
	struct sockaddr_in recvaddr;
	struct sockaddr_in sendaddr;
	socklen_t addrlen = sizeof(struct sockaddr_in);

	printf("[BRSERVER] broadcast receive start!\n");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("[BRSERVER] socket create fail err(%d)\n", errno);
		return;
	}

	if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &is_broadcast, sizeof(is_broadcast)) < 0) {
		printf("[BRSERVER] set socket option fail err(%d)\n", errno);
		goto return_with_close;
	}

	memset(&recvaddr, 0, sizeof(recvaddr));
	recvaddr.sin_family = AF_INET;
	recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	recvaddr.sin_port = htons(g_app_target_port);

	if (bind(sockfd, (struct sockaddr *)&recvaddr, sizeof(recvaddr)) < 0) {
		printf("[BRSERVER] bind fail err(%d)\n", errno);
		goto return_with_close;
	}

	count = 0;

	while (1) {
		memset(databuf, 0, sizeof(databuf));

		nbytes = recvfrom(sockfd, databuf, sizeof(databuf), 0, (struct sockaddr *)&sendaddr, (socklen_t *)&addrlen);
		if (nbytes <= 0) {
			printf("[BRSERVER] receive data fail err(%d)\n", errno);
			goto return_with_close;
		}
		count++;
		printf("[BRSERVER] received data from %s #%d : %s\n", inet_ntoa(sendaddr.sin_addr), count, databuf);

		if (num_packets == 0) {
			continue;
		}
		if (count >= num_packets) {
			printf("[BRSERVER] exit BRSERVER as received sufficient packets for testing\n");
			break;
		}
	}

return_with_close:
	close(sockfd);
	return;
}

/* ------------------------------------------------------------ */
/*                                                              */
/* Send Multicast Datagram code example.                        */
/*                                                              */
/* ------------------------------------------------------------ */
void ipmcast_sender_thread(int num_packets, uint32_t sleep_time, const char *intf)
{
	int ret = 0;
	int i = 0;
	int datalen;
	struct in_addr localInterface;
	struct sockaddr_in groupSock;
	int sd;
	char loopch = 0;
	char *databuf = "Test Data: IP Multicast from TinyAra Node to Linux";

	socklen_t addrlen = sizeof(struct sockaddr_in);
	printf("[MCASTCLIENT] start multicast sender\n");
	/*
	* Create a datagram socket on which to send.
	*/
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		printf("[MCASTCLIENT] [ERR] opening datagram socket");
		return;
	}
	printf("[MCASTCLIENT] created socket successfully\n");

	/*
	* Disable loopback so you do not receive your own datagrams.
	*/
	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&loopch, sizeof(loopch)) < 0) {
		printf("[MCASTCLIENT] [ERR] Failed setting IP_MULTICAST_LOOP:");
		goto out_with_socket;
	}
	printf("[MCASTCLIENT] setsockopt MULTICAST_LOOP success\n");

	/*
	* Initialize the group sockaddr structure with a
	* group address of 225.1.1.1 and port 5555.
	*/
	memset((char *)&groupSock, 0, sizeof(groupSock));
	groupSock.sin_family = AF_INET;
	groupSock.sin_addr.s_addr = inet_addr(g_app_target_addr);
	groupSock.sin_port = htons(g_app_target_port);

	if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF, (char *)&localInterface, sizeof(localInterface)) < 0) {
		printf("[MCASTCLIENT] [ERR] Failed setting local interface");
		goto out_with_socket;
	}
	printf("[MCASTCLIENT] setsockopt IP_MULTICAST_IF success\n");

	/*
	* Set local interface for outbound multicast datagrams.
	* The IP address specified must be associated with a local,
	* multicast-capable interface.
	*/
	ret = netlib_get_ipv4addr(intf, &localInterface);
	if (ret == -1) {
		printf("[MCASTCLIENT] fail to get interface's ip address\n");
		goto out_with_socket;
	}
	printf("[MCASTCLIENT] bind interface(%s)\n", intf);
	printf("[MCASTCLIENT] group address(%s)\n", g_app_target_addr);
	printf("[MCASTCLIENT] port(%d)\n", g_app_target_port);

	/*
	* Send a message to the multicast group specified by the
	* groupSock sockaddr structure.
	*/
	datalen = strlen(databuf);
	printf("[MCASTCLIENT] datalen = %d \n", datalen);
	printf("[MCASTCLIENT] data (%s)\n", databuf);
	printf("[MCASTCLIENT] interval(%d)\n", sleep_time);
	for (i = 1; i <= num_packets; i++) {
		printf("[MCASTCLIENT] sending mcast message length (%d) number (%d)\n", datalen, i);
		ret = sendto(sd, databuf, datalen, 0, (struct sockaddr *)&groupSock, addrlen);
		if (ret < 0) {
			printf("[MCASTCLIENT] [ERR] sending datagram message");
		}
		if (sleep_time != 0) {
			usleep(sleep_time);
		}
	}
	printf("[MCASTCLIENT] Terminate multicast sender after sending sufficient messages (%d)\n", num_packets);
out_with_socket:
	close(sd);
	return;
}

/* ------------------------------------------------------------ */
/*                                                              */
/* Receive Multicast Datagram code example.                     */
/*                                                              */
/* ------------------------------------------------------------ */
void ipmcast_receiver_thread(int num_packets, const char *intf)
{
	int ret = 0;
	int count = 0;
	int datalen = 0;
	char databuf[256];
	int sd = -1;
	struct sockaddr_in localSock;
	struct ip_mreq group;
	printf("[MCASTSERV] start multicast receiver\n");
	/*
	* Create a datagram socket on which to receive.
	*/
	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0) {
		printf("[MCASTSERV] ERR : opening datagram socket\n");
		return;
	}
	printf("[MCASTSERV] create socket success\n");
	/*
	* Enable SO_REUSEADDR to allow multiple instances of this
	* application to receive copies of the multicast datagrams.
	*/

	int reuse = 1;
	ret = setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	if (ret < 0) {
		printf("[MCASTSERV] ERR: setting SO_REUSEADDR\n");
		goto out_with_socket;
	}
	printf("[MCASTSERV] set reusable success\n");

	/*
	* Bind to the proper port number with the IP address
	* specified as INADDR_ANY.
	*/
	memset((char *)&localSock, 0, sizeof(localSock));
	localSock.sin_family = AF_INET;
	localSock.sin_port = htons(g_app_target_port);
	localSock.sin_addr.s_addr = INADDR_ANY;

	group.imr_interface.s_addr = INADDR_ANY;
	group.imr_multiaddr.s_addr = inet_addr(g_app_target_addr);
	if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&group, sizeof(group)) < 0) {
		printf("[MCASTSERV] fail: adding multicast group %d\n", errno);
		goto out_with_socket;
	}

	if (bind(sd, (struct sockaddr *)&localSock, sizeof(localSock))) {
		printf("[MCASTSERV] ERR: binding datagram socket\n");
		goto out_with_socket;
	}

	printf("[MCASTSERV] bind socket success\n");

	/*
	* Join the multicast group 225.1.1.1 on the local 192.168.2.10
	* interface.  Note that this IP_ADD_MEMBERSHIP option must be
	* called for each local interface over which the multicast
	* datagrams are to be received.
	*/

	ret = netlib_get_ipv4addr(intf, &group.imr_interface);
	if (ret == -1) {
		printf("[MCASTSERV] fail to get interface's ip address\n");
		goto out_with_socket;
	}
	printf("[MCASTSERV] bind interface(%s)\n", intf);
	printf("[MCASTSERV] group address(%s)\n", g_app_target_addr);
	printf("[MCASTSERV] port(%d)\n", g_app_target_port);

	printf("[MCASTSERV] join multicast success\n");
	/*
	* Read from the socket.
	*/
	while (1) {
		datalen = sizeof(databuf);
		/* any of the below call read / recv is fine */
		ret = read(sd, databuf, datalen);

		if (ret < 0) {
			printf("[MCASTSERV] ERR: reading datagram message (%d)\n", errno);
			break;
		}

		count++;
		printf("[MCASTSERV] - Received Msg # %d] read (%d) bytes\n", count, ret);
		if (num_packets == 0) {
			continue;
		}
		if (count > num_packets) {
			printf("[MCASTSERV] Terminating MCLIENT as received sufficient packets for testing\n");
			break;
		}
	}
out_with_socket:
	close(sd);
	return;
}

int udp_server_thread(int num_packets)
{
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;
	int s;
	int nbytes;
	int count = 0;
	socklen_t addrlen = sizeof(struct sockaddr_in);
	char buf[BUF_SIZE];
	fd_set rfds;
	fd_set fds;
	int ret = 0;
	uint32_t failures = 0; // Check length mismatches

	s = socket(PF_INET, SOCK_DGRAM, 0);
	if (s < 0) {
		perror("[UDPSERV]socket fail");
		return 0;
	}

	printf("[UDPSERV] socket created\n");
	printf("[UDPSERV] debug %d %d\n", addrlen, sizeof(struct sockaddr));
	memset(&cliaddr, 0, addrlen);
	memset(&servaddr, 0, addrlen);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(g_app_target_port);

	ret = bind(s, (struct sockaddr *)&servaddr, addrlen);
	if (ret < 0) {
		perror("[UDPSERV]bind fail\n");
		goto out_with_socket;
	}
	printf("[UDPSERV] socket binded\n");
	printf("[UDPSERV] waiting on port %d\n", g_app_target_port);

	FD_ZERO(&fds);
	FD_SET(s, &fds);

	int max_fd = s + 1;
	struct timeval tv;
	tv.tv_sec = 10;   // set select timeout, close after 10sec of inactivity
	tv.tv_usec = 0;
	while (1) {
		rfds = fds;
		int res = select(max_fd, &rfds, NULL, NULL, &tv);
		if (res < 0) {
			printf("[UDPSERV] select error(%d)\n", errno);
			goto out_with_socket;
		}
		if (res > 0) {
			if (FD_ISSET(s, &rfds)) {
				nbytes = recvfrom(s, buf, BUF_SIZE, 0, (struct sockaddr *)&cliaddr, &addrlen);
				if (nbytes < 0) {
					perror("[UDPSERV] recvfrom fail");
					break;
				}
				if (nbytes == 0) {
					printf("[UDPSERV] socket closed from remote\n");
					goto out_with_socket;
				}
				count++;
				// Parse the 4-byte length header
				if (nbytes >= HDR_SIZE) {
					uint32_t claimed_len;
					memcpy(&claimed_len, buf, HDR_SIZE);

					uint32_t actual_payload = nbytes - HDR_SIZE;

					if (claimed_len == actual_payload) {
						// printf("[UDPSERV] #%d: UDP recv=%lu OK\n",count, nbytes);
					} else {
						failures++;
						printf("[UDPSERV] #%d: MISMATCH header=%lu payload=%lu\n",count, claimed_len, actual_payload);
					}
				} else {
					// Datagram too small to even contain a header
					failures++;
					printf("[UDPSERV] #%d: ###FAIL too small (%d bytes, need %d)\n",count, nbytes, HDR_SIZE);
				}
				if (num_packets == 0) {
					continue;
				}
				if (count >= num_packets) {
					// unsigned int udp_loss = (((num_packets - count)/num_packets) * 100);
					// printf("[UDPSERV] Exiting UDPSERV as received sufficient packets for testing, UDP loss %d%%\n",udp_loss);
					printf("[UDPSERV] Exiting UDPSERV as received sufficient packets for testing\n");
					break;
				}
			}
		} else {
			printf("[UDPSERV] Timeout - ending test\n");
			break;
		}
	}
out_with_socket:
	if (failures == 0) {
		printf("### UDP Server PASS: %d packets, no length mismatches ###\n", count);
	} else {
		printf("### UDP Server FAIL: %d packets, %lu length mismatches###\n", count, failures);
	}
	close(s);
	return failures;
}

void udp_client_thread(int num_packets, uint32_t sleep_time)
{
	int sockfd;
	int clilen;
	int i = 0;
	fd_set wfds;
	fd_set fds;
	int max_fd = 0;
	struct sockaddr_in serveraddr;
	size_t random_len;
	char *test_data;
	size_t send_len;

	clilen = sizeof(serveraddr);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("[UDPCLIENT]socket create error(%d)\n", errno);
		return;
	}
	printf("[UDPCLIENT] socket create(%d), start send\n", sockfd);

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = inet_addr(g_app_target_addr);
	serveraddr.sin_port = htons(g_app_target_port);

	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	max_fd = sockfd + 1;
	for (i = 1; i <= num_packets; i++) {
		random_len = generateRandomLength();
		test_data = generateRandomString(random_len);
		if (test_data == NULL){
			printf("ERROR in generating test data\n");
			close(sockfd);
			return;
		}
		send_len = HDR_SIZE + random_len;
		wfds = fds;
		int res = select(max_fd, NULL, &wfds, NULL, 0);
		if (res < 0) {
			printf("[UDPCLIENT] select error(%d)\n", errno);
			free(test_data);
			close(sockfd);
			return;
		}
		if (res > 0) {
			if (FD_ISSET(sockfd, &wfds)) {
				if (sendto(sockfd, (void *)test_data, send_len, 0, (struct sockaddr *)&serveraddr, clilen) < 0) {
					printf("[UDPCLIENT] sendto failed\n");
				};
				// printf("[UDPCLIENT] -->send msg # %d nbytes (%d) \n", i, send_len);
				free(test_data);
			} else {
				free(test_data);
				assert(0);
			}
		}
		if (sleep_time != 0) {
			usleep(sleep_time);
		}
	}
	if (num_packets == 0){
		while(1){
			random_len = generateRandomLength();
			test_data = generateRandomString(random_len);
			if (test_data == NULL){
				printf("ERROR in generating test data\n");
				close(sockfd);
				return;
			}
			send_len = HDR_SIZE + random_len;
			wfds = fds;
			int res = select(max_fd, NULL, &wfds, NULL, 0);
			if (res < 0) {
				printf("[UDPCLIENT] select error(%d)\n", errno);
				free(test_data);
				close(sockfd);
				return;
			}
			if (res > 0) {
				if (FD_ISSET(sockfd, &wfds)) {
					if (sendto(sockfd, (void *)test_data, send_len, 0, (struct sockaddr *)&serveraddr, clilen) < 0) {
						printf("[UDPCLIENT] sendto failed, errno %d\n",errno);
					};
					// printf("[UDPCLIENT] -->send msg %d bytes\n", send_len);
					free(test_data);
				} else {
					free(test_data);
					assert(0);
				}
			}
			if (sleep_time != 0) {
				usleep(sleep_time);
			}
		}
	}
	printf("[UDPCLIENT] Terminating udpclient after sending sufficient messages (%d)\n", num_packets);
	close(sockfd);
	return;
}

void tcp_server_thread(int num_packets)
{
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;
	int count = 0;
	int listenfd = -1;
	int connfd = -1;
	socklen_t clilen;
	int ret = 0;
	int recv_len = 0;
	char msg[BUF_SIZE];
	unsigned int tcp_server_fail = 0;

	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	if (listenfd < 0) {
		printf("[TCPSERV] TCP socket failure %d\n", errno);
		return;
	}

	/*
	* Enable SO_REUSEADDR to allow multiple instances of this
	* application to receive copies of the multicast datagrams.
	*/

	int reuse = 1;
	ret = setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse));
	if (ret < 0) {
		printf("[TCPSERV] ERR: setting SO_REUSEADDR\n");
		goto out_with_socket;
	}
	printf("[TCPSERV] set reusable success\n");

	/* Connect the socket to the server */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = PF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = HTONS(g_app_target_port);

	ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret < 0) {
		perror("[TCPSERV] bind fail\n");
		goto out_with_socket;
	}

	printf("[TCPSERV] Listening... port %d\n", g_app_target_port);

	ret = listen(listenfd, 1024);
	if (ret < 0) {
		perror("[TCPSERV] listen fail\n");
		goto out_with_socket;
	}

	clilen = sizeof(cliaddr);

	connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
	if (connfd < 0) {
		perror("[TCPSERV] accept fail\n");
		goto out_with_socket;
	}
	printf("[TCPSERV] Accepted\n");

	recv_len = sizeof(msg);
	while (1) {
		uint32_t claimed_len;

		/* Step 1: Read 4-byte header */
		if (recv_all(connfd, (uint8_t *)&claimed_len, HDR_SIZE) != 0) {
			printf("[TCPSERV] connection closed\n");
			break;
		}

		if ((claimed_len == 0)||(claimed_len > CLIENT_MAX_PAYLOAD)) {
			tcp_server_fail++;
			printf("[TCPSERV] ###FAIL: corrupted header len=%lu\n",claimed_len);
			break;
		}

		/* Step 2: Read claimed_len bytes of payload */
		if (recv_all(connfd, (uint8_t *)msg, claimed_len) != 0) {
			printf("[TCPSERV] connection closed mid-payload\n");
			break;
		}

		count++;
		// printf("[TCPSERV] #%d: TCP recv=%lu\n",count, claimed_len+HDR_SIZE);

		if (num_packets == 0) {
			continue;
		}
		if (count >= num_packets) {
			printf("Exiting tcp_server as received sufficient packets for testing\n");
			break;
		}
	}
	if (connfd > 0) {
		close(connfd);
		printf("[TCPSERV] Closed connfd successfully \n");
	}
	printf("[TCPSERV] Closed listenfd successfully \n");
out_with_socket:
	close(listenfd);
	if (!tcp_server_fail){
		printf("### TCP Server PASS: Received %d packets ###\n",count);
	} else {
		printf("### TCP Server FAIL: Received %d packets ###\n",count);
	}
	return;
}

void tcp_client_thread(int num_packets, uint32_t sleep_time)
{
	struct sockaddr_in myaddr;
	int sockfd;
	int ret = 0;
	int i = 0;
	uint32_t send_try = 0;
	socklen_t addrlen;
	size_t random_len;
	char *test_data;
	uint32_t errno_cnt = 0;
	uint32_t send_cnt = 0;
	uint32_t short_send_err = 0;

	sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("[TCPCLIENT] TCP socket failure %d\n", errno);
		return;
	}

	/* Connect the socket to the server */
	myaddr.sin_family = AF_INET;
	myaddr.sin_port = HTONS(g_app_target_port);
	inet_pton(AF_INET, g_app_target_addr, &(myaddr.sin_addr));

	addrlen = sizeof(struct sockaddr_in);

	printf("[TCPCLIENT] Connecting...\n");
	if (connect(sockfd, (struct sockaddr *)&myaddr, addrlen) < 0) {
		printf("[TCPCLIENT] connect fail: %d\n", errno);
		goto out_with_socket;
	}
	printf("[TCPCLIENT] Connected\n");

	/* Then send num_packets number of messages */

	for (i = 1; i <= num_packets; i++) {
		random_len = generateRandomLength();
		test_data = generateRandomString(random_len);
		if (test_data == NULL){
			printf("ERROR in generating test data\n");
			goto out_with_socket;
		}
		size_t send_len = HDR_SIZE + random_len;
		// printf("[TCPCLIENT] -->send msg # %d total=%zu\n",i, send_len);

		ret = send(sockfd, test_data, send_len, 0);

		if (ret <= 0) {

			if (ret == 0) {
				printf("[TCPCLIENT] connection closed\n");
				free(test_data);
				break;
			}

			if (errno == EWOULDBLOCK) {
				errno_cnt++;
				if (send_try++ > 100) {
					printf("[TCPCLIENT] Sending try is more than LIMIT(%d)\n", 100);
					free(test_data);
					break;
				}
				/* we don't take care this sleep time in interval */
				printf("[TCPCLIENT] wouldblock, retry delay 200ms!!\n");
				usleep(200000);
				continue;
			}

			printf("[TCPCLIENT] socket error ret(%d) err(%d) EAGAIN(%d)\n", ret, errno, EWOULDBLOCK);
			free(test_data);
			break;
		}
		send_cnt++;
		send_try = 0;
		free(test_data);
		if (sleep_time != 0) {
			usleep(sleep_time);
		}
	}
	if (num_packets == 0){
		while(1){
			random_len = generateRandomLength();
			test_data = generateRandomString(random_len);
			if (test_data == NULL){
				printf("ERROR in generating test data\n");
				goto out_with_socket;
			}
			size_t send_len = HDR_SIZE + random_len;
			// printf("[TCPCLIENT] -->send msg # %d total=%zu\n",send_cnt, send_len);

			ret = send(sockfd, test_data, send_len, 0);

			if (ret <= 0) {

				if (ret == 0) {
					printf("[TCPCLIENT] connection closed\n");
					free(test_data);
					break;
				}

				if (errno == EWOULDBLOCK) {
					errno_cnt++;
					if (send_try++ > 100) {
						printf("[TCPCLIENT] Sending try is more than LIMIT(%d)\n", 100);
						free(test_data);
						break;
					}
					/* we don't take care this sleep time in interval */
					printf("[TCPCLIENT] wouldblock, retry delay 200ms!!\n");
					usleep(200000);
					continue;
				}

				printf("[TCPCLIENT] socket error ret(%d) err(%d) EAGAIN(%d)\n", ret, errno, EWOULDBLOCK);
				free(test_data);
				break;
			}
			send_cnt++; // Track sent packets
			send_try = 0;
			if (ret < (int)send_len) {
				printf("[TCPCLIENT] short write: sent %d of %zu bytes — heap critical!\n",ret, send_len);
				short_send_err++; // Record send error
			}
			free(test_data);
			if (sleep_time != 0) {
				usleep(sleep_time);
			}
		}
	}
	printf("[TCPCLIENT] Terminating tcpclient after sending sufficient messages (%d)\n", num_packets);
out_with_socket:
	if ((errno_cnt) || (short_send_err)) {
		printf("### TCP Client FAIL: Sent %d packets, errno cnt %d, short send err %d ###\n",send_cnt, errno_cnt, short_send_err);
	} else {
		printf("### TCP Client PASS: Sent %d packets, errno cnt %d, short send err %d ###\n",send_cnt, errno_cnt, short_send_err);
	}
	close(sockfd);
	return;
}

/* Sample App to test Transport Layer (TCP / UDP) / IP Multicast Functionality */
#ifdef CONFIG_BUILD_KERNEL
int main(int argc, FAR char *argv[])
#else
int nettest_main(int argc, char *argv[])
#endif
{
	int mode = 0;
	nettest_proto_e proto = NT_NONE;
	int num_packets_to_process = 0;
	uint32_t interval = 0;
	/* pps - packet per second, default value 1 */
	uint32_t pps = 1;

	if (argc < 5) {
		goto err_with_input;
	}

	mode = atoi(argv[1]);
	if (mode != NETTEST_SERVER_MODE && mode != NETTEST_CLIENT_MODE && mode != NETTEST_INTERNAL_MODE) {
		goto err_with_input;
	}

	if (strlen(argv[2]) != strlen(NETTEST_PROTO_TCP)) {
		goto err_with_input;
	}
	if (!strncmp(argv[2], NETTEST_PROTO_TCP, strlen(NETTEST_PROTO_TCP) + 1)) {
		proto = NT_TCP;
	} else if (!strncmp(argv[2], NETTEST_PROTO_UDP, strlen(NETTEST_PROTO_UDP) + 1)) {
		proto = NT_UDP;
	} else if (!strncmp(argv[2], NETTEST_PROTO_BROADCAST, strlen(NETTEST_PROTO_BROADCAST) + 1)) {
		proto = NT_BROADCAST;
	} else if (!strncmp(argv[2], NETTEST_PROTO_MULTICAST, strlen(NETTEST_PROTO_MULTICAST) + 1)) {
		proto = NT_MULTICAST;
	} else if (!strncmp(argv[2], NETTEST_PROTO_STRESS, strlen(NETTEST_PROTO_STRESS) + 1)) {
		proto = NT_STRESS;
	} else {
		goto err_with_input;
	}

	g_app_target_addr = argv[3];
	g_app_target_port = atoi(argv[4]);

	num_packets_to_process = atoi(argv[5]);
	if (num_packets_to_process < 0 || num_packets_to_process > NETTEST_MAX_PACKETS) {
		goto err_with_input;
	}

	if (mode == NETTEST_SERVER_MODE) {
		if (proto == NT_TCP) {
			tcp_server_thread(num_packets_to_process);
		} else if (proto == NT_UDP) {
			udp_server_thread(num_packets_to_process);
		} else if (proto == NT_BROADCAST) {
			broadcast_receive(num_packets_to_process);
		} else if (proto == NT_MULTICAST) {
			if (argc != 7) {
				goto err_with_input;
			}
			ipmcast_receiver_thread(num_packets_to_process, argv[6]);
		}
	} else if (mode == NETTEST_CLIENT_MODE) {
		if (argc < 7) {
			goto err_with_input;
		}
		/* get interval */
		pps = atoi(argv[6]);
		if (pps <= 0) {
			goto err_with_input;
		}
		interval = 1000000ul / pps; /* sleep interval */

		if (proto == NT_TCP) {
			tcp_client_thread(num_packets_to_process, interval);
		} else if (proto == NT_UDP) {
			udp_client_thread(num_packets_to_process, interval);
		} else if (proto == NT_BROADCAST) {
			broadcast_send(num_packets_to_process, interval);
		} else if (proto == NT_MULTICAST) {
			if (argc != 8) {
				goto err_with_input;
			}
			ipmcast_sender_thread(num_packets_to_process, interval, argv[7]);
		}
	}
	printf("Exiting nettest_main thread, job finished\n");
	return 0;

err_with_input:
	show_usage();
	return -1;
}
