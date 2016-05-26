#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <asm/system.h>
#include <asm/processor.h>
#include <asm/signal.h>
#include <linux/completion.h>       // for DECLARE_COMPLETION()
#include <linux/sched.h>            // for daemonize() and set_current_state() 
#include <linux/delay.h>            // mdelay()
#include "cfm_header.h"
#include "cfm_protocol.h"

static cfm_t cfm = NULL;
static int exit_monitor = 0;
static cfm_thread_t provider_thread_id = -1;
static cfm_lock_t lock = NULL;
static cfm_timer_t timer = NULL;
static int timer_delay = 1000;

static DECLARE_COMPLETION(completion);
static DECLARE_COMPLETION(consumer_completion);

static int vlanTag_check(uint8 *data);
static int cfm_dispatch(void);
static int OutsideCFM_process(uint8* buffer, uint32 len, uint16 srcPortId, uint32 srcFlowId, uint16 VlanId, int MDLevel, int flag, uint8 opcode);
static int InsideCFM_process(uint8* buffer, uint32 len, uint16 srcPortId, uint32 srcFlowId, uint16 VlanId, int MDLevel, int flag, uint8 opcode, uint16 PortID);
static int cfm_send_process(void *tmp, int tMPFlag, uint8* buffer, uint32 buf_len, uint16 srcPortId, uint32 srcFlowId, uint16 VlanId, int MDLevel, int flag, uint8 opcode, uint8 Action, uint8 srcDirection);
static int MEPActiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int result_size, int * result_len, uint8 srcDirection);
static int MEPPassiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int result_size, int* result_len, uint8 srcDirection);
static int MIPActiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int result_size, int* result_len, uint8 srcDirection);
static int MIPPassiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int  result_size,int *result_len, uint8 srcDirection);

static int vlanTag_check(uint8 *data)
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

int cfmPacket_xmit (uint8* pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, uint32 actionId)
{
#ifdef CFM_EMULATE	
	int ret;

	//printk("in cfmPacket_xmit\n");
	if(pkt_data == NULL){
		printk("pkt_data is NULL\n");
		return -1;
	}
	if(cfm == NULL){
		return -1;
	}
	if(cfm->send_queue == NULL){
		return -1;
	}
		
	ret = queue_push(cfm->send_queue, pkt_data, pkt_len, srcPortId, srcFlowId);
/*	while(ret < 0){
		int timeout;
		timeout = 1*HZ;
		set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(timeout);
		ret = queue_push(cfm->send_queue, data, len+3,0,0);
	}
*/
#endif
	return 0;
}

int cfmPacket_receive (uint8* pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId)
{
	int ret;
	int flags;

	if(pkt_data == NULL){
		return -1;
	}
	if(cfm == NULL){
		return -1;
	}
	if(cfm->recv_queue == NULL){
		return -1;
	}
	
	flags = vlanTag_check(pkt_data);
	if((pkt_data[12+flags*VLAN_TAG_LEN] == 0x89)&&(pkt_data[13+flags*VLAN_TAG_LEN] == 0x02)){
		ret = queue_push(cfm->recv_queue, pkt_data, pkt_len, srcPortId, srcFlowId);
	}
	else{
	
		cfmPacket_xmit (pkt_data, pkt_len, srcPortId, srcFlowId, FORWARD);
	}
	
	return 0;
}

static int cfm_dispatch()
{
	uint8 buffer[MAX_CFM_LEN];
	int ret_len;
	uint16 srcPortId;
	uint32 srcFlowId;
	uint8 opcode;
	int flag;
	uint16 VlanID = -1;
	int MDLevel = -1;
	int i = 0;
	int ret;
	uint16 PortID;
	FilteringDatabase_t entry;
	if(cfm == NULL){
		return -1;
	}
	if(cfm->recv_queue == NULL){
		return -1;
	}
	while(!exit_monitor) /*jone20100517:fixed*/
	{
		if(buffer == NULL){
			printk("in cfm_dispatch: buffer is NULL\n");
		}
		memset(buffer, 0, sizeof(buffer));
		ret_len = queue_pull(cfm->recv_queue, buffer, sizeof(buffer),&srcPortId, &srcFlowId);
		

		if(ret_len <= 0){
			return 0;
		}
		printk("ret_len:%d\n", ret_len);
		printk("srcPortId:%u,srcFlowId:%lu\n", srcPortId, srcFlowId);
		i++;
		printk("in cfm_dispatch\n");
		print_hex_data(buffer, ret_len); /*jone20100517: add for debug*/

		flag = vlanTag_check(buffer);
		MDLevel = (buffer[ETHERNETHEAD_LEN+4*flag]>>5)&0xFF;
		opcode = buffer[OPCODE_WITHOUT_VLAN + VLAN_TAG_LEN*flag];
		if(flag >= 1){
			VlanID = (((int)buffer[14]<<8)&0x00000f00)|(((int)buffer[15])&0x000000ff);
		}
		else{
			VlanID = 0;
		}
		printk("mdlevel:%d, flag:%d, opcode:%.2x\n", MDLevel, flag, opcode);
		ret_len = OutsideCFM_process(buffer, ret_len, srcPortId, srcFlowId, VlanID, MDLevel, flag, opcode);
		if(ret_len > 0){
		printk("ret_len:%d\n", ret_len);
		printk("mdlevel:%d, flag:%d, opcode:%.2x\n", MDLevel, flag, opcode);
		print_hex_data(buffer, ret_len); 
	if(isGroupAddr(buffer)==1)
		{
		printk("Group Addr~!\n");
		LIST_TRAVERSE(&gCfm->FilteringDatabase_list,entry,list)
			{
				if(entry->PortID!=srcPortId)
					InsideCFM_process(buffer, ret_len, srcPortId, srcFlowId, VlanID, MDLevel, flag, opcode, entry->PortID);
			}
		}
	else{	
		ret = FilteringDatabase_query(buffer, VlanID, &PortID);
		if(ret == 0){
				InsideCFM_process(buffer, ret_len, srcPortId, srcFlowId, VlanID, MDLevel, flag, opcode, PortID);
			}
		}
		}
	}
	return i;
}

static int OutsideCFM_process(uint8* buffer, uint32 len, uint16 srcPortId, uint32 srcFlowId, uint16 VlanId, int MDLevel, int flag, uint8 opcode)
{
	void *mp = NULL;
	struct MEP_st *mep=NULL;
	struct MIP_st *mip=NULL;
	uint8 *pkt_data;
	uint8 *result_pkt;
	int pkt_len;
	int result_len = -1;
	int MPFlag;
	
	printk("in OutsideCFM_process\n");
	print_hex_data(buffer, len); 

	if(buffer == NULL){
		printk("buffer is NULL");
		return -1;
	}
	pkt_data = (uint8*)kmalloc(MAX_CFM_LEN, GFP_KERNEL);
	result_pkt = (uint8*)kmalloc(MAX_CFM_LEN, GFP_KERNEL);
	if((pkt_data == NULL)||(result_pkt == NULL)){
		printk("fail to kmalloc\n");
		if(pkt_data){
			kfree(pkt_data);
		}
		if(result_pkt){
			kfree(result_pkt);
		}
		return -1;
	}

	memset(result_pkt, 0, MAX_CFM_LEN);
	memset(pkt_data, 0, MAX_CFM_LEN);

	memcpy(pkt_data, buffer, MAX_CFM_LEN);
	pkt_len = len;

	while(1){	
		mp = cfm_GetNextMP(cfm, VlanId, srcPortId, Outside, FORWARD, mp, &MPFlag);
		if(mp == NULL){
			printk("==Can't get the next MEP/MIP!==\n");
			break;
		}
		if(MPFlag == MEP){
			mep = (struct MEP_st *)mp;
			printk("mp type:MEP,mp->updown:%d,mp->srcportid:%u,mp->meid:%d,MDlevel:%d\n",  mep->Direction, mep->srcPortId, mep->MEPBasic.meid,mep->MEPBasic.ma->MDPointer->MDLevel);
		}
		else if(MPFlag == MIP){
			mip = (struct MIP_st *)mp;
			printk("mp type:MIP,mp->updown:%d,mp->srcportid:%u,mp->meid:%d,MDlevel:%d\n",  mip->Direction, mip->srcPortId, mip->meid,mip->MDLevel);
		}

		if((MPFlag == MEP)&&(mep->Direction == DOWNMP)){
			MEPActiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mep, result_pkt, MAX_CFM_LEN, &result_len, Outside);
		}

		else if(MPFlag == MIP){
			MIPActiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mip, result_pkt, MAX_CFM_LEN, &result_len, Outside);
		}
		else if((MPFlag == MEP)&&(mep->Direction == UPMP)){
			MEPPassiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mep, result_pkt, MAX_CFM_LEN, &result_len, Outside);
		}

		memcpy(pkt_data, result_pkt, MAX_CFM_LEN);
		pkt_len = result_len;
		memset(result_pkt, 0, MAX_CFM_LEN);
		result_len = -1;
		if(pkt_len <= 0){
			break;
		}
	}
	memcpy(buffer, pkt_data, MAX_CFM_LEN);
	
	if(pkt_data){
		kfree(pkt_data);
	}
	if(result_pkt){
		kfree(result_pkt);
	}
	
	if(pkt_len > 0){
		return pkt_len;
	}

	return 0;
}

static int InsideCFM_process(uint8* buffer, uint32 len, uint16 srcPortId, uint32 srcFlowId, uint16 VlanId, int MDLevel, int flag, uint8 opcode, uint16 PortID)
{
	void	*mp = NULL;
	struct MEP_st *mep=NULL;
	struct MIP_st *mip=NULL;
	int MPFlag;
	uint8 pkt_data[MAX_CFM_LEN];
	uint8 result_pkt[MAX_CFM_LEN];
	int pkt_len;
	int result_len = -1;
	
	
	printk("in InsideCFM_process\n");

	if(buffer == NULL){
		printk("buffer is NULL\n");
		return -1;
	}

	memset(result_pkt, 0, MAX_CFM_LEN);
	memset(pkt_data, 0, MAX_CFM_LEN);

	memcpy(pkt_data, buffer, MAX_CFM_LEN);
	pkt_len = len;

	while(1){	
		mp = cfm_GetNextMP(cfm, VlanId, PortID, Inside, FORWARD, mp, &MPFlag);
		if(mp == NULL){
			printk("Find NULL MP in InsideCFM_process\n");
			break;
		}
		if(MPFlag == MEP){
			mep = (struct MEP_st *)mp;
			printk("mp type:MEP,mp->updown:%d,mp->srcportid:%u,mp->meid:%d,level=%d\n", mep->Direction, mep->srcPortId, mep->MEPBasic.meid,mep->MEPBasic.ma->MDPointer->MDLevel);
		}
		else if(MPFlag == MIP){
			mip = (struct MIP_st *)mp;
			printk("mp type:MIP,mp->updown:%d,mp->srcportid:%u,mp->meid:%d,level=%d\n", mip->Direction, mip->srcPortId, mip->meid,mip->MDLevel);
		}

		if((MPFlag == MEP)&&(mep!=NULL)&&(mep->Direction == UPMP)){
			MEPActiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mep, result_pkt, MAX_CFM_LEN, &result_len, Inside);
		}

		else if(MPFlag == MIP){
			MIPActiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mip, result_pkt, MAX_CFM_LEN, &result_len, Inside);
		}
		else if((MPFlag == MEP)&&(mep!=NULL)&&(mep->Direction == DOWNMP)){
			MEPPassiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mep, result_pkt, MAX_CFM_LEN, &result_len, Inside);
		}

		memcpy(pkt_data, result_pkt, MAX_CFM_LEN);
		pkt_len = result_len;
		memset(result_pkt, 0, MAX_CFM_LEN);
		result_len = -1;
		if(pkt_len <= 0){
			break;
		}

	}
	printk("in inside pkt_len:%d\n", pkt_len);

	if(pkt_len > 0){
		printk("now call cfmPacket_xmit\n");
		print_hex_data(buffer, pkt_len); 
		cfmPacket_xmit(pkt_data, pkt_len, PortID, srcFlowId, FORWARD);
	}

	return 0;
}
static int MEPActiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int result_size, int * result_len, uint8 srcDirection)
{
	struct MEP_st *mep = (struct MEP_st *)tmp;
	printk("in MEPActiveSAP_process\n");
	if(mep == NULL){
		printk("mep is NULL\n");
		return -1;
	}
	if(pkt_data == NULL){
		printk("pkt_data is NULL\n");
		return -1;
	}
	
	if((MDLevel>7) || (MDLevel<0)){
		printk("MDLevel not present\n");
		return -1;
	}
	else if(MDLevel > mep->MEPBasic.ma->MDPointer->MDLevel){
		printk("in MEPActiveSAP_process\n");
		if((result_pkt == NULL)||(result_len == NULL)){
			printk("result_pkt is NULL\n");
			return -1;
		}
		memcpy(result_pkt, pkt_data, result_size);
		*result_len = pkt_len;
		return 0;
	}
	else if(MDLevel < mep->MEPBasic.ma->MDPointer->MDLevel){
		if(opcode == type_CCM){
			ccm_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mep);
			return 0;
		}
		else
			return -1;	
	}

	switch(opcode)
	{
	case type_CCM:
		ccm_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mep);
		return 0;
	case type_LBR:
		lbr_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mep, MEP);
		return 0;
	case type_LBM:
		lbm_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mep, MEP, result_pkt, result_size, result_len, srcDirection);
		return 0;
	case type_LTR:
		ltr_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mep);
		return 0;
	case type_LTM:
		ltm_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mep,MEP, result_pkt, result_size, result_len, srcDirection);
		return 0;
	default:
		printk("unknown packet!\n");
		return -1;
	}

	return 0;
}
static int MEPPassiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int result_size, int* result_len, uint8 srcDirection)
{
	struct MEP_st *mep = (struct MEP_st *)tmp;
	printk("in MEPPassiveSAP_process\n");
	if(mep == NULL){
		printk("mep is NULL\n");
		return -1;
	}
	if(pkt_data == NULL){
		printk("pkt_data is NULL\n");
		return -1;
	}
	if((MDLevel>7) || (MDLevel<0)){
		printk("MDLevel not present\n");
		return -1;
	}
	else if(MDLevel > mep->MEPBasic.ma->MDPointer->MDLevel){
		if((result_pkt == NULL)||(result_len == NULL)){
			printk("result is NULL\n");
			return -1;
		}
		memcpy(result_pkt, pkt_data, result_size);
		*result_len = pkt_len;
		return 0;
	}
	else{
		printk("error,MDLevel<=mp->MDLevel\n");
		return -1;
	}

	return 0;
}
static int MIPActiveSAP_process(uint8 * pkt_data, uint32 pkt_len,  uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int result_size, int* result_len, uint8 srcDirection)
{
	struct MIP_st *mip = (MIP_t)tmp;
printk("in MIPActiveSAP_process\n");
	if(mip == NULL){
		printk("mip is NULL\n");
		return -1;
	}
	if(pkt_data == NULL){
		printk("pkt_data is NULL\n");
		return -1;
	}
	if((MDLevel>7) || (MDLevel<0)){
		printk("MDLevel not present\n");
		return -1;
	}
	if(MDLevel != mip->MDLevel){
		if((result_pkt == NULL)||(result_len == NULL)){
			printk("result is NULL\n");
			return -1;
		}
		memcpy(result_pkt, pkt_data, pkt_len);
		*result_len = pkt_len;
		return 0;
	}
	switch(opcode)
	{
	case type_CCM:
		MHFprocessCCM(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mip);
		if((result_pkt == NULL)||(result_len == NULL)){
			printk("result_pkt is NULL\n");
			return -1;
		}
		memcpy(result_pkt, pkt_data, result_size);
		*result_len = pkt_len;
		return 0;
		//break;
	case type_LBM:
		printk("now call lbm_process\n");
		lbm_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mip, MIP, result_pkt, result_size, result_len, srcDirection);
		return 0;
	case type_LTM:
		ltm_process(pkt_data, pkt_len, srcPortId, srcFlowId, flag, mip,MIP,  result_pkt, result_size, result_len, srcDirection);
		return 0;
	default:
		if((result_pkt == NULL)||(result_len == NULL)){
			printk("result_pkt is NULL\n");
			return -1;
		}
		memcpy(result_pkt, pkt_data, result_size);
		*result_len = pkt_len;
		return 0;
	}

	return 0;

}
static int MIPPassiveSAP_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int MDLevel, int flag, uint8 opcode, void* tmp, uint8 * result_pkt, int  result_size,int *result_len, uint8 srcDirection)
{
	struct MIP_st *mip = (struct MIP_st *)tmp;
printk("in MIPPassiveSAP_process\n");
	if(mip == NULL){
		printk("mip is NULL\n");
		return -1;
	}
	if(pkt_data == NULL){
		printk("pkt_data is NULL\n");
		return -1;
	}
	if((result_pkt == NULL)||(result_len == NULL)){
		printk("result_pkt is NULL\n");
		return -1;
	}
	memcpy(result_pkt, pkt_data, result_size);
	*result_len = pkt_len;

	return 0;
}
int cfm_send(uint8* pkt_data,int pkt_len,uint16 srcPortId,uint32 srcFlowId,void* tmp, int MPFlag, uint8 srcDirection, uint8 Action)
{
	uint8 opcode;
	int flag;
	int VlanID = -1;
	int MDLevel = -1;
	int ret;
	uint16 PortID;
	int ret_len;
	FilteringDatabase_t entry;
	printk("in cfm_send\n");
	if(tmp == NULL){
		printk("mp is NULL\n");
		return -1;
	}
	if((pkt_data == NULL) || (pkt_len == 0))
	{
		printk("bad parameter in cfm_send()\n");
		return -1;
	}
	
	flag = vlanTag_check(pkt_data);
	MDLevel = (pkt_data[ETHERNETHEAD_LEN+4*flag]>>5);
	opcode = pkt_data[OPCODE_WITHOUT_VLAN + VLAN_TAG_LEN*flag];
	if(flag >= 1){
		VlanID = (((int)pkt_data[14]<<8)&0x00000f00)|(((int)pkt_data[15])&0x000000ff);
	}
	else{
		VlanID = 0;
	}
	
	ret_len = cfm_send_process(tmp, MPFlag, pkt_data, pkt_len, srcPortId, srcFlowId, VlanID, MDLevel, flag, opcode, Action, srcDirection);
	if(ret_len > 0){
			if(isGroupAddr(pkt_data)==1)
				{
					printk("Group Addr~!\n");
					LIST_TRAVERSE(&gCfm->FilteringDatabase_list,entry,list)
						{
				if(entry->PortID!=srcPortId)
					InsideCFM_process(pkt_data, ret_len, srcPortId, srcFlowId, VlanID, MDLevel, flag, opcode, entry->PortID);
						}
				}
			else
				{	
					ret = FilteringDatabase_query(pkt_data, VlanID, &PortID);
					if(ret == 0){
						InsideCFM_process(pkt_data, ret_len, srcPortId, srcFlowId, VlanID, MDLevel, flag, opcode, PortID);
								}
				}
	}
	
	return 0;

}
static int cfm_send_process(void *tmp, int tMPFlag, uint8* buffer, uint32 buf_len, uint16 srcPortId, uint32 srcFlowId, uint16 VlanId, int MDLevel, int flag, uint8 opcode, uint8 Action, uint8 srcDirection)
{
	uint8 pkt_data[MAX_CFM_LEN];
	uint8 result_pkt[MAX_CFM_LEN];
	int pkt_len;
	int result_len = -1;
	void *mp = tmp;
	struct MEP_st *mep=NULL;
	struct MIP_st *mip=NULL;
	int MPFlag = tMPFlag;
	printk("in cfm_send_process\n");
	if(mp == NULL){
		printk("mp is NULL\n");
		return -1;
	}
	if(buffer == NULL){
		printk("buffer is NULL\n");
		return -1;
	}

	memset(result_pkt, 0, MAX_CFM_LEN);
	memset(pkt_data, 0, MAX_CFM_LEN);

	memcpy(pkt_data, buffer, MAX_CFM_LEN);

	pkt_len = buf_len;
	result_len = -1;
	while(1){	
		if(MPFlag == MEP)
			 mp = cfm_GetNextMP(cfm, VlanId,  ((MEP_t)mp)->srcPortId, srcDirection, Action, mp, &MPFlag);
		else mp = cfm_GetNextMP(cfm, VlanId,  ((MIP_t)mp)->srcPortId, srcDirection, Action, mp, &MPFlag);
		if(mp == NULL){
			break;
		}
		if(MPFlag == MEP){
			mep = (struct MEP_st *)mp;
			printk("mp type:MEP,mp->updown:%d,mp->srcportid:%u,mp->meid:%d,level=%d\n", mep->Direction, mep->srcPortId, mep->MEPBasic.meid,mep->MEPBasic.ma->MDPointer->MDLevel);
		}
		else if(MPFlag == MIP){
			mip = (struct MIP_st *)mp;
			printk("mp type:MIP,mp->updown:%d,mp->srcportid:%u,mp->meid:%d,level=%d\n", mip->Direction, mip->srcPortId, mip->meid,mip->MDLevel);
		}
		
		switch(srcDirection){
		case Inside:
			if((MPFlag == MEP)&&(mep!=NULL)&&(mep->Direction == UPMP)){
				printk("Inside\n");
				MEPPassiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mep, result_pkt, MAX_CFM_LEN, &result_len, Action);
			}
			else{
				return -1;
			}
			break;
		case Outside:
			if((MPFlag == MEP)&&(mep!=NULL)&&(mep->Direction == DOWNMP)){
				printk("Outside\n");
				MEPPassiveSAP_process(pkt_data, pkt_len, srcPortId, srcFlowId, MDLevel, flag, opcode, mep, result_pkt, MAX_CFM_LEN, &result_len, Action);
			}
			else{
				return -1;
			}
			break;
		}
		memcpy(pkt_data, result_pkt, MAX_CFM_LEN);
		pkt_len = result_len;
		printk("result_len:%d\n", result_len);
		memset(result_pkt, 0, MAX_CFM_LEN);
		result_len = -1;
		if(pkt_len <= 0){
			break;
		}
	}
	if(srcDirection == Inside){
		if(pkt_len > 0){
			memcpy(buffer, pkt_data, MAX_CFM_LEN);
			return pkt_len;
		}
		
	}
	else if(srcDirection == Outside){
		if(pkt_len > 0){
			print_hex_data(pkt_data, pkt_len);
			cfmPacket_xmit(pkt_data, pkt_len, srcPortId, srcFlowId, FORWARD);
			return 0;
		}
	
	}

	return -1;
}

static int provider(void*arg)
{ 
	 int timeout;
	 timeout = 1*HZ; /*1s = 1*HZ*/
	 daemonize("cfm-thread 1");
	 allow_signal(SIGTERM);
	 while(!signal_pending(current)){
	 	if (exit_monitor){
			break;
		}
	 	set_current_state(TASK_INTERRUPTIBLE);
		schedule_timeout(timeout);
		cfm_dispatch();
		//printk("dispatch package %d\n",cfm_dispatch());
	 }
 
	 complete(&completion); // wake up wait_for_completion	 
	 return 0;
}

static void timer_handle(unsigned long arg)
{
	cfm_timer_continue(timer,timer_delay);
}
int cfm_core_init(void)
{
	 
	uint16   mdid;
	uint16 maid;
	uint16 mepid;
	struct Layer2_Entity_st layer2Entity;
	uint16 associatedVLANs[12]={20,34,23,9};
	uint8 macAddr[ADDR_LEN]={0x00,0x0C,0x29,0x94,0x76,0xAC};
	
	cfm = cfm_create();
	if (cfm == NULL){
		printk("fail to create cfm\n");
		return -1;
	}
	gCfm = cfm;
	memcpy(layer2Entity.MACAddr,macAddr,ADDR_LEN);
	
	#if  1 //Need to update depend on your location of device in your network
	layer2Entity.port=1;
	cfmSupervisory_CreateMD(&mdid,5,MD_Name_Format_None ,0,NULL);
	cfmSupervisory_CreateMA(&maid,mdid,Short_MA_Name_Format_Character_String,5,"MA001",associatedVLANs);
	cfmSupervisory_CreateMEP(&mepid,maid,89,DOWNMP,&layer2Entity,2);


	#endif

	provider_thread_id = cfm_create_thread(provider,NULL,CLONE_FS | CLONE_FILES);
	
	lock = cfm_sem_lock_create();

	timer = cfm_timer_create(timer_handle,timer_delay,0);
	if (timer == NULL){
		printk("can't create timer\n");
		return -1;
	}
	
	printk("create provider thread id %d\n",provider_thread_id);
	/*debug code  jone.li--------*//*
	LIST_TRAVERSE(&cfm->mep_info.mep_list,((MEP_t)temp), list){
		printk("mep id=%d\n",((MEP_t)temp)->meid);
	}
	LIST_TRAVERSE(&cfm->mip_info.mip_list,((MIP_t)temp), list){
		printk("mep id=%d\n",((MIP_t)temp)->meid);
	}
*/	/*debug code  jone.li--------*/
	return 0;
}
int cfm_core_destroy(void)
{
	printk("destroy thread id %d\n",provider_thread_id);
	exit_monitor = 1; 	
	if (provider_thread_id > 0){
		wait_for_completion(&completion);
	}
	if (timer){
		printk("destroy timer\n");
		cfm_timer_destroy(timer);
		timer = NULL;
	}
	if (lock){
		printk("detroy lock\n");
		cfm_sem_lock_destroy(lock);	
		lock = NULL;
	}
	if (cfm){
		printk("destroy cfm\n");
	 	cfm_destroy(cfm);
	}
	return 0;
}
