#ifndef MAINH
#define MAINH

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
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

typedef struct 
{ 
	uint32_t network_addr;
	uint32_t broadcast_addr;
	uint32_t network_mask;
	uint32_t excluded_addr[128];
	uint8_t ex_len;
}arguments;

typedef struct dhcp_option *dhcpOption;
struct dhcp_option
{
	uint8_t id;
	uint8_t len;
	uint8_t data[128];
	dhcpOption next;
	dhcpOption prev;
};

typedef struct
{
	dhcpOption head_option;
	dhcpOption tail_option;

}optList;

typedef struct IP *Ip;
struct IP
{
	uint32_t address;
	int status;
	u_int8_t mac[6];
	time_t active_until;
	Ip next_ip;
};

typedef struct 
{
	Ip tail_ip;
	Ip rdy_ip;
}IpPool;

typedef struct dhcp_msg
{
	u_int8_t	op;		/* packet opcode type */
	u_int8_t	htype;	/* hardware addr type */
	u_int8_t	hlen;	/* hardware addr length */
	u_int8_t	hops;	/* gateway hops */
	u_int32_t	xid;	/* transaction ID */
	u_int16_t	secs;	/* seconds since boot began */
	u_int16_t	flags;	/* flags: 0x8000 is broadcast */
	u_int32_t 	ciaddr;	/* client IP address */
    u_int32_t	yiaddr;	/* 'your' IP address */
    u_int32_t	siaddr;	/* server IP address */
    u_int32_t	giaddr;	/* gateway IP address */
	u_int8_t	chaddr[16];	/* client hardware address */
	u_int8_t	sname[64];	/* server host name */
	u_int8_t	file[128];	/* boot file name */
	u_int8_t	options[256];	/* option area */
}dhcp_msg;

#endif
