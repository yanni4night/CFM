#ifdef LINUX_2_4_xx

#ifndef __KERNEL__
# define __KERNEL__
#endif
#ifndef MODULE
# define MODULE
#endif

/*for flush iptable*/
#ifndef CONFIG_SMP
#define __SMP__
#endif
/* end the usual active */
#include <linux/kernel.h> /* printk() in here */
#include <linux/config.h>

#endif

#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /*copy_to_user*/
#include "cfm_header.h"
#include "cfm_protocol.h"
#include "cfm_cmd.h"
#include "cfmConfig.h"
#include "cfm_types.h"
LIST_DECARE(link_list)  link_list;
struct cfm_st *gCfm = NULL;




static int  cfm_write( struct file *file, const char *data, size_t len, loff_t *ppos )
{
	uint8 buffer[QUEUE_NODE_MAXLEN];
	uint16 srcPortId;
	uint32 srcFlowId;
	uint8 offset=sizeof(srcFlowId)+sizeof(srcPortId);
	memset(buffer, 0, QUEUE_NODE_MAXLEN);
	copy_from_user(buffer, data, sizeof(buffer));
	uint8_to_uint16(srcPortId, buffer);
	uint8_to_uint32(srcFlowId, (buffer+sizeof(srcPortId)));

	cfmPacket_receive(buffer+offset, len-offset, srcPortId, srcFlowId);

	return len;
}
static int  cfm_read( struct file *filp,char *buf,size_t count,loff_t *f_pos)
{
	uint8 data[QUEUE_NODE_MAXLEN];
	int len;
	uint16 srcPortId;
	uint32 srcFlowId;
	uint8 offset=sizeof(srcFlowId)+sizeof(srcPortId);
	
	if(gCfm == NULL){
		return -1;
	}
	if(gCfm->send_queue == NULL){
		return -1;
	}

	memset(data, 0, sizeof(data));
	len = queue_pull(gCfm->send_queue, data+offset, sizeof(data)-offset, &srcPortId, &srcFlowId);

	/*while(len <=0 ){
		len = queue_pull(gCfm->send_queue, data, sizeof(data),NULL,NULL);
	}*/
	if(len > 0){
		uint16_to_uint8(data, srcPortId);
		uint32_to_uint8((data+sizeof(srcPortId)), srcFlowId);
		copy_to_user(buf, data, (len+offset));
	}
	
	return len+6;
}

static int cfm_ioctl(struct inode *inode,
                struct file *filp,
                unsigned int cmd,
                unsigned long arg)
{
	int ret=0;

	if(gCfm == NULL){
		printk("gCfm is NULL\n");
		return -1;
	}
	
   	printk("get command 0x%.2x ,arg=%lu\n",cmd,arg);
        switch (cmd)
	{
	case CFM_STOP:
		{
			ret = cfmSupervisory_Stop(gCfm);
			if(ret != 0){
				return -1;
			}
			break;
		}
	
	case CFM_GET_MDLIST:
		{
			struct Test_MDList_st Test_MDList;
			ret = cfmSupervisory_GetMDList(&Test_MDList);
			if(ret != 0){
				printk("cfmSupervisory_GetMDList returns not 0!!\n");
				return -1;
			}
			copy_to_user((struct Test_MDList_st *)arg, &Test_MDList, sizeof(struct Test_MDList_st));
			break;
		}
	case CFM_CREATE_MD:
		{
			struct Test_MD_st Test_MD;
			copy_from_user(&Test_MD , (struct Test_MD_st *)arg,sizeof(struct Test_MD_st));
			ret = cfmSupervisory_CreateMD(&Test_MD.meid, Test_MD.MDLevel, Test_MD.MDnameFormat, Test_MD.MDnameLength, Test_MD.MDname);
			copy_to_user((struct Test_MD_st *)arg , &Test_MD ,sizeof(struct Test_MD_st));
			if(ret !=4)
				return -1;
			break;
		}
	case CFM_DELETE_MD:
		{
			struct Test_MD_st Test_MD;
			copy_from_user(&Test_MD , (struct Test_MD_st *)arg,sizeof(struct Test_MD_st));
			ret = cfmSupervisory_DeleteMD(Test_MD.meid);
			if(ret !=2)
				return -1;
			break;
		}
	case CFM_SET_MD:
		{
			struct Test_MD_st Test_MD;
			copy_from_user(&Test_MD , (struct Test_MD_st *)arg,sizeof(struct Test_MD_st));
			ret = cfmSupervisory_SetMD(Test_MD.meid, Test_MD.event, Test_MD.value, Test_MD.MDname);//set MHFCreation
			if(ret !=4)
				return -1;
			break;
		}
	case CFM_GET_MD:
		{
			struct Test_MD_st Test_MD;
			copy_from_user(&Test_MD , (struct Test_MD_st *)arg,sizeof(struct Test_MD_st));
			ret = cfmSupervisory_GetMD(&Test_MD);
			if(ret !=2)
				return -1;
			copy_to_user((struct Test_MD_st *)arg,&Test_MD ,sizeof(struct Test_MD_st));
			break;
		}
	
	case CFM_CREATE_MA:
		{
			struct Test_MA_st Test_MA;
			copy_from_user(&Test_MA , (struct Test_MA_st *)arg,sizeof(struct Test_MA_st));
			ret = cfmSupervisory_CreateMA(&(Test_MA.meid), Test_MA.MDid, Test_MA.ShortMAnameFormat, Test_MA.ShortMAnameLength, Test_MA.ShortMAname, Test_MA.AssociatedVLANs);
			copy_to_user((struct Test_MA_st *)arg , &Test_MA ,sizeof(struct Test_MA_st));
			if(ret !=6)
				return -1;
			break;
		}
	case CFM_DELETE_MA:
		{
			struct Test_MA_st Test_MA;
			copy_from_user(&Test_MA , (struct Test_MA_st *)arg,sizeof(struct Test_MA_st));
			ret = cfmSupervisory_DeleteMA(Test_MA.meid);
			if(ret !=2)
				return -1;
			break;
		}
	case CFM_SET_MA:
		{
			struct Test_MA_st Test_MA;
			copy_from_user(&Test_MA , (struct Test_MA_st *)arg,sizeof(struct Test_MA_st));
			ret = cfmSupervisory_SetMA(Test_MA.meid, Test_MA.event, Test_MA.value, Test_MA.MDid, Test_MA.arg);
			
			if(ret !=5)
				return -1;
			break;
		}
	case CFM_GET_MA:
		{
			struct Test_MA_st Test_MA;
			copy_from_user(&Test_MA , (struct Test_MA_st *)arg,sizeof(struct Test_MA_st));
			ret =cfmSupervisory_GetMA(&Test_MA);
			if(ret!=2)return -1;
			copy_to_user((struct Test_MA_st *)arg , &Test_MA ,sizeof(struct Test_MA_st));
			break;
		}
	
	case CFM_CREATE_MEP:
		{
			struct Test_MEP_Create_st Test_MEP_Create;
			struct Layer2_Entity_st layer2Entity;
			copy_from_user(&Test_MEP_Create,(struct Test_MEP_Create_st*)arg,sizeof(struct Test_MEP_Create_st));
			memcpy(layer2Entity.MACAddr,Test_MEP_Create.MACAddr,ADDR_LEN);
			layer2Entity.port=Test_MEP_Create.srcPortId;
			ret=cfmSupervisory_CreateMEP(&Test_MEP_Create.meid,Test_MEP_Create.MAid,Test_MEP_Create.MEPId,Test_MEP_Create.Direction,&layer2Entity,Test_MEP_Create.FlowID);
			if(ret != 9){
							printk("ret=%d of cfmSupervisory_CreateMEP\n",ret);
							return -1;
						}
			copy_to_user((struct Test_MEP_Create_st*)arg , &Test_MEP_Create,sizeof(struct Test_MEP_Create_st));
			break;
		}
	case CFM_DELETE_MEP:
		{
			uint16 meid;
			copy_from_user(&meid,(uint16*)arg,sizeof(uint16));
			ret=cfmSupervisory_DeleteMEP(meid);
			if(ret != 2){
							return -1;
						}
			break;
		}
	case CFM_SET_MEP:
		{
			struct Test_MEP_Set_st Test_MEP;
			copy_from_user(&Test_MEP,(struct Test_MEP_Set_st*)arg,sizeof(struct Test_MEP_Set_st));
			ret=cfmSupervisory_SetMEP(Test_MEP.meid,Test_MEP.event,Test_MEP.value8,Test_MEP.value16);
			if(ret != 4){
							return -1;
						}
			break;
		}
	case CFM_GET_MEP:
		{
			struct Test_MEP_Get_st Test_MEP;
			copy_from_user(&Test_MEP,(struct Test_MEP_Get_st*)arg,sizeof(struct Test_MEP_Get_st));
			ret=cfmSupervisory_GetMEP(Test_MEP.meid,&Test_MEP);
			if(ret != 2){
							return -1;
						}
			copy_to_user((struct Test_MEP_Get_st*)arg,&Test_MEP,sizeof(struct Test_MEP_Get_st));
			break;
		}
	
	case CFM_GET_MPSTATUS:
		{
			struct Test_MPStatusEntry_st MPStatusEntry;
			copy_from_user(&MPStatusEntry,(struct Test_MPStatusEntry_st*)arg,sizeof(struct Test_MPStatusEntry_st));
			ret=cfmSupervisory_GetCFMMPStatus(&MPStatusEntry);
			if(ret != 3){	
							printk("ret=%d\n",ret);
							return -1;
						}
			copy_to_user((struct Test_MPStatusEntry_st*)arg,&MPStatusEntry,sizeof(struct Test_MPStatusEntry_st));
			break;
		}
	case CFM_GET_CONFIGERROR:
		{
			struct Test_ConfigurationErrorEntry_st ConfigurationErrorEntry;
			copy_from_user(&ConfigurationErrorEntry,(struct Test_ConfigurationErrorEntry_st*)arg,sizeof(struct Test_ConfigurationErrorEntry_st));
			ret=cfmSupervisory_GetCFMConfigurationError(&ConfigurationErrorEntry);
			if(ret != 3){
							return -1;
						}
			copy_to_user((struct Test_ConfigurationErrorEntry_st*)arg,&ConfigurationErrorEntry,sizeof(struct Test_ConfigurationErrorEntry_st));
			break;
		}
	case CFM_GET_STACK:
		{
			struct Test_CFMStack_st Test_CFMStack;
			copy_from_user(&Test_CFMStack,(struct Test_CFMStack_st*)arg,sizeof(struct Test_CFMStack_st));
			ret=cfmSupervisory_GetCFMStack(&Test_CFMStack);
			if(ret != 0){
							return -1;
						}
			copy_to_user((struct Test_CFMStack_st*)arg,&Test_CFMStack,sizeof(struct Test_CFMStack_st));
			break;
		}
	case CFM_SET_DEFAULTCATCHALL:
		{
			struct Test_CFMDefaultCatchall_st Test_CFMDefaultCatchall;
			copy_from_user(&Test_CFMDefaultCatchall,(struct Test_CFMDefaultCatchall_st*)arg,sizeof(struct Test_CFMDefaultCatchall_st));
			ret=cfmSupervisory_SetCFMDefaultCatchall(Test_CFMDefaultCatchall.event,&Test_CFMDefaultCatchall.value8);
			if(ret != 0){
							return -1;
						}
			break;
		}
	case CFM_GET_DEFAULTCATCHALL:
		{
			struct Test_CFMDefaultCatchall_st Test_CFMDefaultCatchall;
			copy_from_user(&Test_CFMDefaultCatchall,(struct Test_CFMDefaultCatchall_st*)arg,sizeof(struct Test_CFMDefaultCatchall_st));
			ret=cfmSupervisory_GetCFMDefaultCatchall(Test_CFMDefaultCatchall.event,&Test_CFMDefaultCatchall.value8,&Test_CFMDefaultCatchall.value16);
			copy_to_user((struct Test_CFMDefaultCatchall_st*)arg,&Test_CFMDefaultCatchall,sizeof(struct Test_CFMDefaultCatchall_st));
			if(ret != 1){
							return -1;
						}
			break;
		}
	case CFM_SET_DEFAULTTABLE:
		{
			struct Test_CFMDefaultTable_st Test_CFMDefaultTable;
			copy_from_user(&Test_CFMDefaultTable,(struct Test_CFMDefaultTable_st*)arg,sizeof(struct Test_CFMDefaultTable_st));
			ret=cfmSupervisory_SetCFMDefaultTable(Test_CFMDefaultTable.vidList,Test_CFMDefaultTable.event,Test_CFMDefaultTable.value);
			if(ret != 3){
				return -1;
			}
			break;
		}
	case CFM_GET_DEFAULTTALBE:
		{
			struct Test_DefaultMDLevelEntry_st dme;
			copy_from_user(&dme,(struct Test_DefaultMDLevelEntry_st*)arg,sizeof(struct Test_DefaultMDLevelEntry_st));
			ret=cfmSupervisory_GetCFMDefaultTable(&dme);
			if(ret != 1){
				return -1;
			}
			copy_to_user((struct Test_DefaultMDLevelEntry_st*)arg,&dme,sizeof(struct Test_DefaultMDLevelEntry_st));
			break;
		}

	case CFM_SETPEERMEPID:
		{
			struct Test_PeerMEPID_st Test_PeerMEPID;
			copy_from_user(&Test_PeerMEPID, (struct Test_PeerMEPID_st*)arg, sizeof(struct Test_PeerMEPID_st));
			ret = cfmConfig_SetPeerMEPID(Test_PeerMEPID.peerId, Test_PeerMEPID.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GETPEERMEPID:
		{
			struct Test_PeerMEPID_st Test_PeerMEPID;
			copy_from_user(&Test_PeerMEPID, (struct Test_PeerMEPID_st*)arg, sizeof(struct Test_PeerMEPID_st));
			ret = cfmConfig_GetPeerMEPID(Test_PeerMEPID.peerId, Test_PeerMEPID.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_PeerMEPID_st*)arg, &Test_PeerMEPID, sizeof(struct Test_PeerMEPID_st));
			break;
		}

	case CFM_SET_CHASSISIDLENGTH:
		{
			uint8 len;
			get_user(len, (uint8*)arg);
			ret = cfmConfig_SetChassisIDLength(len);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_CHASSISIDLENGTH:
		{
			uint8 len;
			ret = cfmConfig_GetChassisIDLength(&len);
			if(ret != 0){
				return -1;
			}
			put_user(len, (uint8 *)arg);
			break;
		}
	case CFM_SET_CHASSISIDSBUTYPE:
		{
			uint8 subtype;
			get_user(subtype, (uint8*)arg);
			ret = cfmConfig_SetChassisIDSubtype(subtype);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_CHASSISIDSBUTYPE:
		{
			uint8 subtype;
			ret = cfmConfig_GetChassisIDSubtype(&subtype);
			if(ret != 0){
				return -1;
			}
			put_user(subtype, (uint8*)arg);
			break;
		}
	case CFM_SET_CHASSISID:
		{
			uint8 chassisId[50];
			memset(chassisId, 0, sizeof(chassisId));
			copy_from_user(chassisId, (uint8*)arg, sizeof(chassisId));
			ret = cfmConfig_SetChassisID(chassisId);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_CHASSISID:
		{
			uint8 chassisId[50];
			memset(chassisId, 0, sizeof(chassisId));
			ret = cfmConfig_GetChassisID(chassisId);
			copy_to_user((uint8*)arg, chassisId, sizeof(chassisId));
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_SET_MGMTADDRDOMAINLENGTH:
		{
			uint8 len;
			get_user(len, (uint8*)arg);
			ret = cfmConfig_SetMgmtAddrDomainLength(len);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_MGMTADDRDOMAINLENGTH:
		{
			uint8 len;
			ret = cfmConfig_GetMgmtAddrDomainLength(&len);
			if(ret != 0){
				return -1;
			}
			put_user(len, (uint8*)arg);
			break;
		}
	case CFM_SET_MGMTADDRDOMAINM:
		{
			uint8 domain[50];
			memset(domain, 0, sizeof(domain));
			copy_from_user(domain, (uint8*)arg, sizeof(domain));
			ret = cfmConfig_SetMgmtAddrDomain(domain);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_MGMTADDRDOMAINM:
		{
			uint8 domain[50];
			memset(domain, 0, sizeof(domain));
			ret = cfmConfig_GetMgmtAddrDomain(domain);
			if(ret != 0){
				return -1;
			}
			copy_to_user((uint8*)arg, domain, sizeof(domain));
			break;
		}
	case CFM_SET_MGMTADDRLENGTH:
		{
			uint8 len;
			get_user(len, (uint8*)arg);
			ret = cfmConfig_SetMgmtAddrLength(len);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_MGMTADDRLENGTH:
		{
			uint8 len;
			ret = cfmConfig_GetMgmtAddrLength(&len);
			if(ret != 0){
				return -1;
			}
			put_user(len, (uint8 *)arg);
			break;
		}
	case CFM_SET_MGMTADDR:
		{
			uint8 addr[50];
			memset(addr, 0, sizeof(addr));
			copy_from_user(addr, (uint8*)arg, sizeof(addr));
			ret = cfmConfig_SetMgmtAddr(addr);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_MGMTADDR:
		{
			uint8 addr[50];
			memset(addr, 0, sizeof(addr));
			ret = cfmConfig_GetMgmtAddr(addr);
			if(ret != 0){
				return -1;
			}
			copy_to_user((uint8*)arg, addr, sizeof(addr));
			break;
		}
	case CFM_GET_CHASSISMANAGEMENT:
		{
			struct Test_ChassisManagement_st *Test_ChassisManagement;
			memset(&Test_ChassisManagement,0,sizeof(struct Test_ChassisManagement_st));
			copy_from_user(Test_ChassisManagement,(struct Test_ChassisManagement_st *)arg,sizeof(struct Test_ChassisManagement_st));
			ret=cfmSupervisory_GetChassisManagement(Test_ChassisManagement);
			if(ret!=0)
				{
					return -1;
				}
			copy_to_user((struct Test_ChassisManagement_st *)arg,Test_ChassisManagement,sizeof(struct Test_ChassisManagement_st));
		}
	case CFM_SET_ORGSPELEN:
		{
			uint16 len;
			get_user(len, (uint16*)arg);
			ret = cfmConfig_SetOrgSpecificLength(len);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_ORGSPELEN:
		{
			uint16 len;
			ret = cfmConfig_GetOrgSpecificLength(&len);
			if(ret != 0){
				return -1;
			}
			put_user(len, (uint16*)arg);
			break;
		}
	case CFM_SET_ORGSPEOUI:
		{
			uint8 oui[3];
			memset(oui, 0, sizeof(oui));
			copy_from_user(oui, (uint8*)arg, sizeof(oui));
			ret = cfmConfig_SetOrgSpecificOUI(oui);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_ORGSPEOUI:
		{
			uint8 oui[3];
			memset(oui, 0, sizeof(oui));
			ret = cfmConfig_GetOrgSpecificOUI(oui);
			if(ret != 0){
				return -1;
			}
			copy_to_user((uint8*)arg, oui, sizeof(oui));
			break;
		}
	case CFM_SET_ORGSPESUBTYPE:
		{
			uint8 subtype;
			get_user(subtype, (uint8*)arg);
			ret = cfmConfig_SetOrgSpecificSubtype(subtype);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_ORGSPESUBTYPE:
		{
			uint8 subtype;
			ret = cfmConfig_GetOrgSpecificSubtype(&subtype);
			if(ret != 0){
				return -1;
			}
			put_user(subtype, (uint8*)arg);
			break;
		}
	case CFM_SET_ORGSPEVALUE:
		{
			uint8 value[128];
			memset(value, 0, sizeof(value));
			copy_from_user(value, (uint8*)arg, sizeof(value));
			ret = cfmConfig_SetOrgSpecificValue(value);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_ORGSPEVALUE:
		{
			uint8 value[128];
			memset(value, 0, sizeof(value));
			ret = cfmConfig_GetOrgSpecificValue(value);
			if(ret != 0){
				return -1;
			}
			copy_to_user((uint8*)arg, value, sizeof(value));
			break;
		}
	case CFM_SET_ALARMTHRESHOLD:
		{
			struct Test_AlarmPri_st Test_AlarmPri;
			copy_from_user(&Test_AlarmPri, (struct Test_AlarmPri_st*)arg, sizeof(struct Test_AlarmPri_st));
			ret = cfmConfig_SetAlarmThreshold(Test_AlarmPri.AlarmPri, Test_AlarmPri.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_ALARMTHRESHOLD:
		{
			struct Test_AlarmPri_st Test_AlarmPri;
			copy_from_user(&Test_AlarmPri, (struct Test_AlarmPri_st*)arg, sizeof(struct Test_AlarmPri_st));
			ret = cfmConfig_GetAlarmThreshold(&Test_AlarmPri.AlarmPri, Test_AlarmPri.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_AlarmPri_st*)arg, &Test_AlarmPri, sizeof(struct Test_AlarmPri_st));
			break;
		}
#if 0
	case CFM_CCM_START:
		{
			mp_t mp;
			mp = cfm_GetMEP(gCfm,arg);
			if (mp != NULL){
				ccpm_start(4, mp);
			}
			break;
		}
#endif
	case CFM_SET_CCMINTERVAL:
		{
			struct Test_CCMInterval_st Test_CCMInterval;
			copy_from_user(&Test_CCMInterval, (struct Test_CCMInterval_st*)arg, sizeof(struct Test_CCMInterval_st));
			ret = cfmConfig_SetCCMInterval(Test_CCMInterval.interval, Test_CCMInterval.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_CCMINTERVAL:
		{
			struct Test_CCMInterval_st Test_CCMInterval;
			copy_from_user(&Test_CCMInterval, (struct Test_CCMInterval_st*)arg, sizeof(struct Test_CCMInterval_st));
			ret = cfmConfig_GetCCMInterval(&Test_CCMInterval.interval, Test_CCMInterval.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_CCMInterval_st*)arg, &Test_CCMInterval, sizeof(struct Test_CCMInterval_st));
			break;
		}
	case CFM_SET_CCIENABLED:
		{
			struct Test_CCIenabled_st Test_CCIenabled;
			copy_from_user(&Test_CCIenabled, (struct Test_CCIenabled_st*)arg, sizeof(struct Test_CCIenabled_st));
			ret = cfmConfig_SetCCIenabled(Test_CCIenabled.CCIenabled, Test_CCIenabled.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_LASTRCVDERRORCCM:
		{
			struct Test_ccm_st Test_ccm;
			copy_from_user(&Test_ccm, (struct Test_ccm_st*)arg, sizeof(struct Test_ccm_st));
			ret = cfmConfig_GetLastRcvdErrorCCM(Test_ccm.ccm, &Test_ccm.len, Test_ccm.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_ccm_st*)arg, &Test_ccm, sizeof(struct Test_ccm_st));
			break;
		}
	case CFM_GET_LASTRCVDXCONCCM:
		{
			struct Test_ccm_st Test_ccm;
			copy_from_user(&Test_ccm, (struct Test_ccm_st*)arg, sizeof(struct Test_ccm_st));
			ret = cfmConfig_GetLastRcvdXconCCM(Test_ccm.ccm, &Test_ccm.len, Test_ccm.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_ccm_st*)arg, &Test_ccm, sizeof(struct Test_ccm_st));
			break;
		}
	case CFM_GET_OUTOFSEQCCMCOUNTS:
		{
			struct Test_CCCount_st Test_CCCount;
			copy_from_user(&Test_CCCount, (struct Test_CCCount_st*)arg, sizeof(struct Test_CCCount_st));
			ret = cfmConfig_GetOutOfSeqCCMCounts(&Test_CCCount.count, Test_CCCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_CCCount_st*)arg, &Test_CCCount, sizeof(struct Test_CCCount_st));
			break;
		}
	case CFM_GET_XMITCCMCOUNTS:
		{
			struct Test_CCCount_st Test_CCCount;
			copy_from_user(&Test_CCCount, (struct Test_CCCount_st*)arg, sizeof(struct Test_CCCount_st));
			ret = cfmConfig_GetXmitCCMCounts(&Test_CCCount.count, Test_CCCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_CCCount_st*)arg, &Test_CCCount, sizeof(struct Test_CCCount_st));
			break;
		}
	case CFM_CREATECCMDABASENODE:
		{
			struct Test_CreateCCMDatabaseNode_st Test_CreateCCMDatabaseNode;
			copy_from_user(&Test_CreateCCMDatabaseNode, (struct Test_CreateCCMDatabaseNode_st*)arg, sizeof(struct Test_CreateCCMDatabaseNode_st));
			ret = cfmConfig_CreateCCMDatabaseNode(Test_CreateCCMDatabaseNode.MEPid, Test_CreateCCMDatabaseNode.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_SET_PORTSTATUS:
		{
			struct Test_Status_st Test_Status;
			copy_from_user(&Test_Status, (struct Test_Status_st*)arg, sizeof(struct Test_Status_st));
			ret = cfmConfig_SetPortStatus(Test_Status.value, Test_Status.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_PORTSTATUS:
		{
			struct Test_Status_st Test_Status;
			copy_from_user(&Test_Status, (struct Test_Status_st*)arg, sizeof(struct Test_Status_st));
			ret = cfmConfig_GetPortStatus(&Test_Status.value, Test_Status.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_Status_st*)arg, &Test_Status, sizeof(struct Test_Status_st));
			break;
		}
	case CFM_SET_INTERFASESTATUS:
		{
			struct Test_Status_st Test_Status;
			copy_from_user(&Test_Status, (struct Test_Status_st*)arg, sizeof(struct Test_Status_st));
			ret = cfmConfig_SetInterfaceStatus(Test_Status.value, Test_Status.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_INTERFASESTATUS:
		{
			struct Test_Status_st Test_Status;
			copy_from_user(&Test_Status, (struct Test_Status_st*)arg, sizeof(struct Test_Status_st));
			ret = cfmConfig_GetInterfaceStatus(&Test_Status.value, Test_Status.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_Status_st*)arg, &Test_Status, sizeof(struct Test_Status_st));
			break;
		}
	case CFM_SET_CCORGSPEPERMISSION:
		{
			struct Test_OrgSpePermission_st Test_OrgSpePermission;
			copy_from_user(&Test_OrgSpePermission, (struct Test_OrgSpePermission_st *)arg ,sizeof(struct Test_OrgSpePermission_st));
			ret = cfmConfig_SetCCOrgSpePermission(Test_OrgSpePermission.permission , Test_OrgSpePermission.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GET_CCORGSPEPERMISSION:
		{
			struct Test_OrgSpePermission_st Test_OrgSpePermission;
			copy_from_user(&Test_OrgSpePermission,(struct Test_OrgSpePermission_st *)arg ,sizeof(struct Test_OrgSpePermission_st));
			ret = cfmConfig_GetCCOrgSpePermission(&Test_OrgSpePermission.permission , Test_OrgSpePermission.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_OrgSpePermission_st *)arg, &Test_OrgSpePermission,sizeof(struct Test_OrgSpePermission_st));
			break;
		}
		
	case CFM_LT_START:
		{
			struct Test_LTStart_st Test_LTStart;
			copy_from_user(&Test_LTStart, (struct Test_LTStart_st*)arg, sizeof(struct Test_LTStart_st));
			ret = cfmConfig_LinkTrace_Start(Test_LTStart.MacAddr, Test_LTStart.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LT_GETRESULT:
		{
			struct result_param result;
			int i=0,code=0;
			linkTraceNode_t node,temp,pre;
			
			LIST_HEAD_INIT(&link_list);
			copy_from_user(&result, (struct result_param*)arg, sizeof(struct result_param));
			code = cfmConfig_LinkTrace_GetResult(&link_list, result.mac, result.meid);
			LIST_TRAVERSE(&link_list,node,list){
				memcpy(result.node[i].mac_addr,node->mac_addr,ADDR_LEN);
				result.node[i].TTL = node->TTL;
				result.node[i].ingress_action = node->ingress_action;
				memcpy(result.node[i].ingressMAC,node->ingressMAC, ADDR_LEN);
				result.node[i].egress_action = node->egress_action;
				memcpy(result.node[i].egressMAC,node->egressMAC, ADDR_LEN);
				i++;
				result.node_num++;
				if(i >= 10)
					break;
			}
			temp = LIST_FIRST(&link_list);
			while(temp != NULL)
			{
				pre = temp;
				temp = LIST_NEXT(temp,list);
				kfree(pre);
				pre = NULL;
			}
			LIST_HEAD_DESTROY(&link_list);
			copy_to_user((struct result_param*)arg, &result, sizeof(struct result_param));
			
			break;
		}
	case CFM_LT_GETXMITLTRCOUNTS:
		{
			struct Test_LTCount_st Test_LTCount;
			copy_from_user(&Test_LTCount, (struct Test_LTCount_st*)arg, sizeof(struct Test_LTCount_st));
			ret = cfmConfig_GetXmitLTRCounts(&Test_LTCount.count, Test_LTCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTCount_st*)arg, &Test_LTCount, sizeof(struct Test_LTCount_st));
			break;
		}
	case CFM_LT_GETXMITLTMCOUNTS:
		{
			struct Test_LTCount_st Test_LTCount;
			copy_from_user(&Test_LTCount, (struct Test_LTCount_st*)arg, sizeof(struct Test_LTCount_st));
			ret = cfmConfig_GetXmitLTMCounts(&Test_LTCount.count, Test_LTCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTCount_st*)arg, &Test_LTCount, sizeof(struct Test_LTCount_st));
			break;
		}	
	case CFM_LT_GETLTMRECEIVEDCOUNTS:
		{
			struct Test_LTCount_st Test_LTCount;
			copy_from_user(&Test_LTCount, (struct Test_LTCount_st*)arg, sizeof(struct Test_LTCount_st));
			ret = cfmConfig_GetLTMreceivedCounts(&Test_LTCount.count, Test_LTCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTCount_st*)arg, &Test_LTCount, sizeof(struct Test_LTCount_st));
			break;
		}
	case CFM_LT_GETLTRRECEIVEDCOUNTS:
		{
			struct Test_LTCount_st Test_LTCount;
			copy_from_user(&Test_LTCount, (struct Test_LTCount_st*)arg, sizeof(struct Test_LTCount_st));
			ret = cfmConfig_GetLTRreceivedCounts(&Test_LTCount.count, Test_LTCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTCount_st*)arg, &Test_LTCount, sizeof(struct Test_LTCount_st));
			break;
		}
	case CFM_LT_GETUNEXPECTEDLTRCOUNTS:
		{
			struct Test_LTCount_st Test_LTCount;
			copy_from_user(&Test_LTCount, (struct Test_LTCount_st*)arg, sizeof(struct Test_LTCount_st));
			ret = cfmConfig_GetUnexpectedLTRCounts(&Test_LTCount.count, Test_LTCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTCount_st*)arg, &Test_LTCount, sizeof(struct Test_LTCount_st));
			break;
		}
	case CFM_LT_GETNEXTTRANSID:
		{
			struct Test_LTTransId_st Test_LTTransId;
			copy_from_user(&Test_LTTransId, (struct Test_LTTransId_st *)arg, sizeof(struct Test_LTTransId_st));
			ret = cfmConfig_GetNextLTTranID(&Test_LTTransId.ltid, Test_LTTransId.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTTransId_st *)arg, &Test_LTTransId, sizeof(struct Test_LTTransId_st));
			break;
		}
	case CFM_LT_SETNEXTTRANSID:
		{
			struct Test_LTTransId_st Test_LTTransId;
			copy_from_user(&Test_LTTransId, (struct Test_LTTransId_st *)arg, sizeof(struct Test_LTTransId_st));
			ret = cfmConfig_SetNextLTTranID(Test_LTTransId.ltid, Test_LTTransId.meid);
			if(ret !=0)
				return -1;
			break;
		}
	case CFM_LT_SETLTMTTL:
		{
			struct Test_LTSetLTMTtl_st Test_LTSetLTMTtl;
			copy_from_user(&Test_LTSetLTMTtl, (struct Test_LTSetLTMTtl_st*)arg, sizeof(struct Test_LTSetLTMTtl_st));
			ret = cfmConfig_SetLTMTTL(Test_LTSetLTMTtl.ttl, Test_LTSetLTMTtl.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LT_SETLTMFLAGS:
		{
			struct Test_LTSetLTMFlags_st Test_LTSetLTMFlags;
			copy_from_user(&Test_LTSetLTMFlags, (struct Test_LTSetLTMFlags_st*)arg, sizeof(struct Test_LTSetLTMFlags_st));
			ret = cfmConfig_SetLTMflags(Test_LTSetLTMFlags.flags, Test_LTSetLTMFlags.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_SETEGRESSID:
		{
			struct Test_LTEgressId_st Test_LTEgressId;
			copy_from_user(&Test_LTEgressId, (struct Test_LTEgressId_st*)arg, sizeof(struct Test_LTEgressId_st));
			ret = cfmConfig_SetEgressID(Test_LTEgressId.gressid, Test_LTEgressId.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_GETEGRESSID:
		{
			struct Test_LTEgressId_st Test_LTEgressId;
			copy_from_user(&Test_LTEgressId, (struct Test_LTEgressId_st*)arg, sizeof(struct Test_LTEgressId_st));
			ret = cfmConfig_GetEgressID(Test_LTEgressId.gressid, Test_LTEgressId.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LTEgressId_st*)arg, &Test_LTEgressId, sizeof(struct Test_LTEgressId_st));
			break;
		}
	case CFM_SETLTM_SENDERID_PERMISSION:
		{
			struct Test_LTSenderIDPermission_st Test_LTSenderIDPermission;
			copy_from_user(&Test_LTSenderIDPermission,(struct Test_LTSenderIDPermission_st *)arg, sizeof(struct Test_LTSenderIDPermission_st ));
			ret = cfmConfig_SetLTMSenderIDPermission(Test_LTSenderIDPermission.flags, Test_LTSenderIDPermission.meid);
			if(ret != 0)
				return -1;
			break;
		}
	case CFM_GETLTM_SENDERID_PERMISSION:
		{
			struct Test_LTSenderIDPermission_st Test_LTSenderIDPermission;
			copy_from_user(&Test_LTSenderIDPermission,(struct Test_LTSenderIDPermission_st *)arg, sizeof(struct Test_LTSenderIDPermission_st ));
			ret = cfmConfig_GetLTMSenderIDPermission(&Test_LTSenderIDPermission.flags, Test_LTSenderIDPermission.meid);
			if(ret !=0)
				return -1;
			copy_to_user((struct Test_LTSenderIDPermission_st*)arg, &Test_LTSenderIDPermission, sizeof(struct Test_LTSenderIDPermission_st));
			break;
		}
	case CFM_SETLTR_SENDERID_PERMISSION:
		{
			struct Test_LTSenderIDPermission_st Test_LTSenderIDPermission;
			copy_from_user(&Test_LTSenderIDPermission,(struct Test_LTSenderIDPermission_st *)arg, sizeof(struct Test_LTSenderIDPermission_st ));
			ret = cfmConfig_SetLTRSenderIDPermission(Test_LTSenderIDPermission.flags, Test_LTSenderIDPermission.meid);
			if(ret != 0)
				return -1;
			break;
		}
	case CFM_GETLTR_SENDERID_PERMISSION:
		{
			struct Test_LTSenderIDPermission_st Test_LTSenderIDPermission;
			copy_from_user(&Test_LTSenderIDPermission,(struct Test_LTSenderIDPermission_st *)arg, sizeof(struct Test_LTSenderIDPermission_st ));
			ret = cfmConfig_GetLTRSenderIDPermission(&Test_LTSenderIDPermission.flags, Test_LTSenderIDPermission.meid);
			if(ret != 0)
				return -1;
			copy_to_user((struct Test_LTSenderIDPermission_st*)arg, &Test_LTSenderIDPermission, sizeof(struct Test_LTSenderIDPermission_st));
			break;
		}

//LB
	case CFM_LB_START:
		{
			struct Test_LBStart_st Test_LBStart;
			copy_from_user(&Test_LBStart, (struct Test_LBStart_st*)arg, sizeof(struct Test_LBStart_st));
			ret = cfmConfig_Loopback_Start(Test_LBStart.lbmstosend, Test_LBStart.MacAddr, Test_LBStart.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETRESULT:
		{
			struct Test_LBGetResult_st Test_LBGetResult;
			copy_from_user(&Test_LBGetResult, (struct Test_LBGetResult_st*)arg, sizeof(struct Test_LBGetResult_st));
			ret = cfmConfig_LoopBack_GetResult(&Test_LBGetResult.result, Test_LBGetResult.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBGetResult_st*)arg, &Test_LBGetResult, sizeof(struct Test_LBGetResult_st));
			break;
		}
	case CFM_LB_SETLBAVALIABLE:
		{
			struct Test_LBAvaliable_st Test_LBAvaliable;
			copy_from_user(&Test_LBAvaliable, (struct Test_LBAvaliable_st*)arg, sizeof(struct Test_LBAvaliable_st));
			ret = cfmConfig_SetLBAvaliable(Test_LBAvaliable.LBAvaliable, Test_LBAvaliable.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETLBAVALIABLE:
		{
			struct Test_LBAvaliable_st Test_LBAvaliable;
			copy_from_user(&Test_LBAvaliable, (struct Test_LBAvaliable_st*)arg, sizeof(struct Test_LBAvaliable_st));
			ret = cfmConfig_GetLBAvaliable(&Test_LBAvaliable.LBAvaliable, Test_LBAvaliable.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBAvaliable_st*)arg, &Test_LBAvaliable, sizeof(struct Test_LBAvaliable_st));
			break;
		}
	case CFM_LB_GETERRORRCVDLBRCOUNTS:
		{
			struct Test_LBCount_st Test_LBCount;
			copy_from_user(&Test_LBCount, (struct Test_LBCount_st*)arg, sizeof(struct Test_LBCount_st));
			ret = cfmConfig_GetErrorRcvdLBRCounts(&Test_LBCount.count, Test_LBCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBCount_st*)arg, &Test_LBCount, sizeof(struct Test_LBCount_st));
			break;
		}
	case CFM_LB_GETCORRECTRCVDLBRCOUNTS:
		{
			struct Test_LBCount_st Test_LBCount;
			copy_from_user(&Test_LBCount, (struct Test_LBCount_st*)arg, sizeof(struct Test_LBCount_st));
			ret = cfmConfig_GetCorrectRcvdLBRCounts(&Test_LBCount.count, Test_LBCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBCount_st*)arg, &Test_LBCount, sizeof(struct Test_LBCount_st));
			break;
		}
	case CFM_LB_GETXMITLBMCOUNTS:
		{
			struct Test_LBCount_st Test_LBCount;
			copy_from_user(&Test_LBCount, (struct Test_LBCount_st*)arg, sizeof(struct Test_LBCount_st));
			ret = cfmConfig_GetXmitLBMCounts(&Test_LBCount.count, Test_LBCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBCount_st*)arg, &Test_LBCount, sizeof(struct Test_LBCount_st));
			break;
		}
	case CFM_LB_GETXMITLBRCOUNTS:
		{
			struct Test_LBCount_st Test_LBCount;
			copy_from_user(&Test_LBCount, (struct Test_LBCount_st*)arg, sizeof(struct Test_LBCount_st));
			ret = cfmConfig_GetXmitLBRCounts(&Test_LBCount.count, Test_LBCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBCount_st*)arg, &Test_LBCount, sizeof(struct Test_LBCount_st));
			break;
		}
	case CFM_LB_GETRCVDLBMCOUNTS:
		{
			struct Test_LBCount_st Test_LBCount;
			copy_from_user(&Test_LBCount, (struct Test_LBCount_st*)arg, sizeof(struct Test_LBCount_st));
			ret = cfmConfig_GetRcvdLBMCounts(&Test_LBCount.count, Test_LBCount.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBCount_st*)arg, &Test_LBCount, sizeof(struct Test_LBCount_st));
			break;
		}
	case CFM_LB_GETNEXTLBTRANSID:
		{
			struct Test_LBTransID_st Test_LBTransID;
			copy_from_user(&Test_LBTransID, (struct Test_LBTransID_st*)arg, sizeof(struct Test_LBTransID_st));
			ret = cfmConfig_GetNextLBTransID(&Test_LBTransID.lbid, Test_LBTransID.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBTransID_st*)arg, &Test_LBTransID, sizeof(struct Test_LBTransID_st));
			break;
		}
	case CFM_LB_GETLASTRCVDLBM:
		{
			struct Test_LBPdu_st Test_LBPdu;
			copy_from_user(&Test_LBPdu, (struct Test_LBPdu_st*)arg, sizeof(struct Test_LBPdu_st));
			ret = cfmConfig_GetLastRcvdLBM(Test_LBPdu.pkt_data, &Test_LBPdu.pkt_len, Test_LBPdu.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBPdu_st*)arg, &Test_LBPdu, sizeof(struct Test_LBPdu_st));
			break;
		}
	case CFM_LB_GETLASTRCVDLBR:
		{
			struct Test_LBPdu_st Test_LBPdu;
			copy_from_user(&Test_LBPdu, (struct Test_LBPdu_st*)arg, sizeof(struct Test_LBPdu_st));
			ret = cfmConfig_GetLastRcvdLBR(Test_LBPdu.pkt_data, &Test_LBPdu.pkt_len, Test_LBPdu.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBPdu_st*)arg, &Test_LBPdu, sizeof(struct Test_LBPdu_st));
			break;
		}
	case CFM_LB_GETLASTSENTLBM:
		{
			struct Test_LBPdu_st Test_LBPdu;
			copy_from_user(&Test_LBPdu, (struct Test_LBPdu_st*)arg, sizeof(struct Test_LBPdu_st));
			ret = cfmConfig_GetLastSentLBM(Test_LBPdu.pkt_data, &Test_LBPdu.pkt_len, Test_LBPdu.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBPdu_st*)arg, &Test_LBPdu, sizeof(struct Test_LBPdu_st));
			break;
		}
	case CFM_LB_GETLASTSENTLBR:
		{
			struct Test_LBPdu_st Test_LBPdu;
			copy_from_user(&Test_LBPdu, (struct Test_LBPdu_st*)arg, sizeof(struct Test_LBPdu_st));
			ret = cfmConfig_GetLastSentLBR(Test_LBPdu.pkt_data, &Test_LBPdu.pkt_len, Test_LBPdu.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBPdu_st*)arg, &Test_LBPdu, sizeof(struct Test_LBPdu_st));
			break;
		}
	case CFM_LB_SETTIMEOUT:
		{
			struct Test_LBTimeout_st Test_LBTimeout;
			copy_from_user(&Test_LBTimeout, (struct Test_LBTimeout_st*)arg, sizeof(struct Test_LBTimeout_st));
			ret = cfmConfig_SetLBTimeOut(Test_LBTimeout.time, Test_LBTimeout.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETTIMEOUT:
		{
			struct Test_LBTimeout_st Test_LBTimeout;
			copy_from_user(&Test_LBTimeout, (struct Test_LBTimeout_st*)arg, sizeof(struct Test_LBTimeout_st));
			ret = cfmConfig_GetLBTimeOut(&Test_LBTimeout.time, Test_LBTimeout.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBTimeout_st*)arg, &Test_LBTimeout, sizeof(struct Test_LBTimeout_st));
			break;
		}
	case CFM_LB_SETSNDIDTLVCONTAINED:
		{
			struct Test_LBTLVContained_st Test_LBTLVContained;
			copy_from_user(&Test_LBTLVContained, (struct Test_LBTLVContained_st*)arg, sizeof(struct Test_LBTLVContained_st));
			ret = cfmConfig_SetSenderIdTLVContainedInLBM(Test_LBTLVContained.contained, Test_LBTLVContained.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETSNDIDTLVCONTAINED:
		{
			struct Test_LBTLVContained_st Test_LBTLVContained;
			copy_from_user(&Test_LBTLVContained, (struct Test_LBTLVContained_st*)arg, sizeof(struct Test_LBTLVContained_st));
			ret = cfmConfig_GetSenderIdTLVContainedInLBM(&Test_LBTLVContained.contained, Test_LBTLVContained.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBTLVContained_st*)arg, &Test_LBTLVContained, sizeof(struct Test_LBTLVContained_st));
			break;
		}
	case CFM_LB_SETDATATLVCONTAINED:
		{
			struct Test_LBTLVContained_st Test_LBTLVContained;
			copy_from_user(&Test_LBTLVContained, (struct Test_LBTLVContained_st*)arg, sizeof(struct Test_LBTLVContained_st));
			ret = cfmConfig_SetDataTLVContainedInLBM(Test_LBTLVContained.contained, Test_LBTLVContained.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETDATATLVCONTAINED:
		{
			struct Test_LBTLVContained_st Test_LBTLVContained;
			copy_from_user(&Test_LBTLVContained, (struct Test_LBTLVContained_st*)arg, sizeof(struct Test_LBTLVContained_st));
			ret = cfmConfig_GetDataTLVContainedInLBM(&Test_LBTLVContained.contained, Test_LBTLVContained.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBTLVContained_st*)arg, &Test_LBTLVContained, sizeof(struct Test_LBTLVContained_st));
			break;
		}
	case CFM_LB_SETORGSPCIFICTLVCONTAINED:
		{
			struct Test_LBTLVContained_st Test_LBTLVContained;
			copy_from_user(&Test_LBTLVContained, (struct Test_LBTLVContained_st*)arg, sizeof(struct Test_LBTLVContained_st));
			ret = cfmConfig_SetOrgSpecificTLVContainedInLBM(Test_LBTLVContained.contained, Test_LBTLVContained.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETORGSPCIFICTLVCONTAINED:
		{
			struct Test_LBTLVContained_st Test_LBTLVContained;
			copy_from_user(&Test_LBTLVContained, (struct Test_LBTLVContained_st*)arg, sizeof(struct Test_LBTLVContained_st));
			ret = cfmConfig_GetOrgSpecificTLVContainedInLBM(&Test_LBTLVContained.contained, Test_LBTLVContained.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBTLVContained_st*)arg, &Test_LBTLVContained, sizeof(struct Test_LBTLVContained_st));
			break;
		}
	case CFM_LB_SETDATATLV:
		{
			struct Test_LBDataTLV_st Test_LBDataTLV;
			copy_from_user(&Test_LBDataTLV, (struct Test_LBDataTLV_st*)arg, sizeof(struct Test_LBDataTLV_st));
			ret = cfmConfig_SetDataTLVInLBM(Test_LBDataTLV.pkt_data, Test_LBDataTLV.pkt_len, Test_LBDataTLV.meid);
			if(ret != 0){
				return -1;
			}
			break;
		}
	case CFM_LB_GETDATATLV:
		{
			struct Test_LBDataTLV_st Test_LBDataTLV;
			copy_from_user(&Test_LBDataTLV, (struct Test_LBDataTLV_st*)arg, sizeof(struct Test_LBDataTLV_st));
			ret = cfmConfig_GetDataTLVInLBM(Test_LBDataTLV.pkt_data, &Test_LBDataTLV.pkt_len, Test_LBDataTLV.meid);
			if(ret != 0){
				return -1;
			}
			copy_to_user((struct Test_LBDataTLV_st*)arg, &Test_LBDataTLV, sizeof(struct Test_LBDataTLV_st));
			break;
		}
	
	default:
		return -1;

	}
        return 0;
}
static int  cfm_open( struct inode *inode, struct file *file )
{
	printk("open cfm module\n");
	return 0;
}
static int  cfm_close(struct inode *inode, struct file *file )
{
	printk("close cfm module\n");
	return 0;
}
static struct file_operations cfm_fops=
{
	owner:		THIS_MODULE,
	write:		cfm_write,
	read:       cfm_read,
	ioctl:		cfm_ioctl,
	open:		cfm_open,
	release:	cfm_close,
};
#define CFM_MINOR 130
static struct miscdevice cfm_miscdev=
{
	CFM_MINOR,
	"802.1ag CFM Module",
	&cfm_fops
};
static int  cfm_init(void)
{
	int ret;
	ret = misc_register(&cfm_miscdev);
	printk("Hello !--CFM Module\n");
	ret = cfm_core_init();
	return 0;
}

static void cfm_exit(void)
{
	cfm_core_destroy();
	misc_deregister(&cfm_miscdev);
	printk("Bye !--CFM Module\n");
}
MODULE_AUTHOR("Jone.Li");
MODULE_DESCRIPTION("802.1ag CFM Driver");
MODULE_LICENSE("Browan");

/* you must include the header file <linux/init.h>,if want use the two function*/
module_init(cfm_init);
module_exit(cfm_exit);
