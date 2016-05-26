/*
LinkTrace Protocol implement file.
现在的做法：
不同类型的MP收到LTM后，若是目的MP，则回复LTR，终止。
若不是目的MP，则根据情况需要转发。
对于MEP，只需回复LTR，不需转发LTM。
对于MIP，收到LTM后，查询Egress port，并根据Egress Port上配置的MP，分别处理。
若需要Egress Port上的MP处理，则直接将LTM转发出去，让Egress Port上的MP处理；
若不需要Egress Port上的MP处理，则在本MP上处理，处理完后，将改变后的LTM后原LTM消息继续传递下去。

LTM消息会沿着逻辑路线经过每个MP，若MP对他不处理，则继续传递下去，若处理了，可能会终止LTM的传递，也有可能会修改LTM之后转发出去。
*/

#include <linux/fs.h>
#include "cfm_protocol.h"
#include "cfm_header.h"
#include "cfm_tlv.h"

#define LTM_LEN 22
#define LTR_LEN 11
#define TRANSID_LEN 4
#define TTL_LEN 1
#define REPLY_LIST_SIZE 20
#define RESULT_LIST_SIZE 60
#define LTMTTL 64
#define LTR_MDELAY 1000
#define LTR_LIST_MAXSIZE 10
#define RlyHit 1
#define RlyFDB 2
#define RlyMPDB 3
#define FIRST_TLV  ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN+TTL_LEN+2*ADDR_LEN
#define useFDBonly  0x00
#define useALL 0x00
#define FwdYes  0x40
#define TerminalMEP 0x20
#define LTM_EGRESS_IDENTIFIER_TLV_LEN 11
#define LTR_EGRESS_IDENTIFIER_TLV_LEN  19

static dataStream_t ltm_format(uint8 source_address[6],uint8 priority,int vlanid,ltmpdu_t ltmpdu,uint32 tlv_count,...);
static int xmitLTM(uint8 *pkt_data, uint32 pkt_len,uint32 bridge_id,uint32 flow_id,int flags, MEP_t mep);
static dataStream_t ltr_format(uint8 * pkt_data,int flags,void* mp,uint8 mptype,uint8 priority,ltrpdu_t ltrpdu ,uint32 tlv_count,...);
static ltrpdu_t create_ltr_pdu(uint8 * pkt_data, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,int relay_action);
static int ltm_validate(uint8 * pkt_data ,int flags);
static int upmhf_process(uint8 * pkt_data,uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection);
static int downmhf_process(uint8 * pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection);
static int upmep_process(uint8 * pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection);
static int downmep_process(uint8 * pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId, int flags, void*  mp,uint8 mptype, uint8* result, int result_size, int* result_len, uint8 srcDirection);
static int mp_type_check(uint8 * pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype, uint8* result, int result_size, int* result_len, uint8 srcDirection);
static int is_equal_addr(uint8* pkt_data, int flags, void* mp,uint8 mptype);
static int forward_ltm(uint8 * pkt_data,uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len);
static int saveTo_ltemReplyList(MEP_t mep,uint8* pkt_data,uint32 pkt_len);
////////////////////////////////////
//创建TLV
dataStream_t create_LTR_Egress_Identifier_TLV(uint8* pkt_data, int flags, void* mp,uint8 mptype);
dataStream_t create_Reply_Ingress_TLV(void* mp,uint8 mptype);
dataStream_t create_Reply_Egress_TLV(void* mp,uint8 mptype);
dataStream_t create_LTM_Egress_Identifier_TLV(void* mp,uint8 mptype);

//查询Egress Port
static bool search_mp_onport(uint16 port, uint8 direction,uint8 mptype,int MDlevel);
static bool search_egress_port(uint8 * pkt_data,int flags,uint16* egress_port_id,int* type);
static bool check_spanning_forward(uint16 PortID);

static int save_ltemReplyList_to_linkTraceList(ltemReplyList_t source, linkTraceList_t dest);
static ltr_state_machine_t ltr_state_machine_create(ltpm_t ltpm,int mdelay);
//运行这个状态机 定时器处理函数
static void ltr_state_machine_run(unsigned long machine_run);
//LTR 的发送函数
static int xmitOldestLTR(void* mp,uint8 mptype, uint8 *pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId,int srcDirection);
//向LTR队列中插入LTR
static int ltr_list_append(ltr_state_machine_t ltr_machine, uint8 *pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId,int srcDirection);
//销毁状态机
static int ltr_state_machine_destroy(ltr_state_machine_t ltr_machine);
static void ltmr_time_out(unsigned long timer_param);
static dataStream_t create_SenderID_TLV(void* mp,uint8 mptype);
/*static int ltpm_tlv_init(ltpm_t ltpm);

static int ltpm_tlv_init(ltpm_t ltpm)
{
	if(ltpm == NULL){
		return -1;
	}
	ltpm->Reply_Ingress_TLV.type = type_Reply_Ingress_TLV;
	ltpm->Reply_Egress_TLV.type = type_Reply_Egress_TLV;
	return 0;
}
*/
//////////////////////////////////////////////////////上层接口函数 ///////////////////////////////////////////////////////////////////////////////
//启动LT协议模块
ltpm_t ltpm_init(void* mp,uint8 mptype)
{
    ltpm_t ltpm;
	int i;
	
	ltpm = (ltpm_t)kmalloc(sizeof(struct ltpm_st), GFP_KERNEL);
	if(ltpm == NULL){
		printk("fail to kmalloc ltpm\n");
		return NULL;
	}
	memset(ltpm, 0, sizeof(struct ltpm_st));
	
	ltpm->mp = mp;
	ltpm->mptype=mptype;
	ltpm->index =0;
	ltpm->reply_list_size = REPLY_LIST_SIZE; 
	ltpm->reply_list = (ltemReplyList_t)kmalloc(sizeof(struct ltemReplyList_st) * ltpm->reply_list_size, GFP_KERNEL);
	if(ltpm->reply_list == NULL){
		printk("fail to kmalloc ltpm->reply_list\n");
		kfree(ltpm);
		ltpm = NULL;
		return NULL;
	}
	memset(ltpm->reply_list, 0 ,sizeof(struct ltemReplyList_st) * ltpm->reply_list_size);
	for (i=0;i<ltpm->reply_list_size;i++)
	{
		LIST_HEAD_INIT(&ltpm->reply_list[i].ltmr_list);
	}
	//初始化 结果链表
	ltpm->result_index = 0;
	ltpm->result_list_size = RESULT_LIST_SIZE;
	ltpm->result_list = (linkTraceList_t)kmalloc(sizeof(struct linkTraceList_st)*ltpm->result_list_size, GFP_KERNEL);
	if (ltpm->result_list == NULL)
	{
		printk("fail to kmalloc ltpm->result_list\n");
		if(ltpm->reply_list){
			kfree(ltpm->reply_list);
			ltpm->reply_list = NULL;
		}
		kfree(ltpm);
		ltpm = NULL;
		return NULL;
	}
	memset(ltpm->result_list,0,sizeof(struct linkTraceList_st)*ltpm->result_list_size);
	for (i=0;i<ltpm->result_list_size;i++)
	{
		LIST_HEAD_INIT(&ltpm->result_list[i].trace_list);
	}
	ltpm->LTMttl = LTMTTL;
	ltpm->flags = useFDBonly;
	ltpm->ltr_machine = ltr_state_machine_create(ltpm,LTR_MDELAY);
	if(ltpm->ltr_machine == NULL){
		printk("failt to create ltr state machine\n");
		if(ltpm->reply_list){
			kfree(ltpm->reply_list);
			ltpm->reply_list = NULL;
		}
		if(ltpm->result_list){
			kfree(ltpm->result_list);
			ltpm->result_list = NULL;
		}
		return NULL;
	}
//	ltpm_tlv_init(ltpm);
	ltpm->LTM_SenderID_Permission = false;
	ltpm->LTR_SenderID_Permission = false;
	return ltpm ;
}

///销毁LT协议模块
int ltpm_destroy(ltpm_t ltpm)
{
	int i;
	ltmReplyListNode_t tmp, pre;
	linkTraceNode_t next;
	
	if(ltpm == NULL){
		printk("ltpm is NULL");
		return -1;
	}

	if(ltpm->reply_list){
		for (i=0;i<ltpm->reply_list_size;i++){
			tmp = LIST_FIRST(&ltpm->reply_list[i].ltmr_list);
			while(NULL != tmp){
				pre = tmp;
				tmp = LIST_NEXT(tmp, list);
				kfree(pre);
				pre = NULL;	
			}
			LIST_HEAD_DESTROY(&ltpm->reply_list[i].ltmr_list);
			if(ltpm->reply_list[i].timer){
				printk("destroy timer\n");
				cfm_timer_destroy(ltpm->reply_list[i].timer);
			}
		}
		kfree(ltpm->reply_list);
		ltpm->reply_list = NULL;
	}

	//释放 结果链表
	if (ltpm->result_list)
	{
		for (i=0;i<ltpm->result_list_size;i++)
		{
			
			LIST_TRAVERSE_SAFE_BEGIN(&ltpm->result_list[i].trace_list, next, list)
			LIST_REMOVE_CURRENT(&ltpm->result_list[i].trace_list, list);
			LIST_TRAVERSE_SAFE_END
			LIST_HEAD_DESTROY(&ltpm->result_list[i].trace_list);
		}
		kfree(ltpm->result_list);
		ltpm->result_list = NULL;
	}

	if(ltpm->ltr_machine){
		ltr_state_machine_destroy(ltpm->ltr_machine);
	}

	kfree(ltpm);
	ltpm = NULL;

	return 0;
}


//***************************************************LTM 处理**********************************************************////////////////
//具体的内部函数

//向目的地MP 发起一个LTM
int ltpm_start(uint8 *tarAddr,MEP_t mep)
{
	//int i;
	struct ltmpdu_st * ltmpdu;
	dataStream_t ltmds,ltrds;
	ltrpdu_t ltrpdu;
	dataStream_t Egress_ID_TLV;
	dataStream_t SenderID_TLV=NULL;
	printk("in ltpm_start\n");
	//创建LTM PDU
	ltmpdu = (struct ltmpdu_st *)kmalloc(sizeof(struct ltmpdu_st),GFP_KERNEL);
	if(ltmpdu == NULL) {
		printk("fail to kmalloc ltmpdu\n");
		return -1;
	}
	memset(ltmpdu,0,sizeof(struct ltmpdu_st));
	ltmpdu->header = generateCFMHeader(mep->MEPBasic.ma->MDPointer->MDLevel,type_LTM,mep->ltpm->flags);  //要根据mp类型或设置来确定flags
	if(ltmpdu->header == NULL){
		printk("fail to generate CFM Header\n");
		kfree(ltmpdu);
		ltmpdu = NULL;
		return -1;
	}
	ltmpdu->ltmTransID = mep->ltpm->nextLTMtransID;
	ltmpdu->ltmTtl = mep->ltpm->LTMttl;
	memcpy(ltmpdu->oriAddr, mep->MEPStatus.MACAddress, ADDR_LEN);
	memcpy(ltmpdu->tarAddr, tarAddr, ADDR_LEN);
	//添加LTM Egress Identifier TLV
	Egress_ID_TLV = create_LTM_Egress_Identifier_TLV(mep,MEP);
	if (Egress_ID_TLV == NULL)
	{
		printk("create LTM Egress Identifier TLV failed \n");
		if(ltmpdu){
			if(ltmpdu->header){
				kfree(ltmpdu->header);
				ltmpdu->header = NULL;
			}
			kfree(ltmpdu);
			ltmpdu = NULL;
		}
		return -1;
	}

	if(mep->ltpm->LTM_SenderID_Permission)     //need to include SenderID TLV
	{
		printk("permission is ok\n");
		SenderID_TLV = create_SenderID_TLV(mep,MEP);
		if(SenderID_TLV == NULL)
			ltmds = ltm_format(mep->MEPStatus.MACAddress, 1, mep->MEPBasic.PrimaryVlan, ltmpdu, 1,Egress_ID_TLV);
		else
			ltmds = ltm_format(mep->MEPStatus.MACAddress, 1, mep->MEPBasic.PrimaryVlan, ltmpdu, 2,Egress_ID_TLV,SenderID_TLV);
	}
	else
	{
		ltmds = ltm_format(mep->MEPStatus.MACAddress, 1, mep->MEPBasic.PrimaryVlan, ltmpdu, 1,Egress_ID_TLV);
	}
	print_hex_data(ltmds->data, ltmds->length);

	if(ltmpdu){
		if(ltmpdu->header){
			kfree(ltmpdu->header);
			ltmpdu->header = NULL;
		}
		kfree(ltmpdu);
		ltmpdu = NULL;
	}

	if (Egress_ID_TLV)
	{
		kfree(Egress_ID_TLV);
	}

	if(SenderID_TLV){
		print_hex_data(SenderID_TLV->data, SenderID_TLV->length);
		kfree(SenderID_TLV);
	}

	if(ltmds == NULL){
		printk("format error\n");
		return -1;
	}

	//若发起者是UP MEP，则查找Egress Port ,添加Reply Ingress TLV 和Reply Egress TLV
	if (mep->Direction== UPMP)
	{
		uint16 egress_port_id;
		int type;
		if (search_egress_port(ltmds->data,1,&egress_port_id,&type))
		{
			if (!search_mp_onport(egress_port_id,UPMP,MEP,mep->MEPBasic.ma->MDPointer->MDLevel)||!search_mp_onport(egress_port_id,UPMP,MIP,mep->MEPBasic.ma->MDPointer->MDLevel))
			{
				ltrpdu = create_ltr_pdu(ltmds->data, 0,0,1,mep,MEP,type);
				if(ltrpdu == NULL){
					printk("ltrpdu is NULL\n");
					return -1;
				}
				ltrds = ltr_format(ltmds->data,1,mep,MEP,1,ltrpdu,0);
				if(ltrpdu){
					if(ltrpdu->header){
						kfree(ltrpdu->header);
						ltrpdu->header = NULL;
					}
					kfree(ltrpdu);
					ltrpdu = NULL;
				}
				if(ltrds == NULL){
					return -1;
				}
				//该放进队列中然后发出按照协议处理发出去   
				/*ltr_list_append(mp->ltpm->ltr_machine,ltrds->data,ltrds->length, 0, 0,Inside);*/
				//将自身产生的ltr 加入到ltemReplyList中
				printk("up mep send ltm,put into ltr list,and forward it\n");
				saveTo_ltemReplyList(mep,ltrds->data,ltrds->length);
				if(ltrds){
					kfree(ltrds);
					ltrds = NULL;
				}
				//判断是否可以转发,ttl ?= 1
				if (*(ltmds->data+ETHERNETHEAD_LEN+1*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN) == 1)
				{

					return 0;
				}
				//可以转发
				cfm_send(ltmds->data,ltmds->length,mep->srcPortId,mep->FlowId,mep,MEP,Inside,SENDTO);
				if(ltmds){
					kfree(ltmds);
					ltmds = NULL;
				}
				mep->ltpm->LTMcount++;
				return 0;
			}
			//将LTM继续传下去发送ltm

		}

	}
	xmitLTM(ltmds->data,ltmds->length,mep->srcPortId,mep->FlowId,1,mep);
	mep->ltpm->LTMcount++;
	if(ltmds){
		kfree(ltmds);
		ltmds = NULL;
	}

	return 0;
}

static int saveTo_ltemReplyList(MEP_t mep, uint8* pkt_data, uint32 pkt_len)
{
	ltemReplyList_t reply_list;
	ltmReplyListNode_t temp;
	ltmReplyListNode_t tmp,pre;
	struct timeout_param* param=NULL;

	if((pkt_data == NULL)||mep == NULL){
		printk("pkt_dat is NULL||mep is NULL");
		return -1;
	}

	//下面应对链表进行操作
	reply_list = mep->ltpm->reply_list;

	//如果数组已经满了，则从第一个开始覆盖
	if(mep->ltpm->index > mep->ltpm->reply_list_size-1)
	{
		mep->ltpm->index =0;		
	}
	reply_list[mep->ltpm->index].TransID = mep->ltpm->nextLTMtransID;
	reply_list[mep->ltpm->index].is_time_out = false;
	reply_list[mep->ltpm->index].success_flag = false;
	memcpy(reply_list[mep->ltpm->index].dest_mac,pkt_data+33,ADDR_LEN);       //将目标MEP的地址记录下来
	//如果链表里有数据，就先删除，再填充
	if (!LIST_EMPTY(&reply_list[mep->ltpm->index].ltmr_list))
	{
		tmp = LIST_FIRST(&reply_list[mep->ltpm->index].ltmr_list);
		while(NULL != tmp){
			pre = tmp;
			tmp = LIST_NEXT(tmp, list);
			kfree(pre);
			pre =NULL;
		}
		//把原来的定时器 销毁
		if(reply_list[mep->ltpm->index].timer !=NULL)
			cfm_timer_destroy(reply_list[mep->ltpm->index].timer);
	}
	//填充新的节点
	temp = (ltmReplyListNode_t)kmalloc(sizeof(struct ltmReplyListNode_st), GFP_KERNEL);
	memset(temp, 0, sizeof(struct ltmReplyListNode_st));
	memcpy(temp->node, pkt_data, pkt_len);
	temp->length = pkt_len;
	LIST_INSERT_TAIL(&(reply_list[mep->ltpm->index].ltmr_list), temp, list);

	mep->ltpm->nextLTMtransID++;
	//开启5s定时器
	param = (struct timeout_param*)kmalloc(sizeof(struct timeout_param),GFP_KERNEL);
	if(param ==NULL)
	{
		printk("timer param kmalloc failed in xmitLTM()\n");
		return -1;
	}
	param->list = &reply_list[mep->ltpm->index];
	param->mep = mep;
	reply_list[mep->ltpm->index].timer = cfm_timer_create(ltmr_time_out,5000,(long)param);
	mep->ltpm->index++;
	return 0;
}


//组建各种TLV
dataStream_t create_LTM_Egress_Identifier_TLV(void* mp,uint8 mptype)
{
	dataStream_t Egress_ID_TLV;

	Egress_ID_TLV = (dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if (Egress_ID_TLV == NULL)
	{
		printk("failed to kmalloc egress id tlv in create_LTM_Egress_Identifier_TLV()\n");
		return NULL;
	}
	memset(Egress_ID_TLV,0,sizeof(struct dataStream_st));
	Egress_ID_TLV->length=LTM_EGRESS_IDENTIFIER_TLV_LEN;
	Egress_ID_TLV->data[0]= 7;							//type  7
	Egress_ID_TLV->data[1] =0;
	Egress_ID_TLV->data[2] =8;   //length 8
	if(mptype==MEP)
	{
              MEP_t mep;
		mep=(MEP_t)mp;
	       memcpy(Egress_ID_TLV->data+5,mep->MEPStatus.MACAddress,ADDR_LEN);
	}
	else
	{
	       MIP_t mip;
		mip=(MIP_t)mp;
		memcpy(Egress_ID_TLV->data+5,mip->MACAddr,ADDR_LEN);
	}

	return Egress_ID_TLV;
}

static dataStream_t create_SenderID_TLV(void* mp, uint8 mptype )
{
	dataStream_t SenderID_TLV;
	uint8  sender_id_permission;
	if(mptype==MEP){
		MEP_t mep;
		mep=(MEP_t)mp;
		sender_id_permission=mep->MEPBasic.ma->SenderIDPermission;
		}
	else{
		MIP_t mip;
		mip=(MIP_t)mp;
		sender_id_permission=mip->ma->SenderIDPermission;
		}		
	printk("in LT create_SenderID_TLV\n");
	SenderID_TLV = (dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if (SenderID_TLV == NULL)
	{
		printk("failed to kmalloc LT sender_id TLV\n");
		return NULL;
	}
	memset(SenderID_TLV,0,sizeof(struct dataStream_st));
	if(sender_id_permission==1)
	{
	       printk("No sender_id_TLV  demanded");
	}
	if(sender_id_permission==2)
	{
	       SenderID_TLV->data[0] = 0x01;
		SenderID_TLV->data[1] = 0;
		SenderID_TLV->data[2] = gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+ 2;
		SenderID_TLV->data[3] = gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
		SenderID_TLV->data[4] = gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
		if(gCfm->TLV.Sender_ID_TLV.chassis_ID_Length> 0)
			memcpy(SenderID_TLV->data+5, gCfm->TLV.Sender_ID_TLV.chassis_ID, gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
		SenderID_TLV->length = gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+5;
	}
	if(sender_id_permission==3)
	{
              SenderID_TLV->data[0] = 0x01;
		uint16_to_uint8((SenderID_TLV->data+1), (gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length
			+ gCfm->TLV.Sender_ID_TLV.management_Address_Length + 3));
		SenderID_TLV->data[3] = 0x00;
		SenderID_TLV->data[4] = gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
		memcpy(SenderID_TLV->data+5, gCfm->TLV.Sender_ID_TLV.management_Address_Domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
		SenderID_TLV->data[gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+5] =gCfm->TLV.Sender_ID_TLV.management_Address_Length;
		memcpy(SenderID_TLV->data+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+6, gCfm->TLV.Sender_ID_TLV.management_Address,
			      gCfm->TLV.Sender_ID_TLV.management_Address_Length);
		SenderID_TLV->length = gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Length +6;
	}
	if(sender_id_permission==4)
	{
	       SenderID_TLV->data[0] = 0x01;
		uint16_to_uint8((SenderID_TLV->data+1), (gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + 2+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length
			+gCfm->TLV.Sender_ID_TLV.management_Address_Length + 2));
		SenderID_TLV->data[3] = gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
		SenderID_TLV->data[4] =gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
		memcpy(SenderID_TLV->data+5, gCfm->TLV.Sender_ID_TLV.chassis_ID, gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
		SenderID_TLV->data[gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+5] =gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
		memcpy(SenderID_TLV->data+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+6,gCfm->TLV.Sender_ID_TLV.management_Address_Domain,
			gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
		SenderID_TLV->data[gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+6] =gCfm->TLV.Sender_ID_TLV.management_Address_Length;
		memcpy(SenderID_TLV->data + gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+7, gCfm->TLV.Sender_ID_TLV.management_Address,
			gCfm->TLV.Sender_ID_TLV.management_Address_Length);
		SenderID_TLV->length = gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + 2+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Length + 5;

	}
	print_hex_data(SenderID_TLV->data, SenderID_TLV->length);
	return SenderID_TLV;
}

//组建LTM消息，并格式化成字符流
static dataStream_t ltm_format(uint8 source_address[6],uint8 priority,int vlanid,ltmpdu_t ltmpdu,uint32 tlv_count,...)
{
	int i,tlv_total_len=0,cnt=0 ;
	uint32 j ;
	va_list ap ;
	dataStream_t ltmds,tlv ;
	uint8 desAddr[6]={0x01,0x80,0xc2,0x00,0x00,0x30};
	desAddr[5]=(ltmpdu->header->mdLevel_version>>5)+8+0x30;
	

	ltmds=(dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if(ltmds == NULL){
		printk("fail to kmalloc datastream\n");
		return NULL;
	}
	memset(ltmds,0,sizeof(struct dataStream_st));
	va_start(ap,tlv_count);
	for(j=1;j<=tlv_count;j++)
	{
		tlv=va_arg(ap,dataStream_t);
		tlv_total_len+=tlv->length ;
	}

	ltmds->length=ETHERNETHEAD_LEN+LTM_LEN+VLAN_TAG_LEN+tlv_total_len;
	memcpy(ltmds->data, desAddr, ADDR_LEN);
	memcpy(ltmds->data+ADDR_LEN, source_address ,ADDR_LEN);
	cnt=12;
	//添加vlan_tag
	ltmds->data[cnt++]=0x81 ;
	ltmds->data[cnt++]=0x00 ;
	ltmds->data[cnt++]=(uint8)(priority<<5)+(uint8)(vlanid>>8);
	ltmds->data[cnt++] = vlanid & 0xFF;
	//帧类型
	ltmds->data[cnt++]=0x89 ;
	ltmds->data[cnt++]=0x02 ;
	ltmds->data[cnt++]=ltmpdu->header->mdLevel_version ;
	ltmds->data[cnt++]=ltmpdu->header->opCode ;
	ltmds->data[cnt++]=ltmpdu->header->flags ;
	ltmds->data[cnt++]=ltmpdu->header->firstTLVOffset ;
	ltmds->data[cnt++] = (ltmpdu->ltmTransID>>24) & 0xFF;
	ltmds->data[cnt++] = (ltmpdu->ltmTransID>>16) & 0xFF;
	ltmds->data[cnt++] = (ltmpdu->ltmTransID>>8) & 0xFF;
	ltmds->data[cnt++] = (ltmpdu->ltmTransID) & 0xFF;
	ltmds->data[cnt++]=ltmpdu->ltmTtl;
	memcpy(&ltmds->data[cnt],ltmpdu->oriAddr, ADDR_LEN);
	memcpy(&ltmds->data[cnt+ADDR_LEN],ltmpdu->tarAddr, ADDR_LEN);
	cnt = cnt+12;
	va_start(ap,tlv_count);
	for(j=1;j<=tlv_count;j++)
	{
		tlv=va_arg(ap,dataStream_t);
		for(i=0;i<(tlv->length);i++)
			ltmds->data[cnt++]=tlv->data[i];
	}
	ltmds->data[cnt]=0 ;
	va_end(ap);
	return ltmds ;
}

//发送LTM
static int xmitLTM(uint8 *pkt_data, uint32 pkt_len,uint32 bridge_id,uint32 flow_id,int flags, MEP_t mep)
{
	ltemReplyList_t reply_list;
//	ltmReplyListNode_t temp;
	ltmReplyListNode_t tmp,pre;
	struct timeout_param* param=NULL;
       printk("in xmitLTM\n");
	if((pkt_data == NULL)||mep == NULL){
		printk("pkt_dat is NULL||mep is NULL");
		return -1;
	}

	//下面应对链表进行操作
	reply_list = mep->ltpm->reply_list;

	//如果数组已经满了，则从第一个开始覆盖
	if(mep->ltpm->index > mep->ltpm->reply_list_size-1)
	{
		mep->ltpm->index =0;		
	}
	reply_list[mep->ltpm->index].TransID = mep->ltpm->nextLTMtransID;
	reply_list[mep->ltpm->index].is_time_out = false;
	reply_list[mep->ltpm->index].success_flag = false;
	memcpy(reply_list[mep->ltpm->index].dest_mac,pkt_data+33,ADDR_LEN);       //将目标MEP的地址记录下来
	//如果链表里有数据，就先删除，再填充
	if (!LIST_EMPTY(&reply_list[mep->ltpm->index].ltmr_list))
	{
		tmp = LIST_FIRST(&reply_list[mep->ltpm->index].ltmr_list);
		while(NULL != tmp){
			pre = tmp;
			tmp = LIST_NEXT(tmp, list);
			kfree(pre);
			pre =NULL;
		}
	}
		//把原来的定时器 销毁
		if(reply_list[mep->ltpm->index].timer !=NULL)
			cfm_timer_destroy(reply_list[mep->ltpm->index].timer);

	//填充新的节点
	/*temp = (ltmReplyListNode_t)kmalloc(sizeof(struct ltmReplyListNode_st), GFP_KERNEL);
	memset(temp, 0, sizeof(struct ltmReplyListNode_st));
	memcpy(temp->node, pkt_data, pkt_len);
	temp->length = pkt_len;
	LIST_INSERT_TAIL(&(reply_list[mp->ltpm->index].ltmr_list), temp, list);*/

	mep->ltpm->nextLTMtransID++;
	cfm_send(pkt_data,pkt_len,bridge_id,flow_id,mep,MEP,Outside,SENDTO);
	//开启5s定时器
	param = (struct timeout_param*)kmalloc(sizeof(struct timeout_param),GFP_KERNEL);
	if(param ==NULL)
	{
		printk("timer param kmalloc failed in xmitLTM()\n");
		return -1;
	}
	param->list = &reply_list[mep->ltpm->index];
	param->mep = mep;
	reply_list[mep->ltpm->index].timer = cfm_timer_create(ltmr_time_out,5000,(long)param);
	mep->ltpm->index++;
	return 0;

}

//5s定时器到期函数
void ltmr_time_out(unsigned long timer_param)
{
	struct timeout_param*  param = (struct timeout_param*)timer_param;
	MEP_t mep=NULL;
	ltemReplyList_t reply_list=NULL;
	ltmReplyListNode_t temp=NULL,prec=NULL;
	linkTraceNode_t pre=NULL,tmp=NULL; 
	int j;
	printk("in ltmr_time_out\n");
	mep = param->mep;
	reply_list = param->list;
	
	reply_list->is_time_out = true;
	reply_list->success_flag = false;
	if(reply_list->timer != NULL)
	{
		cfm_timer_destroy(reply_list->timer);
		reply_list->timer =NULL;
	}
	//将所有结果都存储到 路径链表中
	//如果结果链表中 已经有了这个记录，则更新它 
	for (j=0;j<mep->ltpm->result_list_size;j++)
	{
		if( !memcmp(mep->ltpm->result_list[j].dest_mac,reply_list->dest_mac,ADDR_LEN))
		{
			//删除原先的链表，更新
			if ( !LIST_EMPTY(&mep->ltpm->result_list[j].trace_list))
			{
					
					LIST_TRAVERSE_SAFE_BEGIN(&mep->ltpm->result_list[j].trace_list, tmp, list)
					LIST_REMOVE_CURRENT(&mep->ltpm->result_list[j].trace_list, list);
					LIST_TRAVERSE_SAFE_END
					//tmp = LIST_NEXT(tmp,list);
					//kfree(pre);
					//pre = NULL;
				 
			}
			save_ltemReplyList_to_linkTraceList(reply_list, &mep->ltpm->result_list[j]);
			//将此过期结点初始化
			memset(reply_list->dest_mac,0,ADDR_LEN);
			if (!LIST_EMPTY(&reply_list->ltmr_list))
			{
				temp = LIST_FIRST(&reply_list->ltmr_list);
				while(NULL != temp){
					prec = temp;
					temp = LIST_NEXT(temp, list);
					kfree(prec);
					prec =NULL;
				}
			}
			goto end;
		}
			
	}
	
	///否则，存储到新的位置上
	//如果 链表上没有新的位置了，则从头开始覆盖
	if (mep->ltpm->result_index > mep->ltpm->result_list_size-1)
	{
		mep->ltpm->result_index = 0;
	}
	//如果位置上 有数据，则清除它
	if ( !LIST_EMPTY(&mep->ltpm->result_list[mep->ltpm->result_index].trace_list))
	{
		tmp = LIST_FIRST(&mep->ltpm->result_list[mep->ltpm->result_index].trace_list);
		while(tmp != NULL)
		{
			pre = tmp;
			tmp = LIST_NEXT(tmp,list);
			kfree(pre);
			pre = NULL;
		}
	}
	save_ltemReplyList_to_linkTraceList(reply_list, &mep->ltpm->result_list[mep->ltpm->result_index]);
	mep->ltpm->result_index++;
end:
	param->list = NULL;
	param->mep = NULL;
	kfree(param);
	
}
//
//收到ltm后，认证LTM是否合法
static int ltm_validate(uint8 * pkt_data ,int flags)
{
	uint8 ltm_ttl, md_level;
	
	if(pkt_data == NULL){
		printk("pkt_data is NULL\n");
		return -1;
	}
		
	ltm_ttl=*(pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN);
	md_level=(*(pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN))>>5;
	if((ltm_ttl==0)||((md_level+8)!= (pkt_data[ADDR_LEN-1]&0x0F)))
		return 0;
	else 
		return 1;
}
//比较PDU中的目标地址是不是 自己
static int is_equal_addr(uint8* pkt_data, int flags, void* mp,uint8 mptype)
{
	int ret = -1;

	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	if(mptype==MEP)
	{
        MEP_t mep;
        mep=(MEP_t)mp;
	 ret = memcmp(mep->MEPStatus.MACAddress, pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN+TTL_LEN+ADDR_LEN, ADDR_LEN);
        }
	else
	{
	  MIP_t mip;
	  mip=(MIP_t)mp;
	 ret = memcmp(mip->MACAddr, pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN+TTL_LEN+ADDR_LEN, ADDR_LEN);
	}
	if(ret == 0){
		return 1;
	}
	else{
		return 0;
	}
}
//////////////************************************ LTR 处理***************************************************************////////////////
//将LTR PDU和其他信息组建成LTR，并格式化成字符流
static dataStream_t ltr_format(uint8 * pkt_data,int flags,void* mp,uint8 mptype,uint8 priority,ltrpdu_t ltrpdu ,uint32 tlv_count,...)
{
	dataStream_t  ltrds, tlv;
	uint32 j;
	int i,tlv_total_len=0,cnt=0 ;
	va_list ap ;

	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return NULL;
	}
	ltrds=(dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if(ltrds == NULL){
		printk("fail to kmalloc ltrds\n");
		return NULL;
	}
	memset(ltrds,0,sizeof(struct dataStream_st));
	va_start(ap,tlv_count);
	for(j=1;j<=tlv_count;j++)
	{
		tlv=va_arg(ap,dataStream_t);
		tlv_total_len+=tlv->length ;
	}
	ltrds->length=ETHERNETHEAD_LEN+LTR_LEN+VLAN_TAG_LEN*flags+tlv_total_len;
	memcpy(ltrds->data,&pkt_data[ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN+TTL_LEN],6);
       if(mptype==MEP)
	{
	       MEP_t mep;
		   mep=(MEP_t)mp;
	       memcpy(ltrds->data+ADDR_LEN,mep->MEPStatus.MACAddress,ADDR_LEN);
	}
	   else
	   	{
                        MIP_t  mip;
			   mip=(MIP_t)mp;
			   memcpy(ltrds->data+ADDR_LEN,mip->MACAddr,ADDR_LEN);
	   	}
	memcpy(ltrds->data+12,&pkt_data[ETHERNETHEAD_LEN-2],flags*VLAN_TAG_LEN+2);	
	cnt=ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN;
	ltrds->data[cnt++]=ltrpdu->header->mdLevel_version ;
	ltrds->data[cnt++]=ltrpdu->header->opCode ;
	ltrds->data[cnt++]=ltrpdu->header->flags ;
	ltrds->data[cnt++]=ltrpdu->header->firstTLVOffset ;
	ltrds->data[cnt++]=(ltrpdu->ltrTransId>>24) & 0xFF;
	ltrds->data[cnt++]=(ltrpdu->ltrTransId>>16)&0xFF;
	ltrds->data[cnt++]=(ltrpdu->ltrTransId>>8)&0xFF;
	ltrds->data[cnt++]=(ltrpdu->ltrTransId) & 0xFF;
	ltrds->data[cnt++]=ltrpdu->repTtl;
	ltrds->data[cnt++]=ltrpdu->relAction;
	
	va_start(ap,tlv_count);
	for(j=1;j<=tlv_count;j++)
	{
		tlv=va_arg(ap,dataStream_t);
		for(i=0;i<(tlv->length);i++)
			ltrds->data[cnt++]=tlv->data[i];
	}
	ltrds->data[cnt++]=0 ;
	va_end(ap);
	return ltrds ;
}
//生成LTR的 PDU
static ltrpdu_t create_ltr_pdu(uint8 * pkt_data, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8  mptype,int relay_action)
{
	ltrpdu_t ltrpdu;
	uint8 header_flags;
	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return NULL;
	}
	ltrpdu=(ltrpdu_t)kmalloc(sizeof(struct ltrpdu_st),GFP_KERNEL);
	if(ltrpdu == NULL) {
		printk("fail to kmalloc ltrpdu\n");
		return NULL;
	}
	memset(ltrpdu,0,sizeof(struct ltrpdu_st));
	if (mptype!=MEP)
		{
		      header_flags = pkt_data[ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+2] | FwdYes;    //转发 
			{
				MIP_t mip=NULL;
				mip=(MIP_t)mp;
				ltrpdu->header=generateCFMHeader(mip->MDLevel,type_LTR,header_flags);
			}
		}     
	else
		{
		    header_flags = pkt_data[ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+2] | TerminalMEP;    //终  止
                  {
                            MEP_t  mep;
				mep=(MEP_t)mp;
				ltrpdu->header=generateCFMHeader(mep->MEPBasic.ma->MDPointer->MDLevel,type_LTR,header_flags);
		    }
	       }
	if(ltrpdu->header == NULL){
		printk("fail to generate CFM Header\n");
		kfree(ltrpdu);
		ltrpdu = NULL;
		return NULL;
	}
	uint8_to_uint32(ltrpdu->ltrTransId, &pkt_data[ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN]);
	
	ltrpdu->repTtl = pkt_data[ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN]-1;
	ltrpdu->relAction = relay_action;
	return ltrpdu;
}
//////////////////////////////////////创建各种LTR TLV//////////////////////////////////////////////////////////////////
dataStream_t create_LTR_Egress_Identifier_TLV(uint8* pkt_data, int flags,void* mp,uint8 mptype)
{
	dataStream_t Egress_ID;
	uint16 count=0;
	uint16 SenderID_length=0;

	Egress_ID = (dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if (Egress_ID == NULL)
	{
		printk("kmalloc failed in create_LTR_Egress_Identifier_TLV()\n");
		return NULL;
	}
	memset(Egress_ID,0,sizeof(struct dataStream_st));
	Egress_ID->length=LTR_EGRESS_IDENTIFIER_TLV_LEN;
	Egress_ID->data[0] = 8;    //type
	Egress_ID->data[1] = 0;
	Egress_ID->data[2] = 16;	//length
	count = FIRST_TLV;
	//last Egress identifier
	while(pkt_data[count] != 0)
	{
		switch (pkt_data[count])
		{
		case 7:  /*LTM Egress Identifier TLV*/
			memcpy(Egress_ID->data+3,pkt_data+count+3,EGRESS_IDENTIFIER_LEN);
			count+=11;
			break;
		case 1:  /*Sender ID TLV, don't copy*/
			uint8_to_uint16(SenderID_length,&pkt_data[count+1]);
			count+=3+SenderID_length;
			break;
		default:
			break;
		}
	}
	//LTM Egress identifier TLV not present,then 
	//last Egress identifier is 0
	//next egress identifier
	if(mptype==MEP)
	{
	MEP_t  mep;
	mep=(void*)mp;
	memcpy(Egress_ID->data+13,mep->MEPStatus.MACAddress,ADDR_LEN);
	}
	else
	{
       MIP_t  mip;
	   mip=(MIP_t)mp;
	memcpy(Egress_ID->data+13,mip->MACAddr,EGRESS_IDENTIFIER_LEN);
	}
	return Egress_ID;
}

dataStream_t create_Reply_Ingress_TLV(void* mp,uint8 mptype)
{
	dataStream_t reply_ingress;

	reply_ingress = (dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if (reply_ingress == NULL)
	{
		printk("malloc failed in create_Reply_Ingress_TLV()\n");
		return NULL;
	}
	memset(reply_ingress,0,sizeof(struct dataStream_st));

	reply_ingress->length =  10/*sizeof(struct Reply_Ingress_TLV_st)*/;
	reply_ingress->data[0] = 5;    //type
	reply_ingress->data[1] = 0;    
	reply_ingress->data[2] = 7;    //length
	reply_ingress->data[3]= IngOK;   //action
	if(mptype==MEP)
	{
              MEP_t mep;
		mep=(MEP_t)mp;
		memcpy(reply_ingress->data+4,mep->MEPStatus.MACAddress,ADDR_LEN);
	}
	else
	{
              MIP_t  mip;
	       mip=(MIP_t)mp;
		memcpy(reply_ingress->data+4,mip->MACAddr,ADDR_LEN);
	}
	
	return reply_ingress;
}
//
dataStream_t create_Reply_Egress_TLV(void* mp,uint8 mptype)
{
	dataStream_t reply_egress;

	reply_egress = (dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if (reply_egress == NULL)
	{
		printk("malloc failed in create_Reply_Egress_TLV()\n");
		return NULL;
	}
	memset(reply_egress,0,sizeof(struct dataStream_st));

	reply_egress->length =  10/*sizeof(struct Reply_Egress_TLV_st)*/;
	
	reply_egress->data[0] = 6;    //type
	reply_egress->data[1] = 0;    
	reply_egress->data[2] = 7;    //length   只包括Action和 Egress MAC地址
	reply_egress->data[3]= EgrOK;   //action
	if(mptype==MEP)
	{
              MEP_t  mep;
		mep=(MEP_t)mp;
		memcpy(reply_egress->data+4,mep->MEPStatus.MACAddress,ADDR_LEN);
	}
	else
	{
              MIP_t   mip;
		mip=(MIP_t)mp;
		memcpy(reply_egress->data+4,mip->MACAddr,ADDR_LEN);
	}
	
	return reply_egress;
}

////////////////////////////////////////不同类型的MP收到LTM后的转发与回应处理/////////////////////////////////////////////////////////////////////

//Up MHF收到LTM后的处理过程
static int upmhf_process(uint8 * pkt_data,uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection)
{
	int eq_addr=0;
	ltrpdu_t ltrpdu;
	dataStream_t ltrds,ltr_egress_id_tlv,reply_egress_tlv,senderID_tlv=NULL;
	MIP_t  mip=NULL;
	MEP_t mep=NULL;
	if(mptype==MIP)
		mip=(MIP_t)mp;
	else
		mep=(MEP_t)mp;	
	printk("in upmhf_process\n");

	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	eq_addr = is_equal_addr(pkt_data, flags, mp,mptype);
	if(eq_addr==1)
	{
		printk("reach to dest.\n");
		ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,RlyHit);
		if(ltrpdu == NULL){
			printk("ltrpdu is NULL\n");
			return -1;
		}
		//添加Egress Identifier TLV
		ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
		if (ltr_egress_id_tlv == NULL)
		{
			printk("create ltr_egress_id_tlv failed in upmhf_process()\n");
			return -1;
		}
		//添加Reply Egress TLV
		reply_egress_tlv=create_Reply_Egress_TLV(mp,mptype);
		if (reply_egress_tlv ==NULL)
		{
			printk("create reply egress tlv failed in upmhf_process()\n");
			return -1;
		}
		//添加SenderID_TLV
		if(mip->ltpm->LTR_SenderID_Permission)
		{
			senderID_tlv = create_SenderID_TLV( mp,mptype);
			if(senderID_tlv == NULL)
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_egress_tlv,senderID_tlv);
		}
		else
			ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
	
		if(ltrpdu){
			if(ltrpdu->header){
				kfree(ltrpdu->header);
				ltrpdu->header = NULL;
			}
			kfree(ltrpdu);
			ltrpdu = NULL;
		}
		if (ltr_egress_id_tlv)
		{
			kfree(ltr_egress_id_tlv);
		}
		if (reply_egress_tlv)
		{
			kfree(reply_egress_tlv);
		}
		if(senderID_tlv)
			kfree(senderID_tlv);
		
		if(ltrds == NULL){
			return -1;
		}
			//该放进队列中然后发出按照协议处理发出去   
		printk("reply a ltr.\n");
		ltr_list_append(mip->ltpm->ltr_machine, ltrds->data, ltrds->length, srcPortId, srcFlowId,srcDirection);
		if(ltrds){
			kfree(ltrds);
			ltrds = NULL;
		}
	}
	else
	{
		uint16 port;
		int search_type;								
		if(!search_egress_port(pkt_data,flags,&port,&search_type) || port != mip->srcPortId)
			return 0;
		else   //找到了Egress Port，则回复LTR,添加Reply Ingress TLV 和Reply Egress TLV
		{
			printk("find out egress port in up mhf.\n");
			ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,search_type);
			//添加Egress Identifier TLV
			ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
			if (ltr_egress_id_tlv == NULL)
			{
				printk("create ltr_egress_id_tlv failed in upmhf_process()\n");
				return -1;
			}
			//添加Reply Egress TLV
			reply_egress_tlv=create_Reply_Egress_TLV(mp,mptype);
			if (reply_egress_tlv ==NULL)
			{
				printk("create reply egress tlv failed in upmhf_process()\n");
				return -1;
			}
			//添加SenderID_TLV
			if(mip->ltpm->LTR_SenderID_Permission)
			{
				senderID_tlv = create_SenderID_TLV( mp,mptype);
				if(senderID_tlv == NULL)
					ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
				else
					ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_egress_tlv,senderID_tlv);
			}
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
			
			if(ltrpdu){
				if(ltrpdu->header){
					kfree(ltrpdu->header);
					ltrpdu->header = NULL;
				}
				kfree(ltrpdu);
				ltrpdu = NULL;
			}
			if (ltr_egress_id_tlv)
			{
				kfree(ltr_egress_id_tlv);
			}
			if (reply_egress_tlv)
			{
				kfree(reply_egress_tlv);
			}
			if(senderID_tlv)
				kfree(senderID_tlv);
			if(ltrds == NULL){
				return -1;
			}
			//该放进队列中然后发出按照协议处理发出去   
			printk("reply a ltr.\n");
			ltr_list_append(mip->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
			if(ltrds){
				kfree(ltrds);
				ltrds = NULL;
			}
			//判断是否可以转发,ttl ?= 1
			if (*(pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN) == 1)
			{
				return 0;
			}
			//可以转发
			forward_ltm(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len);
		}

	}

	return 0;
	
}

//Down MHF收到LTM后的处理流程
static int downmhf_process(uint8 * pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection)
{
	int eq_addr=0;
	ltrpdu_t ltrpdu;
	dataStream_t ltrds,ltr_egress_id_tlv,reply_ingress_tlv,senderID_tlv=NULL;
	MEP_t  mep;
	MIP_t   mip;
	if(mptype==MEP)
		mep=(MEP_t)mp;
	else
		mip=(MIP_t)mp;	
	printk("in downmhf_process\n");
	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	eq_addr = is_equal_addr(pkt_data, flags, mp,mptype);
	if(eq_addr==1)
	{
		printk("reach to dest in dowm mhf.\n");
		ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,RlyHit);
		if(ltrpdu == NULL){
			printk("ltrpdu is NULL\n");
			return -1;
		}
		//添加Egress Identifier TLV
		ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
		if (ltr_egress_id_tlv == NULL)
		{
			printk("create ltr_egress_id_tlv failed in downmhf_process()\n");
			return -1;
		}
		//添加Reply Ingress TLV
		reply_ingress_tlv =create_Reply_Ingress_TLV(mp,mptype);
		if (reply_ingress_tlv == NULL)
		{
			printk("create reply ingress tlv failed in downmhf_process()\n");
			return -1;
		}
		//添加SenderID_TLV
		if(mip->ltpm->LTR_SenderID_Permission)
		{
			senderID_tlv = create_SenderID_TLV( mp,mptype);
			if(senderID_tlv == NULL)
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_ingress_tlv);
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_ingress_tlv,senderID_tlv);
		}
		else
			ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_ingress_tlv);
		
		if(ltrpdu){
			if(ltrpdu->header){
				kfree(ltrpdu->header);
				ltrpdu->header = NULL;
			}
			kfree(ltrpdu);
			ltrpdu = NULL;
		}
		if (ltr_egress_id_tlv)
		{
			kfree(ltr_egress_id_tlv);
		}
		if (reply_ingress_tlv)
		{
			kfree(reply_ingress_tlv);
		}
		if(senderID_tlv)
			kfree(senderID_tlv);
		if(ltrds == NULL){
			return -1;
		}
			//该放进队列中然后发出按照协议处理发出去  
		printk("put ltr into state machine.\n"); 
		ltr_list_append(mip->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
		if(ltrds){
			kfree(ltrds);
			ltrds = NULL;
		}
	}
	else
	{
		uint16 port;
		int search_type;
		if (!check_spanning_forward(mip->srcPortId))
		{
			//丢弃LTM，完成
			return 0;
		}
		if(!search_egress_port(pkt_data,flags,&port,&search_type))
		{
			//没找到出口，丢弃LTM，完成
			return 0;
		}
		//找到了出口，LTM消息继续向前走
		//如果出口上配置了Up MHF或Up MEP，则将消息直接转发给他们，否则，就在这里处理
		//添加Reply Ingress TLV 和Reply Egress TLV
		if (!search_mp_onport(port,UPMP,MEP,mip->MDLevel)||!search_mp_onport(port,UPMP,MIP,mip->MDLevel))
			printk("find no up mep or up mip.\n");
			ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,search_type);
			if(ltrpdu == NULL){
				printk("ltrpdu is NULL\n");
				return -1;
			}
			ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,0);
			if(ltrpdu){
				if(ltrpdu->header){
					kfree(ltrpdu->header);
					ltrpdu->header = NULL;
				}
				kfree(ltrpdu);
				ltrpdu = NULL;
			}
			if(ltrds == NULL){
				return -1;
			}
			//该放进队列中然后发出按照协议处理发出去  
			printk("put ltr into state machine.\n");
			ltr_list_append(mip->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
			if(ltrds){
				kfree(ltrds);
				ltrds = NULL;
			}
			//判断是否可以转发,ttl ?= 1
			if (*(pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN) == 1)
			{
				return 0;
			}
			//可以转发
			forward_ltm(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len);
			return 0;
		}
		//将LTM继续传下去
		printk("find up mip or up mep on egress port.\n");
		memcpy(result,pkt_data,pkt_len);
		*result_len = pkt_len;
		return 0;
	}

//Up MEP收到LTM后的处理过程
static int upmep_process(uint8 * pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection)
{
	int eq_addr=0;
	ltrpdu_t ltrpdu;
	dataStream_t ltrds,ltr_egress_id_tlv,reply_egress_tlv,senderID_tlv=NULL;
	MEP_t  mep=NULL;
	MIP_t   mip=NULL;
	if(mptype==MEP)
		mep=(MEP_t)mp;
	else
		mip=(MIP_t)mp;
	printk("in upmep_process\n");

	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	eq_addr = is_equal_addr(pkt_data, flags, mp,mptype);
	if(eq_addr==1)
	{
		//达到了最终目的地，回复LTR，完成
		printk("reach to dest.\n");
		ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,RlyHit);

		if(ltrpdu == NULL){
			printk("ltrpdu is NULL\n");
			return -1;
		}
		//添加Egress Identifier TLV
		ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
		if (ltr_egress_id_tlv == NULL)
		{
			printk("create ltr_egress_id_tlv failed in upmep_process()\n");
			return -1;
		}
		//添加Reply Egress TLV
		reply_egress_tlv =create_Reply_Egress_TLV(mp,mptype);
		if (reply_egress_tlv ==NULL)
		{
			printk("create reply egress tlv failed in upmep_process()\n");
			return -1;
		}
		//添加SenderID_TLV
		if(mep->ltpm->LTR_SenderID_Permission)
		{
			senderID_tlv = create_SenderID_TLV( mp,mptype);
			if(senderID_tlv == NULL)
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_egress_tlv,senderID_tlv);
		}
		else
			ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
		
		if(ltrpdu){
			if(ltrpdu->header){
				kfree(ltrpdu->header);
				ltrpdu->header = NULL;
			}
			kfree(ltrpdu);
			ltrpdu = NULL;
		}
		if (ltr_egress_id_tlv)
		{
			kfree(ltr_egress_id_tlv);
		}
		if (reply_egress_tlv)
		{
			kfree(reply_egress_tlv);
		}
		if(senderID_tlv)
			kfree(senderID_tlv);
		if(ltrds == NULL){
			return -1;
		}
		//该放进队列中然后发出按照协议处理发出去   
		printk("put ltr into state machine");
		ltr_list_append(mep->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
		if(ltrds){
			kfree(ltrds);
			ltrds = NULL;
		}
	}
	else      //未到达目标
	{
		uint16 port;
		int search_type;	
		if(!search_egress_port(pkt_data,flags,&port,&search_type) || port != mep->srcPortId)
		{
			//Egress Port不确定或错误，丢弃LTM，不回复LTR。
			return 0;
		}
		else   //找到了Egress Port，则回复LTR
		{
			printk("find out egress port in up mep process..\n");
			ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,search_type);
			//添加Egress Identifier TLV
			ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
			if (ltr_egress_id_tlv == NULL)
			{
				printk("create ltr_egress_id_tlv failed in up mep_process()\n");
				return -1;
			}
			//添加Reply Egress TLV
			reply_egress_tlv =create_Reply_Egress_TLV(mp,mptype);
			if (reply_egress_tlv ==NULL)
			{
				printk("create reply egress tlv failed in upmep_process()\n");
				return -1;
			}
			//添加SenderID_TLV
			if(mep->ltpm->LTR_SenderID_Permission)
			{
				senderID_tlv = create_SenderID_TLV( mp,mptype);
				if(senderID_tlv == NULL)
					ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
				else
					ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_egress_tlv,senderID_tlv);
			}
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_egress_tlv);
			
			if(ltrpdu){
				if(ltrpdu->header){
					kfree(ltrpdu->header);
					ltrpdu->header = NULL;
				}
				kfree(ltrpdu);
				ltrpdu = NULL;
			}
			if (ltr_egress_id_tlv)
			{
				kfree(ltr_egress_id_tlv);
			}
			if (reply_egress_tlv)
			{
				kfree(reply_egress_tlv);
			}
			if(ltrds == NULL){
				return -1;
			}
			//该放进队列中然后发出按照协议处理发出去   
			printk("reply ltr.\n");
			ltr_list_append(mep->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
			if(ltrds){
				kfree(ltrds);
				ltrds = NULL;
			}
			return 0;
		}
		

	}

	return 0;
	
}

//Down MEP收到LTM后的处理流程
static int downmep_process(uint8 * pkt_data,uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection)
{
	int eq_addr=0;
	ltrpdu_t ltrpdu;
	dataStream_t ltrds,ltr_egress_id_tlv,reply_ingress_tlv,senderID_tlv;
	MEP_t  mep=NULL;
	MIP_t   mip=NULL;
	if(mptype==MEP)
		mep=(MEP_t)mp;
	else
		mip=(MIP_t)mp;
	printk("in downmep_process\n");

	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	eq_addr = is_equal_addr(pkt_data, flags, mp,mptype);
	if(eq_addr==1)
	{	
		printk("reach to dest.\n");
		ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,RlyHit);
			//这里需要再设置一下flags中的Terminal位，和tlv有关，可以根据接收mp设置，再具体时添加；
		if(ltrpdu == NULL){
			printk("ltrpdu is NULL\n");
			return -1;
		}
		//添加Egress Identifier TLV
		ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
		if (ltr_egress_id_tlv == NULL)
		{
			printk("create ltr_egress_id_tlv failed in downmep_process()\n");
			return -1;
		}
		//添加Reply Ingress TLV
		reply_ingress_tlv =create_Reply_Ingress_TLV(mp,mptype);
		if (reply_ingress_tlv == NULL)
		{
			printk("create reply ingress tlv failed in downmep_process()\n");
			return -1;
		}
		//添加SenderID_TLV
		if(mep->ltpm->LTR_SenderID_Permission)
		{
			senderID_tlv = create_SenderID_TLV( mp,mptype);
			if(senderID_tlv == NULL)
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_ingress_tlv);
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_ingress_tlv,senderID_tlv);
		}
		else
			ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_ingress_tlv);
		
		if(ltrpdu){
			if(ltrpdu->header){
				kfree(ltrpdu->header);
				ltrpdu->header = NULL;
			}
			kfree(ltrpdu);
			ltrpdu = NULL;
		}
		if (ltr_egress_id_tlv)
		{
			kfree(ltr_egress_id_tlv);
		}
		if (reply_ingress_tlv)
		{
			kfree(reply_ingress_tlv);
		}
		if(ltrds == NULL){
			return -1;
		}
			//该放进队列中然后发出按照协议处理发出去   
		printk("reply a LTR\n");
		ltr_list_append(mep->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
		if(ltrds){
			kfree(ltrds);
			ltrds = NULL;
		}
	}
	else   //未达到目的地
	{
		uint16 port;
		int search_type;
		if (!check_spanning_forward(mep->srcPortId))
		{
			return 0;
		}
		
		if(!search_egress_port(pkt_data,flags,&port,&search_type))
		{
			return 0;
		}
		//找到了唯一的Egress Port，回应LTR
		ltrpdu = create_ltr_pdu(pkt_data, srcPortId,srcFlowId,flags,mp,mptype,search_type);
		//这里需要再设置一下flags中的Terminal位，和tlv有关，可以根据接收mp设置，再具体时添加；
		if(ltrpdu == NULL){
			printk("ltrpdu is NULL\n");
			return -1;
		}
		//添加Egress Identifier TLV
		ltr_egress_id_tlv=create_LTR_Egress_Identifier_TLV(pkt_data,flags,mp,mptype);
		if (ltr_egress_id_tlv == NULL)
		{
			printk("create ltr_egress_id_tlv failed in downmep_process()\n");
			return -1;
		}
		//添加Reply Ingress TLV
		reply_ingress_tlv =create_Reply_Ingress_TLV(mp,mptype);
		if (reply_ingress_tlv == NULL)
		{
			printk("create reply ingress tlv failed in downmep_process()\n");
			return -1;
		}
		//添加SenderID_TLV
		if(mep->ltpm->LTR_SenderID_Permission)
		{
			senderID_tlv = create_SenderID_TLV( mp,mptype);
			if(senderID_tlv == NULL)
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_ingress_tlv);
			else
				ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,3,ltr_egress_id_tlv,reply_ingress_tlv,senderID_tlv);
		}
		else
			ltrds = ltr_format(pkt_data,flags,mp,mptype,1,ltrpdu,2,ltr_egress_id_tlv,reply_ingress_tlv);
		
		if(ltrpdu){
			if(ltrpdu->header){
				kfree(ltrpdu->header);
				ltrpdu->header = NULL;
			}
			kfree(ltrpdu);
			ltrpdu = NULL;
		}
		if (ltr_egress_id_tlv)
		{
			kfree(ltr_egress_id_tlv);
		}
		if (reply_ingress_tlv)
		{
			kfree(reply_ingress_tlv);
		}
		if(ltrds == NULL){
			return -1;
		}
		//该放进队列中然后发出按照协议处理发出去   
		printk("reply a ltr.\n");
		ltr_list_append(mep->ltpm->ltr_machine,ltrds->data,ltrds->length, srcPortId, srcFlowId,srcDirection);
		if(ltrds){
			kfree(ltrds);
			ltrds = NULL;
		}

	}

	return 0;
		
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//检查MP本身的类型，并分别处理
static int mp_type_check(uint8 * pkt_data,uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len, uint8 srcDirection)
{
 MEP_t  mep=NULL;
	MIP_t   mip=NULL;
	if(mptype==MEP)
		mep=(MEP_t)mp;
	else
		mip=(MIP_t)mp;
printk("in mp_type_check\n");
	if((pkt_data == NULL)||(mp == NULL)){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	if(mptype==MEP)
	{
		if(mep->Direction==DOWNMP){
			downmep_process(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len,srcDirection);
		}
		else {
			upmep_process(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len,srcDirection);
		}
	}
	else
	{
		if(srcDirection == Outside){
			downmhf_process(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len,srcDirection);
		}
		else {
			upmhf_process(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len,srcDirection);
		}
	}

	return 0;
}
//收到LTM后，最外层的处理函数 
int ltm_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype, uint8* result, int result_size, int* result_len, uint8 srcDirection)
{
	int ltm_valid_result;
	MEP_t  mep=NULL;
	MIP_t   mip=NULL;
	if(mptype==MEP)
		mep=(MEP_t)mp;
	else
		mip=(MIP_t)mp;	
	printk("in ltm_process\n");
	if((pkt_data == NULL)||mp == NULL){
		printk("pkt_data is NULL||mp is NULL");
		return -1;
	}
	
	ltm_valid_result = ltm_validate(pkt_data,flags);
	if(ltm_valid_result == -1 || ltm_valid_result == 0){
		printk("ltm_process: ltm valid failed.\n");		
		return -1;
	}
	if(mptype==MEP)
		mep->ltpm->LTMreceived++; 
	else
		mip->ltpm->LTMreceived++;  
	mp_type_check(pkt_data,pkt_len,srcPortId,srcFlowId,flags,mp,mptype,result,result_size,result_len,srcDirection);	
	printk("ltm_process end\n");
	return 0;
}
//*************************************************源MEP收到ltr后处理过程******************************************************************************
int ltr_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, MEP_t mep)
{
	int i,j;
	int ret;
	int list_size;
	ltmReplyListNode_t temp;
	linkTraceNode_t  tmp,pre; 
	uint32 ltr_id;

	if((pkt_data == NULL)||mep == NULL){
		printk("pkt_data is NULL||mep is NULL");
		return -1;
	}
	
	uint8_to_uint32(ltr_id, &pkt_data[ETHERNETHEAD_LEN+VLAN_TAG_LEN*flags+CFMHEADER_LEN]);
	ret = memcmp(pkt_data, mep->MEPStatus.MACAddress, ADDR_LEN);
	if(ret != 0){
		return -1;
	}
	printk("in ltr_process :ltr_id:%lu\n",ltr_id);
	
	mep->ltpm->LTRreceived++;
	list_size =  mep->ltpm->reply_list_size;
	for (i=0;i< list_size;i++)
	{
		//收到的LTR对应于一个发起的LTM，并且还没有超时
		if(mep->ltpm->reply_list[i].TransID == ltr_id && !mep->ltpm->reply_list[i].is_time_out && !mep->ltpm->reply_list[i].success_flag)
		{
			//printk("receive corresponding ltr\n");
			temp = (ltmReplyListNode_t)kmalloc(sizeof(struct ltmReplyListNode_st), GFP_KERNEL);
			memset(temp,0,sizeof(struct ltmReplyListNode_st));
			memcpy(temp->node, pkt_data, pkt_len);
			temp->length = pkt_len;
			LIST_INSERT_TAIL(&(mep->ltpm->reply_list[i].ltmr_list), temp, list);
			//收到了目标MP发回的LTR，跟踪结束，保存结果
			if( !memcmp(pkt_data+ADDR_LEN,mep->ltpm->reply_list[i].dest_mac,ADDR_LEN))
			{
				printk("receive dest ltr.\n");
				mep->ltpm->reply_list[i].success_flag = true;
				cfm_timer_destroy(mep->ltpm->reply_list[i].timer);
				mep->ltpm->reply_list[i].timer =NULL;
				//将所有结果都存储到 路径链表中
				//如果结果链表中 已经有了这个记录，则更新它 
				for (j=0;j<mep->ltpm->result_list_size;j++)
				{
					if( !memcmp(mep->ltpm->result_list[j].dest_mac,pkt_data+ADDR_LEN,ADDR_LEN))
					{
						//删除原先的链表，更新
						if ( !LIST_EMPTY(&mep->ltpm->result_list[j].trace_list))
						{
							tmp = LIST_FIRST(&mep->ltpm->result_list[j].trace_list);
							while(tmp != NULL)
							{
								pre = tmp;
								tmp = LIST_NEXT(tmp,list);
								kfree(pre);
								pre = NULL;
							}
						}
						printk("update a old list.\n");
						save_ltemReplyList_to_linkTraceList(&mep->ltpm->reply_list[i], &mep->ltpm->result_list[j]);
						return 0;
					}
				}
				///否则，存储到新的位置上
				//如果 链表上没有新的位置了，则从头开始覆盖
				if (mep->ltpm->result_index > mep->ltpm->result_list_size-1)
				{
					mep->ltpm->result_index = 0;
				}
				//如果位置上 有数据，则清除它
				if ( !LIST_EMPTY(&mep->ltpm->result_list[mep->ltpm->result_index].trace_list))
				{
					tmp = LIST_FIRST(&mep->ltpm->result_list[mep->ltpm->result_index].trace_list);
					while(tmp != NULL)
					{
						pre = tmp;
						tmp = LIST_NEXT(tmp,list);
						kfree(pre);
						pre = NULL;
					}
				}
				printk("save to new node.\n");
				save_ltemReplyList_to_linkTraceList(&mep->ltpm->reply_list[i], &mep->ltpm->result_list[mep->ltpm->result_index]);
				mep->ltpm->result_index++;
			return 0;
			}
			
		}
	
	}
	mep->MEPStatus.UnexpectedLTRsCount++;
	return 0;

}
static int save_ltemReplyList_to_linkTraceList(ltemReplyList_t source, linkTraceList_t dest)
{
	ltmReplyListNode_t temp,pre;
	struct linkTraceNode_st node;
	uint16 count;
	uint16 offset;
	if ((source == NULL) || (dest == NULL)){
		return -1;
	}
	printk("in save_ltemReplyList_to_linkTraceList.\n");
	memcpy(dest->dest_mac,source->dest_mac,ADDR_LEN);
	dest->success_flag = source->success_flag;
	temp = LIST_FIRST(&source->ltmr_list);
	while(temp != NULL)
	{
		pre = temp;
		temp = LIST_NEXT(temp,list);
		
		memset(&node,0,sizeof(struct linkTraceNode_st));
		memcpy(node.mac_addr,pre->node+ADDR_LEN,ADDR_LEN);
		count = ETHERNETHEAD_LEN+VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN;
		node.TTL = pre->node[count] ;
		count += TTL_LEN+1;
		while(pre->node[count] !=0)
		{
			switch(pre->node[count])
			{
			 case 8:  /*LTR Egress Identifier TLV*/
			 	 uint8_to_uint16(offset,(pre->node+count+1));  //length
				 count += offset+3;                    //after value
				 break;
			case 5:     /*Reply Ingress TLV*/
				node.ingress_action = pre->node[count+3];
				memcpy(node.ingressMAC,pre->node+count+4,ADDR_LEN);
				count += 10;
				break;
			case 6:    /*Reply Egress TLV*/
				node.egress_action = pre->node[count+3];
				memcpy(node.egressMAC,pre->node+count+4,ADDR_LEN);
				count +=10;
				break;
			case 1:  /*Sender ID TLV*/
				uint8_to_uint16(offset, (pre->node+count+1));
				count += offset+3;
				break;					
			default:
				break;
			}

		}
		LIST_INSERT_TAIL(&dest->trace_list,&node,list);
	}
	return 0;

}
#if 0
static int save_ltemReplyList_to_linkTraceList(ltemReplyList_t source, linkTraceList_t dest)
{
	ltmReplyListNode_t temp,pre;
	linkTraceNode_t node;
	uint16 count;
	uint16 offset;
	
	if ((source == NULL) || (dest == NULL)){
		return -1;
	}
	printk("in save_ltemReplyList_to_linkTraceList.\n");
	memcpy(dest->dest_mac,source->dest_mac,ADDR_LEN);
	dest->success_flag = source->success_flag;
	temp = LIST_FIRST(&source->ltmr_list);
	while(temp != NULL)
	{
		pre = temp;
		temp = LIST_NEXT(temp,list);
		node = (linkTraceNode_t)kmalloc(sizeof(struct linkTraceNode_st), GFP_KERNEL);
		if (node == NULL)        //分配节点空间不足，如何处理？
		{
			printk("kmalloc failed in save_ltemReplyList_to_linkTraceList()\n");
			return -1;
		}
		memset(node,0,sizeof(struct linkTraceNode_st));
		memcpy(node->mac_addr,pre->node+ADDR_LEN,ADDR_LEN);
		count = ETHERNETHEAD_LEN+VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN;
		node->TTL = pre->node[count] ;
		count += TTL_LEN+1;
		while(pre->node[count] !=0)
		{
			switch(pre->node[count])
			{
			 case 8:  /*LTR Egress Identifier TLV*/
			 	 uint8_to_uint16(offset,(pre->node+count+1));  //length
				 count += offset+3;                    //after value
				 break;
			case 5:     /*Reply Ingress TLV*/
				node->ingress_action = pre->node[count+3];
				memcpy(node->ingressMAC,pre->node+count+4,ADDR_LEN);
				count += 10;
				break;
			case 6:    /*Reply Egress TLV*/
				node->egress_action = pre->node[count+3];
				memcpy(node->egressMAC,pre->node+count+4,ADDR_LEN);
				count +=10;
				break;
			case 1:  /*Sender ID TLV*/
				uint8_to_uint16(offset, (pre->node+count+1));
				count += offset+3;
				break;					
			default:
				break;
			}

		}
		LIST_INSERT_TAIL(&dest->trace_list,node,list);
	}
	return 0;

}
#endif
///////////////////////////////////////////LTR发送状态机操作////////////////////////////////////////////////////////

//创建一个LTR发送状态机
static ltr_state_machine_t ltr_state_machine_create(ltpm_t ltpm,int mdelay)
{
	ltr_state_machine_t ltr_machine;

	printk("in ltr_state_machine_create\n");
	ltr_machine = (ltr_state_machine_t)kmalloc(sizeof(struct ltr_state_machine_st), GFP_KERNEL);
	if (ltr_machine == NULL)
	{
		printk("create ltr state machine failed in  ltr_state_machine_create()\n");
		return NULL;
	}
	memset(ltr_machine, 0, sizeof(struct ltr_state_machine_st));

	ltr_machine->ltr_state = RT_IDLE;
	ltr_machine->nPendingLTRs = 0;
	ltr_machine->timer_delay = mdelay;
	//初始化ltr list
	LIST_HEAD_INIT(&ltr_machine->ltr_list);
	
	//创建1s定时器
	ltr_machine->ltr_timer = cfm_timer_create(ltr_state_machine_run,mdelay,(long)ltr_machine);
	if (ltr_machine->ltr_timer == NULL)
	{
		printk("create ltr timer failed in  ltr_state_machine_create()\n");
		goto err1;
	}
	ltr_machine->ltpm = ltpm;
	cfm_mutex_init_nosleep(&ltr_machine->lock);
	return ltr_machine;


err1:
	LIST_HEAD_DESTROY(&ltr_machine->ltr_list);
	kfree(ltr_machine);
	ltr_machine =NULL;
	return NULL;
}

//运行这个状态机  主要完成功能：0~1s定时器到期时，查询状态机状态，查询队列，有数据就发送出去
static void ltr_state_machine_run(unsigned long machine_run)
{
	ltrListNode_t temp,pre;
	ltr_state_machine_t ltr_machine = (ltr_state_machine_t)machine_run;

	//首先查询状态机状态
	switch (ltr_machine->ltr_state)
	{
	case RT_IDLE:
		//clearPendingLTRs(ltr_machine->ltr_queue);
		ltr_machine->nPendingLTRs =0;
		ltr_machine->ltr_state = RT_WAITING;
		break;
	case RT_WAITING:
		//等待....直接退出
		
		break;
	case RT_TRANSMITTING:	
		//取出一个LTR
		
		cfm_mutex_lock_nosleep(&ltr_machine->lock);
		temp = LIST_FIRST(&ltr_machine->ltr_list);
		while(temp != NULL)
		{
			pre = temp;
			temp = LIST_NEXT(temp,list);
			//发送LTR
			printk("state machine runing,send a ltr.\n");
			xmitOldestLTR(ltr_machine->ltpm->mp,ltr_machine->ltpm->mptype,pre->node,pre->length,pre->srcPortId,pre->srcFlowId,pre->srcDirection);
			ltr_machine->nPendingLTRs--;
			LIST_REMOVE(&ltr_machine->ltr_list,pre,list);
			kfree(pre);
			pre = NULL;
		}
		
		ltr_machine->ltr_state = RT_WAITING;
		cfm_mutex_unlock_nosleep(&ltr_machine->lock);
		break;
	default:
		printk("ltr state machine's state is wrong!\n");
		break;
	}
	cfm_timer_continue(ltr_machine->ltr_timer,ltr_machine->timer_delay);
	return;

}

//LTR 的发送函数  等待完善
static int xmitOldestLTR(void* mp,uint8 mptype, uint8 *pkt_data, uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId,int srcDirection)
{

	if(pkt_data == NULL){
		printk("pkt_data is NULL");
		return -1;
	}
	cfm_send(pkt_data,pkt_len,srcPortId,srcFlowId,mp,mptype,srcDirection,REPLY);
	if (mptype==MEP)
	{
          MEP_t  mep;
		  mep=(MEP_t)mp;
	   mep->ltpm->LTRcount++;
	}
	else
	{
              MIP_t  mip;
	       mip=(MIP_t)mp;
		mip->ltpm->LTRcount++;
	}
	return 0;
}


//向LTR队列中插入LTR
static int ltr_list_append(ltr_state_machine_t ltr_machine, uint8 *pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId,int srcDirection)
{
	ltrListNode_t ltr;
	//int ret;
	//int timeout;
	
	if(pkt_data == NULL)
	{
		printk("buf is <NULL> in ltr_queue_push().\n");
		return -1;
	}

	ltr = (ltrListNode_t)kmalloc(sizeof(struct ltrListNode_st),GFP_KERNEL);
	if (ltr == NULL)
	{
		printk("kmalloc failed in ltr_list_append()\n");
		return -1;
	}
	memset(ltr,0,sizeof(struct ltrListNode_st));
	memcpy(ltr->node,pkt_data,pkt_len);
	ltr->length = pkt_len;
	ltr->srcPortId = srcPortId;
	ltr->srcFlowId = srcFlowId;
	ltr->srcDirection = srcDirection;

	cfm_mutex_lock_nosleep(&ltr_machine->lock);
	LIST_INSERT_TAIL(&ltr_machine->ltr_list,ltr,list);
	ltr_machine->nPendingLTRs++;
	ltr_machine->ltr_state = RT_TRANSMITTING;
	cfm_mutex_unlock_nosleep(&ltr_machine->lock);

	return 0;

}

//销毁状态机
static int ltr_state_machine_destroy(ltr_state_machine_t ltr_machine)
{
	ltrListNode_t temp,pre;
	if (ltr_machine == NULL)
		return -1;
	cfm_mutex_lock_nosleep(&ltr_machine->lock);
	//销毁list
	temp = LIST_FIRST(&ltr_machine->ltr_list);
	while(temp != NULL)
	{
		pre = temp;
		temp = LIST_NEXT(temp,list);
		kfree(pre);
		pre = NULL;

	}
	LIST_HEAD_DESTROY(&ltr_machine->ltr_list);
	cfm_mutex_unlock_nosleep(&ltr_machine->lock);
	//销毁定时器
	if(ltr_machine->ltr_timer){
		cfm_timer_destroy(ltr_machine->ltr_timer);
	}
	//销毁状态机
	kfree(ltr_machine);
	ltr_machine =NULL;
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////查询Filtering Database，确定Egress Port操作//////////////////////////////////////////////////////////
//从Filtering Database中查找Egress Port，成功就返回port，否则返回NULL
//查询出口，成功返回true，失败返回false，
//参数： egress_port_id：存储得到的出口
//   type：查询的数据库类型
static bool search_egress_port(uint8 * pkt_data,int flags,uint16* egress_port_id,int* type)
{
	uint8 targetMAC[6];
	uint8 originMAC[6];
	uint16 vlan_id;


	memcpy(originMAC,pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN+TTL_LEN,ADDR_LEN);
	memcpy(targetMAC,pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN+TTL_LEN+ADDR_LEN,ADDR_LEN);
	uint8_to_uint16(vlan_id,&pkt_data[ETHERNETHEAD_LEN]);
	vlan_id=vlan_id & 0x0FFF;

	if(FilteringDatabase_query(targetMAC,vlan_id,egress_port_id) == 0)
	{
		*type = RlyFDB;
		return true;
	}
	else
	{
		if (MIPdatabase_query(targetMAC,vlan_id,egress_port_id) == 0)
				{
			              *type = RlyMPDB;
			               return true;
		               }
	}
	return false;
}
static bool search_mp_onport(uint16 port,uint8 direction,uint8 mptype,int MDlevel)
{
  if(mptype==MEP)
   {MEP_t mep;
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
	{
		if (mep->Direction== direction  && mep->MEPBasic.ma->MDPointer->MDLevel == MDlevel&& mep->srcPortId == port)
		{
			printk("find mep.\n");
			return true;
		}
	}
     }
  else
   {MIP_t mip;
	LIST_TRAVERSE(&gCfm->mip_info.mip_list,mip,list)
	{
		if (mip->Direction== direction  && mip->MDLevel == MDlevel&& mip->srcPortId == port)
		{
			printk("find mip.\n");
			return true;
		}
	}
     }
	return false;	
}
static bool check_spanning_forward(uint16 PortID)
{
	return true;
}
static int forward_ltm(uint8 * pkt_data,uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype,uint8* result, int result_size, int* result_len)
{
	dataStream_t ltmds;
	uint16 SenderTLV_length;
	uint32 count,count_source;
	dataStream_t SenderID_TLV;
	MEP_t  mep=NULL;
	MIP_t   mip=NULL;
	if(mptype==MEP)
		mep=(MEP_t)mp;
	else
		mip=(MIP_t)mp;

	printk("in forward_ltm().\n");
	ltmds = (dataStream_t)kmalloc(sizeof(struct dataStream_st),GFP_KERNEL);
	if (ltmds == NULL)
	{
		printk("allocate memory failed in forward_ltm()\n");
		return -1;
	}
	memset(ltmds,0, sizeof(struct dataStream_st));
	count = 0;
	memcpy(ltmds->data ,pkt_data,ADDR_LEN);        //复制DA
	memcpy(ltmds->data +ADDR_LEN,mip->MACAddr,ADDR_LEN);      //设置出口的source_address
	count=12;
	memcpy(ltmds->data +count,pkt_data+count,2+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN); //拷贝vlan_tag，type，Common CFM Header，TransID
	count+=2+flags*VLAN_TAG_LEN+CFMHEADER_LEN+TRANSID_LEN;
	ltmds->data [count]=pkt_data[count]-1;    //设置ttl
	count++;
	memcpy(ltmds->data +count,pkt_data+count,2*ADDR_LEN);    //复制origin MAC和Target MAC
	count+=2*ADDR_LEN;
	count_source = count;
	while(pkt_data[count_source] != 0)
	{
		switch(pkt_data[count_source])
		{
		case 7:  /*LTM Egress Identifier TLV*/
			memcpy(ltmds->data +count,pkt_data+count_source,5);
			count+=5;
			{
                            MIP_t  mip;
				mip=(MIP_t)mp;
				memcpy(ltmds->data +count,mip->MACAddr,ADDR_LEN);
			}    //设置出口MAC地址
			count+=ADDR_LEN;
			count_source = count;
			break;
		case 1: /*Sender ID TLV*/
			uint8_to_uint16(SenderTLV_length, (pkt_data+count+1));
			count_source+=SenderTLV_length+1+2;
			break;
		default:
			break;
		}
	}
	//添加上自己的SenderID TLV	 
	if(mip->ltpm->LTM_SenderID_Permission)
	{
		SenderID_TLV =create_SenderID_TLV( mp,mptype);
		if(SenderID_TLV == NULL)
			ltmds->data[count] = 0;
		else{
			memcpy(ltmds->data+count, SenderID_TLV->data, SenderID_TLV->length);
			ltmds->data[count+SenderID_TLV->length] = 0;
			count += SenderID_TLV->length;
			kfree(SenderID_TLV);
			}
		
	}
	else
		ltmds->data [count]=0;


	//从Egress Port上发送出去
	memcpy(result,ltmds->data,count+1);
	*result_len = count+1;
	if(ltmds){
		kfree(ltmds);
		ltmds = NULL;
	}

	return 0;
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        			
