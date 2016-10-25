/*
* ISA - DHCP server
* @author Marian Durco (xdurco00)
*/
#ifndef IPPOOL
#define IPPOOL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>

#include "main.h"
#include "dserver.h"

#define FREE 1
#define BINDING 2 
#define TAKEN 3
#define SERVER 4

unsigned int createBitmask(const char *bitmask);
int split (char *str, char c, char ***arr);
void initPool(IpPool *);
void deletePool(IpPool *);
int addIpToPool(IpPool *,uint32_t);
int avaiableIpInPool(IpPool *);
int deleteIpFromPool(IpPool *,uint32_t);
int initDhcpPool(IpPool *,arguments);
void printIpInPool(IpPool *);
int setFirstAvailableIp(IpPool *);
int getNewIp(IpPool *,uint32_t *);
int setFirstAvailableIp(IpPool *);
Ip searchIp(IpPool *,uint32_t );
Ip searchIpByMac(IpPool *,uint8_t*);
int setServerIP(IpPool *l,uint32_t *);
int getIpForOffer(IpPool *,uint32_t *, uint8_t *);
int checkRequestIpFromClient(dhcp_msg* ,optList *,IpPool *,uint32_t *);
int checkServerId(optList *,uint32_t );
int setForAck(IpPool *,uint32_t);
bool isValidIpAddress(char *);
bool isValidMask(char *);
int isNumeric (const char *);
int releaseAddress(IpPool *, uint8_t* );
void checkIntegrityOfPool(IpPool *);

#endif
