/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * Original version from Austin Martin:
 * https://gist.github.com/austinmarton/2862515
 *
 * Adapted by Paul Craven
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <unistd.h>

/* Replace this with this computer's MAC address
   In this case, the MAC address is b8:27:eb:44:08:62.
   You need to update this.
*/
#define DEST_MAC0 0x54
#define DEST_MAC1 0xE1
#define DEST_MAC2 0xAD
#define DEST_MAC3 0x74
#define DEST_MAC4 0x33
#define DEST_MAC5 0xFD

#define ETHER_TYPE  0x0800

/* Change this to your interface name.
   Such as "wlan0" or "eth0".
   This program seems to ignore this parameter, and
   listens to everything.
   */

#define DEFAULT_IF  "enp0s31f6"
#define BUF_SIZ     1024

int main(int argc, char *argv[])
{
    int sockfd, i;
    int sockopt;
    ssize_t numbytes;
    struct ifreq ifopts;    /* set promiscuous mode */
    uint8_t buf[BUF_SIZ];
    char ifName[IFNAMSIZ];

    /* --- Get ready to listen --- */

    /* Copy the interface name to a character buffer */
    strcpy(ifName, DEFAULT_IF);

    /* Open PF_PACKET socket, listening for EtherType ETHER_TYPE */
    if ((sockfd = socket(PF_PACKET, SOCK_RAW, htons(ETHER_TYPE))) == -1) {
        perror("listener: socket");
        return -1;
    }

    /* Set interface to promiscuous mode - do we need to do this every time? */
    strncpy(ifopts.ifr_name, ifName, IFNAMSIZ-1);
    ioctl(sockfd, SIOCGIFFLAGS, &ifopts);
    ifopts.ifr_flags |= IFF_PROMISC;
    ioctl(sockfd, SIOCSIFFLAGS, &ifopts);

    /* Allow the socket to be reused - in case connection is closed prematurely */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
        perror("setsockopt");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    /* Bind to device */
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifName, IFNAMSIZ-1) == -1)  {
        perror("SO_BINDTODEVICE");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* --- Main loop to listen, and print a packet --- */

    // Loop forever
    while(1) {

        /* Receive the data */
        printf("Listener: Waiting to receive...\n");
        numbytes = recvfrom(sockfd, buf, BUF_SIZ, 0, NULL, NULL);
        printf("Listener: got packet %lu bytes\n", numbytes);

        /* Check the packet is for me
           by looking at the destination address. */
        if (buf[0] == DEST_MAC0 &&
            buf[1] == DEST_MAC1 &&
            buf[2] == DEST_MAC2 &&
            buf[3] == DEST_MAC3 &&
            buf[4] == DEST_MAC4 &&
            buf[5] == DEST_MAC5) {

            printf("Correct destination MAC address\n");

            if(buf[14] == 0x45) {
                /* This is likely just an SSH packet from a
                   remove SSH terminal. Let's ignore those
                   and not do anything here.
                */

            } else {
                /* We've got data!
                   Print the packet */
                printf("Data:");
                for (i=0; i<numbytes; i++)
                    printf("%02x:", buf[i]);
                printf("\n");
            }

        } else {
            /* Ok, this data was not intended for us.
                   Which means we plugged in the wrong MAC
               Or it was sent to ff:ff:ff:ff:ff:ff which is
                   broadcast for 'everyone'
                   */

            printf("Wrong destination MAC: %x:%x:%x:%x:%x:%x\n",
                            buf[0],
                            buf[1],
                            buf[2],
                            buf[3],
                            buf[4],
                            buf[5]);
        }

    }
    close(sockfd);
    return 0;
}
