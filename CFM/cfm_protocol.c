
#include <linux/fs.h>
#include "cfm_protocol.h"
#include "cfm_header.h"
#include "cfm_cmd.h"

static int cfm_tlv_init(cfm_t cfm);
static void cfm_InitCFMDefault(uint16 meid, struct DefaultMDLevel_st *DefaultMDLevel);
static void cfm_DestroyCFMDefault(struct DefaultMDLevel_st *DefaultMDLevel);
static void cfm_InitCFMStack(uint16 meid, struct cfm_stack_st * stack);
static void cfm_DestroyCFMStack(struct cfm_stack_st * stack);


static int cfm_tlv_init(cfm_t cfm)
{
	if(cfm == NULL){
		return -1;
	}
	cfm->TLV.Sender_ID_TLV.type = type_Sender_ID_TLV;
	cfm->TLV.Organization_Specific_TLV.type = type_Organization_Specific_TLV;
	return 0;
}

cfm_t cfm_create(void)
{
	cfm_t new_cfm = NULL;

	new_cfm = (cfm_t)kmalloc(sizeof(struct cfm_st), GFP_KERNEL);
	if(new_cfm == NULL){
		printk("fail to init cfm_t\n");
		return NULL;
	}
	memset(new_cfm, 0, sizeof(struct cfm_st));
	LIST_HEAD_INIT(&new_cfm->md_info.md_list);
	cfm_InitCFMStack(1, &new_cfm->cfm_stack);//todo cfm stack meid
	cfm_InitCFMDefault(1, &new_cfm->DefaultMDLevel);
	LIST_HEAD_INIT(&new_cfm->ma_info.ma_list);
	LIST_HEAD_INIT(&new_cfm->mep_info.mep_list);
	LIST_HEAD_INIT(&new_cfm->mip_info.mip_list);
#ifdef CFM_EMULATE
	FilteringDatabase_init(new_cfm);
#endif
	new_cfm->recv_queue = queue_create(QUEUE_MAXSIZE);
	new_cfm->send_queue = queue_create(QUEUE_MAXSIZE);
	cfm_tlv_init(new_cfm);
	
	return new_cfm;
}

int cfm_destroy(cfm_t cfm)
{
	struct MD_st *tmd, *pmd;
	struct MA_st *tma, *pma;
	struct MEP_st *tmep, *pmep;
	struct MIP_st *tmip, *pmip;
	printk("in cfm_destroy\n");
	if(cfm == NULL){
		printk("cfm is NULL\n");
		return -1;
	}
	/*process MD list*/
	tmd = LIST_FIRST(&cfm->md_info.md_list);
	while(NULL != tmd){
		printk("delete md id=%d\n",tmd->meid);
		pmd = tmd;
		tmd = LIST_NEXT(tmd, list);
		kfree(pmd);
		pmd = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->md_info.md_list);
	/*process MA list*/
	tma= LIST_FIRST(&cfm->ma_info.ma_list);
	while(NULL != tma){
		printk("delete ma id=%d\n",tma->meid);
		pma = tma;
		tma = LIST_NEXT(tma, list);
		kfree(pma);
		pma = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->ma_info.ma_list);
	/*process MEP list*/
	tmep = LIST_FIRST(&cfm->mep_info.mep_list);
	while(NULL != tmep){
		printk("delete MEP id=%d\n",tmep->MEPBasic.meid);
		pmep = tmep;
		tmep = LIST_NEXT(tmep, list);
		if(pmep->lbpm){
			lbpm_destroy(pmep->lbpm);
		}
		if(pmep->ccpm){
			ccpm_destroy(pmep->ccpm);
		}
		if(pmep->ltpm){
			ltpm_destroy(pmep->ltpm);
		}
		kfree(pmep);
		pmep = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->mep_info.mep_list);
	/*process MIP list*/
	tmip = LIST_FIRST(&cfm->mip_info.mip_list);
	while(NULL != tmip){
		printk("delete MIP id=%d\n",tmip->meid);
		pmip = tmip;
		tmip = LIST_NEXT(tmip, list);
		if(pmip->lbpm){
			lbpm_destroy(pmip->lbpm);
		}
		if(pmip->MHF_ccr){
			MHF_CCR_destroy(pmip->MHF_ccr);
		}
		if(pmip->ltpm){
			ltpm_destroy(pmip->ltpm);
		}
		kfree(pmip);
		pmip = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->mip_info.mip_list);
	/*process CFM Stack*/
	printk("DestroyCFMStack\n");
	cfm_DestroyCFMStack(&cfm->cfm_stack);
	/*process CFM Default MD Level*/
	printk("DestroyCFMDefault\n");
	cfm_DestroyCFMDefault(&cfm->DefaultMDLevel);

#ifdef CFM_EMULATE
	FilteringDatabase_destroy(cfm);
#endif

	if (cfm->recv_queue){
		queue_destroy(cfm->recv_queue);
	}
	if (cfm->send_queue){
		queue_destroy(cfm->send_queue);
	}

	kfree(cfm);
	cfm = NULL;

	return 0;
}

int cfmSupervisory_Stop(cfm_t cfm)
{
	struct MEP_st *tmep, *pmep;
	struct MIP_st *tmip, *pmip;
	printk("now in cfmSupervisory_Stop\n");
	if(cfm == NULL){
		printk("no cfm or mp_list\n");
		return -1;
	}

	/*process MEP list*/
	tmep = LIST_FIRST(&cfm->mep_info.mep_list);
	while(NULL != tmep){
		printk("delete MEP id=%d\n",tmep->MEPBasic.meid);
		pmep = tmep;
		tmep = LIST_NEXT(tmep, list);
		if(pmep->lbpm){
			lbpm_destroy(pmep->lbpm);
		}
		if(pmep->ccpm){
			ccpm_destroy(pmep->ccpm);
		}
		if(pmep->ltpm){
			ltpm_destroy(pmep->ltpm);
		}
		kfree(pmep);
		pmep = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->mep_info.mep_list);
	/*process MIP list*/
	tmip = LIST_FIRST(&cfm->mip_info.mip_list);
	while(NULL != tmip){
		printk("delete MEP id=%d\n",tmip->meid);
		pmip = tmip;
		tmip = LIST_NEXT(tmip, list);
		if(pmip->lbpm){
			lbpm_destroy(pmip->lbpm);
		}
		if(pmip->MHF_ccr){
			MHF_CCR_destroy(pmip->MHF_ccr);
		}
		if(pmip->ltpm){
			ltpm_destroy(pmip->ltpm);
		}
		kfree(pmip);
		pmip = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->mip_info.mip_list);
	printk("cfmSupervisory_Stop end\n");
	return 0;
}
//MD list operations



static void cfm_InitCFMDefault(uint16 meid, struct DefaultMDLevel_st *DefaultMDLevel)
{
	DefaultMDLevel->meid=meid;
	DefaultMDLevel->L2_type = 0x00;
	DefaultMDLevel->CatchallLevel = 0x00;
	DefaultMDLevel->CatchallMHFCreation = MHF_Creation_None;
	DefaultMDLevel->CatchallSenderIDPermission = None;
	LIST_HEAD_INIT(&DefaultMDLevel->defaultMDLevel_list);
}

static void cfm_DestroyCFMDefault(struct DefaultMDLevel_st *DefaultMDLevel)
{
	struct DefaultMDLevelEntry_st *temp, *pre;
	temp = LIST_FIRST(&DefaultMDLevel->defaultMDLevel_list);
	while(NULL != temp){
		pre = temp;
		temp = LIST_NEXT(temp, list);
		kfree(pre);
		pre = NULL;	
	}
	LIST_HEAD_DESTROY(&DefaultMDLevel->defaultMDLevel_list);
	printk("in cfm_DestroyCFMDefault\n");
}
/*
event:
1. meid
2. Layer 2 type
3. Catchall level
4. Catchall MHF creation
5. Catchall sender ID permission
return value:
-1: common error
0: get ok
*/
int cfmSupervisory_GetCFMDefaultCatchall(uint8 event, uint8 *value8, uint8 *value16)
{
	if(gCfm == NULL){
		return -1;
	}

	switch(event){
		case 1:
			{
				*value16 = gCfm->DefaultMDLevel.meid;
				break;
			}
		case 2:
			{
				*value8 = gCfm->DefaultMDLevel.L2_type;
				break;
			}
		case 3:
			{
				*value8 = gCfm->DefaultMDLevel.CatchallLevel;
				break;
			}
		case 4:
			{
				*value8 = gCfm->DefaultMDLevel.CatchallMHFCreation;
				break;
			}
		case 5:
			{
				*value8 = gCfm->DefaultMDLevel.CatchallSenderIDPermission;
			}
		default:
			{
				return -1;
			}
	}

	return 0;
}
/*
event:
1. Catchall level
2. Catchall MHF creation
3. Catchall sender ID permission
return value:
-1: common error
0: set ok
*/
int cfmSupervisory_SetCFMDefaultCatchall(uint8 event, uint8 *value8)
{
	if(gCfm == NULL){
		return -1;
	}

	switch(event){
		case 1:
			{
				gCfm->DefaultMDLevel.CatchallLevel = *value8;
				break;
			}
		case 2:
			{
				gCfm->DefaultMDLevel.CatchallMHFCreation = *value8;
				break;
			}
		case 3:
			{
				gCfm->DefaultMDLevel.CatchallSenderIDPermission =* value8;
				break;
			}
		default:
			{
				return -1;
			}
	}
	return 0;
}
/*
-1: common error
0: not found
1: found
*/
int cfmSupervisory_GetCFMDefaultTable(void *param)
{
	struct DefaultMDLevelEntry_st *temp=NULL;
	struct Test_DefaultMDLevelEntry_st *DefaultMDLevel=(struct Test_DefaultMDLevelEntry_st*)param;
	if(gCfm == NULL){
		return -1;
	}
	
	LIST_TRAVERSE(&gCfm->DefaultMDLevel.defaultMDLevel_list, temp, list){
		if(temp->PrimaryVlanId == DefaultMDLevel->PrimaryVlanId){
			memcpy(DefaultMDLevel->AssociatedVLANs, temp->AssociatedVLANs, 11*sizeof(uint16));
			DefaultMDLevel->Status = temp->Status;
			DefaultMDLevel->Level = temp->Level;
			DefaultMDLevel->MHFCreation = temp->MHFCreation;
			DefaultMDLevel->SenderIDPermission = temp->SenderIDPermission;
			return 1;
		}
	}

	return 0;
}
/*
event:
1.Level;2.MHFCreation;3.SenderIDPermission.
//
return value:
-1: common error
1: Operation rejected because creating or assigning these VIDs to this MD Level would exceed
the number of MD Levels supported by some Bridge Port
2:Operation rejected because the VID list provided [item a) in 12.14.3.2.2] specifies a different
Primary VID, or contains one or more but not all of the VIDs, in the definition of an MA or
some other entry in the Default MD Level managed object; 
3: Operation accepted
*/
int cfmSupervisory_SetCFMDefaultTable(uint16 *vidList, uint8 event, uint8 value)
{
	struct DefaultMDLevelEntry_st *temp;

	if(gCfm == NULL){
		return -1;
	}
	
	LIST_TRAVERSE(&gCfm->DefaultMDLevel.defaultMDLevel_list, temp, list){
		if(temp->PrimaryVlanId == vidList[0]){
			break;;
		}
	}
	if(temp==NULL)return -1;
	switch(event){
		case 1:
			{
				temp->Level = value;
				break;
			}
		case 2:
			{
				temp->MHFCreation = value;
				break;
			}
		case 3:
			{
				temp->SenderIDPermission = value;
				break;
			}
		default:
			{
				break;
			}
	}

	return 3;
}

/*CFM Stack operate start*/
static void cfm_InitCFMStack(uint16 meid, struct cfm_stack_st * stack)
{
	stack->meid = meid;
	stack->L2_type = 0x00;
	LIST_HEAD_INIT(&stack->mpStatus_list);
	LIST_HEAD_INIT(&stack->configurationError_list);
}
static void cfm_DestroyCFMStack(struct cfm_stack_st * stack)
{
	struct MPStatusEntry_st *temp, *pre;
	struct ConfigurationErrorEntry_st *tmp, *pr;
	
	temp = LIST_FIRST(&stack->mpStatus_list);
	while(NULL != temp){
		pre = temp;
		temp = LIST_NEXT(temp, list);
		kfree(pre);
		pre = NULL;	
	}
	LIST_HEAD_DESTROY(&stack->mpStatus_list);

	tmp = LIST_FIRST(&stack->configurationError_list);
	while(NULL != tmp){
		pr = tmp;
		tmp = LIST_NEXT(tmp, list);
		kfree(pr);
		pr = NULL;	
	}
	LIST_HEAD_DESTROY(&stack->configurationError_list);
}
int cfm_AddCFMMPStatus(struct MPStatusEntry_st *MPStatusEntry)
{
	if(gCfm == NULL){
		return -1;
	}
	gCfm->cfm_stack.MPStatusEntry_num++;
	LIST_INSERT_TAIL(&gCfm->cfm_stack.mpStatus_list, MPStatusEntry, list);
	return 0;
}
int cfm_DeleteCFMMPStatus(uint16 PortId, uint8 Level, uint8 Direction, uint16 VlanId)
{
	struct MPStatusEntry_st *temp;

	if(gCfm == NULL){
		return -1;
	}
	LIST_TRAVERSE_SAFE_BEGIN(&gCfm->cfm_stack.mpStatus_list, temp, list){
		if((temp->PortId == PortId)&&(temp->Level = Level)&&(temp->Direction == Direction)&&(temp->VlanId == VlanId)){
			LIST_REMOVE_CURRENT(&gCfm->cfm_stack.mpStatus_list, list);
			kfree(temp);
			temp = NULL;
			gCfm->cfm_stack.MPStatusEntry_num--;
			//return 0;
		}
	}
	LIST_TRAVERSE_SAFE_END

	return 0;
}
/*
return value:
-1: common error
1: error input
2: no MEP or MHF
3: MEP
4: MHF
*/
int cfmSupervisory_GetCFMMPStatus(void *param)
{
	struct Test_MPStatusEntry_st *MPStatusEntry=(struct Test_MPStatusEntry_st*)param;
	struct MPStatusEntry_st *temp=NULL;
	if(gCfm == NULL){
		return -1;
	}

	if(MPStatusEntry->Level > 7){
		return 1;
	}
	if((MPStatusEntry->Direction < 1)||(MPStatusEntry->Direction > 2)){
		return 1;
	}
	
	LIST_TRAVERSE(&gCfm->cfm_stack.mpStatus_list, temp, list){
		if((temp->PortId == MPStatusEntry->PortId)&&(temp->Level = MPStatusEntry->Level)&&(temp->Direction == MPStatusEntry->Direction)&&(temp->VlanId == MPStatusEntry->VlanId)){
			MPStatusEntry->MD = temp->MD;
			MPStatusEntry->MA = temp->MA;
			MPStatusEntry->MEPId = temp->MEPId;
			memcpy(MPStatusEntry->MAC, temp->MAC, ADDR_LEN);
			if(MPStatusEntry->MEPId == 0){
				return 4;
			}
			return 3;
		}
	}

	return 2;
}
/*
return value:
-1: common error
1: illegal vlan_ident
2: illegal or non-existent Bridge Port or aggregated port
3: Operation accepted
*/
int cfmSupervisory_GetCFMConfigurationError(void*param)
{
	struct ConfigurationErrorEntry_st *temp=NULL;
	struct Test_ConfigurationErrorEntry_st* ConfigurationErrorEntry=(struct Test_ConfigurationErrorEntry_st*)param;;
	if(gCfm == NULL){
		return 1;
	}
	
	LIST_TRAVERSE(&gCfm->cfm_stack.configurationError_list, temp, list){
		if((temp->PortId == ConfigurationErrorEntry->PortId)&&(temp->VlanId == ConfigurationErrorEntry->VlanId)){
			ConfigurationErrorEntry->DetectedConfigurationError = temp->DetectedConfigurationError;
			return 3;
		}
	}
	return -1;
}


/*
return value:
-1: common error
0: get is ok
*/
int cfmSupervisory_GetCFMStack(void *pstack)
{
	struct Test_CFMStack_st *stack = (struct Test_CFMStack_st *)pstack;
	struct MPStatusEntry_st *temp;
	struct ConfigurationErrorEntry_st *tmp;
	int i=0;

	if(gCfm == NULL){
		return -1;
	}
	
	stack->meid = gCfm->cfm_stack.meid;
	stack->L2_type = gCfm->cfm_stack.L2_type;
	LIST_TRAVERSE(&gCfm->cfm_stack.mpStatus_list, temp, list){
		stack->MPStatus[i].PortId = temp->PortId;
		stack->MPStatus[i].Level = temp->Level;
		stack->MPStatus[i].Direction = temp->Direction;
		stack->MPStatus[i].VlanId = temp->VlanId;
		stack->MPStatus[i].MD = temp->MD;
		stack->MPStatus[i].MA = temp->MA;
		stack->MPStatus[i].MEPId = temp->MEPId;
		memcpy(stack->MPStatus[i].MAC, temp->MAC, ADDR_LEN);
		i++;
	}
	stack->MPStatusEntry_num = i;
	i = 0;
	LIST_TRAVERSE(&gCfm->cfm_stack.configurationError_list, tmp, list){
		stack->ConfigurationError[i].PortId = tmp->PortId;
		stack->ConfigurationError[i].VlanId = tmp->VlanId;
		stack->ConfigurationError[i].DetectedConfigurationError = tmp->DetectedConfigurationError;
		i++;
	}
	stack->ConfigurationErrorEntry_num = i;

	return 0;
}

/*CFM MD operate start*/

int cfmSupervisory_GetMDList(void * MDList)
{
	struct Test_MDList_st * Test_MDList = (struct Test_MDList_st *)MDList;
	struct MD_st * temp;
	int i=0;

	if(gCfm == NULL){
		printk("gCfm is NULL!!\n");
		return -1;
	}
	LIST_TRAVERSE(&gCfm->md_info.md_list, temp, list){
		memcpy(Test_MDList->MDList[i].MDname, temp->MDname,50);
		Test_MDList->MDList[i].MDnameFormat = temp->MDnameFormat;
		Test_MDList->MDList[i].meid = temp->meid;
		Test_MDList->MDList[i].MDLevel = temp->MDLevel;
		//Test_MDList->MDList[i].MHFCreation = temp->MHFCreation;
		//Test_MDList->MDList[i].SenderIDPermission = temp->SenderIDPermission;
		i++;
	}
	Test_MDList->num = i;

	return 0;
}

int cfmSupervisory_CreateMD(uint16 *meid, uint8 MDLevel, uint8 MDnameFormat, uint8 MDnameLength, uint8 *MDname)
{
	struct MD_st * temp;
	int id;

	if(gCfm == NULL){
		return -1;
	}
	
	if((MDnameFormat <1)||(MDnameFormat >4)){
		return 1;
	}

	LIST_TRAVERSE(&gCfm->md_info.md_list, temp, list){
		if(temp->MDnameLength==0)continue;
		if(memcmp(temp->MDname, MDname, 50) == 0){
			return 2;
		}
	}

	if(MDLevel > 7){
		return 3;
	}
	/*alloc a meid*/
	for(id=1;;id++){
		temp = cfm_GetMD(id);
		if(temp == NULL){
			*meid = id;
			break;
		}
	}
	
	temp = (struct MD_st *)kmalloc(sizeof(struct MD_st), GFP_KERNEL);
	if(temp == NULL){
		return -1;
	}
	memset(temp, 0, sizeof(struct MD_st));

	temp->MDLevel = MDLevel;
	temp->MDnameFormat = MDnameFormat;
	temp->MDnameLength = MDnameLength;
	memcpy(temp->MDname, MDname, MDnameLength);
	temp->MHFCreation = 0x01;
	temp->SenderIDPermission = 0x01;
	gCfm->md_info.md_num++;
	//*meid = ++gCfm->md_info.md_num;
	temp->meid = *meid;

	LIST_INSERT_TAIL(&gCfm->md_info.md_list, temp, list);
	LIST_TRAVERSE(&gCfm->md_info.md_list,temp,list)printk("MD's meid=%d,level=%d\n",temp->meid,temp->MDLevel);
	printk("UpdateMIPs in createMD\n");
	UpdateMIPS();
	return 4;
}
int cfmSupervisory_DeleteMD(uint16 meid)
{
	struct MD_st * temp=NULL;
	struct MA_st * ma=NULL;
	struct MEP_st *mep=NULL;
	struct MIP_st *mip=NULL;
	if(gCfm == NULL){
		return -1;
	}
	LIST_TRAVERSE_SAFE_BEGIN(&gCfm->md_info.md_list, temp, list){
		if (temp->meid == meid){
			LIST_REMOVE_CURRENT(&gCfm->md_info.md_list, list);

			LIST_TRAVERSE_SAFE_BEGIN(&gCfm->ma_info.ma_list,ma,list)
				if(ma->MDPointer==temp)
					{

						LIST_REMOVE_CURRENT(&gCfm->ma_info.ma_list, list);
						LIST_TRAVERSE_SAFE_BEGIN(&gCfm->mep_info.mep_list,mep,list)
							if(mep->MEPBasic.ma==ma)
								{
									LIST_REMOVE_CURRENT(&gCfm->mep_info.mep_list, list);
									kfree(mep);
									mep=NULL;
									gCfm->mep_info.num--;
								}
						LIST_TRAVERSE_SAFE_END

						LIST_TRAVERSE_SAFE_BEGIN(&gCfm->mip_info.mip_list,mip,list)
							if(mip->ma==ma)
								{
									LIST_REMOVE_CURRENT(&gCfm->mip_info.mip_list,list);
									kfree(mip);
									mip=NULL;
									gCfm->mip_info.num--;
								}
						LIST_TRAVERSE_SAFE_END
						kfree(ma);
						ma=NULL;
						gCfm->ma_info.num--;
					}
			LIST_TRAVERSE_SAFE_END
			
			kfree(temp);
			temp = NULL;
			gCfm->md_info.md_num--;
			UpdateMIPS();
			return 2;
		}
	}
	LIST_TRAVERSE_SAFE_END


	return 1;
}

//MD operations
int cfmSupervisory_GetMD(void *result)
{
	struct MD_st *temp;
	struct Test_MD_st *md = (struct Test_MD_st *)result;
	
	LIST_TRAVERSE(&gCfm->md_info.md_list, temp, list){
		if(temp->meid== md->meid){
			md->MDLevel = temp->MDLevel;
			md->MDnameFormat = temp->MDnameFormat;
			md->MDnameLength = temp->MDnameLength;
			memcpy(md->MDname, temp->MDname, temp->MDnameLength);
			md->MHFCreation = temp->MHFCreation;
			md->SenderIDPermission = temp->SenderIDPermission;
			//MA list
			return 2;
		}
	}
	
	return 1;
}
/* 1.MHFCreation 2.SenderIDPermission ||3.MDLevel 4.MDnameFormat 5.MDname*/
int cfmSupervisory_SetMD(uint16 meid, uint8 event, uint8 value, uint8 *MDname)
{
	struct MD_st *temp;

	if(gCfm == NULL){
		return -1;
	}
	LIST_TRAVERSE(&gCfm->md_info.md_list, temp, list){
		if(temp->meid == meid){
			break;
		}
	}
	if(temp == NULL){
		return 1;
	}
	switch(event){
		case 1:
			{
				if((value < 1)||(value > 3)){
					return 3;
				}
				temp->MHFCreation = value;
				break;
			}
		case 2:
			{
				if((value < 1)||(value > 4)){
					return 3;
				}
				temp->SenderIDPermission = value;
				break;
			}
		case 3:
			{
				if(value > 7){
					return 3;
				}
				temp->MDLevel = value;
				break;
			}
		case 4:
			{
				if((value < 1)||(value > 4)){
					return 3;
				}
				temp->MDnameFormat = value;
				break;
			}
		case 5:
			{
				if(MDname == NULL){
					return 3;
				}
				temp->MDnameLength = value;
				memcpy(temp->MDname, MDname, temp->MDnameLength);
				break;
			}
		default:
			{
				return 2;
			}
		
	}
	UpdateMIPS();
	return 4;
}

struct MD_st * cfm_GetMD(uint16 meid)
{
	struct MD_st *temp;

	if(gCfm == NULL){
		printk("cfm is NULL\n");
		return NULL;
	}
	LIST_TRAVERSE(&gCfm->md_info.md_list, temp, list){
		if(temp->meid == meid){
			return temp;
		}	
	}
	return NULL;
}

int cfmSupervisory_CreateMA(uint16 *meid, uint16 MDId, uint8 ShortMAFormat, uint8 ShortMAnameLength, uint8 *ShortMAname, uint16 *AssociatedVLANs)
{
	struct MA_st *temp;
	struct MD_st *md;
	int id;

	if(gCfm == NULL){
		return -1;
	}

	md = cfm_GetMD(MDId);
	if(md == NULL){
		return 1;
	}
	
	if((ShortMAFormat <1)||(ShortMAFormat >4)){
		return 2;
	}

	LIST_TRAVERSE(&gCfm->ma_info.ma_list, temp, list){
		if((temp->MDPointer == md)&&(memcmp(temp->ShortMAname, ShortMAname, 50) == 0)){
			return 3;
		}
	}
	//return 4;//todo

	//return 5;//todo

	/*alloc a meid*/
	for(id=1;;id++){
		temp = cfm_GetMA(id);
		if(temp == NULL){
			*meid = id;
			break;
		}
	}

	
	temp = (struct MA_st *)kmalloc(sizeof(struct MA_st), GFP_KERNEL);
	if(temp == NULL){
		return -1;
	}
	memset(temp, 0, sizeof(struct MA_st));

	temp->MDPointer = md;
	temp->ShortMAnameFormat = ShortMAFormat;
	temp->ShortMAnameLength = ShortMAnameLength;
	memcpy(temp->ShortMAname, ShortMAname, ShortMAnameLength);
	temp->CCMinterval = 0x04;
	memcpy(temp->AssociatedVLANs, AssociatedVLANs, sizeof(temp->AssociatedVLANs));
	temp->MHFCreation = MHF_Creation_None;
	temp->SenderIDPermission = None;
	
	//*meid = ++gCfm->ma_info.ma_num;
	temp->meid = *meid;

	LIST_INSERT_TAIL(&gCfm->ma_info.ma_list, temp, list);
	gCfm->ma_info.num++;
	printk("UpdateMIPs in CreateMA\n");
	UpdateMIPS();
	return 6;
}
int cfmSupervisory_DeleteMA(uint16 meid)
{
	struct MA_st * temp=NULL;
	struct MEP_st *mep=NULL;
	struct MIP_st *mip=NULL;
	if(gCfm == NULL){
		return -1;
	}
	LIST_TRAVERSE_SAFE_BEGIN(&gCfm->ma_info.ma_list, temp, list){
		if (temp->meid == meid){
			LIST_REMOVE_CURRENT(&gCfm->ma_info.ma_list, list);
			LIST_TRAVERSE_SAFE_BEGIN(&gCfm->mep_info.mep_list,mep,list)
				if(mep->MEPBasic.ma==temp)
					{
					LIST_REMOVE_CURRENT(&gCfm->mep_info.mep_list,list);
					kfree(mep);
					mep=NULL;
					gCfm->mep_info.num--;
					}
			LIST_TRAVERSE_SAFE_END

			LIST_TRAVERSE_SAFE_BEGIN(&gCfm->mip_info.mip_list,mip,list)
				if(mip->ma==temp)
					{
					LIST_REMOVE_CURRENT(&gCfm->mip_info.mip_list,list);
					kfree(mip);
					mip=NULL;
					gCfm->mip_info.num--;
					}
			LIST_TRAVERSE_SAFE_END
			kfree(temp);
			temp = NULL;
			gCfm->ma_info.num--;
			UpdateMIPS();
			return 2;
		}
	}
	LIST_TRAVERSE_SAFE_END

	return 1;
}
int cfmSupervisory_GetMA(void *result)
{
	struct MA_st *temp;
	struct Test_MA_st *ma = (struct Test_MA_st*)result;
	
	LIST_TRAVERSE(&gCfm->ma_info.ma_list, temp, list){
		if(temp->meid == ma->meid){
			ma->MDid= temp->MDPointer->meid;
			ma->ShortMAnameFormat = temp->ShortMAnameFormat;
			ma->ShortMAnameLength = temp->ShortMAnameLength;
			memcpy(ma->ShortMAname, temp->ShortMAname, temp->ShortMAnameLength);
			memcpy(ma->AssociatedVLANs, temp->AssociatedVLANs, sizeof(temp->AssociatedVLANs));
			ma->MHFCreation = temp->MHFCreation;
			ma->SenderIDPermission = temp->SenderIDPermission;
			ma->CCMinterval = temp->CCMinterval;
			//MAEPID list
			
			return 2;
		}
	}
	
	return 1;
}
/*1.MHFCreation 2.SenderIDPermission 3.CCMinterval 4.MEPIDs |5.MDPointer 
6.ShortMAnameFormat 7.ShortMAnameLength 8.ShortMAname 9.AssociatedVLANs*/
int cfmSupervisory_SetMA(uint16 meid, uint8 event, uint8 value,  uint16 mdid, void *arg)
{
	struct MA_st* temp;

	temp = cfm_GetMA(meid);
	if(temp == NULL){
		return 1;
	}
	//return 3 Operation rejected due to lack of authority to set this variable
	switch(event){
		case 1:
			{
				if((value < 1)||(value > 4)){
					return 4;
				}
				temp->MHFCreation = value;
				break;
			}
		case 2:
			{
				if((value < 1)||(value > 5)){
					return 4;
				}
				temp->SenderIDPermission = value;
				break;
			}
		case 3:
			{
				if(value > 7){
					return 4;
				}
				temp->CCMinterval = value;
				break;
			}
		case 4:
			{
				return 4;
				break;
			}
		case 5:
			{	MD_t md=cfm_GetMD(mdid);
				if(md==NULL)return 4;
				temp->MDPointer = md;
				UpdateMIPS();
				break;
			}
		case 6:
			{
				if((value < 1)||(value > 4)){
					return 4;
				}
				temp->ShortMAnameFormat = value;
				break;
			}
		case 7:
			{
				temp->ShortMAnameLength = value;
				break;
			}
		case 8:
			{
				memcpy(temp->ShortMAname, (uint8 *)arg, temp->ShortMAnameLength);
				break;
			}
		case 9:
			{
				memcpy(temp->AssociatedVLANs, (uint16 *)arg, 12*sizeof(uint16));
				break;
			}
		default:
			{
				return 2;
			}
	}
	UpdateMIPS();
	return 5;
}
struct MA_st * cfm_GetMA(uint16 meid)
{
	struct MA_st *temp;

	if(gCfm == NULL){
		printk("cfm is NULL\n");
		return NULL;
	}
	LIST_TRAVERSE(&gCfm->ma_info.ma_list, temp, list){
		if(temp->meid == meid){
			return temp;
		}	
	}
	return NULL;
}

int cfmSupervisory_CreateMEP(uint16 *meid, uint16 MAid, uint16 MEPId, uint8 Direction, Layer2_Entity_t layer2Entity, uint16 FlowID)
{
	struct MA_st  *ma;
	struct MEP_st *temp;
	struct MEP_st *tempMEP;
	int id;
	
	int MEP_id=MEPId;
	int MEP_direction;
	struct MPStatusEntry_st * MPStatusEntry;

	if((Direction!=UPMP)&&(Direction!=DOWNMP))
		return -1;
	else
	{
		MEP_direction = Direction;// 0x01 UPMP;0x02 DOWNMP
	}
	
	
	ma = cfm_GetMA(MAid);
	if(ma == NULL){
		return 1;
	}
	if(layer2Entity==NULL)return 1;
	LIST_TRAVERSE(&gCfm->mep_info.mep_list, temp, list){
		if((temp->MEPBasic.ma == ma)&&(MEP_id == temp->MEPBasic.MEPId)){
			return 2;
		}
	}
	//return 3; MEPId 不在 MA的MEPIDlist中//12.14.6.3.3
	//return 4;
	//return 5;
	//return 6;the Bridge is incapable of instantiating a MEP
	//return 7;an Up MEP cannot be configured for an MA that has no VID
	//return 8;a MEP exists on this MA that has a different value for its Up/Down parameter
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,tempMEP,list)
		{
		if(tempMEP->MEPBasic.ma->MDPointer->MDLevel==ma->MDPointer->MDLevel)return 4;
		
		}
	
	/*alloc a meid*/
	for(id=1;;id++){
		temp = cfm_GetMEP(id);
		if(temp == NULL){
			*meid = id;
			break;
		}
	}

	temp = (struct MEP_st*)kmalloc(sizeof(struct MEP_st), GFP_KERNEL);
	if(temp == NULL){
		return -1;
	}
	memset(temp, 0, sizeof(struct MEP_st));
	temp->meid = *meid;
	temp->MEPBasic.mep = temp;
	temp->MEPStatus.mep = temp;
	temp->MEPBasic.meid = *meid;
	temp->MEPStatus.meid = temp->MEPBasic.meid;

	temp->srcPortId = layer2Entity->port;
	memcpy(temp->MEPStatus.MACAddress,layer2Entity->MACAddr,ADDR_LEN);
	temp->Direction = MEP_direction;
	temp->MEPBasic.MEPControl=(MEP_direction&&0x01)<<4;//0001 0000
	temp->FlowId= FlowID;
	temp->MEPBasic.ma = ma;
	temp->MEPBasic.MEPId = MEPId;
	

	temp->MEPBasic.L2_type = 0x00;
	temp->MEPBasic.PrimaryVlan = ma->AssociatedVLANs[0];
	temp->MEPBasic.AdministrativeState = 0x01;
	temp->MEPBasic.FaultAarmThreshold = 0x02;
	temp->MEPBasic.AlarmDeclarationSoakTime = 250;
	temp->MEPBasic.AlarmClearSoakTime = 1000;

	
	temp->ccpm = ccpm_init(temp);
	temp->lbpm = lbpm_init(temp,MEP);
	temp->ltpm = ltpm_init(temp,MEP);

	MPStatusEntry=(struct MPStatusEntry_st*)alloc(sizeof(struct MPStatusEntry_st));
	memset(MPStatusEntry,0,sizeof(struct MPStatusEntry_st));
	if(MPStatusEntry!=NULL)
		{
			MPStatusEntry->PortId=temp->srcPortId;
			MPStatusEntry->Level=ma->MDPointer->MDLevel;
			MPStatusEntry->Direction=temp->Direction;
			MPStatusEntry->VlanId=temp->MEPBasic.PrimaryVlan;
			MPStatusEntry->MD=ma->MDPointer->meid;
			MPStatusEntry->MA=ma->meid;
			MPStatusEntry->MEPId=MEPId;
			memcpy(MPStatusEntry->MAC,layer2Entity->MACAddr,ADDR_LEN);
			cfm_AddCFMMPStatus(MPStatusEntry);
		}
	LIST_INSERT_TAIL(&gCfm->mep_info.mep_list, temp, list);
	gCfm->mep_info.num++;
	printk("New MEP created! meid: %d\n",temp->MEPBasic.meid);
	printk("UpdateMIPs in createMEP\n");
	UpdateMIPS();
	return 9;
	
};
int cfmSupervisory_DeleteMEP(uint16 meid)
{
	struct MEP_st *temp;

	LIST_TRAVERSE_SAFE_BEGIN(&(gCfm->mep_info.mep_list), temp, list){
		if(temp->MEPBasic.meid == meid)
		{
			LIST_REMOVE_CURRENT(&gCfm->mep_info.mep_list,list);
			if(temp->ccpm){ccpm_destroy(temp->ccpm);}
			if(temp->lbpm){lbpm_destroy(temp->lbpm);}
			if(temp->ltpm){ltpm_destroy(temp->ltpm);}
			cfm_DeleteCFMMPStatus(temp->srcPortId,temp->MEPBasic.ma->MDPointer->MDLevel,temp->Direction,temp->MEPBasic.PrimaryVlan);
			kfree(temp);
			gCfm->mep_info.num--;
			UpdateMIPS();
			return 2;
		}
	}
	LIST_TRAVERSE_SAFE_END
	
	return 1;
}
int cfmSupervisory_GetMEP(uint16 meid, void *result)
{
	struct MEP_st *temp;
	struct Test_MEP_Get_st *mep = (struct Test_MEP_Get_st *)result;

	LIST_TRAVERSE(&gCfm->mep_info.mep_list, temp, list){
		if(temp->MEPBasic.meid == meid){
			break;
		}
	}
	if(temp == NULL){
		return 1;
	}
	//when create,An interface, either a Bridge Port or an aggregated IEEE 802.3 port within a Bridge Port
	/*
	mep->Direction = temp->Direction;
	mep->srcPortId = temp->srcPortId;
	mep->FlowId= temp->FlowId;
	mep->MEPBasic.PrimaryVlan = temp->MEPBasic.PrimaryVlan;
	mep->MEPBasic.AdministrativeState = temp->MEPBasic.AdministrativeState;
	
	mep->MEPStatus.FaultNotificationGeneratorState = temp->MEPStatus.FaultNotificationGeneratorState;
	//CCIenabled
	mep->MEPBasic.CCM_LTM_priority = temp->MEPBasic.CCM_LTM_priority;
	memcpy(mep->MEPStatus.MACAddress, temp->MEPStatus.MACAddress, ADDR_LEN);
	//The network address to which Fault Alarms (12.14.7.7) are to be transmitted
	mep->MEPBasic.FaultAarmThreshold = temp->MEPBasic.FaultAarmThreshold;
	mep->MEPBasic.AlarmDeclarationSoakTime = temp->MEPBasic.AlarmDeclarationSoakTime;
	mep->MEPBasic.AlarmClearSoakTime = temp->MEPBasic.AlarmClearSoakTime;
	mep->MEPStatus.HighestPriorityDefectObserved = temp->MEPStatus.HighestPriorityDefectObserved;
	mep->MEPStatus.CurrentDefects = temp->MEPStatus.CurrentDefects;
	//o~s 
	memcpy(mep->MEPStatus.LastReceivedErrorCCM, temp->MEPStatus.LastReceivedErrorCCM, 128);
	memcpy(mep->MEPStatus.LastReceivedXconCCM, temp->MEPStatus.LastReceivedXconCCM, 128);
	mep->MEPStatus.OutOfSequenceCCMsCount = temp->MEPStatus.OutOfSequenceCCMsCount;
	mep->MEPStatus.CCMsTransmittedCount = temp->MEPStatus.CCMsTransmittedCount;
	mep->MEPStatus.NextLBTransId = temp->MEPStatus.NextLBTransId;
	//recv lbr count (valid and invalid) 3
	mep->MEPStatus.NextLTTransId = temp->MEPStatus.NextLTTransId;
	mep->MEPStatus.UnexpectedLTRsCount = temp->MEPStatus.UnexpectedLTRsCount;
	mep->MEPStatus.LBRsTransmittedCount = temp->MEPStatus.LBRsTransmittedCount;
	*/

	mep->Direction = temp->Direction;
	mep->srcPortId = temp->srcPortId;
	mep->FlowId= temp->FlowId;
	mep->PrimaryVlan = temp->MEPBasic.PrimaryVlan;
	mep->AdministrativeState = temp->MEPBasic.AdministrativeState;
	
	mep->FaultNotificationGeneratorState = temp->MEPStatus.FaultNotificationGeneratorState;
	//CCIenabled
	mep->CCM_LTM_priority = temp->MEPBasic.CCM_LTM_priority;
	memcpy(mep->MACAddress, temp->MEPStatus.MACAddress, ADDR_LEN);
	//The network address to which Fault Alarms (12.14.7.7) are to be transmitted
	mep->FaultAarmThreshold = temp->MEPBasic.FaultAarmThreshold;
	mep->AlarmDeclarationSoakTime = temp->MEPBasic.AlarmDeclarationSoakTime;
	mep->AlarmClearSoakTime = temp->MEPBasic.AlarmClearSoakTime;
	mep->HighestPriorityDefectObserved = temp->MEPStatus.HighestPriorityDefectObserved;
	mep->CurrentDefects = temp->MEPStatus.CurrentDefects;
	//o~s 
	memcpy(mep->LastReceivedErrorCCM, temp->MEPStatus.LastReceivedErrorCCM, 128);
	memcpy(mep->LastReceivedXconCCM, temp->MEPStatus.LastReceivedXconCCM, 128);
	mep->OutOfSequenceCCMsCount = temp->MEPStatus.OutOfSequenceCCMsCount;
	mep->CCMsTransmittedCount = temp->MEPStatus.CCMsTransmittedCount;
	mep->NextLBTransId = temp->MEPStatus.NextLBTransId;
	//recv lbr count (valid and invalid) 3
	mep->NextLTTransId = temp->MEPStatus.NextLTTransId;
	mep->UnexpectedLTRsCount = temp->MEPStatus.UnexpectedLTRsCount;
	mep->LBRsTransmittedCount = temp->MEPStatus.LBRsTransmittedCount;
	return 2;
}
/*
0x02:AdministrativeState
0x03:CCIenabled
0x04:CCM_LTM_priority
0x05:FaultAarmThreshold

0x01:PrimaryVlan
0x06:AlarmDeclarationSoakTime
0x07:AlarmClearSoakTime
//
0x08:MEP control
*/
int cfmSupervisory_SetMEP(uint16 meid, uint8 event, uint8 value8, uint16 value16 )
{
	struct MEP_st *mep;

	mep = cfm_GetMEP(meid);
	if(mep == NULL){
		return 1;
	}
	switch(event){
		case 0x01:
			{
				//todo MA Vlan list
				mep->MEPBasic.PrimaryVlan = value16;
				break;
			}
		case 0x02:/*1,lock;0,unlock*/
			{
				if(value8 > 1){
					return 3;
				}
				mep->MEPBasic.AdministrativeState = value8;
				break;
			}
		case 0x03:
			{
				if(value8 > 1){
					return 3;
				}
				mep->ccpm->CCIenabled = value8;
				if(mep->ccpm->CCIenabled)
			  {
				mep->MEPBasic.MEPControl = mep->MEPBasic.MEPControl || 0x02;
			  }
				else
				{
					mep->MEPBasic.MEPControl = mep->MEPBasic.MEPControl && 0xFD;
				}
				break;
			}
		case 0x04:
			{
				if(value8 > 7){
					return 3;
				}
				mep->MEPBasic.CCM_LTM_priority = value8;
				break;
			}
		case 0x05:
			{
				if((value8 < 1)||(value8 > 6)){
					return 3;
				}
				mep->MEPBasic.FaultAarmThreshold = value8;
				break;
			}
		case 0x06:
			{
				if((value16 < 250)||(value16 > 1000)){
					return 3;
				}
				mep->MEPBasic.AlarmDeclarationSoakTime = value16;
				break;
			}
		case 0x07:
			{
				if((value16 < 250)||(value16 > 1000)){
					return 3;
				}
				mep->MEPBasic.AlarmClearSoakTime = value16;
				break;
			}
		case 0x08:
			{
				if((value8 < 2)||(value8 > 5)){
					return 3;
				}
				mep->MEPBasic.MEPControl = value8;
				break;
			}
		//todo more
		default:
			{
				return 2;
			}
	}

	return 4;
}


int cfmSupervisory_GetChassisManagement(void * result)
{
	struct Test_ChassisManagement_st * Test_ChassisManagement=(struct Test_ChassisManagement_st *)result;
	if(gCfm->TLV.Sender_ID_TLV.chassis_ID_Length==0)return -1;
	Test_ChassisManagement->chassis_ID_Length=gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
	Test_ChassisManagement->chassis_ID_Subtype=gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
	Test_ChassisManagement->management_Address_Domain_Length=gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
	Test_ChassisManagement->management_Address_Length=gCfm->TLV.Sender_ID_TLV.management_Address_Length;
	memcpy(	Test_ChassisManagement->chassis_ID,gCfm->TLV.Sender_ID_TLV.chassis_ID,Test_ChassisManagement->chassis_ID_Length);
	memcpy(Test_ChassisManagement->management_Address,gCfm->TLV.Sender_ID_TLV.management_Address,Test_ChassisManagement->management_Address_Length);
	memcpy(Test_ChassisManagement->management_Address_Domain,gCfm->TLV.Sender_ID_TLV.management_Address_Domain,Test_ChassisManagement->management_Address_Domain_Length);
	return 0;
}


struct MEP_st * cfm_GetMEP(uint16 meid)
{
	struct MEP_st *temp;

	if(gCfm == NULL){
		printk("cfm is NULL\n");
		return NULL;
	}
	LIST_TRAVERSE(&gCfm->mep_info.mep_list, temp, list){
		if(temp->MEPBasic.meid == meid){
			return temp;
		}	
	}
	return NULL;
}
/***************************Error List Start*******************************/
static bool isMA_no_UpMEP_on_Port(MA_t ma,uint16 port)
{	
	MEP_t mep=NULL;
	if(ma==NULL)return true;
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
		{
			if(mep->MEPBasic.ma==ma&&mep->Direction==UPMP&&mep->srcPortId==port)
				return false;
		}
	return true;
}
static bool isMA_no_MEP_on_Port(MA_t ma,uint16 port)
{	
	MEP_t mep=NULL;
	if(ma==NULL)return true;
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
		{
			if(mep->MEPBasic.ma==ma&&mep->srcPortId==port)
				return false;
		}
	return true;
}

static bool isMA_no_DownMEP(MA_t ma)
{
	MEP_t mep=NULL;
	if(ma==NULL)return true;
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
		{
			if(mep->MEPBasic.ma==ma&&mep->Direction==DOWNMP)
				return false;
		}
	return true;
}
static bool isMA_no_UpMEP(MA_t ma)
{
	MEP_t mep=NULL;
	if(ma==NULL)return true;
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
		{
			if(mep->MEPBasic.ma==ma&&mep->Direction==UPMP)
				return false;
		}
	return true;
}

static bool do_MAs_have_common_vid(MA_t ma,MA_t ma_c)
{
	int i,j;
	for(i=0;i<12;i++)
		{
			if(ma->AssociatedVLANs[i]==0x00)continue;
			for(j=0;j<12;j++)
				{
					if(ma_c->AssociatedVLANs[j]==0x00)
						continue;
					if(ma->AssociatedVLANs[i]==ma_c->AssociatedVLANs[j])return true;
				}
		}
	return	false;
}
static int UpdateErrorList()
{	
	MD_t md=NULL;
	MA_t ma=NULL,ma_c=NULL;
	MEP_t mep=NULL,mep_c=NULL;
	uint16 mdCnt=0;
	struct ConfigurationErrorEntry_st *errEntry=NULL;
	struct	FilteringDatabase_st *FilteringDatabase=NULL;

	LIST_TRAVERSE_SAFE_BEGIN(&gCfm->cfm_stack.configurationError_list,errEntry,list)
		{
			LIST_REMOVE_CURRENT(&gCfm->cfm_stack.configurationError_list,list);
			if(errEntry!=NULL)
				{
					kfree(errEntry);
					errEntry=NULL;
				}
		}
	LIST_TRAVERSE_SAFE_END
	LIST_TRAVERSE(&gCfm->FilteringDatabase_list,FilteringDatabase,list)
		{
			LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma,list)
				{
					if(isMA_no_UpMEP_on_Port( ma,FilteringDatabase->PortID)&&isMA_no_DownMEP(ma))
						{
							LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma_c,list)
								{
									if(ma_c==ma)continue;
									if(ma_c->MDPointer->MDLevel>ma->MDPointer->MDLevel&&(!isMA_no_MEP_on_Port(ma_c,FilteringDatabase->PortID)))
										{	
												errEntry=(struct ConfigurationErrorEntry_st*)alloc(sizeof(struct ConfigurationErrorEntry_st));
												memset(errEntry,0,sizeof(struct ConfigurationErrorEntry_st));
												errEntry->DetectedConfigurationError=Detected_Configuration_Error_CFM_Leak;
												errEntry->PortId=FilteringDatabase->PortID;
												LIST_INSERT_TAIL(&gCfm->cfm_stack.configurationError_list,errEntry,list);
												goto NET;
										}
								}
						}
					else if(!isMA_no_UpMEP_on_Port( ma,FilteringDatabase->PortID))
						{
							LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma_c,list)
								{
									if(ma_c==ma)continue;
									if(do_MAs_have_common_vid( ma,ma_c)&&ma_c->MDPointer->MDLevel==ma->MDPointer->MDLevel&&(!isMA_no_UpMEP(ma_c)))
										{
											errEntry=(struct ConfigurationErrorEntry_st*)alloc(sizeof(struct ConfigurationErrorEntry_st));
											memset(errEntry,0,sizeof(struct ConfigurationErrorEntry_st));
											errEntry->DetectedConfigurationError=Detected_Configuration_Error_Conflicting_VIDs;
											errEntry->PortId=FilteringDatabase->PortID;
											LIST_INSERT_TAIL(&gCfm->cfm_stack.configurationError_list,errEntry,list);
											goto NET;
										}
								}
						}
				}



			#if 1
			LIST_TRAVERSE(&gCfm->md_info.md_list,md,list)
				{
					mdCnt++;
				}
		
			if(mdCnt>8189)
				{
					errEntry=(struct ConfigurationErrorEntry_st*)alloc(sizeof(struct ConfigurationErrorEntry_st));
					memset(errEntry,0,sizeof(struct ConfigurationErrorEntry_st));
					errEntry->DetectedConfigurationError=Detected_Configuration_Error_Excessive_Levels;
					errEntry->PortId=FilteringDatabase->PortID;
					LIST_INSERT_TAIL(&gCfm->cfm_stack.configurationError_list,errEntry,list);
					goto NET;
				}
			#endif

			LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
				{
					if(mep->srcPortId==FilteringDatabase->PortID)
						{
							LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep_c,list)
								{
									if(mep==mep_c)continue;
									if(mep_c->MEPBasic.PrimaryVlan!=mep->MEPBasic.PrimaryVlan&&mep_c->MEPBasic.ma->MDPointer->MDLevel>=mep->MEPBasic.ma->MDPointer->MDLevel)
										{
											errEntry=(struct ConfigurationErrorEntry_st*)alloc(sizeof(struct ConfigurationErrorEntry_st));
											memset(errEntry,0,sizeof(struct ConfigurationErrorEntry_st));
											errEntry->DetectedConfigurationError=Detected_Configuration_Error_Overlapped_Levels;
											errEntry->PortId=FilteringDatabase->PortID;
											errEntry->VlanId=mep->MEPBasic.PrimaryVlan;
											LIST_INSERT_TAIL(&gCfm->cfm_stack.configurationError_list,errEntry,list);
											goto NET;
										}
								}
						}
				}
			NET:continue;
		}
	return 0;
}
/***************************Error List End*********************************/

/******************************Create MIP Start**********************************/
bool CheckCondition_1(MD_t md,uint16 VID,uint16 port)
{	
	MA_t ma=NULL;
	MEP_t mep=NULL;
	bool maMatch=false;
	int i;
	return 0;
	if(md==NULL)
		{
			printk("md is null\n");
			return false;
		}
	LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma,list)
	if(ma->MDPointer==md)
	{	maMatch=false;
		for( i=0;i<12;i++)
		{
			if(ma->AssociatedVLANs[i]==VID)
			{	
				maMatch=true;
				LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
				if(mep->MEPBasic.ma==ma)
					{
						if(mep->srcPortId==port)
							{goto MTK;}
					}
				printk("No MEP is on the port(%d) in this MA(MEID=%d) of the MD(LEVEL=%d)\n",port,ma->meid,md->MDLevel);
				return false;			
			}
		}
		MTK:
			if(maMatch==false)
			{
				printk("This MA has no VID(%d) in this MD(LEVEL=%d)\n",VID,md->MDLevel);
				return false;
			}

	}
	 return true;
}
bool CheckCondition_2(MD_t md,uint16 VID,uint16 port)
{
	MA_t ma;
	MEP_t mep;
	bool maMatch=false;
	int i;
	if(md==NULL)return false;
	LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma,list)
	if(ma->MDPointer==md)
	{	maMatch=false;
		for( i=0;i<12;i++)
		if(ma->AssociatedVLANs[i]==VID)
		{
			maMatch=true;
			LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
			if(mep->MEPBasic.ma==ma)
				{
					if(mep->srcPortId!=port&&mep->Direction==UPMP)
						{goto MTK;}
				}
			return false;
		}
	MTK:
			if(maMatch==false)
			{
				printk("This MA has no VID(%d) in this MD(LEVEL=%d)\n",VID,md->MDLevel);
				return false;
			}	
	}
	 return true;
}

bool CheckCondition_3(MD_t md,uint16 VID,uint16 port)
{
	MA_t ma=NULL;
	MEP_t mep=NULL;
	uint32 cnt=0;
	bool maMatch=true;
	int i;
	if(md==NULL)return false;
	LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma,list)
	if(ma->MDPointer==md)
	{	
		maMatch=false;
		for( i=0;i<12;i++)
		{
			if(ma->AssociatedVLANs[i]==VID)
			{
				maMatch=true;
				LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
				{
					if(mep->MEPBasic.ma==ma)
					{
					 cnt++;
					}
					
				}
				if(cnt==0)goto MTK;
				return false;
			}
		}
	MTK:
			if(maMatch==false)
			{
				printk("This MA has no VID(%d) in this MD(LEVEL=%d)\n",VID,md->MDLevel);
				return false;
			}		
	}
	 return true;
	
}
bool CheckCondition_4(MD_t md,uint16 VID,uint16 port)
{	
	struct DefaultMDLevelEntry_st* dme=NULL;
	LIST_TRAVERSE(&gCfm->DefaultMDLevel.defaultMDLevel_list,dme,list)
	{
		if(dme->Level==md->MDLevel)return true;
	}
	return false;
}
bool CheckCondition(MD_t md,uint16 VID,uint16 port)
{
	bool con1=CheckCondition_1(md,VID,port);
	bool con2=CheckCondition_2(md,VID,port);
	bool con3=CheckCondition_3(md,VID,port);
	bool con4=CheckCondition_4(md,VID,port);
	
	printk("C1=%d,C2=%d,C3=%d,C4=%d\n",con1,con2,con3,con4);
	return con1||con2||con3||con4;
}

int cfm_createMIPs(uint16 VID,uint16 port)
{	
	uint8 miniLevel=8;
	MD_t md=NULL;
	MIP_t mip=NULL;
	MEP_t mep=NULL;
	MA_t ma=NULL;
	bool configedMEPonPort=false;
	uint8 MIP_creation;
	struct FilteringDatabase_st *fd;

	printk("Computing MIP(VID=%u,PORT=%u)\n",VID,port);
	
	LIST_TRAVERSE(&gCfm->mep_info.mep_list,mep,list)
	{
		if(mep->srcPortId==port)configedMEPonPort=true;
	}
	
	
	
	LIST_TRAVERSE(&gCfm->md_info.md_list,md,list)
	{
		if( CheckCondition(md, VID, port))
		{
			if(md->MDLevel<miniLevel)miniLevel=md->MDLevel;
			printk("LEVEL %d is OK\n",md->MDLevel);
		}
		else 	printk("LEVEL %d isn't OK\n",md->MDLevel);
	}
	
	if(miniLevel==8)
		{
			printk("(VID=%d,PORT=%d)No MD Matched\n",VID,port);
		}

	
	MIP_creation=gCfm->DefaultMDLevel.CatchallMHFCreation;
	
	LIST_TRAVERSE(&gCfm->ma_info.ma_list,ma,list)
		{
			if(ma->AssociatedVLANs[0]==VID&&ma->MDPointer->MDLevel==miniLevel)
				{
					MIP_creation=ma->MHFCreation;
				}
		}

	if(MIP_creation==MHF_Creation_None)
		{	printk("MIP_creation==MHF_Creation_None\n");
			return -1;
		}
	else if(MIP_creation==MHF_Creation_Explicit&&configedMEPonPort==false)
		{
			printk("MIP_creation==MHF_Creation_Explicit&&configedMEPonPort==false\n");
			return -1;
		}

	LIST_TRAVERSE(&(gCfm->FilteringDatabase_list),fd,list)
		{
		if(fd->PortID==port)break;
		}
	if(fd==NULL){printk("Can't find PORT(%d)\n",port);return -1;}
	
	mip=(MIP_t)kmalloc(sizeof(struct MIP_st),GFP_KERNEL);
	if(mip==NULL)return -1;
	memset(mip,0,sizeof(struct MIP_st));
	mip->srcPortId=port;
	mip->MDLevel=miniLevel;
	mip->VlanId=VID;
	memcpy(mip->MACAddr,fd->MacAddr,ADDR_LEN);
	mip->MHF_ccr= MHF_CCR_init();
	mip->lbpm = lbpm_init(mip,MIP);
	mip->ltpm = ltpm_init(mip,MIP);
	
	LIST_INSERT_TAIL(&gCfm->mip_info.mip_list,mip,list);
	printk("A MIP Created\n");
	return 0;
}
int UpdateMIPS(void)
{	
	MIP_t temp=NULL;
	int cnt=0;
	struct FilteringDatabase_st *fd;
	struct FilteringDatabaseVlan_st* fdv;
	printk("DDDDDDDDDDDDDDDDDDDDDDDDDD-Delete All MIPs-DDDDDDDDDDDDDDDDDDDDDDDDDDDD\n");
	LIST_TRAVERSE_SAFE_BEGIN(&(gCfm->mip_info.mip_list),temp,list)
		{
			
			if(temp!=NULL)
				{
					if(temp->lbpm)lbpm_destroy(temp->lbpm);
					if(temp->MHF_ccr)MHF_CCR_destroy(temp->MHF_ccr);
					if(temp->ltpm)ltpm_destroy(temp->ltpm);
					kfree(temp);					
					temp=NULL;
				}
			LIST_REMOVE_CURRENT(&(gCfm->mip_info.mip_list),list);
		}
	LIST_TRAVERSE_SAFE_END
	LIST_TRAVERSE(&(gCfm->FilteringDatabase_list),fd,list)
		LIST_TRAVERSE(&(fd->FilteringDatabaseVlan_list),fdv,list)
			{
				cfm_createMIPs(fdv->VlanId,fd->PortID);
			}
	printk("MIPs Updated ,All MIPs Following:\n");
	LIST_TRAVERSE(&(gCfm->mip_info.mip_list),temp,list)
		{	
			if(temp!=NULL)
				{
				printk("MDLEVEL=%d,PORT=%d,VLANID=%d\n",temp->MDLevel,temp->srcPortId,temp->VlanId);
				printk("MIPs count=%d\n",++cnt);
				}
		}
	UpdateErrorList();
	return 0;
}
/****************************Create MIP End*******************************/
void * cfm_GetNextMP(cfm_t cfm, uint16 VlanId, uint16 PortID, uint8 srcDirection, uint8 Action, void * mp, int *MPFlag)
{
	struct MEP_st *nextMEP=NULL, *temp=NULL;
	struct MIP_st *nextMIP=NULL, *tmp=NULL;
	struct MEP_st *mep = (struct MEP_st *)mp;
	struct MIP_st *mip = (struct MIP_st *)mp;
	int count=0;

	if(cfm == NULL){
		return NULL;
	}
	printk("in cfm_GetNextMP\n");
	switch(Action){
	case SENDTO:{//MEP
		if(mep == NULL){
			return NULL;
		}
		LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){//test
			if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == mep->Direction)&&(temp->MEPBasic.ma->MDPointer->MDLevel < mep->MEPBasic.ma->MDPointer->MDLevel)){
				if(count == 0){
					nextMEP = temp;
					count++;
				}
				else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel){
					nextMEP = temp;
				}
			}
		}
		if(nextMEP != NULL){
			*MPFlag = MEP;
			return nextMEP;
		}
		break;
	}
	case REPLY:
		printk("Action:REPLY\n");
		if(mp == NULL){
			return NULL;
		}
		if(*MPFlag == MEP){//test
			LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){
				if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == mep->Direction)&&(temp->MEPBasic.ma->MDPointer->MDLevel < mep->MEPBasic.ma->MDPointer->MDLevel)){
					if(count == 0){
						nextMEP = temp;
						count++;
					}
					else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel){
						nextMEP = temp;
					}
				}
			}
			if(nextMEP != NULL){
				*MPFlag = MEP;
				return nextMEP;
			}
		}
		else if(*MPFlag == MIP){
			if(srcDirection == Outside){//test
				printk("mp->type == MIP, srcDirection == Outside\n");
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == DOWNMP)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
			}
			else if(srcDirection == Inside){
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction== UPMP)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
			}
		}
		break;
	case FORWARD:
		printk("Action:FORWARD\n");
		if(srcDirection == Outside){// 1 no way
		printk("srcDirection == Outside\n");
			if(mp == NULL){
				LIST_TRAVERSE(&(cfm->mep_info.mep_list), temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == DOWNMP)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel > temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
				LIST_TRAVERSE(&cfm->mip_info.mip_list, tmp, list){//test
					if((tmp->VlanId == VlanId)&&(tmp->srcPortId == PortID)){
						if(count == 0){
							nextMIP = tmp;
							count++;
						}
						//else if(nextMIP->MDLevel > temp->MDLevel){
							//nextMIP = temp;
						//}
					}
				}
				if(nextMIP != NULL){
					*MPFlag = MIP;
					return nextMIP;
				}
				else{
					return NULL;
				}
			}
			else if((*MPFlag == MEP)&&(mep->Direction == DOWNMP)){
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == DOWNMP)&&(temp->MEPBasic.ma->MDPointer->MDLevel> mep->MEPBasic.ma->MDPointer->MDLevel)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel > temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
				LIST_TRAVERSE(&cfm->mip_info.mip_list, tmp, list){//test
					if((tmp->VlanId == VlanId)&&(tmp->srcPortId == PortID)){
						if(count == 0){
							nextMIP = tmp;
							count++;
						}
						else if(nextMIP->MDLevel > tmp->MDLevel){
							nextMIP = tmp;
						}
					}
				}
				if(nextMIP != NULL){
					*MPFlag = MIP;
					return nextMIP;
				}
			}
			else if(*MPFlag == MIP){
				LIST_TRAVERSE(&cfm->mip_info.mip_list, tmp, list){//no
					if((tmp->VlanId== VlanId)&&(tmp->srcPortId == PortID)&&(tmp->MDLevel > mip->MDLevel)){
						if(count == 0){
							nextMIP = tmp;
							count++;
						}
						else if(nextMIP->MDLevel > tmp->MDLevel){
							nextMIP = tmp;
						}
					}
	
				}
				if(nextMIP != NULL){
					*MPFlag = MIP;
					return nextMIP;
				}
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction== UPMP)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
			}
			else if((*MPFlag == MEP)&&(mep->Direction == UPMP)){
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == UPMP)&&(temp->MEPBasic.ma->MDPointer->MDLevel< mep->MEPBasic.ma->MDPointer->MDLevel)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel ){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
			}
		}
		else if(srcDirection == Inside){// 1 no way 
			printk("srcDirection == Inside\n");
			if(mp == NULL){//test
				LIST_TRAVERSE(&(cfm->mep_info.mep_list), temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == UPMP)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel > temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
				LIST_TRAVERSE(&(cfm->mip_info.mip_list), tmp, list){//test
					if((tmp->VlanId == VlanId)&&(tmp->srcPortId == PortID)){
						if(count == 0){
							nextMIP = tmp;
							count++;
						}
						else if(nextMIP->MDLevel > tmp->MDLevel){
							nextMIP = tmp;
						}
					}
				}
				if(nextMIP != NULL){
					*MPFlag = MIP;
					return nextMIP;
				}
				else{
					return NULL;
				}
			}
			else if((*MPFlag == MEP)&&(mep->Direction == UPMP)){//test
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction== UPMP)&&(temp->MEPBasic.ma->MDPointer->MDLevel > mep->MEPBasic.ma->MDPointer->MDLevel)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel > temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
				LIST_TRAVERSE(&cfm->mip_info.mip_list, tmp, list){//test
					if((tmp->VlanId== VlanId)&&(tmp->srcPortId == PortID)){
						if(count == 0){
							nextMIP = tmp;
							count++;
						}
						else if(nextMIP->MDLevel > tmp->MDLevel){
							nextMIP = tmp;
						}
					}
				}
				if(nextMIP != NULL){
					*MPFlag = MIP;
					return nextMIP;
				}
			}
			else if(*MPFlag == MIP){//test
				printk("mp->type == MIP\n");
				LIST_TRAVERSE(&cfm->mip_info.mip_list, tmp, list){//no
					if((tmp->VlanId == VlanId)&&(tmp->srcPortId == PortID)&&(tmp->MDLevel > mip->MDLevel)){
						if(count == 0){
							nextMIP = tmp;
							count++;
						}
						else if(nextMIP->MDLevel > tmp->MDLevel){
							nextMIP = tmp;
						}
					}
				}
				if(nextMIP != NULL){
					*MPFlag = MIP;
					return nextMIP;
				}
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){//test
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == DOWNMP)){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel ){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
			}
			else if((*MPFlag == MEP)&&(mep->Direction == DOWNMP)){//test
				LIST_TRAVERSE(&cfm->mep_info.mep_list, temp, list){
					if((temp->MEPBasic.PrimaryVlan == VlanId)&&(temp->srcPortId == PortID)&&(temp->Direction == DOWNMP)&&(temp->MEPBasic.ma->MDPointer->MDLevel  < mep->MEPBasic.ma->MDPointer->MDLevel )){
						if(count == 0){
							nextMEP = temp;
							count++;
						}
						else if(nextMEP->MEPBasic.ma->MDPointer->MDLevel < temp->MEPBasic.ma->MDPointer->MDLevel){
							nextMEP = temp;
						}
					}
				}
				if(nextMEP != NULL){
					*MPFlag = MEP;
					return nextMEP;
				}
			}
		}
		break;
	default:
		break;
	}
		
	return NULL;
	
}


