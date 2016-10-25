/*
* ISA - DHCP server
* @author Marian Durco (xdurco00)
*/

#include "dhcpopt.h"

const uint8_t magic_cookie[4] = { 0x63, 0x82, 0x53, 0x63 };

void initOptList(optList *optionList)
{
	optionList->head_option = NULL;
	optionList->tail_option = NULL;
}

/*
 *@function delete option list from memory
 */
void deleteList(optList *optionList)
{
	optionList->tail_option = NULL;
	dhcpOption tmp_pointer = NULL;
	while(optionList->head_option != NULL)
	{
		tmp_pointer = optionList->head_option;	
		optionList->head_option = tmp_pointer->next;
		free(tmp_pointer);
	}
}

/*
 *@function add option to list
 */
int addOpt(optList *optionList,int where,dhcpOption option)
{
	dhcpOption tmp_option = malloc(sizeof(struct dhcp_option));
	if(tmp_option != NULL)
	{
		tmp_option->id = option->id;
		tmp_option->len = option->len;
		memcpy(tmp_option->data,option->data,option->len);

		tmp_option->next=NULL;
		tmp_option->prev=NULL;

		if(lenList(optionList)==0)
		{
			optionList->head_option=tmp_option;	
			optionList->tail_option=tmp_option;
		}
		else
		{
			if (where==HEAD)
			{
				tmp_option->next = optionList->head_option;
				optionList->head_option->prev = tmp_option;
				optionList->head_option = tmp_option;
			}
			else if (where==TAIL)
			{
				tmp_option->prev =optionList->tail_option; 
				optionList->tail_option->next = tmp_option;
				optionList->tail_option = tmp_option;
			}
			else
				return ERRQUEUE;
		}
	}
	else /* err malloc */
		return ERRMEM;
	return OK;
}

/*
 *@function return lenght of list
 */
int lenList(optList *optionList)
{
	int len;
	if(optionList->head_option==NULL)
		return 0; // empty queue
	else
	{
		len = 0;
		dhcpOption tmp_option = optionList->head_option;
		while(tmp_option != NULL)
		{
			tmp_option = tmp_option->next;
			len++;
		}
	}
	return len;
}

/*
 *@function print the option list
 */
void printList(optList *optionList)
{
	if(lenList(optionList)==0)
	{
		printf("List is empty\n");
		return;
	}
	dhcpOption tmp_pointer = optionList->head_option;
	int counter = 1;
	while(tmp_pointer != NULL)
	{
		printf("Element #%d :",counter);
		printf(" ID: %d",tmp_pointer->id);
		printf(" LEN: %d # ",tmp_pointer->len);
		for(int i=0;i<tmp_pointer->len;i++)
			printf("%02x",tmp_pointer->data[i]);
		printf("\n");
		tmp_pointer = tmp_pointer->next;
		counter++;
	}
}

/*
 *@function search in option list based on id of option
 *@id of option
 */
dhcpOption searchOption(optList *optionList,uint8_t id)
{
	dhcpOption tmp_pointer = optionList->head_option;
	while(tmp_pointer != NULL)
	{
		if(tmp_pointer->id==id)
			return tmp_pointer;
		tmp_pointer = tmp_pointer->next;
	}
	return NULL;
}

/*
 *@function fill option list for response of server
 *@replyOptions pointer on reply option list
 *@server_msg server packet
 *@message type of server response
 *@server_ip ip of server
 *@mask mask ip pool
 */
int fillOptions(optList *replyOptions,dhcp_msg *server_msg,int message,uint32_t server_ip,uint32_t mask)
{
	memcpy(server_msg->options,magic_cookie,4);
	// fill option structure with options
	createDHCPmsgType(replyOptions,message);
	int err_flag = OK;
	switch (message)
	{
		case DHCPOFFER: case DHCPACK:
			if((err_flag = createSubnetMask(replyOptions,mask))!=OK)
				break;
			if((err_flag = leaseTime(replyOptions))!=OK)
				break;
	}
	createServerIdentifier(replyOptions,server_ip);
	convertOptions(replyOptions,server_msg);
	if(err_flag!=OK)
	{
		reportErr("error in filling reply options");
		return ERROPT;
	}
	return OK;
}

/*
 *@function create option for type of message 
 */
int createDHCPmsgType(optList *optionList,int msg_type)
{
	struct dhcp_option msgType;
	msgType.id = (uint8_t)DHCP_MESSAGE_TYPE;
	msgType.len = 1;
	u_int8_t dhcp_type[1];
	switch(msg_type)
	{
		case DHCPOFFER:
			dhcp_type[0]=DHCPOFFER;
			break;
		case DHCPACK:
			dhcp_type[0]=DHCPACK;
			break;
		case DHCPNAK:
			dhcp_type[0]=DHCPNAK;
			break;
	}
	memcpy(msgType.data,&dhcp_type,msgType.len);
	if(addOpt(optionList,HEAD,&msgType)!=OK)
	{
		reportErr("err in adding option in creatingDHCPmsgType");
		return ERRQUEUE;
	}
	return OK;
}

/*
 *@function create option for subnetmask
 */
int createSubnetMask(optList* optionList,uint32_t mask)
{
	struct dhcp_option maskOpt;
	maskOpt.id = (uint8_t)SUBNET_MASK;
	maskOpt.len = 4;
	mask = htonl(mask);
	memcpy(maskOpt.data,&mask,maskOpt.len);
	if(addOpt(optionList,TAIL,&maskOpt)!=OK)
	{
		reportErr("err in adding option in createSubnetMask");
		return ERRQUEUE;
	}
	return OK;
}

/*
 *@function create option for server ID
 */
int createServerIdentifier(optList* optionList,uint32_t server_ip)
{
	struct dhcp_option serverId;
	serverId.id = (uint8_t)SERVER_IDENTIFIER;
	serverId.len = 4;
	memcpy(serverId.data,&server_ip,serverId.len);
	if(addOpt(optionList,TAIL,&serverId)!=OK)
	{
		reportErr("err in adding option in createServerIdentifier");
		return ERRQUEUE;
	}
	return OK;
}

/*
 *@function create option for lease time
 */
int leaseTime(optList* optionList)
{
	struct dhcp_option timeLease;
	timeLease.id = IP_ADDRESS_LEASE_TIME;
	timeLease.len = 4;
	uint8_t one_hour[4] = { 0x00, 0x00, 0x0e, 0x10 }; //3600 s
	memcpy(timeLease.data,&one_hour,timeLease.len);
	if(addOpt(optionList,TAIL,&timeLease)!=OK)
	{
		reportErr("err in adding option in leaseTime");
		return ERRQUEUE;
	}
	return OK;
}

/*
 *@function convert options from option list into server reply
 */
void convertOptions(optList *optionList, dhcp_msg *server_msg)
{
	
	bool was_id = false;
	bool was_len =false;
	int data_counter = 0;
	dhcpOption tmp_pointer = optionList->head_option;
	for(int i=4;i<251;i++) // skipping magic cookie
	{
		if(tmp_pointer==NULL)
		{
			server_msg->options[i]=END;
			return;
		}
		else
		{
			if(!was_id)
			{
				server_msg->options[i] = tmp_pointer->id;
				was_id = true;
			}
			else if(!was_len)
			{
				server_msg->options[i] = tmp_pointer->len;
				was_len = true;
			}
			else
			{
				server_msg->options[i]=tmp_pointer->data[data_counter];
				data_counter++;
				if(data_counter==tmp_pointer->len)
				{
					tmp_pointer=tmp_pointer->next;
					was_id = false;
					was_len = false;
					data_counter = 0;
				}
			}
		}
	}
}
