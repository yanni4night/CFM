#include <linux/fs.h>
#include "cfm_header.h"
#include "cfm_protocol.h"

static int add_OrgSpeTLV(uint8* ccm_frame, MEP_t mep);
static int consturct_TLV(uint8 *ccm_frame, MEP_t mep,int senderIDpermission);
static int transinterval_count(uint8 interval);
static int CCMdatabase_init(ccpm_t ccpm);
static int  CCMdatabase_update(MEP_t mep,unsigned char * recvCCM,int flag);
static int CCMdatabase_destroy(MEP_t mep);
static int xmitCCM(MEP_t mep);
static void ccmXmitTimer(uint32 arg);
static int CCM_discard(uint8 *buffer);
static int CCM_validation(uint8 *buffer,MEP_t mep,int flag,uint32 len);
static int MEPprocessLowCCM(uint8*buffer, uint32 len, MEP_t mep, int flag);
static int MEPprocessEqualCCM(uint8 * buffer, uint32 len, MEP_t mep, int flag);
//static struct MHF_ccr_st * MHF_CCR_init(void);
static int MIPdatabase_update(unsigned char * recvCCM, int flag, uint32 bridge_id);
//int MHFprocessCCM(uint8 * buffer, uint32 len,uint32 bridge_id, uint32 flow_id, int flag,MIP_t mip );
int update_TLV(MEP_t mep,struct MEPDatabase_st*  temp);
static int ccpm_tlv_init(ccpm_t ccpm);

static int add_OrgSpeTLV(uint8* ccm_frame, MEP_t mep)
{
	if(mep->ccpm->org_spe_permission != true)
	{
		*ccm_frame = 0x00;
		return 0;
	}
	*ccm_frame = gCfm->TLV.Organization_Specific_TLV.type;
	uint16_to_uint8((ccm_frame+1) ,gCfm->TLV.Organization_Specific_TLV.length);
	memcpy(ccm_frame+3 ,gCfm->TLV.Organization_Specific_TLV.OUI ,3 );
	*(ccm_frame+6) = gCfm->TLV.Organization_Specific_TLV.sub_Type;
	memcpy(ccm_frame+7 ,gCfm->TLV.Organization_Specific_TLV.value, gCfm->TLV.Organization_Specific_TLV.length);
	*(ccm_frame+7+gCfm->TLV.Organization_Specific_TLV.length)=0x00;
	return 0;
}
int consturct_TLV(uint8 *ccm_frame, MEP_t mep,int senderIDpermission)
{
	
	ccm_frame[92]=mep->ccpm->Port_Status_TLV.type;			    //Port Status TLV
	uint16_to_uint8(&ccm_frame[93], mep->ccpm->Port_Status_TLV.length);
	ccm_frame[95]=mep->ccpm->Port_Status_TLV.port_status_value;
	
	ccm_frame[96]=mep->ccpm->Interface_Status_TLV.type;//interface status TLV
	uint16_to_uint8(&ccm_frame[97],mep->ccpm->Interface_Status_TLV.length);
	ccm_frame[99]=mep->ccpm->Interface_Status_TLV.interface_status;
	
// -------------- the judgement of senderIDpermission-------------------
	if (senderIDpermission ==None)
		{	
			add_OrgSpeTLV(ccm_frame+100 ,mep );
			return 0;			
		}
	
	ccm_frame[100]=gCfm->TLV.Sender_ID_TLV.type;	//sender ID TLV
	
// -------------- the judgement of senderIDpermission-------------------

	if(senderIDpermission ==Manage)
		{
			ccm_frame[103]= 0x00;//gCfm->TLV.Sender_ID_TLV.length set to 0!! 
			ccm_frame[104]=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
			memcpy(ccm_frame+105 , gCfm->TLV.Sender_ID_TLV.management_Address_Domain , gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);

			
			if(gCfm->TLV.Sender_ID_TLV.management_Address_Length != 0)
			{
				ccm_frame[105+ gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length]=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
				memcpy(ccm_frame+ 106 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length, gCfm->TLV.Sender_ID_TLV.management_Address , gCfm->TLV.Sender_ID_TLV.management_Address_Length);
				// ccm_frame[106+ gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+ gCfm->TLV.Sender_ID_TLV.management_Address_Length]=0x00;
				add_OrgSpeTLV( ccm_frame+106+ gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+ gCfm->TLV.Sender_ID_TLV.management_Address_Length , mep);
				gCfm->TLV.Sender_ID_TLV.length = 1 + 1 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length +1 +gCfm->TLV.Sender_ID_TLV.management_Address_Length;
				uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
				
				return 0;
			}
			//ccm_frame[105 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length]= 0x00;
			add_OrgSpeTLV(ccm_frame+105+ gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length,  mep);
			gCfm->TLV.Sender_ID_TLV.length = 1 + 1 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length +1 ;
			uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
			return 0;
		}
	//uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
	ccm_frame[103]=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
	if(gCfm->TLV.Sender_ID_TLV.chassis_ID_Length != 0)
	{
		ccm_frame[104]=gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
		memcpy(&ccm_frame[105],gCfm->TLV.Sender_ID_TLV.chassis_ID , gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);

// -------------- the judgement of senderIDpermission-------------------
		if(senderIDpermission ==Chassis)
			{
					// ccm_frame[105+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length]=0x00;
					add_OrgSpeTLV( ccm_frame+105+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length,  mep);
					gCfm->TLV.Sender_ID_TLV.length = 2+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
					 uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
					 return 0;
			}
		if(gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length !=0)
		{
			ccm_frame[105 + gCfm->TLV.Sender_ID_TLV.chassis_ID_Length]=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
			memcpy(ccm_frame+106+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length, gCfm->TLV.Sender_ID_TLV.management_Address_Domain , gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);

			
			if(gCfm->TLV.Sender_ID_TLV.management_Address_Length != 0)
			{
				ccm_frame[106+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length]=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
				memcpy(ccm_frame+107+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length, gCfm->TLV.Sender_ID_TLV.management_Address , gCfm->TLV.Sender_ID_TLV.management_Address_Length);
				//ccm_frame[108+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+ gCfm->TLV.Sender_ID_TLV.management_Address_Length]=0x00;
				add_OrgSpeTLV(ccm_frame+108+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+ gCfm->TLV.Sender_ID_TLV.management_Address_Length,  mep);
				gCfm->TLV.Sender_ID_TLV.length = 2 + gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + 1 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length +1 +gCfm->TLV.Sender_ID_TLV.management_Address_Length;
				uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
				return 0;
			}
			//ccm_frame[106+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length]= 0x00;
			add_OrgSpeTLV(ccm_frame+106+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length,  mep);
			gCfm->TLV.Sender_ID_TLV.length = 2 + gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + 1 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length +1 ;
			uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
			return 0;
		}
		//ccm_frame[105+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length]=0x00;
		add_OrgSpeTLV(ccm_frame+105+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length,  mep);
		gCfm->TLV.Sender_ID_TLV.length = 2 + gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + 1 ;
		uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
		return 0;
	}
	if( gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length != 0)
	{
		ccm_frame[104] = gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
		memcpy(ccm_frame+105,gCfm->TLV.Sender_ID_TLV.management_Address_Domain,gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);

		if(gCfm->TLV.Sender_ID_TLV.management_Address_Length !=0)
			{
				ccm_frame[105+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length] = gCfm->TLV.Sender_ID_TLV.management_Address_Length;
				memcpy(ccm_frame+106+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length, gCfm->TLV.Sender_ID_TLV.management_Address,gCfm->TLV.Sender_ID_TLV.management_Address_Length);
				ccm_frame[106+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Length] = 0x00;
				gCfm->TLV.Sender_ID_TLV.length = 1 + 1 + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length +1 +gCfm->TLV.Sender_ID_TLV.management_Address_Length;
				uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
				return 0;
			}
//		ccm_frame[105+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length]=0x00;
		add_OrgSpeTLV(ccm_frame+105+gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length,  mep);
		gCfm->TLV.Sender_ID_TLV.length = 1+1+ gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length +1 ;
		uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
		return 0;
	}
	ccm_frame[104] = gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
	if(gCfm->TLV.Sender_ID_TLV.management_Address_Length != 0)
	{
		ccm_frame[105]=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
		memcpy(ccm_frame + 106, gCfm->TLV.Sender_ID_TLV.management_Address, gCfm->TLV.Sender_ID_TLV.management_Address_Length);
		add_OrgSpeTLV(ccm_frame+106+gCfm->TLV.Sender_ID_TLV.management_Address_Length,  mep);
		gCfm->TLV.Sender_ID_TLV.length = 1+1+1+gCfm->TLV.Sender_ID_TLV.management_Address_Length;
		uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
		return 0;
	}
	//ccm_frame[105]=0x00;
	add_OrgSpeTLV( ccm_frame+105,  mep);
	gCfm->TLV.Sender_ID_TLV.length=1+1+1;
	uint16_to_uint8(&ccm_frame[101], gCfm->TLV.Sender_ID_TLV.length);
	return 0;
}
int update_TLV(MEP_t mep,struct MEPDatabase_st* temp)
{
			temp->PortStatus= mep->ccpm->recvdPortState;   //update tlv
			temp->InterfaceStatus= mep->ccpm->recvdInterfaceStatus;
			temp->sender_ID_TLV.type = 1;
			temp->sender_ID_TLV.length=mep->ccpm->recvdSenderId.length;
			if(temp->sender_ID_TLV.length < 1)
				return 0;
			temp->sender_ID_TLV.chassis_ID_Length = mep->ccpm->recvdSenderId.chassis_ID_Length;
			if(temp->sender_ID_TLV.chassis_ID_Length <1 )goto MADL;
			temp->sender_ID_TLV.chassis_ID_Subtype = mep->ccpm->recvdSenderId.chassis_ID_Subtype;
			
			memcpy(temp->sender_ID_TLV.chassis_ID , mep->ccpm->recvdSenderId.chassis_ID ,temp->sender_ID_TLV.chassis_ID_Length );
	MADL:	if(mep->ccpm->recvdSenderId.management_Address_Domain_Length < 1)
				return 0;
			temp->sender_ID_TLV.management_Address_Domain_Length =mep->ccpm->recvdSenderId.management_Address_Domain_Length;
			
			memcpy(temp->sender_ID_TLV.management_Address_Domain , mep->ccpm->recvdSenderId.management_Address_Domain , temp->sender_ID_TLV.management_Address_Domain_Length);

			if(mep->ccpm->recvdSenderId.management_Address_Length < 1)
				return 0;
			temp->sender_ID_TLV.management_Address_Length = mep->ccpm->recvdSenderId.management_Address_Length ;
			
			memcpy(temp->sender_ID_TLV.management_Address , mep->ccpm->recvdSenderId.management_Address,temp->sender_ID_TLV.management_Address_Length);
			
			return 0;
}

static int transinterval_count(uint8 interval)
{
	switch(interval)
		{
			
			case 1:
				return 4;
			case 2:
				return 10;
			case 3:
				return 100;
			case 4:
				return 1000;
			case 5:
				return 10000;
			case 6:
				return 60000;
			case 7:
				return 600000;
			default:
				return -1;
		}
	
	return 0;
}

int CCMdatabase_init(ccpm_t ccpm)
{
	if(ccpm == NULL){
		printk("ccpm is NULL\n");
		return -1;
	}
	 LIST_HEAD_INIT(&(ccpm->mep->MEPCCMDatabase.CCMdatabase)); 
	 return 0;
}

static int  CCMdatabase_update(MEP_t mep,unsigned char * recvCCM,int flag)
{
	uint32 recvd_sequence;
	uint16 MEPid;
	struct MEPDatabase_st* temp;
	printk("in CCMdatabase_update\n");
	if (recvCCM == NULL){
		return -1;
	}
	uint8_to_uint16(MEPid, (recvCCM+22+VLAN_TAG_LEN*flag));

	LIST_TRAVERSE(&mep->MEPCCMDatabase.CCMdatabase, temp, list){
		if (temp->RMepId== MEPid){
			memcpy(temp->MACAddr, recvCCM+ADDR_LEN, ADDR_LEN);
			temp->RDI=*(recvCCM+16+VLAN_TAG_LEN*flag)>>7;
	printk(">>>>>>>>>>>RDI:%d  <<<<<<<<<<\n",temp->RDI);
			if((temp->RDI == 1) && (someRDIdefect_pri>= mep->MEPBasic.FaultAarmThreshold))
			{
				mep->ccpm->someRDIdefect=true;
				printk("==============someRDIdefect===========\n");
			}
	printk("recv ccm,MEPID:%d;temp->count:%d\n", temp->RMepId, temp->count);
			temp->count = 0;	

			uint8_to_uint32(recvd_sequence, (recvCCM + VLAN_TAG_LEN*flag+18));
			if(temp->expectedSequence!=recvd_sequence)
				mep->MEPStatus.OutOfSequenceCCMsCount ++;
				//mep->ccpm->out_of_sequence_ccms ++;
			temp->expectedSequence++;
			update_TLV(mep,temp);
			//update tlv
			break;
		}
	}
    if (temp != NULL){
		return temp->RMepId;
	}


	temp = (struct MEPDatabase_st* )kmalloc(sizeof(struct MEPDatabase_st),GFP_KERNEL);
	if (temp == NULL){
		return -1;
	}
	memset(temp, 0, sizeof(struct MEPDatabase_st));
	temp->RMepId=MEPid;
	memcpy(temp->MACAddr, recvCCM+ADDR_LEN, ADDR_LEN);
  	temp->RDI=*(recvCCM+16+VLAN_TAG_LEN*flag)>>7;
	temp->count = 0;

	uint8_to_uint32(recvd_sequence, (recvCCM + VLAN_TAG_LEN*flag + 18));
	temp->expectedSequence = recvd_sequence+1;
	update_TLV(mep,temp);
	LIST_INSERT_TAIL(&mep->MEPCCMDatabase.CCMdatabase, temp, list); 

	mep->ccpm->CCMreceivedEqual=false;
	return MEPid;	
}


static int CCMdatabase_destroy(MEP_t mep)
{
	struct MEPDatabase_st * tmp;
	struct MEPDatabase_st * pre; 

	if(mep->ccpm == NULL){
		printk("ccpm is NULL\n");
		return -1;
	}

	tmp = LIST_FIRST(&mep->MEPCCMDatabase.CCMdatabase); 
	while(tmp!=NULL){ 
		pre = tmp; 
		tmp = LIST_NEXT(tmp, list); 
#if 0
		if(pre->MAID){
			kfree(pre->MAID);
			pre->MAID = NULL;
		}
#endif
		kfree(pre); 
		pre = NULL;  
	} 
	LIST_HEAD_DESTROY(&mep->MEPCCMDatabase.CCMdatabase); 
	return 0;
}

static int ccpm_tlv_init(ccpm_t ccpm)
{
	if(ccpm == NULL){
		return -1;
	}
	ccpm->Port_Status_TLV.type = type_Port_status_TLV;
	ccpm->Interface_Status_TLV.type = type_Interface_Status_TLV;
	return 0;
}

ccpm_t ccpm_init(MEP_t mep)
{	
	 ccpm_t ccpm=NULL;
	 int ret;
	 
	 ccpm=(ccpm_t)kmalloc(sizeof(struct ccpm_st),GFP_KERNEL);
	 if (ccpm == NULL){
	 	printk("fail to kmalloc a new ccpm!\n");
	 	return NULL;
	 }
	 memset(ccpm, 0, sizeof(struct ccpm_st));
	 ccpm->mep = mep;
	 mep->MEPBasic.ma->CCMinterval= 4;
/*
	if(mp->type == MIP)// if MIP,won't initialize CCMdatabase
	 {
		ccpm->MHF_ccr = MHF_CCR_init();
		if(ccpm->MHF_ccr != NULL){
			printk("====================MIP CCR initialized!=============\n");
		}
		else{ 
			printk("==================MIP CCR NOT initialized!===================\n");
			kfree(ccpm);
			return NULL;
		}
	 }
	 */
	 //else
	 {
		ret = CCMdatabase_init(ccpm);
		if(ret < 0){
			printk("fail to init CCM database\n");
			kfree(ccpm);
			ccpm = NULL;
			return NULL;
		}
 		printk("===================ccpm initialized!!++++++++++++++++++\n");
	}
	ccpm_tlv_init(ccpm);
	ccpm_start(ccpm);

	return ccpm;
	
}

int ccpm_start( ccpm_t ccpm)
{
	int interval_value;
	
	//ccpm->CCIsentCCMs=0;//actually initialize to 1 in  documents
	ccpm->mep->MEPStatus.CCMsTransmittedCount = 0;
	ccpm->interval_count=0;
	interval_value = transinterval_count(ccpm->mep->MEPBasic.ma->CCMinterval)/2;//采用0.5ccmlilnterval定时间隔
	if(interval_value <= 0){
		printk("error CCMinterval\n");
		return -1;
	}
	printk("===================ccpm start++++++++++++++++++\n");
	ccpm->CCIenabled = false;//default value
	ccpm->CCIwhile = interval_value;
	ccpm->timer = cfm_timer_create(ccmXmitTimer, ccpm->CCIwhile, (uint32)(ccpm->mep));

	return 0;
}
int ccpm_destroy(ccpm_t ccpm)
{
	if(ccpm == NULL){
		printk("ccpm is NULL\n");
		return -1;
	}
/*
	if(ccpm->MHF_ccr == NULL)
	{
		kfree(ccpm->MHF_ccr);
		ccpm->MHF_ccr = NULL;
	}
*/	
	if (ccpm->timer){
		cfm_timer_destroy(ccpm->timer);
	}
	 CCMdatabase_destroy(ccpm->mep);

	 kfree(ccpm);
	 ccpm = NULL;
	 printk("===================ccpm destroy++++++++++++++++++\n");
 
	 return 0;
}

static int xmitCCM(MEP_t mep)
{
	
	struct ccmpdu_st ccm_frame;
	unsigned char ether_head[20];
	unsigned char test[MAX_CFM_LEN];
	uint16 vlanid;
	uint32 CCM_length;
	printk("in xmitCCM\n");
	if(mep == NULL){
		return -1;
	}
	

	memset(&ccm_frame,0,sizeof(struct ccmpdu_st));
	//ccm_frame.header=* generateCFMHeader(mp->MDLevel,type_CCM);
	//ccm_frame.header.flags=(mp->presentRDI<<7)&(mep->ccpm->mep->MEPBasic.ma->CCMinterval);

	ccm_frame.header.mdLevel_version=(mep->MEPBasic.ma->MDPointer->MDLevel<<5)&0xff;
	ccm_frame.header.opCode=0x01;
	ccm_frame.seqNumber=mep->MEPStatus.CCMsTransmittedCount;
	ccm_frame.MEPid=mep->MEPBasic.MEPId;
	printk("________MEPid:%d________\n",ccm_frame.MEPid);
	ccm_frame.header.firstTLVOffset=70;
	ccm_frame.header.flags=(mep->ccpm->someRMEPCCMdefect<<7)|(mep->ccpm->mep->MEPBasic.ma->CCMinterval);
	
	memset(test,0,sizeof(test));
	memset(ether_head,0,sizeof(ether_head));
	memcpy(ether_head+ADDR_LEN, mep->MEPStatus.MACAddress, ADDR_LEN);
										
	
	ether_head[0]=0x01;ether_head[1]=0x80;
	ether_head[2]=0xc2;ether_head[3]=0x00;
	ether_head[4]=0X00;ether_head[5]=0x30+mep->MEPBasic.ma->MDPointer->MDLevel;
	ether_head[12]=0x81;ether_head[13]=0x00;

	if(mep->MEPBasic.PrimaryVlan != 0)
		vlanid = mep->MEPBasic.PrimaryVlan;
	else vlanid = mep->MEPBasic.ma->AssociatedVLANs[0];
	ether_head[14] = (vlanid>>8) & 0x0F;
	ether_head[15] = vlanid & 0xFF;

	ether_head[16]=0x89;
	ether_head[17]=0x02;
	memcpy(test,ether_head,18);
	
	test[18]=ccm_frame.header.mdLevel_version;
	test[19]=ccm_frame.header.opCode;
	test[20]=ccm_frame.header.flags;
	test[21]=ccm_frame.header.firstTLVOffset;
	uint32_to_uint8(&test[22],ccm_frame.seqNumber);
	uint16_to_uint8(&test[26], ccm_frame.MEPid);
	memcpy(test+28, mep->MEPBasic.ma->ShortMAname, 48);
	printk("-------------------------------\n");
	printk("MAID:%s\n",mep->MEPBasic.ma->ShortMAname);
	print_hex_data(mep->MEPBasic.ma->ShortMAname, 48);
	printk("-------------------------------\n");
	printk("SenderID TLV type: %x\n",gCfm->TLV.Sender_ID_TLV.type);

							//[76]~[91] bytes ,defined by ITU-T Y.1731
	
		 consturct_TLV(test, mep ,mep->MEPBasic.ma->SenderIDPermission);
#if 0	
	test[92]=mep->ccpm->Port_Status_TLV.type;			    //Port Status TLV
	uint16_to_uint8(&test[93], mep->ccpm->Port_Status_TLV.length);
	test[95]=mep->ccpm->Port_Status_TLV.port_status_value;
	
	test[96]=mep->ccpm->Interface_Status_TLV.type;//interface status TLV
	uint16_to_uint8(&test[97],mep->ccpm->Interface_Status_TLV.length);
	test[99]=mep->ccpm->Interface_Status_TLV.interface_status;

	test[100]=gCfm->TLV.Sender_ID_TLV.type;				//sender ID TLV
	uint16_to_uint8(&test[101], gCfm->TLV.Sender_ID_TLV.length);
	test[103]=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
	test[104]=gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
	memcpy(&test[105],gCfm->TLV.Sender_ID_TLV.chassis_ID , gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	test[105+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length]=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
	memcpy(test+106+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length, gCfm->TLV.Sender_ID_TLV.management_Address_Domain , gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	test[106+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length]=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
	memcpy(test+107+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length, gCfm->TLV.Sender_ID_TLV.management_Address , gCfm->TLV.Sender_ID_TLV.management_Address_Length);
	test[108+gCfm->TLV.Sender_ID_TLV.chassis_ID_Length + gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length+ gCfm->TLV.Sender_ID_TLV.management_Address_Length]=0x00;
#endif
	printk("================chassis ID TLV==============\n");
	print_hex_data(gCfm->TLV.Sender_ID_TLV.chassis_ID, gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	printk("================management address domain===================\n");
	print_hex_data(gCfm->TLV.Sender_ID_TLV.management_Address_Domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	if(mep->MEPBasic.ma->SenderIDPermission == None)
		CCM_length=101;
	else	CCM_length=92+4+4+3+gCfm->TLV.Sender_ID_TLV.length+1;
	mep->MEPStatus.CCMsTransmittedCount++;
	print_hex_data(test, CCM_length);
#if 1
	if(mep->Direction== UPMP){
		cfm_send(test,CCM_length,mep->srcPortId,mep->FlowId,mep,MEP,Inside,SENDTO);
	}
	else{
		cfm_send(test,CCM_length,mep->srcPortId,mep->FlowId,mep,MEP,Outside,SENDTO);
	}
#endif
	printk("xmitCCM end\n");
 	return 0;                  

}

static void ccmXmitTimer(uint32 arg)
{	
	MEP_t mep = (MEP_t)arg;
	struct MEPDatabase_st * temp;
	//printk("in ccmXmitTimer\n");

	if(mep->ccpm->CCIenabled == false)
	{
		//printk("CCI is not activated!!\n");
	}
	else
	{
		printk("ccm ok\n");
#if 1
		if(mep->ccpm->interval_count%2==0)
		{
			printk("++++++++++++CCMreceivedEqual:%d+++++++++++++++++++++++++\n",mep->ccpm->CCMreceivedEqual);
			xmitCCM(mep);//every 2*0.5ccminterval send one ccm 
		}
	 	LIST_TRAVERSE(&mep->MEPCCMDatabase.CCMdatabase,temp, list)
	 	{
	 		temp->count++;
			printk("=======ccmXmitTimer    count:%d   =======\n", temp->count);
			if(temp->count >= 7)
			{
				temp->count = 0;
				mep->rMEPCCMdefect= true;
				if(someRMEPCCMdefect_pri >= mep->MEPBasic.FaultAarmThreshold)
				mep->ccpm->someRMEPCCMdefect=true;
			printk("=============someRMEPCCMdefect===========\n");
			
			}
		 }
#endif
	}

	mep->ccpm->interval_count++;//是否要加模数循环?
	
	fault_notification_generator(mep);
	mep->ccpm->CCIwhile = transinterval_count(mep->ccpm->mep->MEPBasic.ma->CCMinterval)/2;
	cfm_timer_continue(mep->ccpm->timer, mep->ccpm->CCIwhile);
	
}



static int CCM_discard(uint8 *buffer)
{
	printk("in CCM_discard");
	
	if(buffer == NULL){
		printk("buffer is NULL\n");
		return -1;
	}

	return 0;
}


static int CCM_validation(uint8 *buffer,MEP_t mep,int flag,uint32 len)
{
	uint16 mepid;

	printk("=======in CCM_validation=======\n");
	uint8_to_uint16(mepid,(buffer+22+VLAN_TAG_LEN*flag));
	if((buffer == NULL)||(mep == NULL)||(len < 71)){
		printk("buffer is NULL||mp is NULL|| not enough length\n");
		return -1;
	}
	if( memcmp(buffer+24+VLAN_TAG_LEN*flag,mep->MEPBasic.ma->ShortMAname,48) )//MAID不一致,暂设置为比较前10bytes
	{
		return -2;
	}
	if( mepid == mep->MEPBasic.MEPId)//MEPid 相同
	{
		return 0;
	}
	else if((*(buffer+16+VLAN_TAG_LEN*flag)&0x07)!=mep->ccpm->mep->MEPBasic.ma->CCMinterval||(*(buffer+16+VLAN_TAG_LEN*flag)&0x07)==0){
		return 0;
	}
	//else if()//MEPid of the received CCM not configed

	return 1;
}


static int MEPprocessLowCCM(uint8*buffer, uint32 len, MEP_t mep, int flag)
{
	int ret;
	printk("in MEPprocessLowCCM\n");
	//if((buffer == NULL)||(mp == NULL)||(len < 71)){
	//	printk("buffer is NULL||mp is NULL|| not enough length\n");
	//}

	ret = CCM_validation(buffer,mep,flag,len);
	
	if(ret == -1){
		return -1;
	}
	else if(ret == 0)
	{
		CCM_discard(buffer);
		return -1;
	}

	mep->xconCCMreceived=true;
	if( xconCCMdefect_pri >= mep->MEPBasic.FaultAarmThreshold )mep->xconCCMdefect = true;
printk("============xconCCMdefect============\n");
	memset(mep->ccpm->CCMlowPDU.data, 0, sizeof(mep->ccpm->CCMlowPDU.data));
	memcpy(mep->ccpm->CCMlowPDU.data, buffer, len);
	mep->ccpm->CCMlowPDU.len= len;
	
	mep->ccpm->recvdInterval=buffer[16+VLAN_TAG_LEN*flag]&0x07;//set timer=recvdInterval ,time out&not receive lowCCM again
	//memset(&mep->xconCCMlastFailure,0,sizeof(struct dataStream_st));
	memcpy(mep->MEPStatus.LastReceivedXconCCM, buffer, 128);
	//mep->xconCCMlastFailure.length=len;
	return 0;
}

static int MEPprocessEqualCCM(uint8 * buffer, uint32 len, MEP_t mep, int flag)
{
	int ret;
	uint16 mark;
	uint16 tlv_length;
	
printk("in MEPprocessEqualCCM\n");
printk("----------you got this good stuff---------\n");
printk("###########interval_value(1/2 CCM interval): %d #########\n",mep->ccpm->CCIwhile);\

	if((buffer == NULL)||(mep == NULL)){
		printk("buffer is NULL||mp is NULL\n");
	}

	ret = CCM_validation(buffer,mep,flag,len);
	if(ret == -1){
		return -1;
	}
	else if(ret == -2)
		{
			if(xconCCMdefect_pri >= mep->MEPBasic.FaultAarmThreshold)mep->xconCCMdefect = true;
	printk("============xconCCMdefect============\n");

			mep->ccpm->recvdInterval=buffer[16+VLAN_TAG_LEN*flag]&0x07;
			//memset(&mep->xconCCMlastFailure,0,sizeof(struct dataStream_st));
			//memcpy(mep->xconCCMlastFailure.data, buffer, sizeof(mep->xconCCMlastFailure.data));
			memcpy(mep->MEPStatus.LastReceivedXconCCM ,buffer ,128);
			//mep->xconCCMlastFailure.length=len;
			return -1;
		}
	else if(ret == 0)
	{  
		printk("=============errorCCMreceived!==========\n");
		mep->errorCCMreceived=true;
		if(errorCCMdefect_pri >= mep->MEPBasic.FaultAarmThreshold)mep->errorCCMdefect =true;

		mep->ccpm->recvdInterval=buffer[16+VLAN_TAG_LEN*flag]&0x07;
		//memset(mep->errorCCMlastFailure,0,sizeof(struct dataStream_st));
		//memcpy(mep->errorCCMlastFailure.data, buffer, sizeof(mep->xconCCMlastFailure.data));
		memcpy(mep->MEPStatus.LastReceivedErrorCCM , buffer ,128);
		//mep->errorCCMlastFailure.length=len;
		return -1;
	}//timer set to recvdInterval=CCM Interval field in the received CCM

	if((*(buffer+16+VLAN_TAG_LEN*flag)&0x80)==0x80)mep->ccpm->recvdRDI=true;
	memcpy(mep->ccpm->recvdMacAddress,buffer+ADDR_LEN,ADDR_LEN);
	mep->ccpm->recvdInterval=buffer[16+VLAN_TAG_LEN*flag]&0x07;
	mark=14+VLAN_TAG_LEN*flag+74;
start:while((buffer[mark] != 0)&&(buffer!= NULL))
	{
		switch(buffer[mark])
		{
			case 2 : 
				{	uint8_to_uint16(tlv_length, (buffer+mark+1));
					if(tlv_length < 1)
						{
							printk("No port status TLV present!\n ");
							mark+=3;
							break;
						}
					mep->ccpm->recvdPortState  = buffer[mark+3];
					mark+=4;
					goto start;
					break;
				}
			case 4 :
				{	uint8_to_uint16(tlv_length, (buffer+mark+1));
					if(tlv_length < 1)
						{
							printk("No interface status TLV present!\n ");
							mark+=3;
							goto start;
							break;
						}
					mep->ccpm->recvdInterfaceStatus = buffer[mark+3];
					mark+=4;
					goto start;
					break;
				}
			case 1 :
				{	//&(mep->ccpm->recvdSenderId) = (Sender_ID_TLV_t)kmalloc(sizeof(struct Sender_ID_TLV_st),GFP_KERNEL);
					mep->ccpm->recvdSenderId.type = 1;
					uint8_to_uint16(tlv_length, (buffer+mark+1));
					if(tlv_length < 1)
						{
							printk("No sender ID TLV present!\n");
							mark+=3;
							goto start;
							break;
						}
					mep->ccpm->recvdSenderId.length = tlv_length;
					mark+=3;
					mep->ccpm->recvdSenderId.chassis_ID_Length = buffer[mark];
					if(buffer[mark] < 1)
						{
							printk("No chassis ID&chassis ID subtype present!\n");
							mark+=1;
							goto MADL;	
							
						
						}
					mark+=1;
					mep->ccpm->recvdSenderId.chassis_ID_Subtype = buffer[mark];
					mark+=1;
					
					memcpy(mep->ccpm->recvdSenderId.chassis_ID,buffer+mark,mep->ccpm->recvdSenderId.chassis_ID_Length );
					mark+=mep->ccpm->recvdSenderId.chassis_ID_Length;
			 MADL:  mep->ccpm->recvdSenderId.management_Address_Domain_Length = buffer[mark];
			 		if(buffer[mark] < 1)
			 			{
			 				printk("Management Address Domain, Management Address Length,\n");
							printk("and Management Address fields are not present!!\n ");
							mark+=1;
							goto start;
							break;
			 			}
					mark+=1;
					
					memcpy(mep->ccpm->recvdSenderId.management_Address_Domain, buffer+mark ,mep->ccpm->recvdSenderId.management_Address_Domain_Length);
					mark += mep->ccpm->recvdSenderId.management_Address_Domain_Length;
					mep->ccpm->recvdSenderId.management_Address_Length = buffer[mark];
					if(buffer[mark] < 1)
			 			{
			 				printk("No Management Address field is present!\n");
							mark+=1;
							goto start;
							break;
						}
					mark += 1;
					
					
					memcpy(	mep->ccpm->recvdSenderId.management_Address , buffer+mark,mep->ccpm->recvdSenderId.management_Address_Length );
					mark += mep->ccpm->recvdSenderId.management_Address_Length ;
				}
			case 31:
				{
					uint8_to_uint16(tlv_length, buffer+mark+1);
					mark +=(7+tlv_length);
					goto start;
				}		
			case 0 :break;	
			default:printk("============undefined TLV!!============\n");
		}
	}
	memset(mep->ccpm->CCMequalPDU.data, 0, sizeof(mep->ccpm->CCMequalPDU.data));
	mep->ccpm->CCMequalPDU.len = len;
	memcpy(mep->ccpm->CCMequalPDU.data,buffer,len);
	
	printk("############## CCMdatabase_update returns %d #####################\n",	CCMdatabase_update(mep,buffer,flag));
	mep->rMEPlastPortState = mep->ccpm->recvdPortState;
	mep->rMEPlastInterfaceStatus = mep->ccpm->recvdInterfaceStatus;
	memcpy(&mep->rMEPlastSenderId ,& mep->ccpm->recvdSenderId,mep->ccpm->recvdSenderId.length+3);
	mep->ccpm->CCMreceivedEqual=true;
	
	mep->rMEPportStatusDefect =mep->rMEPportStatusDefect && ((mep->rMEPlastPortState != psUp)&& (mep->rMEPlastPortState != 0));
	mep->rMEPinterfaceStatusDefect = mep->rMEPinterfaceStatusDefect ||((mep->rMEPlastInterfaceStatus != isUp )&& (mep->rMEPlastInterfaceStatus != 0));
	if(someMACstatusDefect_pri >= mep->MEPBasic.FaultAarmThreshold)
		mep->ccpm->someMACstatusDefect = mep->rMEPportStatusDefect || mep->rMEPinterfaceStatusDefect;
	return 0;
}

int ccm_process(uint8 * buffer, uint32 len, uint32 bridge_id, uint32 flow_id, int flag,MEP_t mep)
{
printk("\n--------------- in ccp_process!!------------\n");
	if ((buffer == NULL)||(mep == NULL)){
		printk("no ccm to process!\n");
		return -1;
	}
/*
	if(mp->type == MIP)
	{
		return MHFprocessCCM( buffer,  len,  bridge_id,  flow_id,  flag,  mp);
	}
	*/ 		
	 if(((*(buffer+14+VLAN_TAG_LEN*flag))>>5)==(mep->MEPBasic.ma->MDPointer->MDLevel))MEPprocessEqualCCM(buffer,len,mep,flag);//check (mp->MDLevel) and decide which function to process
	 else if(((*(buffer+14+VLAN_TAG_LEN*flag))>>5)<(mep->MEPBasic.ma->MDPointer->MDLevel)) MEPprocessLowCCM(buffer,len,mep,flag);
	 else CCM_discard(buffer);

 	return 0;
}


struct MHF_ccr_st * MHF_CCR_init(void)
{		
	struct MHF_ccr_st * MHF_ccr;
	MHF_ccr=(struct MHF_ccr_st *)kmalloc(sizeof(struct MHF_ccr_st),GFP_KERNEL);
	if(MHF_ccr == NULL)
		{
			printk("fail to kmalloc for a new MHF CCR!\n");
			return NULL;
		}
	memset(MHF_ccr,0,sizeof(struct MHF_ccr_st));
	//LIST_HEAD_INIT(&MHF_ccr->MIPdatabase);
	return MHF_ccr;	
}
int MHF_CCR_destroy(struct MHF_ccr_st * ccpm)
{
	if(ccpm==NULL)
		return -1;
	kfree(ccpm);
	return 0;
}

int MIPdatabase_destroy(cfm_t cfm)
{
	struct MIPdatabase_st *tmp,*pre;
	
	if(cfm == NULL){
		printk("cfm is NULL\n");
		return -1;
	}

	tmp = LIST_FIRST(&cfm->mip_info.MIPdatabase); 
 	while(NULL != tmp){ 
 	 pre = tmp; 
	 tmp = LIST_NEXT(tmp, list); 
  	 kfree(pre); 
  	 pre = NULL;  
	} 
	LIST_HEAD_DESTROY(&cfm->mip_info.MIPdatabase); 
	
	return 0;
}
int MIPdatabase_query(uint8* MACaddr,int vlan_id, uint16 *PortID)
{
	struct MIPdatabase_st* temp;
	LIST_TRAVERSE(&gCfm->mip_info.MIPdatabase,temp, list)
		{
			if((memcmp(temp->source_address,MACaddr,6)==0)&&(temp->VID == vlan_id))
				*PortID = temp->port_number;
			return 0;
		}
	return -1;
}
int MIPdatabase_update(unsigned char * recvCCM, int flag, uint32 bridge_id)
{
	struct MIPdatabase_st* temp;
	uint8 source_mac[ADDR_LEN];
	uint16 vid;

	printk("=========in MIPdatabase_update========\n");
	memcpy(source_mac,recvCCM+ADDR_LEN,ADDR_LEN);
	memcpy(&vid,recvCCM+14,2);
	vid=0x0FFF&vid;
	
	LIST_TRAVERSE(&gCfm->mip_info.MIPdatabase, temp, list)
		{
			if( memcmp(temp->source_address,source_mac,ADDR_LEN)==0)
			{										
				temp->VID=vid;
				temp->port_number=bridge_id;
				return 0;
			}
		}
	
		temp = (struct MIPdatabase_st*)kmalloc(sizeof(struct MIPdatabase_st),GFP_KERNEL);
	if (temp == NULL){
		printk("kmalloc of new MIPdatabase node fail!\n");
		return -1;
	}
	
	memset(temp, 0, sizeof(struct MIPdatabase_st));
	memcpy(temp->source_address,source_mac,ADDR_LEN);
	temp->VID=vid;
	temp->port_number=bridge_id;
	LIST_INSERT_TAIL(&gCfm->mip_info.MIPdatabase, temp, list); 
	return 0;
	
}

int MHFprocessCCM(uint8 * buffer, uint32 len,uint32 bridge_id, uint32 flow_id, int flag,MIP_t mip )
{
printk("==================in MHFprocess!================\n");

	if((buffer == NULL)||(mip == NULL)||(len < 71)) //MEPID MAID not examined
		{
			printk("buffer is NULL||mp is NULL|| not enough length\n");
			CCM_discard(buffer);
			return -1;
		}
	mip->MHF_ccr->MHFrecvdCCM=true;
	memcpy(mip->MHF_ccr->MHFCCMPDU.data, buffer , len );
	mip->MHF_ccr->MHFCCMPDU.len=len;
	MIPdatabase_update(buffer,flag,bridge_id);
	return 0;
	
}

