/*
* ISA - DHCP server
* @author Marian Durco (xdurco00)
*/

#ifndef DSERVERH
#define DSERVERH

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#include "main.h"
#include "dhcpopt.h"
#include "ippool.h"

#define	BOOTPS		67
#define	BOOTPC		68

/* DHCP Message types */
#define		DHCPDISCOVER	1
#define		DHCPOFFER	2
#define		DHCPREQUEST	3
#define		DHCPACK		5
#define		DHCPNAK		6
#define		DHCPRELEASE	7

#define REPLY 2

#define MINARGUMENT 3

#define SIZEOFMSG 500

#define LEASETIMER 3600
#define LEASEOFFER 60


enum err_codes
{
	ERRIP = -1,
	OK = 0,
	ERRARG,
	ERRCONNECT,
	ERRMEM,
	ERRQUEUE,
	ERROPT,
};

void reportErr(char*);
int setUpConnectivity(int *socket);
int communicate(int socket,IpPool *);
int processProgramParam(int, char**, arguments *);
int discoverHandler(dhcp_msg *,dhcp_msg *, optList *, IpPool *);
int requestHandler(dhcp_msg *,dhcp_msg *, optList *, IpPool *,int,uint32_t);
int releaseHandler(dhcp_msg *, IpPool *);
void prepReply(dhcp_msg *,dhcp_msg *);
int sendReply(int ,struct sockaddr_in *,dhcp_msg *,bool);
void dhcpOutput(IpPool *,uint32_t);
void signalHandler(int );
void *threadFunct();

#endif
