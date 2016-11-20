/*
* ISA - DHCP server
* @author Marian Durco (xdurco00)
*/
#include "ippool.h"

/*
 *@function check if ip address is valid
 *@st checking ip
 */
bool isValidIpAddress(char *st)
{
	char *string = malloc(strlen(st));
	if(string==NULL)
		return ERRMEM;
	memcpy(string,st,strlen(st));
    int num, i, len;
    char *ch;

    //counting number of quads present in a given IP address
    int quadsCnt=0;
    len = strlen(string);

    //  Check if the string is valid
    if(len<7 || len>15)
	{
		free(string);
		return false;
	}
    ch = strtok(string, ".");
    while (ch != NULL) 
    {
        quadsCnt++;
		if(!isNumeric(ch))
		{
			free(string);
			return false;
		}
        num = 0;
        i = 0;

        //  Get the current token and convert to an integer value
        while(ch[i]!='\0')
        {
            num = num*10;
            num = num+(ch[i]-'0');
            i++;
        }

        if(num<0 || num>255)
        {
			free(string);
            return false;
        }

        if( (quadsCnt == 1 && num == 0))
        {
			free(string);
            return false;
        }

        ch = strtok(NULL, ".");
    }

    //  Check the address string, should be n.n.n.n format
    if(quadsCnt!=4)
    {
		free(string);
        return false;
    }

    //  Looks like a valid IP address
	free(string);
    return true;
}


/*
 *@function check if mask is valid
 *@mask checking mask 
 */
bool isValidMask(char *mask)
{
	if(!isNumeric(mask))
		return false;
	int imask = atoi(mask);
	if(imask < 1 || imask > 30)
		return false;
	return true;
}

/*
 *@function check if string is number
 *@s checking string
 */
int isNumeric (const char * s)
{
    if (s == NULL || *s == '\0' || isspace(*s))
      return 0;
    char * p;
    strtod (s, &p);
    return *p == '\0';
}

/*
 *@function create mask of network from CIDR
 *@bitmask mask in CIRD notation
 */
unsigned int createBitmask(const char *bitmask)
{
	unsigned int times = (unsigned int)atol(bitmask)-1, i, bitmaskAsUInt = 1;
	/* Fill in set bits (1) from the right. */
	for(i=0; i<times; ++i)
	{
		bitmaskAsUInt <<= 1;
		bitmaskAsUInt |= 1;
	}
	/* Shift in unset bits from the right. */
	for(i=0; i<32-times-1; ++i)
		bitmaskAsUInt <<= 1;
	return bitmaskAsUInt;
}

/*
 *@function for split string based on separator
 *@str string for split
 *@c separator
 *@arr result of spliting
 */
int split (char *str, char c, char ***arr)
{
    int count = 1;
    int token_len = 1;
    int i = 0;
    char *p;
    char *t;

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
            count++;
        p++;
    }

    *arr = (char**) malloc(sizeof(char*) * count);
    if (*arr == NULL)
		return -1;

    p = str;
    while (*p != '\0')
    {
        if (*p == c)
        {
            (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
            if ((*arr)[i] == NULL)
				return -1;
            token_len = 0;
            i++;
        }
        p++;
        token_len++;
    }
    (*arr)[i] = (char*) malloc( sizeof(char) * token_len );
    if ((*arr)[i] == NULL)
		return -1;

    i = 0;
    p = str;
    t = ((*arr)[i]);
    while (*p != '\0')
    {
        if (*p != c && *p != '\0')
        {
            *t = *p;
            t++;
        }
        else
        {
            *t = '\0';
            i++;
            t = ((*arr)[i]);
        }
        p++;
    }

    return count;
}

void initPool(IpPool *ip_pool)
{
	ip_pool->tail_ip = NULL;
	ip_pool->rdy_ip = NULL;
}

/*
 *@function remove ip pool from memory
 *@ip_pool removed ip_pool
 */
void deletePool(IpPool *ip_pool)
{
	while(ip_pool->tail_ip!=NULL)
	{
		ip_pool->rdy_ip = ip_pool->tail_ip;
		ip_pool->tail_ip = ip_pool->tail_ip->next_ip;
		free(ip_pool->rdy_ip);
	}
	ip_pool->rdy_ip=NULL;
}

/*
 *@function add ip to pool
 */
int addIpToPool(IpPool *ip_pool,uint32_t ip_address)
{
	Ip tmp_Ip = malloc(sizeof(struct IP));
	if(tmp_Ip!=NULL)
	{
		tmp_Ip->address = ntohl(ip_address);
		tmp_Ip->status = FREE;
		memset(tmp_Ip->mac,0,6*sizeof(tmp_Ip->mac[0]));
		tmp_Ip->active_until=0;
		tmp_Ip->next_ip = NULL;
		if(ip_pool->tail_ip==NULL) // list is empty
			ip_pool->tail_ip = tmp_Ip;
		else
		{
			tmp_Ip->next_ip = ip_pool->tail_ip;
			ip_pool->tail_ip = tmp_Ip;
		}
	}
	else 
		return ERRMEM;
	ip_pool->rdy_ip = ip_pool->tail_ip;
	return OK;
} 

/*
 *@function delete ip from pool
 */
int deleteIpFromPool(IpPool *ip_pool,uint32_t ip_address) 
{
	Ip tmp_pointer = ip_pool->tail_ip;
	Ip tmp_tail = ip_pool->tail_ip;
	while(tmp_pointer!=NULL)
	{
		if(ntohl(ip_address)==tmp_pointer->address)
		{
			if(tmp_pointer==ip_pool->tail_ip)
			{
				ip_pool->tail_ip = tmp_pointer->next_ip;
				free(tmp_pointer);
			}
			else
			{
				tmp_tail->next_ip = tmp_pointer->next_ip;
				free(tmp_pointer);
			}
			return OK;
		}
		tmp_tail = tmp_pointer;
		tmp_pointer = tmp_pointer->next_ip;
	}
	return ERRIP; // ip not found
}

/*
 *@function get number of ip addresses in pool
 */
int avaiableIpInPool(IpPool *ip_pool)
{
	Ip tmp_pointer = ip_pool->tail_ip;
	int count = 0;
	while(tmp_pointer!=NULL)
	{
		if(tmp_pointer->status==FREE)
			count++;
		tmp_pointer = tmp_pointer->next_ip;
	}
	return count;
} 

/*
 *@function dhcp pool base of program arguments
 */
int initDhcpPool(IpPool *ip_pool,arguments arg)
{
	initPool(ip_pool);
	for(uint32_t i=arg.broadcast_addr;i>=arg.network_addr;i--)
	{
		if(addIpToPool(ip_pool,i)!=OK)
			return ERRMEM;
	}
	deleteIpFromPool(ip_pool,arg.network_addr);
	deleteIpFromPool(ip_pool,arg.broadcast_addr);

	for(uint8_t i=0;i<arg.ex_len;i++)
	{
		if(deleteIpFromPool(ip_pool,arg.excluded_addr[i])!=OK)
			reportErr("Cannot find excluded IP in IP range");
	}
	return OK;
}

void printIpInPool(IpPool *ip_pool)
{
	Ip tmp_pointer = ip_pool->tail_ip;
	while(tmp_pointer!=NULL)
	{
		char ip[32];
		uint32_t tmp_address = tmp_pointer->address;
		inet_ntop(AF_INET,&tmp_address,ip,32);
		printf("PoolIP: %s\n",ip);
		tmp_pointer = tmp_pointer->next_ip;
	}
}


/*
*@function give you first free address from pool
*@ip_ret - the pointer on free ip address
*/
int getNewIp(IpPool *ip_pool,uint32_t *ip_ret)
{
	if(setFirstAvailableIp(ip_pool)!=OK)
		return ERRIP; // no avaiable IP
	*(ip_ret) = ip_pool->rdy_ip->address;
	return OK;
}

/*
 *@function find first free ip from pool
 */
int setFirstAvailableIp(IpPool *ip_pool)
{
	Ip tmp_pointer = ip_pool->tail_ip;
	while(tmp_pointer!=NULL)
	{
		if(tmp_pointer->status == FREE)
		{
			ip_pool->rdy_ip = tmp_pointer;
			return OK;
		}
		tmp_pointer = tmp_pointer->next_ip;
	}
	return ERRIP;
}

/*
 *@function set the first ip from pool to serever
 */
int setServerIP(IpPool *ip_pool,uint32_t *address)
{
	uint32_t ip;
	if(getNewIp(ip_pool,&ip) == ERRIP)
	{
		reportErr("error in giving server IP get ip");
		return ERRIP;
	}
	Ip foundIp;
	if((foundIp = searchIp(ip_pool,ip))==NULL)
	{
		reportErr("error in giving server IP");
		return ERRIP;
	}
	foundIp->status = SERVER;
	*(address) = foundIp->address;
	return OK;
}

/*
 *@function search for ip in pool
 */
Ip searchIp(IpPool *ip_pool,uint32_t address)
{
	Ip tmp_pointer = ip_pool->tail_ip;
	while(tmp_pointer!=NULL)
	{
		if(tmp_pointer->address==address)
			return tmp_pointer;
		tmp_pointer = tmp_pointer->next_ip;
	}
	return NULL;
}

/*
 *@function search for record in pool based of mac address
 */
Ip searchIpByMac(IpPool *ip_pool,uint8_t* mac_address)
{
	Ip tmp_pointer = ip_pool->tail_ip;
	while(tmp_pointer!=NULL)
	{
		int i = 0;
		for(i=0;i<6;i++)
		{
			if(tmp_pointer->mac[i]!=mac_address[i])
				break;
		}
		if(i>5)
			return tmp_pointer;
		tmp_pointer = tmp_pointer->next_ip;
	}
	return NULL;
}

/*
 *@function set ip in pool in binding status
 */
int getIpForOffer(IpPool *ip_pool,uint32_t *address, uint8_t *mac_address)
{
	if(getNewIp(ip_pool,address)!=OK)
		return ERRIP;
	ip_pool->rdy_ip->status=BINDING;
	ip_pool->rdy_ip->active_until = time(NULL)+LEASEOFFER;
	memcpy(&ip_pool->rdy_ip->mac,mac_address,6);
	return OK;
}

/*
 *@function contol if ip requested from client is correct from same client and in binding status
 */
int checkRequestIpFromClient(dhcp_msg* dhcp_request,optList *replyOptions,IpPool *ip_pool,uint32_t *requestedIp)
{
	dhcpOption ipOpt = searchOption(replyOptions,REQUESTED_IP_ADDRESS);
	if (ipOpt==NULL)
	{
		reportErr("Haven't found REQUESTED IP in request message");
		return ERRIP;
	}
	uint32_t ip_from_option;
	memcpy(&ip_from_option,&ipOpt->data,ipOpt->len);
	*(requestedIp)=ip_from_option;
	Ip requested_ip = searchIp(ip_pool,ip_from_option);
	if(requested_ip==NULL)
	{
		reportErr("Haven't found REQUESTED IP in dhcp pool");
		return ERRIP;
	}

	if (requested_ip->status != BINDING)
		return ERRIP;

	for(int i=0;i<6;i++)
	{
		if(requested_ip->mac[i]!=dhcp_request->chaddr[i])
		{
			reportErr("MAC address in request is not equeal with one in dhcp pool");
			return ERRIP;
		}
	}
	return OK;
}

/*
 *@function chceck server identifier from dhcp options
 */
int checkServerId(optList *replyOptions,uint32_t serverIP)
{
	dhcpOption server_id = searchOption(replyOptions,SERVER_IDENTIFIER);
	if (server_id==NULL)
	{
		reportErr("Haven't found SERVER ID in request message");
		return ERRIP;
	}
	uint32_t serverid;
	memcpy(&serverid,&server_id->data,server_id->len);
	if(serverIP!=serverid)
	{
		reportErr("Server ID from request is not equeal with this server ID");
		return ERRIP;
	}
	return OK;
}

/*
 *@function chceck requested IP from dhcp options
 */
int checkRequestedIp(optList *replyOptions)
{
	dhcpOption requested_ip = searchOption(replyOptions,REQUESTED_IP_ADDRESS);
	if (requested_ip==NULL)
	{
		return ERRIP;
	}
	return OK;
}

/*
 *@function set ip status in taken status
 */
int setForAck(IpPool *ip_pool,uint32_t requested_ip)
{
	Ip requestedIp = searchIp(ip_pool,requested_ip);
	requestedIp->status = TAKEN;
	requestedIp->active_until=time(NULL)+LEASETIMER;
	return OK;
}

/*
 *@function set ip status in free status
 */
int releaseAddress(IpPool *ip_pool, uint8_t* releasing_mac)
{
	Ip found = searchIpByMac(ip_pool,releasing_mac);
	if(found==NULL)
	{
		reportErr("Haven't found MAC for release");
		return ERRIP;
	}
	found->status = FREE;
	memset(found->mac,0,6*sizeof(found->mac[0]));
	found->active_until=0;
	return OK;
}

/*
 *@function contol or alter the ip pool base on ip lease time
 */
void checkIntegrityOfPool(IpPool *ip_pool)
{
	Ip tmp_pointer = ip_pool->tail_ip;
	while(tmp_pointer!=NULL)
	{
		switch(tmp_pointer->status)
		{
			double diff;
			case TAKEN:
			case BINDING:
				if((diff = difftime(tmp_pointer->active_until,time(NULL)))<=0)
				{
					char ip[32];
					inet_ntop(AF_INET,&tmp_pointer->address,ip,32);
					printf("integrita porusena pre IP: %s",ip);
					tmp_pointer->status = FREE;
					memset(tmp_pointer->mac,0,6*sizeof(tmp_pointer->mac[0]));
					tmp_pointer->active_until=0;
				}
				break;
			case FREE: 
			case SERVER:
				break;
		}
		tmp_pointer = tmp_pointer->next_ip;
	}
}
