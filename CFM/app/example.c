/*
1.how to create a cfm module device?
#/bin/mknod /dev/cfmemulator   c 10  130 

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_arp.h>
#include <linux/if_ether.h>
#include <signal.h> 
#include <errno.h>

#include "../cfm_types.h"
#include "cfm_md5.h"
#include "../cfm_cmd.h"

#define DEBUG 1
#define TWONIC 0
struct MD5Packet_queue *MD5Pkt_queue;
#define MD5LIST_SIZE		100
#define	MAX_BUFFER_LEN	1600
#if 0
/*ioctl cmd*/

#define	CFM_CREATE_MEP	0x0
#define	CFM_DELETE_MEP	0x01
#define	CFM_LBM_START	0x02
#define	CFM_CCM_START	0x03
#define	CFM_LTM_START	0x04
#define	CFM_LTM_GET	0x05
#endif

unsigned char MAC_broadcast[6];
unsigned char hostmac_eth0[6];
#if TWONIC
unsigned char hostmac_eth1[6];
#endif
int Exit=0;

int vlanTag_check(uint8 *data);
void print_hex_data(char* data,int length);
int gethostmac(int sockfd, const char* dev, uint8 * mac_addr);
int bind_dev(int sockfd, const char* dev);
int set_promisc(int sockfd, const char* dev) ;
int recv_process(int fd, int sockfd, const char * dev);
int send_process(int fd, int sock_eth0, int sock_eth1);
int MD5_process(unsigned char * buffer, int len, int optFlag);

int vlanTag_check(uint8 *data)
{       
        int flag = 0;

	if(data == NULL){
		return -1;
	}
        if(((data[12] == 0x81)&&(data[13] == 0x00)) || ((data[12] == 0x88)&&(data[13] == 0x98))){
                if(((data[16] == 0x81)&&(data[17] == 0x00)) || ((data[16] == 0x88)&&(data[17] == 0x98)))
                        flag = 2;
                else    
                        flag = 1;
        }

	return flag;
}
void print_hex_data(char* data,int length)
{
	int num =0;
	while(num < length){
		printf("%2.2x ",(unsigned char)data[num]);
		if (((num+1) %16) == 0){
			printf("\n");
		}
		num ++;
	}
	printf("\n");	
}

int gethostmac(int sockfd, const char* dev, uint8 * mac_addr)
{
	struct ifreq if_mac;
	int i;

	if(mac_addr == NULL){
		return -1;
	}
	
	memset(&if_mac, 0, sizeof(struct ifreq));
	memcpy(if_mac.ifr_name, dev, sizeof(if_mac.ifr_name));
	if(ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0)
	{
		printf("erro when getting host mac addr with %s\n", dev);
		return -1;
	}
	for(i=0; i<ADDR_LEN; i++){
		*(mac_addr+i) = (unsigned char)if_mac.ifr_hwaddr.sa_data[i];
	}
	return 0;
}
int bind_dev(int sockfd, const char* dev)

{
	struct sockaddr_ll addr;   
	struct ifreq ifr;         

	memset(&ifr, 0, sizeof(ifr));
	memcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));
	if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
		printf("in bind_dev. error get ifindex with %s\n", dev);
		return -1;
	}

	/* bind the packet socket */
	memset(&addr, 0, sizeof(addr));
	addr.sll_family = AF_PACKET;
	addr.sll_protocol =htons(ETH_P_ALL);
	addr.sll_ifindex = ifr.ifr_ifindex;
	addr.sll_hatype =ARPHRD_ETHER;
	addr.sll_pkttype = 0;
	addr.sll_halen = ETH_ALEN;
	if (-1 == bind(sockfd, (struct sockaddr *)&addr, sizeof(addr))) {
		printf("in bind_dev. error bind with %s\n", dev);
		return -1;
	}

	return 0;

}

int set_promisc(int sockfd, const char* dev) 

{
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	memcpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name));
	if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) == -1) {
		printf("in set_promisc. error get IFFLAGS with %s\n", dev);
		return -1;
	 }

	ifr.ifr_flags |= IFF_PROMISC;
	if ((ioctl(sockfd, SIOCSIFFLAGS, &ifr)) == -1){
		printf("in set_promisc. error set IFFLAGS with %s\n", dev);
		return -1;
    	}

  	return 0;
}

int recv_process(int fd, int sockfd, const char * dev)
{
	uint8 buffer[MAX_BUFFER_LEN];
	uint8 data[MAX_BUFFER_LEN];
	int recv_len;
	uint16 srcPortId = 0;
	uint32 srcFlowId = 0;
	int flag;
	int VlanID;
	int write_len;
	int ret;
	uint8 offset=sizeof(srcFlowId)+sizeof(srcPortId);
	memset(buffer, 0 ,MAX_BUFFER_LEN);
	memset(data, 0, MAX_BUFFER_LEN);
	recv_len = recvfrom(sockfd, buffer, MAX_BUFFER_LEN, 0, NULL, NULL);
	if(recv_len > 0){
#if TWONIC
		if((memcmp(buffer+ADDR_LEN, hostmac_eth0, ADDR_LEN) != 0)&&(memcmp(buffer+ADDR_LEN, hostmac_eth1, ADDR_LEN) != 0))
#else
		if(memcmp(buffer+ADDR_LEN, hostmac_eth0, ADDR_LEN) != 0)
#endif
		{
			ret = MD5_process(buffer, recv_len, 0);
			if(ret != 0){
				return 0;
			}
			if(memcmp(dev, "eth0", 4) == 0){
				srcPortId = 1;
			}
			else if(memcmp(dev, "eth1", 4) == 0){
				srcPortId = 2;
			}
			flag = vlanTag_check(buffer);

			if(flag >= 1){
				VlanID = (((int)buffer[14]<<8)&0x00000f00)|(((int)buffer[15])&0x000000ff);
				srcFlowId = VlanID/10;
			}
#if DEBUG
			if((buffer[ADDR_LEN*2+flag*VLAN_TAG_LEN] == 0x89)&&(buffer[ADDR_LEN*2+1+flag*VLAN_TAG_LEN] == 0x02)){//CFM pkt
				printf("in recv_process.flag:%d;srcPortId:%d;srcFlowId:%d\n", flag, srcPortId, srcFlowId);
				print_hex_data(buffer, recv_len);
			}
#endif
			uint16_to_uint8(data, srcPortId);
			uint32_to_uint8((data+sizeof(srcPortId)), srcFlowId);
			memcpy(data+offset, buffer, MAX_BUFFER_LEN-6);

			write_len = write(fd, data, recv_len+offset);
			if(write_len < 0){
				printf("fail to write data into kernel\n");
			}
		}
	}

	return 0;
}

int send_process(int fd, int sock_eth0, int sock_eth1)
{
	uint8 buffer[MAX_BUFFER_LEN];
	int read_len;
	int send_len;
	uint16 PortID = 0;
	int flag;
	int offset=6;
	memset(buffer, 0, MAX_BUFFER_LEN);
	read_len = read(fd, buffer, sizeof(buffer));
	if (read_len < 0){
		printf("fail to read data from kernel\n");
	}
	else if(read_len==0){
		//printf("read data from kernel is 0\n");
	}
	else{
		if(read_len > offset)
			{
			//print_hex_data(buffer, read_len);
			uint8_to_uint16(PortID, buffer);
#if DEBUG
			flag = vlanTag_check(buffer+offset);
			if((buffer[ADDR_LEN*2+offset+flag*VLAN_TAG_LEN] == 0x89)&&(buffer[ADDR_LEN*2+offset+1+flag*VLAN_TAG_LEN] == 0x02))
				{
				printf("PortID:%d\n", PortID);
				printf("read_len:%d\n", read_len);
				print_hex_data(buffer, read_len);
				}
#endif
			}
		if(PortID == 1){
			if(memcmp(buffer+offset+ADDR_LEN, hostmac_eth0, ADDR_LEN) != 0){
				//printf("now call MD5_process\n");
				MD5_process(buffer+offset, read_len-offset, 1);
			}
			send_len = send(sock_eth0,buffer+offset,read_len-offset,0);
			if(send_len < 0){
				printf("sendto error\n");
			}
		}
		else if(PortID == 2){
			
			send_len = send(sock_eth0,buffer+offset,read_len-offset,0);
			 
			if(send_len < 0){
				printf("sendto error\n");
			}
		}
		else{
			if(read_len > offset);
			//printf("Port not exist!%d\n",PortID);
		}
	}

	return 0;
}
static void handle_term(int sig)
{
	printf("Closing...\n");
	Exit = 1;
}
 
int main (int argc, char *argv[])
{
	int fd = -1;	
	int sock_eth0 = -1;
#if TWONIC
	int sock_eth1 = -1;
#endif
	int ret;
	struct sockaddr_ll destaddr_eth0;
#if TWONIC
	struct sockaddr_ll destaddr_eth1;
#endif
	struct ifreq ifstruct;
	fd_set fread_in, fread_out;
	struct timeval tv;
	int sock_max;

	int i;

	printf("begin...\n");
	sock_eth0 = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sock_eth0 < 0){
		printf("create new socket error\n");
		goto end;
	}
#if TWONIC
	sock_eth1 = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
	if(sock_eth1< 0){
		printf("create new socket error\n");
		goto end;
	}
#endif
	//bind
	bind_dev(sock_eth0, "eth0");
#if TWONIC
	bind_dev(sock_eth1, "eth1");
#endif	
	//set promisc
	set_promisc(sock_eth0, "eth0");
#if TWONIC
	set_promisc(sock_eth1, "eth1");
#endif


	//set socket option
	int nSendBuf=32*1024;
	if(setsockopt(sock_eth0,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int))==-1){
		printf("set SNDBUF error\n");
	}

	int flagSocketOpt = 1;
	if(setsockopt(sock_eth0,SOL_SOCKET,SO_BROADCAST,(char *)&flagSocketOpt,sizeof(flagSocketOpt))==-1){
		printf("set BROADCAST error\n");
	}
#if TWONIC
	if(setsockopt(sock_eth1,SOL_SOCKET,SO_SNDBUF,(const char*)&nSendBuf,sizeof(int))==-1){
		printf("set SNDBUF error\n");
	}

	flagSocketOpt = 1;
	if(setsockopt(sock_eth1,SOL_SOCKET,SO_BROADCAST,(char *)&flagSocketOpt,sizeof(flagSocketOpt))==-1){
		printf("set BROADCAST error\n");
	}
#endif
	memset(hostmac_eth0, 0 ,7);
	if(gethostmac(sock_eth0,"eth0", hostmac_eth0) != 0){
		printf("get hostmac error\n");
	}
#if TWONIC
	memset(hostmac_eth1, 0 ,7);
	if(gethostmac(sock_eth1,"eth1", hostmac_eth1) != 0){
		printf("get hostmac error\n");
	}
#endif
	fd = open("/dev/cfmemulator", O_RDWR);
	if (fd < 0){
		printf("open cfm device error\n");
		goto end;
	}

	tv.tv_sec = 0;
	tv.tv_usec = 20;
	FD_ZERO(&fread_in);
	FD_SET(sock_eth0, &fread_in);
#if TWONIC
	FD_SET(sock_eth1, &fread_in);
#endif
#if TWONIC
	sock_max = (sock_eth0 > sock_eth1) ? sock_eth0 : sock_eth1;
#else
	sock_max = sock_eth0;
#endif
	MD5Pkt_queue = (struct MD5Packet_queue *)malloc(sizeof(struct MD5Packet_queue));
	if(MD5Pkt_queue == NULL){
		printf("fail to malloc MD5Pkt_queue\n");
		goto end;
	}
	memset(MD5Pkt_queue, 0, sizeof(struct MD5Packet_queue));
	
	MD5Pkt_queue->MD5Packet_list = (struct MD5Packet_st *)malloc(sizeof(struct MD5Packet_st) * MD5LIST_SIZE);
	if(MD5Pkt_queue->MD5Packet_list  == NULL){
		printf("fail to malloc MD5Pkt_queue->MD5Packet_list \n");
		goto end;
	}
	memset(MD5Pkt_queue->MD5Packet_list, 0, sizeof(struct MD5Packet_st) * MD5LIST_SIZE);
	 /*initialize signal proccessing*/
	 if (SIG_ERR == signal(SIGTERM, handle_term)){
		  printf("%d:%s", errno, strerror(errno));
       	 return -1;
	 }
	 if (SIG_ERR == signal(SIGINT, handle_term)){
 		 printf("%d:%s", errno, strerror(errno));
      	  return -1;
	 }

/*management API test start*/
////////////////////////NEWLY ADDED!!/////////////////////////

#if 0
{//GET MDLIST
	struct Test_MDList_st Test_MDList;
	int i;

	memset(&Test_MDList, 0, sizeof(struct Test_MDList_st));
	ioctl(fd, CFM_GET_MDLIST, &Test_MDList);
	if(ret != 0){
		printf("ioctl error\n");
	}
	for(i=0;i<Test_MDList.num;i++){
		printf("meid:%")}
}
#endif

#if 0
{//CREATE MD
	struct Test_MD_st Test_MD;
	memset(&Test_MD, 0 ,sizeof(struct Test_MD_st ));
	Test_MD.MDLevel = 4;
	Test_MD.MDnameFormat = 1;
	Test_MD.MDnameLength = 20;
	memcpy(Test_MD.MDname,"the new MD" ,20);
	ret = ioctl(fd, CFM_CREATE_MD ,&Test_MD);
	if(ret != 0){
		printf("ioctl error\n");
	}
	else printf("new MD created!MEID:%d\n",Test_MD.meid);
}
#endif
#if 0

//CFM_DELETE_MD
{
	struct Test_MD_st Test_MD;
	memset(&Test_MD, 0 ,sizeof(struct Test_MD_st ));
	Test_MD.meid =1;
	ret = ioctl(fd, CFM_DELETE_MD,&Test_MD);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0

//CFM_SET_MD
{
	struct Test_MD_st Test_MD;
	memset(&Test_MD, 0 ,sizeof(struct Test_MD_st ));
	Test_MD.meid = 1;
	Test_MD.event=3;//Level
	Test_MD.value=6;
	ret = ioctl(fd, CFM_SET_MD,&Test_MD);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0

//CFM_GET_MD
{
	struct Test_MD_st Test_MD;
	memset(&Test_MD, 0 ,sizeof(struct Test_MD_st ));
	Test_MD.meid = 1;
	ret = ioctl(fd, CFM_GET_MD,&Test_MD);
	if(ret != 0){
		printf("ioctl error\n");
	}
	else printf("Level:%d,NameFormat:%d,MHFCreation:%d\n",Test_MD.MDLevel,Test_MD.MDnameFormat,Test_MD.MHFCreation);
}
#endif
#if 0

//CFM_CREATE_MA
{
	struct Test_MA_st Test_MA;
	memset(&Test_MA, 0 ,sizeof(struct Test_MA_st));
	Test_MA.MDid = 1;
	Test_MA.ShortMAnameFormat = 0x01;
	Test_MA.ShortMAnameLength = 20;
	memcpy( Test_MA.ShortMAname, "the new MA ",20);
	ret = ioctl(fd, CFM_CREATE_MA,&Test_MA);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0

//CFM_DELETE_MA
{
	struct Test_MA_st Test_MA;
	memset(&Test_MA, 0 ,sizeof(struct Test_MA_st));
	Test_MA.meid=1;
	ret = ioctl(fd, CFM_DELETE_MA,&Test_MA);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0

//CFM_SET_MA
{
	struct Test_MA_st Test_MA;
	memset(&Test_MA, 0 ,sizeof(struct Test_MA_st));
	Test_MA.meid=1;
	Test_MA.event=2;
	Test_MA.value=0x02;
	ret = ioctl(fd, CFM_SET_MA,&Test_MA);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif


#if 0

//CFM_GET_MA
{
	struct Test_MA_st Test_MA;
	memset(&Test_MA, 0 ,sizeof(struct Test_MA_st));
	Test_MA.meid= 1;
	ret = ioctl(fd, CFM_GET_MA,&Test_MA);
	if(ret != 0){
		printf("ioctl error\n");
	}
	else
		printf("MDID:%d,MAName:%s\n",Test_MA.MDid,Test_MA.ShortMAname);
}
#endif


#if 0
//CFM_CREATE_MEP

struct Test_MEP_Create_st Test_MEP_Create;
uint8 TMAC[ADDR_LEN]={0x00,0x0c,0x29,0xed,0xfc,0xdd};
memset(&Test_MEP_Create,0,sizeof(struct Test_MEP_Create_st));
Test_MEP_Create.Direction=0x02;
Test_MEP_Create.FlowID=2;
Test_MEP_Create.srcPortId=1;
Test_MEP_Create.MAid=2;
memcpy(Test_MEP_Create.MACAddr,TMAC,ADDR_LEN);
ret=ioctl(fd,CFM_CREATE_MEP,&Test_MEP_Create);
if(ret!=0)
	{
		printf("ioctl error\n");
	}
else
	printf("Create MEPID=%d\n",Test_MEP_Create.meid);
#endif

#if 0
uint16 meid=2;

ret=ioctl(fd,CFM_DELETE_MEP,&meid);
if(ret!=0)
	{
		printf("ioctl error\n");
	}

#endif

#if 0
struct Test_MEP_Set_st Test_MEP_Set;
struct Test_MEP_Get_st Test_MEP_Get;
memset(&Test_MEP_Set,0,sizeof(struct Test_MEP_Set_st));
memset(&Test_MEP_Get,0,sizeof(struct Test_MEP_Get_st));

Test_MEP_Set.meid=1;
Test_MEP_Set.event=1;
Test_MEP_Set.value16=2;

ret=ioctl(fd,CFM_SET_MEP,&Test_MEP_Set);
if(ret != 0){
		printf("ioctl error\n");
	}
Test_MEP_Get.meid=Test_MEP_Set.meid;
ret=ioctl(fd,CFM_GET_MEP,&Test_MEP_Get);
if(ret != 0){
		printf("ioctl error\n");
	}
else
	printf("FlowID:%d,Direction:%d,VLAN:%d\n",Test_MEP_Get.FlowId,Test_MEP_Get.Direction,Test_MEP_Get.PrimaryVlan);
#endif

#if 0
struct Test_MPStatusEntry_st MPStatusEntry;
memset(&MPStatusEntry,0,sizeof(struct Test_MEP_Get_st));
MPStatusEntry.Direction=1;
MPStatusEntry.VlanId=20;
MPStatusEntry.Level=5;
MPStatusEntry.PortId=1;


ret=ioctl(fd,CFM_GET_MPSTATUS,&MPStatusEntry);
if(ret != 0){
		printf("ioctl error\n");
	}
else printf("meid:%d\n",MPStatusEntry.MEPId);
#endif

#if 0

struct Test_ConfigurationErrorEntry_st ConfigurationErrorEntry;
memset(&ConfigurationErrorEntry,0,sizeof(struct Test_ConfigurationErrorEntry_st));	
ret=ioctl(fd,CFM_GET_CONFIGERROR,&ConfigurationErrorEntry);
if(ret != 0){
		printf("ioctl error\n");
	}
else
	printf("DetectedConfigurationError=%d\n",ConfigurationErrorEntry.DetectedConfigurationError);
#endif

#if 0

struct Test_CFMStack_st Test_CFMStack;
memset(&Test_CFMStack,0,sizeof(struct Test_CFMStack_st));
ret=ioctl(fd,CFM_GET_STACK,&Test_CFMStack);
if(ret != 0){
		printf("ioctl error\n");
	}

#endif

#if 0
struct Test_CFMDefaultCatchall_st Test_CFMDefaultCatchall;
memset(&Test_CFMDefaultCatchall,0,sizeof(struct Test_CFMDefaultCatchall_st));
Test_CFMDefaultCatchall.event=1;
Test_CFMDefaultCatchall.value8=2;

ret=ioctl(fd,CFM_SET_DEFAULTCATCHALL,&Test_CFMDefaultCatchall);
if(ret != 0){
		printf("ioctl error\n");
	}
memset(&Test_CFMDefaultCatchall,0,sizeof(struct Test_CFMDefaultCatchall_st));
Test_CFMDefaultCatchall.event=1;

ret=ioctl(fd,CFM_GET_DEFAULTCATCHALL,&Test_CFMDefaultCatchall);
if(ret != 0){
		printf("ioctl error\n");
	}
else printf("Test_CFMDefaultCatchall.value8=%d\n",Test_CFMDefaultCatchall.value8);
#endif
#if 0
struct Test_CFMDefaultTable_st Test_CFMDefaultTable;
struct Test_DefaultMDLevelEntry_st DefaultMDLevelEntry;
memset(&DefaultMDLevelEntry,0,sizeof(struct Test_DefaultMDLevelEntry_st));
memset(&Test_CFMDefaultTable,0,sizeof(struct Test_CFMDefaultTable_st));
uint16 vidlist[5]={20,34,13,9,22};
Test_CFMDefaultTable.vidList=&vidlist;
Test_CFMDefaultTable.event=1;
Test_CFMDefaultTable.value=3;

ret=ioctl(fd,CFM_SET_DEFAULTTABLE,&Test_CFMDefaultTable);
if(ret != 0){
		printf("ioctl error\n");
	}
DefaultMDLevelEntry.PrimaryVlanId=12;

ret=ioctl(fd,CFM_GET_DEFAULTTALBE,&DefaultMDLevelEntry);
if(ret != 0){
		printf("ioctl error\n");
	}
else printf("level:%d\n",DefaultMDLevelEntry.Level);
#endif

#if 0
struct Test_ChassisManagement_st Test_ChassisManagement;
memset(&Test_ChassisManagement,0,sizeof(struct Test_ChassisManagement_st));
ret=ioctl(fd,CFM_GET_CHASSISMANAGEMENT,&Test_ChassisManagement);
if(ret != 0){
		printf("ioctl error\n");
	}
else
	{
		printf("chassis_ID:%s\n chassis_ID_Length:%u\n chassis_ID_Subtype:%u\n management_Address:%s\n management_Address_Domain:%s\n management_Address_Domain_Length:%u\n management_Address_Length:%u\n",Test_ChassisManagement.chassis_ID,Test_ChassisManagement.chassis_ID_Length,Test_ChassisManagement.chassis_ID_Subtype,Test_ChassisManagement.management_Address,Test_ChassisManagement.management_Address_Domain,Test_ChassisManagement.management_Address_Domain_Length,Test_ChassisManagement.management_Address_Length);
	}
#endif
///////////////////END OF NEWLY ADDED!!/////////////////////////

#if 0
{
	ret = ioctl(fd, CFM_STOP);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif

#if 0
{
	struct Test_PeerMEPID_st Test_PeerMEPID;
	struct Test_PeerMEPID_st rTest_PeerMEPID;
	memset(&Test_PeerMEPID, 0, sizeof(struct Test_PeerMEPID_st));
	memset(&rTest_PeerMEPID, 0, sizeof(struct Test_PeerMEPID_st));
//set
	Test_PeerMEPID.meid = 1;
	for(i=0;i<12;i++){
		Test_PeerMEPID.peerId[i] = i;
	}
	ret = ioctl(fd, CFM_SETPEERMEPID, &Test_PeerMEPID);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_PeerMEPID.meid = 1;
	ret = ioctl(fd, CFM_GETPEERMEPID, &rTest_PeerMEPID);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("peerID:\n");
	for(i=0;i<12;i++){
		printf("%d ", rTest_PeerMEPID.peerId[i]);
	}
	printf("\n");
}
#endif
#if 0
{
	uint8 len, subtype, rlen, rsubtype;
	len = 0x0A;
	subtype = 0x03;
	uint8 ChassisID[50];
	uint8 rChassisID[50];
	printf("/****SenderId TLV Management API Test****/\n");
//set
	ret = ioctl(fd, CFM_SET_CHASSISIDLENGTH, &len);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_SET_CHASSISIDSBUTYPE, &subtype);
	if(ret != 0){
		printf("ioctl error\n");
	}
	for(i=0;i<len;i++){
		ChassisID[i] = 0x88;
	}
	ret = ioctl(fd, CFM_SET_CHASSISID, ChassisID);
	if(ret != 0){
		printf("ioctl error\n");
	}

//get
	ret = ioctl(fd, CFM_GET_CHASSISIDLENGTH, &rlen);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_GET_CHASSISIDSBUTYPE, &rsubtype);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("rlen:%.2x, rsubtype:%.2x\n", rlen, rsubtype);
	ret = ioctl(fd, CFM_GET_CHASSISID, rChassisID);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("rChassisID:\n");
	for(i=0;i<rlen;i++){
		printf("%.2x ", rChassisID[i]);
	}
	printf("\n");
}
#endif
#if 0
{
	uint8 len, rlen;
	len = 0x0A;
	uint8 domain[50];
	uint8 rdomain[50];
//set
	ret = ioctl(fd, CFM_SET_MGMTADDRDOMAINLENGTH, &len);
	if(ret != 0){
		printf("ioctl error\n");
	}
	for(i=0;i<len;i++){
		domain[i] = 0x77;
	}
	ret = ioctl(fd, CFM_SET_MGMTADDRDOMAINM, domain);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	ret = ioctl(fd, CFM_GET_MGMTADDRDOMAINLENGTH, &rlen);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("rlen:%.2x\n", rlen);
	ret = ioctl(fd, CFM_GET_MGMTADDRDOMAINM, rdomain);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("rdomain:\n");
	for(i=0;i<rlen;i++){
		printf("%.2x ", rdomain[i]);
	}
	printf("\n");
}
#endif
#if 0
{
	uint8 len, rlen;
	len = 0x0A;
	uint8 addr[50];
	uint8 raddr[50];
//set
	ret = ioctl(fd, CFM_SET_MGMTADDRLENGTH, &len);
	if(ret != 0){
		printf("ioctl error\n");
	}
	for(i=0;i<len;i++){
		addr[i] = 0x66;
	}
	ret = ioctl(fd, CFM_SET_MGMTADDR, addr);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	ret = ioctl(fd, CFM_GET_MGMTADDRLENGTH, &rlen);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("rlen:%.2x\n", rlen);
	ret = ioctl(fd, CFM_GET_MGMTADDR, raddr);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("raddr:\n");
	for(i=0;i<rlen;i++){
		printf("%.2x ", raddr[i]);
	}
	printf("\n");
}
#endif

#if 0
{
	uint16 len, rlen;
	uint8 oui[3];
	uint8 roui[3];
	uint8 subtype, rsubtype;
	uint8 value[128], rvalue[128];
	memset(oui, 0, sizeof(oui));
	memset(roui, 0, sizeof(roui));
	memset(value, 0, sizeof(value));
	memset(rvalue, 0, sizeof(rvalue));
	printf("/****Organization-Specific TLV Management API Test****/\n");
//set
	len = 8;
	for(i=0;i<3;i++){
		oui[i] = 0x22;
	}
	subtype = 0x01;
	for(i=0;i<len-4;i++){
		value[i] = 0x55;
	}
	ret = ioctl(fd, CFM_SET_ORGSPELEN, &len);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_SET_ORGSPEOUI, oui);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_SET_ORGSPESUBTYPE, &subtype);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_SET_ORGSPEVALUE, value);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	ret = ioctl(fd, CFM_GET_ORGSPELEN, &rlen);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_GET_ORGSPEOUI, roui);
	if(ret != 0){
		printf("ioctl error\n");
	}
	ret = ioctl(fd, CFM_GET_ORGSPESUBTYPE, &rsubtype);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("os_len:%.2x, os_subtype:%.2x\n", rlen, rsubtype);
	printf("os_OUI:\n");
	print_hex_data(roui, 3);
	printf("\n");
	ret = ioctl(fd, CFM_GET_ORGSPEVALUE, rvalue);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("os_value:\n");
	print_hex_data(rvalue, rlen-4);
	printf("\n");
}
#endif
/****Alarm Management API Test****/
#if 0
{
	struct Test_AlarmPri_st Test_AlarmPri;
	struct Test_AlarmPri_st rTest_AlarmPri;
	memset(&Test_AlarmPri, 0, sizeof(struct Test_AlarmPri_st));
	memset(&rTest_AlarmPri, 0, sizeof(struct Test_AlarmPri_st));
	printf("/****Alarm Management API Test****/\n");
//set
	Test_AlarmPri.meid = 1;
	Test_AlarmPri.AlarmPri = 2;
	ret = ioctl(fd, CFM_SET_ALARMTHRESHOLD, &Test_AlarmPri);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_AlarmPri.meid = 1;
	ret = ioctl(fd, CFM_GET_ALARMTHRESHOLD, &rTest_AlarmPri);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("AlarmPri:%d\n", rTest_AlarmPri.AlarmPri);
}
#endif
/*CC Management API Test*/
#if 0
{
	struct Test_CCMInterval_st Test_CCMInterval;
	struct Test_CCMInterval_st rTest_CCMInterval;
	memset(&Test_CCMInterval, 0, sizeof(struct Test_CCMInterval_st));
	memset(&rTest_CCMInterval, 0, sizeof(struct Test_CCMInterval_st));
	printf("/****CC Management API Test****/\n");
//set CCMInterval
	Test_CCMInterval.meid = 1;
	Test_CCMInterval.interval = 4;
	ret = ioctl(fd, CFM_SET_CCMINTERVAL, &Test_CCMInterval);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get CCMInterval
	rTest_CCMInterval.meid = 1;
	ret = ioctl(fd, CFM_GET_CCMINTERVAL, &rTest_CCMInterval);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("CCMInterval:%d\n", rTest_CCMInterval.interval);
}
#endif

#if 0
{
	struct Test_ccm_st Test_ccm;
//get last recv error ccm
	memset(&Test_ccm, 0, sizeof(struct Test_ccm_st));
	Test_ccm.meid = 1;
	ret = ioctl(fd, CFM_GET_LASTRCVDERRORCCM, &Test_ccm);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("last recv error ccm:\n");
	print_hex_data(Test_ccm.ccm, Test_ccm.len);
//get last recv xcon ccm
	memset(&Test_ccm, 0, sizeof(struct Test_ccm_st));
	Test_ccm.meid = 1;
	ret = ioctl(fd, CFM_GET_LASTRCVDXCONCCM, &Test_ccm);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("last recv xcon ccm:\n");
	print_hex_data(Test_ccm.ccm, Test_ccm.len);
}
#endif
#if 0
{
	struct Test_CCCount_st Test_CCCount;
//get OutOfSeqCCMCounts
	memset(&Test_CCCount, 0, sizeof(struct Test_CCCount_st));
	Test_CCCount.meid = 1;
	ret = ioctl(fd, CFM_GET_OUTOFSEQCCMCOUNTS, &Test_CCCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("OutOfSeqCCMCounts:%d\n", Test_CCCount.count);
//get XmitCCMCounts
	memset(&Test_CCCount, 0, sizeof(struct Test_CCCount_st));
	Test_CCCount.meid = 1;
	ret = ioctl(fd, CFM_GET_XMITCCMCOUNTS, &Test_CCCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("XmitCCMCounts:%d\n", Test_CCCount.count);

}
#endif
#if 0
{
	struct Test_CreateCCMDatabaseNode_st Test_CreateCCMDatabaseNode;
	memset(&Test_CreateCCMDatabaseNode, 0, sizeof(struct Test_CreateCCMDatabaseNode_st));
	Test_CreateCCMDatabaseNode.meid = 1;
	Test_CreateCCMDatabaseNode.MEPid = 2;
	ret = ioctl(fd, CFM_CREATECCMDABASENODE, &Test_CreateCCMDatabaseNode);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0
{
	struct Test_Status_st Test_Status;
	struct Test_Status_st rTest_Status;
//PortStatus
	memset(&Test_Status, 0, sizeof(struct Test_Status_st));
	memset(&rTest_Status, 0, sizeof(struct Test_Status_st));
//set
	Test_Status.meid = 1;
	Test_Status.value = 0x02;
	ret = ioctl(fd, CFM_SET_PORTSTATUS, &Test_Status);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_Status.meid = 1;
	ret = ioctl(fd, CFM_GET_PORTSTATUS, &rTest_Status);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("PortStatus:%.2x\n", rTest_Status.value);
//InterfaceStatus
	memset(&Test_Status, 0, sizeof(struct Test_Status_st));
	memset(&rTest_Status, 0, sizeof(struct Test_Status_st));
//set
	Test_Status.meid = 1;
	Test_Status.value = 0x02;
	ret = ioctl(fd, CFM_SET_INTERFASESTATUS, &Test_Status);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_Status.meid = 1;
	ret = ioctl(fd, CFM_GET_INTERFASESTATUS, &rTest_Status);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("InterfaceStatus:%.2x\n", rTest_Status.value);
}
#endif

#if 0
{
	struct Test_OrgSpePermission_st Test_OrgSpePermission;
	struct Test_OrgSpePermission_st rTest_OrgSpePermission;
	memset(&Test_OrgSpePermission, 0, sizeof(struct Test_OrgSpePermission_st));
	memset(&rTest_OrgSpePermission, 0, sizeof(struct Test_OrgSpePermission_st));
//set
	Test_OrgSpePermission.meid = 1;
	Test_OrgSpePermission.permission = 1;
	ioctl(fd, CFM_SET_CCORGSPEPERMISSION, &Test_OrgSpePermission);
//get
	rTest_OrgSpePermission.meid = 1;
	ioctl(fd, CFM_GET_CCORGSPEPERMISSION, &rTest_OrgSpePermission);
	printf("CC OrgSpe Permission:%d\n", rTest_OrgSpePermission.permission);
}
#endif

#if 0
{
	struct Test_CCIenabled_st Test_CCIenabled;
	memset(&Test_CCIenabled, 0, sizeof(struct Test_CCIenabled_st));
	Test_CCIenabled.meid = 1;
	Test_CCIenabled.CCIenabled = 1;
	ret = ioctl(fd, CFM_SET_CCIENABLED, &Test_CCIenabled);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0
{
	struct Test_CCIenabled_st Test_CCIenabled;
	memset(&Test_CCIenabled, 0, sizeof(struct Test_CCIenabled_st));
	Test_CCIenabled.meid = 1;
	Test_CCIenabled.CCIenabled = 0;
	ret = ioctl(fd, CFM_SET_CCIENABLED, &Test_CCIenabled);
	if(ret != 0){
		printf("ioctl error\n");
				}
}
#endif
#if 0
{
	struct Test_CCCount_st Test_CCCount;
//get OutOfSeqCCMCounts
	memset(&Test_CCCount, 0, sizeof(struct Test_CCCount_st));
	Test_CCCount.meid = 1;
	ret = ioctl(fd, CFM_GET_OUTOFSEQCCMCOUNTS, &Test_CCCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("OutOfSeqCCMCounts:%d\n", Test_CCCount.count);
//get XmitCCMCounts
	memset(&Test_CCCount, 0, sizeof(struct Test_CCCount_st));
	Test_CCCount.meid = 1;
	ret = ioctl(fd, CFM_GET_XMITCCMCOUNTS, &Test_CCCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("XmitCCMCounts:%d\n", Test_CCCount.count);

}


#endif
/*LT Management API Test*/
#if 0
{
	struct Test_LTCount_st Test_LTCount;
//xmit LTR
	memset(&Test_LTCount, 0, sizeof(struct Test_LTCount_st));
	Test_LTCount.meid = 1;
	ret = ioctl(fd, CFM_LT_GETXMITLTRCOUNTS, &Test_LTCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("xmit LTR:%d\n", Test_LTCount.count);
//xmit LTM
	memset(&Test_LTCount, 0, sizeof(struct Test_LTCount_st));
	Test_LTCount.meid = 1;
	ret = ioctl(fd, CFM_LT_GETXMITLTMCOUNTS, &Test_LTCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("xmit LTM:%d\n", Test_LTCount.count);
//recv LTM
	memset(&Test_LTCount, 0, sizeof(struct Test_LTCount_st));
	Test_LTCount.meid = 1;
	ret = ioctl(fd, CFM_LT_GETLTMRECEIVEDCOUNTS, &Test_LTCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("recv LTM:%d\n", Test_LTCount.count);
//recv LTR
	memset(&Test_LTCount, 0, sizeof(struct Test_LTCount_st));
	Test_LTCount.meid = 1;
	ret = ioctl(fd, CFM_LT_GETLTRRECEIVEDCOUNTS, &Test_LTCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("recv LTR:%d\n", Test_LTCount.count);
//unexpect LTR
	memset(&Test_LTCount, 0, sizeof(struct Test_LTCount_st));
	Test_LTCount.meid = 1;
	ret = ioctl(fd, CFM_LT_GETUNEXPECTEDLTRCOUNTS, &Test_LTCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("unexpect LTR:%d\n", Test_LTCount.count);
}
#endif
#if 0
{
	struct Test_LTTransId_st Test_LTTransId;
	//set
	memset(&Test_LTTransId, 0, sizeof(struct Test_LTTransId_st));
	Test_LTTransId.meid = 1;
	Test_LTTransId.ltid = 100;
	ret = ioctl(fd, CFM_LT_SETNEXTTRANSID, &Test_LTTransId);
	if(ret != 0){
		printf("ioctl error\n");
	}
	//get	
	memset(&Test_LTTransId, 0, sizeof(struct Test_LTTransId_st));
	Test_LTTransId.meid = 1;
	ret = ioctl(fd, CFM_LT_GETNEXTTRANSID, &Test_LTTransId);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("Next LTTransId:%d\n", Test_LTTransId.ltid);

}

#endif
#if 0
{
	struct Test_LTSetLTMTtl_st Test_LTSetLTMTtl;
	memset(&Test_LTSetLTMTtl, 0, sizeof(struct Test_LTSetLTMTtl_st));
	Test_LTSetLTMTtl.meid = 1;
	Test_LTSetLTMTtl.ttl = 0x40;
	ret = ioctl(fd, CFM_LT_SETLTMTTL, &Test_LTSetLTMTtl);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0
{
	struct Test_LTSetLTMFlags_st Test_LTSetLTMFlags;
	memset(&Test_LTSetLTMFlags, 0, sizeof(struct Test_LTSetLTMFlags_st));
	Test_LTSetLTMFlags.meid = 1;
	Test_LTSetLTMFlags.flags = 0x80;    //useFDBonly
	ret = ioctl(fd, CFM_LT_SETLTMFLAGS, &Test_LTSetLTMFlags);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0
{
	struct Test_LTEgressId_st Test_LTEgressId;
	struct Test_LTEgressId_st rTest_LTEgressId;
	memset(&Test_LTEgressId, 0, sizeof(struct Test_LTEgressId_st));
	memset(&rTest_LTEgressId, 0, sizeof(struct Test_LTEgressId_st));
//set
	Test_LTEgressId.meid = 1;
	Test_LTEgressId.gressid[0] = 0x00;
	Test_LTEgressId.gressid[1] = 0x11;
	memcpy(Test_LTEgressId.gressid+2, hostmac_eth0, ADDR_LEN);
	ret = ioctl(fd, CFM_SETEGRESSID, &Test_LTEgressId);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_LTEgressId.meid = 1;
	ret = ioctl(fd, CFM_GETEGRESSID, &rTest_LTEgressId);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("EgressId:\n");
	for(i=0;i<8;i++){
		printf("%.2x ", rTest_LTEgressId.gressid[i]);
	}
	printf("\n");
}
#endif

#if 0
{
	struct Test_LTSenderIDPermission_st permission;
	permission.flags = 1;
	permission.meid = 1;
	//set
	ret = ioctl(fd,CFM_SETLTM_SENDERID_PERMISSION, &permission);
	if(ret != 0)
		printf("ioctl error\n");
	//get
	permission.meid = 1;
	ret = ioctl(fd,CFM_GETLTM_SENDERID_PERMISSION, &permission);
	if(ret != 0)
		printf("ioctl error\n");
	else
		printf("LTM SenderID permission :%d\n", permission.flags);
}
#endif

#if 0
{
	struct Test_LTSenderIDPermission_st permission;
	memset(&permission, 0, sizeof(struct Test_LTSenderIDPermission_st));
	permission.flags = 1;
	permission.meid = 1;
	//set
	ret = ioctl(fd,CFM_SETLTR_SENDERID_PERMISSION, &permission);
	if(ret != 0)
		printf("ioctl error\n");
	//get
	memset(&permission, 0, sizeof(struct Test_LTSenderIDPermission_st));
	permission.meid = 1;
	ret = ioctl(fd,CFM_GETLTR_SENDERID_PERMISSION, &permission);
	if(ret != 0)
		printf("ioctl error\n");
	else
		printf("LTR SenderID permission :%d\n", permission.flags);
}
#endif

#if 0
{
	struct Test_LTStart_st Test_LTStart;
	char Dest_mac[6] = {0x00,0x0C,0x29,0xF1,0x0C,0xC6};
	memset(&Test_LTStart, 0, sizeof(struct Test_LTStart_st));
	Test_LTStart.meid = 1;
	memcpy(Test_LTStart.MacAddr, Dest_mac, ADDR_LEN);
	ret = ioctl(fd, CFM_LT_START, &Test_LTStart);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif

#if 0
{
	struct result_param result;
	int i=0,j=0;
	char Dest_mac[6] = {0x00,0x0C,0x29,0xF1,0x0C,0xC6};

	memset(&result,0, sizeof(struct result_param));
	result.meid = 1;

	memcpy(result.mac,Dest_mac,ADDR_LEN);

	ret = ioctl(fd,CFM_LT_GETRESULT, &result);
	if(ret != 0)
	{
		printf("ioctl error!\n");
	}
	if(ret == 1)
		printf("link trace's node number is beyond 10, so not all node printed!\n");
	printf("LT result:\n");
	printf("mac_addr           TTL               Ingress Action        IngressMac          Egress Action           EgressMac          \n");
	for(;i< result.node_num;i++)
	{
		for(;j<ADDR_LEN;j++)
			printf("%2.2x",result.node[i].mac_addr);
		printf("          %d",result.node[i].TTL);
		printf("            %d", result.node[i].ingress_action);
		printf("            ");
		for(j=0;j<ADDR_LEN;j++)
				printf("%2.2x", result.node[i].ingressMAC);
	
		printf("       %d",result.node[i].egress_action);
			for(j=0;j<ADDR_LEN;j++)
				printf("%2.2x", result.node[i].egressMAC);
		
		printf("\n");
		
	}
}
#endif

/*LB Management API Test*/
#if 0
{
	printf("/********LB Management API Test*************/\n");
	struct Test_LBAvaliable_st Test_LBAvaliable;
	struct Test_LBAvaliable_st rTest_LBAvaliable;
	memset(&Test_LBAvaliable, 0, sizeof(struct Test_LBAvaliable_st));
	memset(&rTest_LBAvaliable, 0, sizeof(struct Test_LBAvaliable_st));
//set LBAvaliable
	Test_LBAvaliable.meid = 1;
	Test_LBAvaliable.LBAvaliable = 1;
	ret = ioctl(fd, CFM_LB_SETLBAVALIABLE, &Test_LBAvaliable);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get LBAvaliable
	rTest_LBAvaliable.meid = 1;
	ret = ioctl(fd, CFM_LB_GETLBAVALIABLE, &rTest_LBAvaliable);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LBAvaliable:%d\n", rTest_LBAvaliable.LBAvaliable);
}
#endif
#if 0
{
	struct Test_LBCount_st Test_LBCount;
//ErrorRcvdLBRCounts
	memset(&Test_LBCount, 0, sizeof(struct Test_LBCount_st));
	Test_LBCount.meid = 1;
	ret = ioctl(fd, CFM_LB_GETERRORRCVDLBRCOUNTS, &Test_LBCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("ErrorRcvdLBRCounts:%d\n", Test_LBCount.count);
//CorrectRcvdLBRCounts
	memset(&Test_LBCount, 0, sizeof(struct Test_LBCount_st));
	Test_LBCount.meid = 1;
	ret = ioctl(fd, CFM_LB_GETCORRECTRCVDLBRCOUNTS, &Test_LBCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("CorrectRcvdLBRCounts:%d\n", Test_LBCount.count);
//XmitLBMCounts
	memset(&Test_LBCount, 0, sizeof(struct Test_LBCount_st));
	Test_LBCount.meid = 1;
	ret = ioctl(fd, CFM_LB_GETXMITLBMCOUNTS, &Test_LBCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("XmitLBMCounts:%d\n", Test_LBCount.count);
//XmitLBRCounts
	memset(&Test_LBCount, 0, sizeof(struct Test_LBCount_st));
	Test_LBCount.meid = 1;
	ret = ioctl(fd, CFM_LB_GETXMITLBRCOUNTS, &Test_LBCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("XmitLBRCounts:%d\n", Test_LBCount.count);
//RcvdLBMCounts
	memset(&Test_LBCount, 0, sizeof(struct Test_LBCount_st));
	Test_LBCount.meid = 1;
	ret = ioctl(fd, CFM_LB_GETRCVDLBMCOUNTS, &Test_LBCount);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("RcvdLBMCounts:%d\n", Test_LBCount.count);
}
#endif
#if 0
{
	struct Test_LBTransID_st Test_LBTransID;
	memset(&Test_LBTransID, 0, sizeof(struct Test_LBTransID_st));
	Test_LBTransID.meid = 1;
	ret = ioctl(fd, CFM_LB_GETNEXTLBTRANSID, &Test_LBTransID);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("NextLBTransID:%d\n", Test_LBTransID.lbid);
}
#endif
#if 0
{
struct Test_LBPdu_st Test_LBPdu;
//LastRcvdLBM
	memset(&Test_LBPdu, 0, sizeof(struct Test_LBPdu_st));
	Test_LBPdu.meid = 1;
	ret = ioctl(fd, CFM_LB_GETLASTRCVDLBM, &Test_LBPdu);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LastRcvdLBM:\n");
	print_hex_data(Test_LBPdu.pkt_data, Test_LBPdu.pkt_len);
//LastRcvdLBR
	memset(&Test_LBPdu, 0, sizeof(struct Test_LBPdu_st));
	Test_LBPdu.meid = 1;
	ret = ioctl(fd, CFM_LB_GETLASTRCVDLBR, &Test_LBPdu);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LastRcvdLBR:\n");
	print_hex_data(Test_LBPdu.pkt_data, Test_LBPdu.pkt_len);

//LastSentLBM
	memset(&Test_LBPdu, 0, sizeof(struct Test_LBPdu_st));
	Test_LBPdu.meid = 1;
	ret = ioctl(fd, CFM_LB_GETLASTSENTLBM, &Test_LBPdu);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LastSentLBM:\n");
	print_hex_data(Test_LBPdu.pkt_data, Test_LBPdu.pkt_len);
//LastSentLBR
	memset(&Test_LBPdu, 0, sizeof(struct Test_LBPdu_st));
	Test_LBPdu.meid = 1;
	ret = ioctl(fd, CFM_LB_GETLASTSENTLBR, &Test_LBPdu);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LastSentLBR:\n");
	print_hex_data(Test_LBPdu.pkt_data, Test_LBPdu.pkt_len);
}

#endif
#if 0
{
	struct Test_LBTimeout_st Test_LBTimeout;
	struct Test_LBTimeout_st rTest_LBTimeout;
	memset(&Test_LBTimeout, 0, sizeof(struct Test_LBTimeout_st));
	memset(&rTest_LBTimeout, 0, sizeof(struct Test_LBTimeout_st));
//set
	Test_LBTimeout.meid = 1;
	Test_LBTimeout.time = 5000;
	ret = ioctl(fd, CFM_LB_SETTIMEOUT, &Test_LBTimeout);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_LBTimeout.meid = 1;
	ret = ioctl(fd, CFM_LB_GETTIMEOUT, &rTest_LBTimeout);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LB Timeout:%d\n", rTest_LBTimeout.time);
}
#endif
#if 0
{
	struct Test_LBTLVContained_st Test_LBTLVContained;
	struct Test_LBTLVContained_st rTest_LBTLVContained;
//SenderIdTLVContained
	memset(&Test_LBTLVContained, 0, sizeof(struct Test_LBTLVContained_st));
	memset(&rTest_LBTLVContained, 0, sizeof(struct Test_LBTLVContained_st));
//set
	Test_LBTLVContained.meid = 1;
	Test_LBTLVContained .contained = 1;
	ret = ioctl(fd, CFM_LB_SETSNDIDTLVCONTAINED, &Test_LBTLVContained);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_LBTLVContained.meid = 1;
	ret = ioctl(fd, CFM_LB_GETSNDIDTLVCONTAINED, &rTest_LBTLVContained);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("SenderIdTLVContained:%d\n", rTest_LBTLVContained.contained);
//DataTLVContained
	memset(&Test_LBTLVContained, 0, sizeof(struct Test_LBTLVContained_st));
	memset(&rTest_LBTLVContained, 0, sizeof(struct Test_LBTLVContained_st));
//set
	Test_LBTLVContained.meid = 1;
	Test_LBTLVContained .contained = 1;
	ret = ioctl(fd, CFM_LB_SETDATATLVCONTAINED, &Test_LBTLVContained);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_LBTLVContained.meid = 1;
	ret = ioctl(fd, CFM_LB_GETDATATLVCONTAINED, &rTest_LBTLVContained);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("DataTLVContained:%d\n", rTest_LBTLVContained.contained);
//OrgSpecificTLVContained
	memset(&Test_LBTLVContained, 0, sizeof(struct Test_LBTLVContained_st));
	memset(&rTest_LBTLVContained, 0, sizeof(struct Test_LBTLVContained_st));
//set
	Test_LBTLVContained.meid = 1;
	Test_LBTLVContained.contained = 0;
	ret = ioctl(fd, CFM_LB_SETORGSPCIFICTLVCONTAINED, &Test_LBTLVContained);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_LBTLVContained.meid = 1;
	ret = ioctl(fd, CFM_LB_GETORGSPCIFICTLVCONTAINED, &rTest_LBTLVContained);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("OrgSpecificTLVContained:%d\n", rTest_LBTLVContained.contained);
}
#endif
#if 0
{
	struct Test_LBDataTLV_st Test_LBDataTLV;
	struct Test_LBDataTLV_st rTest_LBDataTLV;
	memset(&Test_LBDataTLV, 0, sizeof(struct Test_LBDataTLV_st));
	memset(&rTest_LBDataTLV, 0, sizeof(struct Test_LBDataTLV_st));
//set
	Test_LBDataTLV.meid = 1;
	Test_LBDataTLV.pkt_len = 10;
	for(i=0;i<Test_LBDataTLV.pkt_len;i++){
		Test_LBDataTLV.pkt_data[i] = 0x11;
	}
	ret = ioctl(fd, CFM_LB_SETDATATLV, &Test_LBDataTLV);
	if(ret != 0){
		printf("ioctl error\n");
	}
//get
	rTest_LBDataTLV.meid = 1;
	ret = ioctl(fd, CFM_LB_GETDATATLV, &rTest_LBDataTLV);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("DataTLV:\n");
	print_hex_data(rTest_LBDataTLV.pkt_data, rTest_LBDataTLV.pkt_len);
}
#endif
#if 0
{
	struct Test_LBStart_st Test_LBStart;
	char Dest_mac[6] = {0x00,0x0C,0x29,0x9F,0x98,0xD7};
	memset(&Test_LBStart, 0, sizeof(struct Test_LBStart_st));
	Test_LBStart.meid = 1;
	//to do
	memcpy(Test_LBStart.MacAddr, Dest_mac, ADDR_LEN);
	Test_LBStart.lbmstosend = 5;
	ret = ioctl(fd, CFM_LB_START, &Test_LBStart);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
#if 0
{
	struct Test_LBGetResult_st Test_LBGetResult;
	memset(&Test_LBGetResult, 0, sizeof(struct Test_LBGetResult_st));
	Test_LBGetResult.meid = 1;
	ret = ioctl(fd, CFM_LB_GETRESULT, &Test_LBGetResult);
	if(ret != 0){
		printf("ioctl error\n");
	}
	printf("LB result:%d\n", Test_LBGetResult.result);
}
#endif
/*management API test end*/
	while(!Exit)
	{
		fread_out = fread_in;
		ret = select(sock_max+1, &fread_out,NULL,NULL,&tv);
		if (ret > 0)
		{	
			if (FD_ISSET(sock_eth0, &fread_out))
			{
				recv_process(fd, sock_eth0, "eth0");
			}
#if TWONIC
			if (FD_ISSET(sock_eth1, &fread_out))
			{
				recv_process(fd, sock_eth1, "eth1");
			}
#endif
		}
#if TWONIC
		send_process(fd, sock_eth0, sock_eth1);
#else
		send_process(fd, sock_eth0, 0);
#endif
		}
#if 0
{
	ret = ioctl(fd, CFM_STOP);
	if(ret != 0){
		printf("ioctl error\n");
	}
}
#endif
	
end:
	if(sock_eth0 != -1){
		close(sock_eth0);
	}
#if TWONIC
	if(sock_eth1 != -1){
		close(sock_eth1);
	}
#endif
	if(fd != -1){
		close(fd);
	}
	
	if(MD5Pkt_queue){
		if(MD5Pkt_queue->MD5Packet_list){
			free(MD5Pkt_queue->MD5Packet_list);
		}
		free(MD5Pkt_queue);
	}


	return 0;
 
}

int MD5_process(unsigned char * buffer, int len, int optFlag)
{
	int i;
	int ret = 0;
	int resultLen;
	unsigned char md5Result[64];

	//printf("in MD5_process\n");
	resultLen = MD5_convert(buffer, len, md5Result);
	switch(optFlag){
	case 0:
		for(i=0;i<MD5LIST_SIZE;i++){
			if((resultLen == MD5Pkt_queue->MD5Packet_list[i].md5Len)&&(memcmp(MD5Pkt_queue->MD5Packet_list[i].md5Result, md5Result, resultLen) == 0)){
				ret = 1;
				return ret;
			}
		}
		break;
	case 1:
		memcpy(MD5Pkt_queue->MD5Packet_list[MD5Pkt_queue->temp].md5Result, md5Result, 64);
		MD5Pkt_queue->temp = (MD5Pkt_queue->temp + 1)%MD5LIST_SIZE;
		//printf("MD5Pkt_queue->temp:%d\n", MD5Pkt_queue->temp);
		break;
	default:
		printf("error optFlag\n");
		break;
	}
	
	return 0;
}

