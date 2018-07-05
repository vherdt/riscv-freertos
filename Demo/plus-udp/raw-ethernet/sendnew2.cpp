/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

#define MY_DEST_MAC0	0x54
#define MY_DEST_MAC1	0xe1
#define MY_DEST_MAC2	0xad
#define MY_DEST_MAC3	0x74
#define MY_DEST_MAC4	0x33
#define MY_DEST_MAC5	0xfd

#define DEFAULT_IF	"vpeth1"     // "vpeth1"
#define BUF_SIZ		1024



#define FREERTOS_LITTLE_ENDIAN 1
#define ipconfigBYTE_ORDER FREERTOS_LITTLE_ENDIAN

static uint16_t prvGenerateChecksum( const uint8_t * const pucNextData, const uint16_t usDataLengthBytes )
{
uint32_t ulChecksum = 0;
uint16_t us, usDataLength16BitWords, *pusNextData, usReturn;

	/* There are half as many 16 bit words than bytes. */
	usDataLength16BitWords = ( usDataLengthBytes >> 1U );

	pusNextData = ( uint16_t * ) pucNextData;

	for( us = 0U; us < usDataLength16BitWords; us++ )
	{
		ulChecksum += ( uint32_t ) pusNextData[ us ];
	}

	if( ( usDataLengthBytes & 0x01U ) != 0x00 )
	{
		/* There is one byte left over. */
		#if ipconfigBYTE_ORDER == FREERTOS_LITTLE_ENDIAN
		{
			ulChecksum += ( uint32_t ) pucNextData[ usDataLengthBytes - 1 ];
		}
		#else
		{
			us = ( uint16_t ) pucNextData[ usDataLengthBytes - 1 ];
			ulChecksum += ( uint32_t ) ( us << 8 );
		}
		#endif
	}

	while( ( ulChecksum >> 16UL ) != 0x00UL )
	{
		ulChecksum = ( ulChecksum & 0xffffUL ) + ( ulChecksum >> 16UL );
	}

	usReturn = ~( ( uint16_t ) ulChecksum );

	return usReturn;
}



int main(int argc, char *argv[])
{
/*
    uint8_t ip_hdr_data[] = {0x45, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x80, 0x11, 0x00, 0x00, 0xC0, 0xA8, 0x00, 0xBC, 0x7F, 0x00, 0x00, 0x01};
    uint16_t ip_hdr_checksum = prvGenerateChecksum(ip_hdr_data, sizeof(ip_hdr_data));
    printf("IP header checksum = %u\n", ip_hdr_checksum);
    exit(0);
    */

	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZ];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	struct sockaddr_ll socket_address;
	memset(&socket_address, 0, sizeof(struct sockaddr_ll));
	char ifName[IFNAMSIZ];
	
	/* Get interface name */
	if (argc > 1)
		strcpy(ifName, argv[1]);
	else
		strcpy(ifName, DEFAULT_IF);

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
	    perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0)
	    perror("SIOCGIFINDEX");
	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	    perror("SIOCGIFHWADDR");

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZ);
	/*
	// Ethernet header
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = MY_DEST_MAC0;
	eh->ether_dhost[1] = MY_DEST_MAC1;
	eh->ether_dhost[2] = MY_DEST_MAC2;
	eh->ether_dhost[3] = MY_DEST_MAC3;
	eh->ether_dhost[4] = MY_DEST_MAC4;
	eh->ether_dhost[5] = MY_DEST_MAC5;
	// Ethertype field
	eh->ether_type = htons(ETH_P_IP);
	tx_len += sizeof(struct ether_header);
	*/
	
	/*
	54 E1 AD 74 33 FD 
	72 5C B4 2C AC 4A 
	08 00 
	45 00 00 3C 00 00 00 00 80 11 B8 4A C0 A8 00 BC C0 A8 00 5A C0 00 27 10 00 28 0A 8E 53 74 61 6E 64 61 72 64 20 73 65 6E 64 20 6D 65 73 73 61 67 65 20 6E 75 6D 62 65 72 20 39 0D 0A
	*/
	
	//C0 A8 00 5A   // 192.168.0.90         -> checksum: B8 4A
	//86 66 da fb   // 134.102.218.251      -> checksum: 17 EB
	//7F 00 00 01   // 127.0.0.1            -> checksum: FA 4B
	//C0 A8 00 BC   // 192.168.0.188
	
	//45 00 00 20 b5 ea 40 00 40 11 86 e0 7f 00 00 01 7f 00 00 01 9d 55 27 10 00 0c fe 1f 74 65 73 74
	
	uint8_t data[46] = { 
	// ETHERNET HEADER
	0x54, 0xE1, 0xAD, 0x74, 0x33, 0xFD, 0x72, 0x5C, 0xB4, 0x2C, 0xAC, 0x4A, 0x08, 0x00, 
	
	// IP HEADER, UDP HEADER and PAYLOAD
	0x45, 0x00, 0x00, 0x20, 0xb5, 0xea, 0x40, 0x00, 0x40, 0x11, 0x86, 0xe0, 0x7f, 0x00, 0x00, 0x01, 0x7f, 0x00, 0x00, 0x01, 0x9d, 0x55, 0x27, 0x10, 0x00, 0x0c, 0xfe, 0x1f, 0x74, 0x65, 0x73, 0x74
	};
	
	
	memcpy(sendbuf, data, 46);
	tx_len = 46;

	/* Index of the network device */
	socket_address.sll_ifindex = if_idx.ifr_ifindex;
	/* Address length*/
	socket_address.sll_halen = ETH_ALEN;
	socket_address.sll_family = AF_PACKET;
	socket_address.sll_protocol = ntohs(0x0800);
	/* Destination MAC */
	socket_address.sll_addr[0] = MY_DEST_MAC0;
	socket_address.sll_addr[1] = MY_DEST_MAC1;
	socket_address.sll_addr[2] = MY_DEST_MAC2;
	socket_address.sll_addr[3] = MY_DEST_MAC3;
	socket_address.sll_addr[4] = MY_DEST_MAC4;
	socket_address.sll_addr[5] = MY_DEST_MAC5;

    for (int i=0; i<5; ++i) {
	    // Send packet
	    usleep(1000);
	    printf("Sending packet ...\n");
	    ssize_t ans = sendto(sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll));
	    //ssize_t ans = sendto(sockfd, sendbuf, tx_len, 0, 0, 0);
	    if (ans < 0)
	        printf("Send failed\n");
	   else
	        printf("Sending successful\n");
	}

	return 0;
}
