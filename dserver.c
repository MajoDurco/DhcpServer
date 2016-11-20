/*
* ISA - DHCP server
* @author Marian Durco (xdurco00)
*/

#include "dserver.h"

int parseOptions(dhcp_msg, optList*);

uint32_t BROADCAST;
uint32_t SERVER_IP,NET_MASK;
IpPool *CLEAN_POOL = NULL;

 /* @function - main */
int main(int argc, char **argv) 
{
	int socket;

	signal(SIGINT,signalHandler);

	arguments program_arg;
	if(processProgramParam(argc,argv,&program_arg)!=OK)
	{
		reportErr("Error in program arguments");
		return EXIT_FAILURE;
	}
	IpPool iplist;
	CLEAN_POOL = &iplist;
	if(initDhcpPool(&iplist,program_arg)!=OK)
	{
		reportErr("Error in initializin dhcp pool");
		return EXIT_FAILURE;
	}
	if(setServerIP(&iplist,&SERVER_IP)!=OK)
	{
		reportErr("Error in setting IP for server");
		deletePool(&iplist);
		return EXIT_FAILURE;
	}
	char server_ip[32];
	inet_ntop(AF_INET,&SERVER_IP,server_ip,32);
	char brod[32];
	inet_ntop(AF_INET,&BROADCAST,brod,32);

	if((setUpConnectivity(&socket)) != OK)
	{
		reportErr("setUpConnectivity Err, check if you run it as root");
		return EXIT_FAILURE;
	}
	if(communicate(socket,&iplist) != OK)
	{
		reportErr("communicate Errl!");
		return EXIT_FAILURE;
	}
	deletePool(&iplist);
	return EXIT_SUCCESS;
}

/*
 * @function for handling SIGINT
 * @signal - signal ID
 */
void signalHandler(int signal)
{
	if(CLEAN_POOL!=NULL)
		deletePool(CLEAN_POOL);
	exit(signal);
}

/*
 * @function print out the error message to stderr
 * @mesg - the message
 */
void reportErr(char *mesg)
{
	fprintf(stderr,"%s\n",mesg);
}


/* @function controls the program arugment and also fill the argument structure with data
 * @argc - num of program arguments
 * @argv - the program arguments
 * @*arg - pointer to arumnents structure
 */
int processProgramParam(int argc,char *argv[],arguments *arg) 
{
	memset(arg,0,sizeof(arguments));
	if (argc<MINARGUMENT)
	{
		reportErr("Not enough arguments!");
		return ERRARG;
	}
	if (strcmp(argv[1],"-p")!=0)
	{
		reportErr("Wrong program arguments!");
		return ERRARG;
	}
	else
	{
		char *ip,*bitmask;
		ip = strtok(argv[2], "/");
		bitmask = strtok(NULL, "\0");
		if(!ip || !bitmask)
		{
			reportErr("Wrong program arguments!");
			return ERRARG;
		}
		if(!isValidIpAddress(ip) || !isValidMask(bitmask))
		{
			reportErr("The Ip address of the pool or mask is wrong");
			return ERRARG;
		}

		uint32_t input_ip;
		inet_pton(AF_INET,ip,&input_ip);
		uint32_t bitmask_to_int = ntohl(createBitmask(bitmask));
		char input_ip_str[32];
		inet_ntop(AF_INET,&input_ip,input_ip_str,32);
		char bitmask_str[32];
		inet_ntop(AF_INET,&bitmask_to_int,bitmask_str,32);

		uint32_t network_int = (input_ip & bitmask_to_int);
		uint32_t broadcast_addr = (input_ip | ~bitmask_to_int);

		arg->network_addr=ntohl(network_int);
		arg->broadcast_addr=ntohl(broadcast_addr);
		arg->network_mask=ntohl(bitmask_to_int);
		NET_MASK=ntohl(bitmask_to_int);
		BROADCAST = broadcast_addr;
	} 
	if(argc>MINARGUMENT)
	{
		if ((strcmp(argv[3],"-e")!=0) || argc!=MINARGUMENT+2) // +2 -> -e and excluded addresses
		{
			reportErr("Wrong program arguments!");
			return ERRARG;
		}
		else
		{
			char **arr = NULL;
			int pieces = split(argv[4],',',&arr);
			if(pieces<0)
			{
				reportErr("Wrong excluded IP");
				return ERRIP;
			}

			arg->ex_len = pieces;
			int flag=OK;
			for (int i=0;i<pieces;i++)
			{
				uint32_t excluded_ip;
				if(!isValidIpAddress(arr[i]))
					flag=ERRIP;
				inet_pton(AF_INET,arr[i],&excluded_ip);
				arg->excluded_addr[i]=ntohl(excluded_ip);
				free(arr[i]);
			}
			free(arr);
			if(flag==ERRIP)
			{
				reportErr("Wrong excluded IP");
				return ERRIP;
			}
		}	
	}
	return OK;
}

/*
 * @function create socket and needed strctures for server 
 * @socket - the opened server socket
 */
int setUpConnectivity(int *socket_func)
{
	int server_socket;
	struct sockaddr_in server_address;
	struct protoent *proto;
	struct servent *name;

	if ((name = getservbyname("bootps","udp")) == 0)
	{
		reportErr("getservbyname Err!");
		return ERRCONNECT;
	}

	if ((proto = getprotobyname("udp")) == 0)
	{
		reportErr("getprotobyname Err!");
		return ERRCONNECT;
	}

	/* create socket */
	if ((server_socket = socket(AF_INET, SOCK_DGRAM, proto->p_proto)) <= 0)
		return ERRCONNECT;

	int enable = 1;
	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&enable , sizeof(int))!=OK)
	{
		reportErr("err in setting SO_REUSEADDR socket option");
		return ERRCONNECT;
	}
	enable = 1;
	if(setsockopt(server_socket, SOL_SOCKET, SO_BROADCAST, (const void *)&enable , sizeof(int))!=OK)
	{
		reportErr("err in setting SO_BROADCAST socket option");
		return ERRCONNECT;
	}

	bzero((char *) &server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);
	server_address.sin_port = name->s_port;

	if (bind(server_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) 
	return ERRCONNECT;
	
	printf("Listening on port: %d\n",ntohs(server_address.sin_port));

	*socket_func = server_socket;

	return OK;
}

 /* @function create connectible dhcp server, recive mesg from clients and sent them response
 * @socket the opened socket
 * @ip_pool pointer on ip pool
 */
int communicate(int server_socket,IpPool *ip_pool)
{
    while(1) 
	{   
		struct sockaddr_in client_socket;
		socklen_t client_socket_len=sizeof(client_socket);
		size_t msg_len;
		msg_len = msg_len;

		dhcp_msg client_msg;
		msg_len = recvfrom(server_socket, &client_msg, sizeof(client_msg), 0, (struct sockaddr *) &client_socket, &client_socket_len);

		checkIntegrityOfPool(ip_pool);
		
		optList parsedOptions;
		initOptList(&parsedOptions);
		if(parseOptions(client_msg,&parsedOptions)!=OK)
			return ERRQUEUE;

		/*printList(&parsedOptions);*/
		dhcpOption message_type = searchOption(&parsedOptions,DHCP_MESSAGE_TYPE);
		if(message_type==NULL)
			return ERROPT;

		dhcp_msg server_msg; // response of server
		optList replyOptions;
		initOptList(&replyOptions);
		int nak = 0;
		int send_flag=OK;
		bool renew = false;
		switch(message_type->data[0])
		{
			case DHCPDISCOVER:
				if(discoverHandler(&server_msg,&client_msg,&replyOptions,ip_pool)!=OK)
				{
					send_flag=-1;
					printf("No more avaiable IP addresses in the pool\n");
				}
				break;
			case DHCPREQUEST:
				;
				uint32_t offered_ip = 0;
				if( (checkRequestedIp(&parsedOptions)) == OK )
				{
					if(checkServerId(&parsedOptions,SERVER_IP)!=OK)
					{
						send_flag=-1;
						break;
					}
					if(checkRequestIpFromClient(&client_msg,&parsedOptions,ip_pool,&offered_ip)!=OK)
						nak=1;
				} else // it's renew
				{
					renew = true;
					offered_ip = client_msg.ciaddr;
				}

				requestHandler(&client_msg,&server_msg,&replyOptions,ip_pool,nak,offered_ip);
				break;
			case DHCPRELEASE:
				send_flag=-1;
				if(checkServerId(&parsedOptions,SERVER_IP)!=OK)
					break;
				releaseHandler(&client_msg,ip_pool);
				break;
			default:
				deleteList(&parsedOptions);
				deleteList(&replyOptions);
				send_flag=-1;
		}
		int ret_send;
		if(send_flag==OK)
			ret_send = sendReply(server_socket,&client_socket,&server_msg,renew);
		else
			ret_send=OK;
		nak=0;
		deleteList(&replyOptions);
		deleteList(&parsedOptions);
		if (ret_send!=OK)
			return ERRCONNECT;
    }
	return OK;
}

/*
 *@function send reply of dhcp server
 *@server_socket fd of socket
 *@client_socket structure of client dest port etc..
 *@server_msg structure of server dest port etc..
 *@renew if reply is renew(unicast)
 */
int sendReply(int server_socket,struct sockaddr_in *client_socket,dhcp_msg *server_msg,bool renew)
{
	int data;
	if(!renew) // if it is renew it should be send as unicast
	{
		client_socket->sin_family = AF_INET;
		client_socket->sin_port = htons(BOOTPC);
		client_socket->sin_addr.s_addr = BROADCAST;
	}

	data = sendto(server_socket,server_msg,SIZEOFMSG,0,(struct sockaddr *)client_socket,sizeof(*client_socket));
	if(data<0)
		return ERRCONNECT;
	return OK;
}

/*
 *@function parse dhcp option from received packet and fill the structure with them
 *@message parsed message
 *@list_options pointer on structure which will store options
 */
int parseOptions(dhcp_msg message, optList* list_options)
{
	bool was_id = false;
	bool was_len = false;
	int data_counter = 0;
	struct dhcp_option parsed_option;
	for(int i=4;i<251;i++) // zacinam od 4 lebo preskakujem magic cookie
	{
		if(message.options[i]==255 && !was_id && !was_len) // END
			break;
		if(message.options[i]==0 && !was_id && !was_len) // PAD
			continue;

		if(!was_id)
		{ // was_id == false 
			parsed_option.id = message.options[i];
			was_id = true;
		}
		else if(!was_len)
		{ // was_len == false
			parsed_option.len = message.options[i];
			was_len = true;
		}
		else
		{ // now it's data
			parsed_option.data[data_counter]=message.options[i];
			data_counter++;
			if(data_counter==parsed_option.len)
			{
				if(addOpt(list_options,TAIL,&parsed_option)!=OK)
					return ERRQUEUE;
				was_id = false;
				was_len = false;
				data_counter=0;
				memset(&parsed_option,0,sizeof(struct dhcp_option));
			}
		}
	}
	return OK;
}

/*
 *@function handler for discover message prepare packet for sending
 *@dhcp_offer dhcp offer message
 *@dhcp_discover dhcp discover message
 *@replyOptions structure of options from discover
 *@ip_pool pointer on ip pool
 */
int discoverHandler(dhcp_msg *dhcp_offer,dhcp_msg *dhcp_discover, optList *replyOptions, IpPool *ip_pool)
{
	prepReply(dhcp_offer,dhcp_discover);
	uint32_t offered_ip;
	if(getIpForOffer(ip_pool,&offered_ip,dhcp_discover->chaddr)!=OK)
		return ERRIP;
	dhcp_offer->yiaddr = offered_ip;
	dhcp_offer->siaddr = SERVER_IP;
	fillOptions(replyOptions,dhcp_offer, DHCPOFFER,SERVER_IP,NET_MASK);
	return OK;
}

/*
 *@function handler for request message prepare packet for sending
 *@dhcp_request dhcp request message 
 *@dhcp_ack_nak dhcp ack or nak message
 *@replyOptions structure of options from discover
 *@ip_pool pointer on ip pool
 *@nak if the reply will be nak
 *@requested_ip the ip for assignment
 */
int requestHandler(dhcp_msg *dhcp_request,dhcp_msg *dhcp_ack_nak, optList *replyOptions, IpPool *ip_pool,int nak,uint32_t requested_ip)
{
	prepReply(dhcp_ack_nak,dhcp_request);
	if(nak!=1)
	{ // ACK
		dhcp_ack_nak->yiaddr = requested_ip;
		dhcp_ack_nak->siaddr = SERVER_IP;
		setForAck(ip_pool,requested_ip);
		fillOptions(replyOptions,dhcp_ack_nak,DHCPACK,SERVER_IP,NET_MASK);
		dhcpOutput(ip_pool,requested_ip);
	}
	else
	{ // NAK
		fillOptions(replyOptions,dhcp_ack_nak,DHCPNAK,SERVER_IP,NET_MASK);
	}
	return OK;
} 

/*
 *@function handling release message
 *@release_msg dhcp release message
 *@ip_pool pointer on ip pool
 */
int releaseHandler(dhcp_msg *release_msg, IpPool *ip_pool)
{
	if(releaseAddress(ip_pool,release_msg->chaddr)!=OK)
		return ERRIP;
	return OK;
}

/*
 *@function prepare packet based on received message
 *@server_msg message to be sent
 *@client_msg message from client
 */
void prepReply(dhcp_msg *server_msg,dhcp_msg* client_msg)
{
	memset(server_msg,0,sizeof(struct dhcp_msg));
	server_msg->op = REPLY;
	server_msg->xid = client_msg->xid;
	server_msg->flags = client_msg->flags;
	server_msg->giaddr = client_msg->giaddr;
	memcpy(server_msg->chaddr,client_msg->chaddr,16);
	server_msg->htype = client_msg->htype;
	server_msg->hlen = client_msg->hlen;
}

/*
 *@function print out information about succesful binded ip
 *@ip_pool pointer on ip pool
 *@requested_ip the ip for assignment
 */
void dhcpOutput(IpPool *ip_pool,uint32_t requested_ip)
{
	Ip ip_structure = searchIp(ip_pool,requested_ip);

	printf("%.2x:%.2x:%.2x:%.2x:%.2x:%.2x ",ip_structure->mac[0],ip_structure->mac[1],ip_structure->mac[2],
										ip_structure->mac[3],ip_structure->mac[4],ip_structure->mac[5]);
	char ip[32];
	inet_ntop(AF_INET,&ip_structure->address,ip,32);
	printf("%s ",ip);

	char time_buff[40];
	struct tm *timeinfo = NULL;
	time_t from = ip_structure->active_until-LEASETIMER;
	timeinfo = localtime(&from);
	strftime(time_buff , 40 , "%Y-%m-%d_%H:%M",timeinfo);
	printf("%s ",time_buff);

	timeinfo = localtime((&ip_structure->active_until));
	strftime(time_buff , 40 , "%Y-%m-%d_%H:%M",timeinfo);
	printf("%s\n",time_buff);
}
