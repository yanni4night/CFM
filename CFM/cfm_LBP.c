#include<linux/module.h>
#include<linux/kernel.h>
#include<linux/slab.h>
#include<linux/timer.h>
#include<stdarg.h>
#include "cfm_header.h"
#include "cfm_protocol.h"
#include <linux/time.h>
#define ALLOC_MAX_TRIES 5
#define DEFAULT_LBM_TIMER 5000
#define LBM_HEAD_LEN 9
static struct timeval t ;

/**************LBP Functions Declare*****************/
static int xmitLBM(uint8*pkt_data, uint32 pkt_len, uint32 bridge_id, uint32 flow_id, int flags, void * mp,int mpFlag);
static int xmitLBR(uint8*pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int flags,uint8 srcDirection,void * mp,int mpFlag);
static void*faultMsg(char*msg);
static int lbr_validate(uint8*pkt_data,uint32 pkt_len,int flags,void * mp,int mpFlag);
static int lbm_validate(uint8*pkt_data,uint32 pkt_len,int flags,void * mp,int mpFlag);
static void lbr_timeout(unsigned long data);
static void timer_reset(void * mp,int mpFlag);
bool getXmitReady(void * mp,int mpFlag);
void setXmitReady(char k,void * mp,int mpFlag);
bool getLBIactive(void * mp,int mpFlag);
void setLBIactive(char k,void * mp,int mpFlag);
static lbmpdu_t generateLbmPdu(void * mp,int mpFlag);
static dataStream_t lbmpdu_format(uint8 destination_address[ADDR_LEN],uint8 priority,void * mp,int mpFlag);
static int lbpm_tlv_init(lbpm_t lbpm);

static int lbpm_tlv_init(lbpm_t lbpm)
{
	if(lbpm == NULL){
		return -1;
	}
	lbpm->Data_TLV.type = type_Data_TLV;
	return 0;
}


/************LoopBack Protocol Funtion API************/


/*
Initialize the LoopBack Module
*/
lbpm_t lbpm_init(void * mp,int mpFlag)
{
	lbpm_t lbpm ;
	lbpm = (lbpm_t)alloc(sizeof(struct lbpm_st));
	if(lbpm==NULL)
	{
		printk("fail to kmalloc lbpm_t\n");
		return NULL ;
	}
	memset(lbpm,0,sizeof(struct lbpm_st));
	lbpm->mp = mp;
	lbpm->mpFlag=mpFlag;
	lbpm->timeout = DEFAULT_LBM_TIMER ;
	lbpm->avaliable = true ;
	lbpm_tlv_init(lbpm);
	
	return lbpm ;
}


/*
Destroy the LoopBack Module
*/
int lbpm_destroy(lbpm_t lbpm)
{
	if(lbpm == NULL)
	{
		return-1 ;
	}
    
	if(lbpm->timer)
	{
		cfm_timer_destroy(lbpm->timer);
	}

	kfree(lbpm);
	lbpm=NULL ;
	
	return 0 ;
}

/*
Without Ethernet-Header,VLAN-Tag or any TLV
*/
static lbmpdu_t generateLbmPdu(void * mp,int mpFlag)//Modified 10:50,07-22
{
	CFMHeader_t header ;
	lbmpdu_t pdu ;
	MEP_t mep;
	
	if(mp==NULL)return faultMsg("mp is NULL in generateLbmPdu");
	if(mpFlag==MEP)mep=(MEP_t)mp;
	else return NULL;
	
	pdu = (lbmpdu_t)alloc(sizeof(struct lbmpdu_st));
	if(pdu==NULL)return faultMsg("Cannot alloc <pdu> in generateLbmPdu()");

	header = generateCFMHeader(mep->MEPBasic.ma->MDPointer->MDLevel,type_LBM, 0);
		
	pdu->header = header ;
	pdu->ltransId = mep->lbpm->nextLBMtransID;
	pdu->endTLVOffset = 0 ;
	
	return pdu ;
}

/*
Add Ethernet-Header,VLAN-Tag and TLVs to char[]
*/
static dataStream_t lbmpdu_format(uint8 destination_address[ADDR_LEN],uint8 priority,void * mp,int mpFlag)//Modified 11:00 07-22
{
	int tlv_total_len=0,cnt=0 ,sender_id_tlv_len=0;
	lbmpdu_t pdu ;
	dataStream_t datas ;
	MEP_t mep=NULL;
	if(mpFlag==MEP)mep=(MEP_t)mp;
	
	datas=(dataStream_t)alloc(sizeof(struct dataStream_st));
	if(datas==NULL)
	{
		return faultMsg("Cannot alloc <datas> in lbmpdu_format()");
	}

	pdu = generateLbmPdu( mep,mpFlag);
	#if 1
		
		if(mep->lbpm->senderID_TLV_Contained)
		{

			switch(mep->MEPBasic.ma->SenderIDPermission)
			{
           case None:break;
		   case Chassis:;sender_id_tlv_len=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+2;break;
		   case Manage:sender_id_tlv_len=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Length+3;break;
		   case ChassisManage:
		   	sender_id_tlv_len=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+gCfm->TLV.Sender_ID_TLV.management_Address_Length+4;
			break;

			}
			tlv_total_len+=sender_id_tlv_len+3;
			
		}
		if(mep->lbpm->data_TLV_Contained)
		{
			tlv_total_len+=mep->lbpm->Data_TLV.length+3;
		}
		if(mep->lbpm->orgSpecificTLVContained)
		{
			tlv_total_len+=gCfm->TLV.Organization_Specific_TLV.length+3;
		}
#endif

	datas->length=ETHERNETHEAD_LEN+LBM_HEAD_LEN+VLAN_TAG_LEN+tlv_total_len ;
    
	memcpy(datas->data,destination_address,ADDR_LEN);
	memcpy(datas->data+ADDR_LEN,mep->MEPStatus.MACAddress,ADDR_LEN);
    
	cnt=ADDR_LEN*2 ;
	datas->data[cnt++]=0x81 ;
	//Vlan-Tag
	datas->data[cnt++]=0x00 ;
    
	datas->data[cnt++]=(uint8)(priority<<5)+(uint8)(mep->MEPBasic.PrimaryVlan>>8);
	datas->data[cnt++]=(uint8)(mep->MEPBasic.PrimaryVlan&0xFF);
	//VlanId
    
	datas->data[cnt++]=0x89 ;
	//802.1agCFM Tag
	datas->data[cnt++]=0x02 ;
    
    
	datas->data[cnt++]=pdu->header->mdLevel_version ;
	//CFM Header
	datas->data[cnt++]=pdu->header->opCode ;
	datas->data[cnt++]=pdu->header->flags ;
	datas->data[cnt++]=pdu->header->firstTLVOffset ;
    
	datas->data[cnt++]=(uint8)(pdu->ltransId>>24);
	//LBM-TransID
	datas->data[cnt++]=(uint8)((pdu->ltransId>>16)&0xFF);
	datas->data[cnt++]=(uint8)((pdu->ltransId>>8)&0xFF);
	datas->data[cnt++]=(uint8)(pdu->ltransId);
    
    

    #if 1
	if(mep->lbpm->senderID_TLV_Contained)
	{
		datas->data[cnt++]=gCfm->TLV.Sender_ID_TLV.type;
		datas->data[cnt++]=(uint8)(sender_id_tlv_len>>8);
		datas->data[cnt++]=(uint8)(sender_id_tlv_len&0x00FF);

		if(mep->MEPBasic.ma->SenderIDPermission==Chassis||mep->MEPBasic.ma->SenderIDPermission==ChassisManage)
		{
			datas->data[cnt++]=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
			datas->data[cnt++]=gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
			memcpy(datas->data+cnt,gCfm->TLV.Sender_ID_TLV.chassis_ID,gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
			cnt+=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
	    	}
		if(mep->MEPBasic.ma->SenderIDPermission==Manage){
			datas->data[cnt++] = 0x00;
		}
		if(mep->MEPBasic.ma->SenderIDPermission==Manage||mep->MEPBasic.ma->SenderIDPermission==ChassisManage)
		{
			datas->data[cnt++]=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
			memcpy(datas->data+cnt,gCfm->TLV.Sender_ID_TLV.management_Address_Domain,gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
			cnt+=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
			datas->data[cnt++]=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
			memcpy(datas->data+cnt,gCfm->TLV.Sender_ID_TLV.management_Address,gCfm->TLV.Sender_ID_TLV.management_Address_Length);
			cnt+=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
		}
	}
	if(mep->lbpm->data_TLV_Contained)
	{
	datas->data[cnt++]=mep->lbpm->Data_TLV.type;
	datas->data[cnt++]=(uint8)(mep->lbpm->Data_TLV.length>>8);
	datas->data[cnt++]=(uint8)(mep->lbpm->Data_TLV.length&0x00FF);
	memcpy(datas->data+cnt,mep->lbpm->Data_TLV.data,mep->lbpm->Data_TLV.length);
	cnt+=mep->lbpm->Data_TLV.length;
	}
	//if(mp->lbpm->orgSpecificTLVContained)
#if 0
	if(0)
	{
		datas->data[cnt++]=gCfm->TLV.Organization_Specific_TLV.type;
		datas->data[cnt++]=(uint8)(gCfm->TLV.Organization_Specific_TLV.length>>8);
		datas->data[cnt++]=(uint8)(gCfm->TLV.Organization_Specific_TLV.length&0x00FF);
		datas->data[cnt++]=(uint8)(gCfm->TLV.Organization_Specific_TLV.OUI>>8);
	    	datas->data[cnt++]=(uint8)(gCfm->TLV.Organization_Specific_TLV.OUI&0x00FF);
		datas->data[cnt++]=gCfm->TLV.Organization_Specific_TLV.sub_Type;
		memcpy(datas->data+cnt,gCfm->TLV.Organization_Specific_TLV.value,gCfm->TLV.Organization_Specific_TLV.length-3);
		cnt+=gCfm->TLV.Organization_Specific_TLV.length-3;
	}
#endif
#endif
	datas->data[cnt++]=0 ;
	//endTLVOffset
    
	if(pdu!=NULL)
	{
		if(pdu->header!=NULL)
		{
			kfree(pdu->header);
			pdu->header=NULL ;
		}
		kfree(pdu);
		pdu=NULL ;
	}
	return datas ;
}

/*
Send LBRs out of LoopBack Module
*/
static int xmitLBR(uint8*pkt_data,uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId,int flags, uint8 srcDirection,void * mp,int mpFlag)//Modified 11:07 07-22
{
 	MEP_t mep=NULL;
	MIP_t mip=NULL;
	
	if(mpFlag==MEP)
		mep=(MEP_t)mp;
	else if(mpFlag==MIP)
		mip=(MIP_t)mp;
	
	if((pkt_data==NULL)||(pkt_len==0)||(mp==NULL))
	{
		printk("error\n");
		return 0 ;
	}
   
	if((mpFlag== MEP)&&(mep->Direction== UPMP)){
		cfm_send(pkt_data,pkt_len,srcPortId,srcFlowId,mp,mpFlag,Inside,REPLY);
	}
	else if((mpFlag == MEP)&&(mep->Direction== DOWNMP)){
		cfm_send(pkt_data,pkt_len,srcPortId,srcFlowId,mp,mpFlag,Outside,REPLY);
	}
	else if(mpFlag  == MIP){
		printk("mp->type == MIP\n");
		cfm_send(pkt_data, pkt_len, srcPortId, srcFlowId, mep,mpFlag, srcDirection, REPLY);
	}
	else{
		return -1;
	}
	
	memcpy(mep->lbpm->lastSentLBR.data, pkt_data,sizeof(mep->lbpm->lastSentLBR.data));
	mep->lbpm->lastSentLBR.length = pkt_len;
	mep->lbpm->lbr_sent_counter++;
	
	return 0 ;
}

/*
Called when a LBM is received
*/
int lbm_process(uint8*pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int flags,void * mp,int mpFlag, uint8* result, int result_size, int* result_len, uint8 srcDirection)
//Modified 11:23 07-22
{   
	uint8 temp[ADDR_LEN];
	uint8 lbrpdu[MAX_CFM_LEN] ;
	MEP_t mep=NULL;
	MIP_t mip=NULL;
	
	if(mpFlag==MEP)
		mep=(MEP_t)mp;
	else if(mpFlag==MIP)
		mip=(MIP_t)mp;
	
	printk("in lbm_process\n");
	if((pkt_data == NULL)||(pkt_len == 0)||(mp == NULL))
	{
		printk("error in lbm_process");
		return -1 ;
	}
	if(mpFlag==MEP)
		{
		memcpy(mep->lbpm->lastRecvdLBM.data,pkt_data,sizeof(mep->lbpm->lastRecvdLBM.data));
		mep->lbpm->lastRecvdLBM.length=pkt_len;
		}
	else if(mpFlag==MIP)
		{
		memcpy(mip->lbpm->lastRecvdLBM.data,pkt_data,sizeof(mip->lbpm->lastRecvdLBM.data));
		mip->lbpm->lastRecvdLBM.length=pkt_len;
		}
	
	
	if((mpFlag==MIP)&&(memcmp(pkt_data, mip->MACAddr, ADDR_LEN) != 0))
	{
		if(result == NULL)
			{
			printk("result is NULL\n");
			return -1;
			}
		memcpy(result, pkt_data, result_size);
		*result_len = pkt_len;
		printk("pkt_len:%d\n", *result_len);
		print_hex_data(result, *result_len);
		return 0;
	}
	 if(mpFlag==MEP)
		mep->lbpm->lbm_received_counter++;
	 else if(mpFlag==MIP)
	 	mip->lbpm->lbm_received_counter++;
	if(!lbm_validate(pkt_data, pkt_len, flags, mp,mpFlag))
	{
		return 0 ;
	}

	memcpy(lbrpdu, pkt_data, sizeof(lbrpdu));
    
	memcpy(temp, lbrpdu+ADDR_LEN,sizeof(temp));
	//Exchange Source/Destination Address
	memcpy(lbrpdu+ADDR_LEN, lbrpdu, ADDR_LEN);
	memcpy(lbrpdu, temp, ADDR_LEN);
    
	lbrpdu[OPCODE_WITHOUT_VLAN+VLAN_TAG_LEN*flags] = type_LBR;
	//Change LBM-Tag to LBR-Tag
	xmitLBR(lbrpdu, pkt_len, srcPortId, srcFlowId, flags, srcDirection,mp,mpFlag);
	return 0;
}

/*
Called when a LBR is received
*/
int lbr_process(uint8*pkt_data,uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId,int flags,void * mp,int mpFlag)//Modified 11:27 07-22
{
	int offset ;
	uint32 lbr_transId ;
	MEP_t mep;
				
	if(mpFlag==MEP)mep=(MEP_t)mp;//Only MEP can process LBR
	else return 0;

	
	if(!mep->lbpm->LBIactive)return 0 ;
	//Not the status waiting for a LBR
	if(!lbr_validate(pkt_data,pkt_len,flags,mep,mpFlag))
	{
		return 0 ;
	}

	memcpy(mep->lbpm->lastRecvdLBR.data,pkt_data,sizeof(mep->lbpm->lastRecvdLBR.data));
	mep->lbpm->lastRecvdLBR.length=pkt_len;
	
	offset=VLAN_TAG_LEN*flags ;
    
	lbr_transId=((uint32)(pkt_data[18+offset])<<24)|((uint32)(pkt_data[19+offset])<<16)|((uint32)(pkt_data[20+offset])<<8)|((uint32)pkt_data[21+offset]);
	if(lbr_transId!=mep->lbpm->expectedLBRtransID)
	{
		//Check the transaction ID
		mep->lbpm->out_of_sequence_counter++;
		printk("\nTransID not match,dicarded:expectedID=[%lx],lbr_transID=[%lx]\n",mep->lbpm->expectedLBRtransID,lbr_transId);
	}
	else 
	{
        if(mep->lbpm->LBIWhile!=0)
        {
            
            cfm_timer_continue(mep->lbpm->timer,0);
            //?Try to stop the timer?
            do_gettimeofday(&t);
            printk("\nlbr_process:%ld,%ld\n",t.tv_sec,t.tv_usec);
            mep->lbpm->LBIactive=false ;
            //Quit the status awaiting a LBR
            mep->lbpm->expectedLBRtransID++;
            //Avoid to accept the same LBR as before
            (mep->lbpm->in_sequence_counter)++;
            if(mep->lbpm->LBMsToSend!=0)
            {
                
                dataStream_t stream ;
                
                stream=lbmpdu_format(mep->lbpm->destination,1,mep,mpFlag);
                //No TLV to be sent now
                xmitLBM(stream->data,stream->length,mep->srcPortId,mep->FlowId,flags,mep,mpFlag);
                
                if(stream!=NULL)
                {
                    kfree(stream);
                    stream=NULL ;
                }
                if(mep->lbpm->LBMsToSend==0)
                printk("\n*****  All LBMs have been sent out!Waiting for the last LBR! *****\n");
            }
            //All LBMs have been sent out
            else 
            {
                printk("\n ******All LBRs have been received!The connectivity is valid! ******\n");
            }
        }
        //time out
        else 
        {
            printk("\nTime out\n");
        }
        
        
    }
    return 1 ;
}

/*
Interface to start the LoopBack Module
@lbmstosend:num of LBMs expected to send;
@destination_address:destionation of LBms;
@m:the MP who calls the function
*/
void lbpm_start(uint8 lbmstosend,uint8 destination_address[ADDR_LEN],void * mp,int mpFlag)//Modified 11:41 07-22
{
	dataStream_t ds ;
	MEP_t mep;

	if(mpFlag==MEP)mep=(MEP_t)mp;
	else return;
	
	if(!mep->lbpm->avaliable)return ;
	//LB Module is not avaliable
	if(lbmstosend==0)return ;
	//Illegal LBMsToSend
	 if(mep->lbpm->LBIactive)return ;
	//Quit if the module is awaiting a LBR
	if(mep->lbpm->timer==NULL)
	mep->lbpm->timer = cfm_timer_create(lbr_timeout,0,(long)mep);
    
	cfm_timer_continue(mep->lbpm->timer,0);
	memcpy(mep->lbpm->destination,destination_address,ADDR_LEN);
	ds=lbmpdu_format(destination_address,1,mep,mpFlag);
	mep->lbpm->LBMsToSend=lbmstosend ;
    
	xmitLBM(ds->data, ds->length, mep->srcPortId, mep->FlowId, 1, mep,mpFlag);
	//Flow_ID,BRIDGE_ID not processed
	if(ds != NULL)
	{
		kfree(ds);
		ds=NULL ;
	}
}

void setLBIactive(char k,void * mp,int mpFlag)
{	
	MEP_t mep;
	MIP_t mip;
	if(mpFlag==MEP)
		{
		mep=(MEP_t)mp;
		mep->lbpm->LBIactive=k ;
		}
	else if(mpFlag==MIP)
		{
		mip=(MIP_t)mp;
		mip->lbpm->LBIactive=k ;
		}
}

bool getLBIactive(void * mp,int mpFlag)
{
    	
	MEP_t mep;
	MIP_t mip;
	if(mpFlag==MEP)
		{
		mep=(MEP_t)mp;
		return mep->lbpm->LBIactive ;
		}
	else if(mpFlag==MIP)
		{
		mip=(MIP_t)mp;
		return mip->lbpm->LBIactive ;
		}
	return false;
}

void setXmitReady(char k,void * mp,int mpFlag)
{
    MEP_t mep;
	MIP_t mip;
	if(mpFlag==MEP)
		{
		mep=(MEP_t)mp;
		mep->lbpm->xmitReady=k ;
		}
	else if(mpFlag==MIP)
		{
		mip=(MIP_t)mp;
		mip->lbpm->xmitReady=k ;
		}
}

bool getXmitReady(void * mp,int mpFlag)
{
	MEP_t mep;
	MIP_t mip;
	if(mpFlag==MEP)
		{
		mep=(MEP_t)mp;
		return mep->lbpm->xmitReady ;
		}
	else if(mpFlag==MIP)
		{
		mip=(MIP_t)mp;
		return mip->lbpm->xmitReady ;
		}
	return false;
}

/*
Validate CFMHeader and TLV
@data:pdu-data starts after 0x8902;
@length:pdu-length starts after 0x8902
*/
int lbpm_pdu_validate(uint8*data,uint32 length)
{
	int cnt=0 ;
	uint8 mdLevel,version,opCode,flags,first_TLV_offset,tlv_type ;
	uint16 tlv_length ;
    
	if(length<9)return 0 ;
	if(data[length-1]!=0)return 0 ;
	//EndTLV must be 0
    
	mdLevel=data[cnt]>>5 ;
	version=data[cnt++]&0x1F ;
	//Not examined
	opCode=data[cnt++];
	flags=data[cnt++];
	//Not examined
	first_TLV_offset=data[cnt++];
	//Check CFMHeader
	if(mdLevel>7)return 0 ;
	if(opCode!=0x02&&opCode!=0x03)return 0 ;
	//Maybe not be necessary
	if(first_TLV_offset!=0x04)return 0 ;
	//first_TLV_Offset is 0x04 in LBPM
	//Check TLVs
	cnt+=4 ;
	//Skip TransID
    
	while(length-1>cnt)
	{
		tlv_type=data[cnt++];
		 //Not examined
		if(tlv_type==0)
		{
			cnt++;
			continue ;
		}
		uint8_to_uint16(tlv_length, &data[cnt]);
		cnt += 2;
		if(tlv_length==0)
		{
			cnt+=2 ;
			continue ;
		}
		cnt+=tlv_length+3 ;
	}
	//EndTLV must be 0
	if(length==cnt)return 1 ;
	else return 0 ; 
}

/*
Validate LBM PDU for discarding/processing
@pkt_data:LBM PDU;
@pkt_len:LBM Length;
@flags:num of Vlan Tag;
@m:the MP who calls the function
*/

static int lbm_validate(uint8*pkt_data, uint32 pkt_len, int flags, void * mp,int mpFlag)//Modified 13:11 07-22
{
	uint8 sAddr[ADDR_LEN], dAddr[ADDR_LEN];
	MEP_t mep=NULL;
	MIP_t mip=NULL;

		
	if(mp == NULL){
		printk("mp is NULL\n");
		return -1;
	}
	if(mpFlag==MEP)
		mep=(MEP_t)mp;
	else if(mpFlag==MIP)
		mip=(MIP_t)mp;

	
	memcpy(dAddr, pkt_data, ADDR_LEN);
	memcpy(sAddr, pkt_data+ADDR_LEN, ADDR_LEN);
  
	if((mpFlag== MEP)&&(memcmp(mep->MEPStatus.MACAddress, dAddr, ADDR_LEN) !=0)&&(isGroupAddr(dAddr) == 1)){
		return 0;
	}
    
	return lbpm_pdu_validate(pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN, pkt_len-ETHERNETHEAD_LEN-flags*VLAN_TAG_LEN);
}

/*
Validate LBR PDU for discarding/processing
@pkt_data:LBR PDU;
@pkt_len:LBR Length;
@flags:num of Vlan Tag;
@m:the MP who calls the function
*/
static int lbr_validate(uint8*pkt_data,uint32 pkt_len,int flags,void * mp,int mpFlag)//Modified 13:13 07-22
{	
	MEP_t mep;
	if(mpFlag==MIP)return 0 ;
	mep=(MEP_t)mp;
	if(memcmp(pkt_data,mep->MEPStatus.MACAddress,6)!=0)return 0 ;
	return lbpm_pdu_validate(pkt_data+ETHERNETHEAD_LEN+flags*VLAN_TAG_LEN,pkt_len-ETHERNETHEAD_LEN-flags*VLAN_TAG_LEN);
}

/*
Send LBMs out of LoopBack Module
*/
static int xmitLBM(uint8*pkt_data, uint32 pkt_len, uint32 bridge_id, uint32 flow_id, int flags, void * mp,int mpFlag)//Modified 13:20 07-22
{
    
	PACKET_ACTION_ID actionId ;
	MEP_t mep;
	printk("in xmitLBM\n");
	print_hex_data(pkt_data,pkt_len);
	if((mp == NULL)||(mpFlag!= MEP))
	{
		faultMsg("Parameter<m> is NULL in xmitLBM()");
		return 0 ;
	}
	mep=(MEP_t)mp;
	mep->lbpm->expectedLBRtransID=mep->lbpm->nextLBMtransID ;
	mep->lbpm->nextLBMtransID++;
	actionId=SENDTO ;
	timer_reset(mep,mpFlag);
    
	if(mep->Direction== UPMP){
		printk("mp->upDownType == UPMP\n");
		cfm_send(pkt_data,pkt_len,bridge_id,flow_id,mep,mpFlag,Inside,SENDTO);
	}
	else{
		printk("mp->upDownType == DOWNMP\n");
		cfm_send(pkt_data,pkt_len,bridge_id,flow_id,mp,mpFlag,Outside,SENDTO);
	}
    
	memcpy(mep->lbpm->lastSentLBM.data,pkt_data,pkt_len);
	mep->lbpm->lastSentLBM.length=pkt_len ;
    
	do_gettimeofday(&t);
	printk("cfmPacket_xmit in xmitLBM:%ld,%ld\n",t.tv_sec,t.tv_usec);
    
	mep->lbpm->LBMsToSend--;
	mep->lbpm->lbm_sent_counter++;
	printk("lbm_sent_counter:[%ld]\n",mep->lbpm->lbm_sent_counter);
	printk("lbmstosend=[%ld]\n",mep->lbpm->LBMsToSend);
	return 1 ;
}


/*
Reset the timer for the expected LBR
*/
static void timer_reset(void * mp,int mpFlag)//Modified 13:22 07-22
{
    MEP_t mep;
	MIP_t mip;
	if(mpFlag==MEP)
		{
		mep=(MEP_t)mp;
		cfm_timer_continue(mep->lbpm->timer,mep->lbpm->timeout);
		mep->lbpm->LBIWhile=1 ;
		mep->lbpm->LBIactive=true ;
		}
	else if(mpFlag==MIP)
		{
		mip=(MIP_t)mp;
		cfm_timer_continue(mip->lbpm->timer,mip->lbpm->timeout);
		mip->lbpm->LBIWhile=1 ;
		mip->lbpm->LBIactive=true ;
		}
	
}
/*
Called when an expected LBR has not been received within the stipulated time
*/

static void lbr_timeout(unsigned long data)//Modified 13:37 07-22
{
    
	MEP_t mep=(MEP_t)data ;
	if(mep==NULL)return ;
	if(mep->lbpm->LBMsToSend==0)return ;
	mep->lbpm->LBIWhile=0 ;
	mep->lbpm->LBIactive=false ;
	printk("***** LBR-Waiting Time Out! active=[%d],lbmstosend=[%ld] *****\n",mep->lbpm->LBIactive,mep->lbpm->LBMsToSend);
}

/*
Max tries to kmalloc
*/
void*alloc(uint32 size)
{
	void*p ;
	int i ;
	for(i=0;i<ALLOC_MAX_TRIES;i++)
	{
		p=kmalloc(size,GFP_KERNEL);
		if(p!=NULL)
		{
			memset(p,0,size);
			break ;
		}
	}
	return p ;
}
static void*faultMsg(char*msg)
{
	printk(msg);
	return NULL ;
}

//Check 01-80-C2-00-00-3y
 int  isGroupAddr(uint8* addr)
{
	if(addr == NULL){
		return -1;
	}
	if((addr[0] != 0x01)||(addr[1] != 0x80)||(addr[2] != 0xC2)||(addr[3] != 0x00)||(addr[4] != 0x00)){
		return 0;
	}
	if((addr[5] >= 0x30)&&(addr[5] <= 0x3F)){
		return 1;
	}
	return 0;
}
