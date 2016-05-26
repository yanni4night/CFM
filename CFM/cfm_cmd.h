#ifndef 	CFM_CMD_H_
#define 	CFM_CMD_H_
#define 	MAX_MD_NUM	7
#define		MAX_MP_NUM 20
#define 	MAX_ERR_NUM 30
#include	"cfm_types.h"

//cmd start
#define	CFM_STOP		0x01

#define	CFM_GET_MDLIST		0x03
struct Test_MD_st{
	uint16 meid;
	uint8 event;
	uint8 value;
	uint8 MDLevel;
	uint8 MDnameFormat;
	uint8 MDnameLength;
	uint8 MDname[50];
	uint8 MHFCreation;
	uint8 SenderIDPermission;
};

struct Test_MDList_st{
	struct Test_MD_st MDList[MAX_MD_NUM];
	int num;
};
#define	CFM_CREATE_MD	0x04	
#define	CFM_DELETE_MD	0x05
#define	CFM_SET_MD		0x06
#define	CFM_GET_MD		0x07

struct Test_MA_st
{
	uint16 meid;
	uint8 event;
	uint8 value;
	void *arg;
	uint8 ShortMAnameFormat;
	uint8 ShortMAnameLength;
	uint8 ShortMAname[50];
	uint8 CCMinterval;
	uint16 AssociatedVLANs[12];
	uint8 MHFCreation;
	uint8 SenderIDPermission;
	uint16 MDid;
};
#define	CFM_CREATE_MA	0x08
#define	CFM_DELETE_MA	0x09
#define	CFM_SET_MA		0x0A
#define	CFM_GET_MA		0x0B

#define	CFM_CREATE_MEP	0x0C
struct Test_MEP_Create_st
{
	uint16 meid;
	uint16 MAid;
	uint16 MEPId;
	uint8 Direction;
	uint16 srcPortId;
	uint8 MACAddr[ADDR_LEN];
	uint16 FlowID;

};
#define	CFM_DELETE_MEP	0x0D
#define	CFM_SET_MEP		0x0E
#define	CFM_GET_MEP		0x0F
struct Test_MEP_Set_st
{
	uint16 meid;
	uint8 event;
	uint8 value8;
	uint16 value16 ;

};
struct Test_MEP_Get_st
{
	uint16 meid;
	uint8 Level;
	uint8 Direction;
	uint16 srcPortId;
	uint32 FlowId;
	uint16 PrimaryVlan;
	uint8 AdministrativeState;
	uint8 FaultNotificationGeneratorState;
	uint8 CCM_LTM_priority;
	uint8 MACAddress[ADDR_LEN];
	uint8 FaultAarmThreshold;
	uint16 AlarmDeclarationSoakTime;
	uint16 AlarmClearSoakTime;
	uint8 HighestPriorityDefectObserved;
	uint8 CurrentDefects;
	uint8 LastReceivedErrorCCM[128];
	uint8 LastReceivedXconCCM[128];
	uint32 OutOfSequenceCCMsCount;
	uint32 CCMsTransmittedCount;
	uint32 NextLBTransId;
	uint32 NextLTTransId;
	uint32 UnexpectedLTRsCount;
	uint32 LBRsTransmittedCount;
	
};
#define	CFM_GET_MPSTATUS	0x10	
#define	CFM_GET_CONFIGERROR	0x11
#define	CFM_GET_STACK		0x12

 struct Test_MPStatusEntry_st{
	uint16 PortId;
	uint8 Level;
	uint8 Direction;
	uint16 VlanId;
	uint16 MD;//struct MD_st *MD;
	uint16 MA;//struct MA_st *MA;
	uint16 MEPId;
	uint8 MAC[ADDR_LEN];
};

struct Test_ConfigurationErrorEntry_st{
	uint16 VlanId;
	uint16 PortId;
	uint8 DetectedConfigurationError;
};

struct Test_CFMStack_st{
	uint16 meid;
	uint8 L2_type;
	struct Test_MPStatusEntry_st MPStatus[MAX_MP_NUM];
	int MPStatusEntry_num;
	struct Test_ConfigurationErrorEntry_st ConfigurationError[MAX_ERR_NUM];
	int ConfigurationErrorEntry_num;	
};

#define	CFM_SET_DEFAULTCATCHALL		0x13
#define	CFM_GET_DEFAULTCATCHALL		0x14
struct Test_CFMDefaultCatchall_st
{
	uint8 event;
	uint8 value8;
	uint8 value16;
};
#define	CFM_SET_DEFAULTTABLE		0x15
#define	CFM_GET_DEFAULTTALBE		0x16
struct Test_CFMDefaultTable_st
{
	uint16 *vidList;
	uint8 event;
	uint8 value;

};
struct Test_DefaultMDLevelEntry_st{
	uint16 PrimaryVlanId;
	uint8 TableControl;
	uint8 Status;
	uint8 Level;
	uint8 MHFCreation;
	uint8 SenderIDPermission;
	uint16 AssociatedVLANs[11];
};

//some public
#define	CFM_SETPEERMEPID				0x1A
#define	CFM_GETPEERMEPID				0x1B
struct Test_PeerMEPID_st{
	int meid;
	uint16 peerId[12];
};


//SenderId TLV
#define	CFM_SET_CHASSISIDLENGTH		0x20
#define	CFM_GET_CHASSISIDLENGTH		0x21
#define	CFM_SET_CHASSISIDSBUTYPE		0x22
#define	CFM_GET_CHASSISIDSBUTYPE		0x23
#define	CFM_SET_CHASSISID				0x24
#define	CFM_GET_CHASSISID				0x25
#define	CFM_SET_MGMTADDRDOMAINLENGTH	0x26
#define	CFM_GET_MGMTADDRDOMAINLENGTH	0x27
#define	CFM_SET_MGMTADDRDOMAINM			0x28
#define	CFM_GET_MGMTADDRDOMAINM			0x29
#define	CFM_SET_MGMTADDRLENGTH			0x2A
#define	CFM_GET_MGMTADDRLENGTH			0x2B
#define	CFM_SET_MGMTADDR					0x2C
#define	CFM_GET_MGMTADDR					0x2D

//Organization-Specific TLV
#define	CFM_SET_ORGSPELEN					0x30
#define	CFM_GET_ORGSPELEN					0x31
#define	CFM_SET_ORGSPEOUI					0x32
#define	CFM_GET_ORGSPEOUI					0x33
#define	CFM_SET_ORGSPESUBTYPE				0x34
#define	CFM_GET_ORGSPESUBTYPE				0x35
#define	CFM_SET_ORGSPEVALUE				0x36
#define	CFM_GET_ORGSPEVALUE				0x37

//alarm
#define	CFM_SET_ALARMTHRESHOLD				0x40
#define	CFM_GET_ALARMTHRESHOLD				0x41
struct Test_AlarmPri_st{
	int meid;
	int AlarmPri;
};
//CCP
#define	CFM_CCM_START					0x50
#define	CFM_SET_CCMINTERVAL			0X51
#define	CFM_GET_CCMINTERVAL			0x52
struct Test_CCMInterval_st{
	int meid;
	int interval;
};
#define	CFM_SET_CCIENABLED				0x53
struct Test_CCIenabled_st{
	int meid;
	int CCIenabled;
};

#define	CFM_GET_LASTRCVDERRORCCM		0x54
#define	CFM_GET_LASTRCVDXCONCCM		0x55
struct Test_ccm_st{
	int meid;
	uint8 ccm[128];
	int len;
};

#define	CFM_GET_OUTOFSEQCCMCOUNTS	0x56
#define	CFM_GET_XMITCCMCOUNTS		0x57
struct Test_CCCount_st{
	int meid;
	int count;
};

#define	CFM_CREATECCMDABASENODE		0x58
struct Test_CreateCCMDatabaseNode_st{
	int meid;
	uint16 MEPid;
};

#define	CFM_SET_PORTSTATUS			0x59
#define	CFM_GET_PORTSTATUS			0x5A
#define	CFM_SET_INTERFASESTATUS		0x5B
#define	CFM_GET_INTERFASESTATUS		0x5C
struct Test_Status_st{
	int meid;
	uint8  value;
};
#define CFM_SET_CCORGSPEPERMISSION 0x5D
#define CFM_GET_CCORGSPEPERMISSION 0x5E
struct Test_OrgSpePermission_st{
	int meid;
	int permission; 
};
//LTP
#define	CFM_LT_START						0x70
struct Test_LTStart_st{
	int meid;
	uint8 MacAddr[ADDR_LEN];
};
#define	CFM_LT_GETRESULT					0x71

#define	CFM_LT_GETXMITLTRCOUNTS			0x72
#define	CFM_LT_GETXMITLTMCOUNTS			0x73
#define	CFM_LT_GETLTMRECEIVEDCOUNTS		0x74
#define	CFM_LT_GETLTRRECEIVEDCOUNTS		0x75
#define	CFM_LT_GETUNEXPECTEDLTRCOUNTS	0x76
struct Test_LTCount_st{
	int meid;
	int count;
};

#define	CFM_LT_GETNEXTTRANSID				0x77
struct Test_LTTransId_st{
	int meid;
	uint32  ltid;
};
#define CFM_LT_SETNEXTTRANSID				0x78


#define	CFM_LT_SETLTMTTL              			0x79
struct Test_LTSetLTMTtl_st{
	int meid;
	uint8 ttl;
};
#define 	CFM_LT_SETLTMFLAGS					0x7A
struct Test_LTSetLTMFlags_st{
	int meid;
	uint8 flags;
};
#define	CFM_SETEGRESSID					0x7B
#define	CFM_GETEGRESSID					0x7C
struct Test_LTEgressId_st{
	int meid;
	uint8 gressid[8];
};
#define CFM_SETLTM_SENDERID_PERMISSION    0x7D
#define CFM_GETLTM_SENDERID_PERMISSION   0x7E
#define CFM_SETLTR_SENDERID_PERMISSION    0x7F
#define CFM_GETLTR_SENDERID_PERMISSION   0x80

struct Test_LTSenderIDPermission_st{
	int meid;
	int flags;
};

struct resultNode_st
{
	uint8 mac_addr[ADDR_LEN];    
	uint8 TTL;
	uint8 ingress_action;
	uint8 ingressMAC[ADDR_LEN];
	uint8 egress_action;
	uint8 egressMAC[ADDR_LEN];
};
struct result_param
{
	int meid;
	uint8 mac[6];
	uint8 node_num;
	struct resultNode_st node[10];	
};
//LBP
#define	CFM_LB_START			0x90
struct Test_LBStart_st{
	int meid;
	uint8 MacAddr[ADDR_LEN];
	int lbmstosend;
};

#define	CFM_LB_GETRESULT		0x91
struct Test_LBGetResult_st{
	int meid;
	int result;
};

#define	CFM_LB_SETLBAVALIABLE	0x92
#define	CFM_LB_GETLBAVALIABLE	0x93
struct Test_LBAvaliable_st{
	int meid;
	int LBAvaliable;
};

#define	CFM_LB_GETERRORRCVDLBRCOUNTS 	0x94
#define	CFM_LB_GETCORRECTRCVDLBRCOUNTS	0x95
#define	CFM_LB_GETXMITLBMCOUNTS			0x96
#define	CFM_LB_GETXMITLBRCOUNTS			0x97
#define	CFM_LB_GETRCVDLBMCOUNTS			0x98
struct Test_LBCount_st{
	int meid;
	int count;
};

#define	CFM_LB_GETNEXTLBTRANSID		0x99
struct Test_LBTransID_st{
	int meid;
	int lbid;
};

#define	CFM_LB_GETLASTRCVDLBM			0x9A
#define	CFM_LB_GETLASTRCVDLBR			0x9B
#define	CFM_LB_GETLASTSENTLBM			0x9C
#define	CFM_LB_GETLASTSENTLBR			0x9D

struct Test_LBPdu_st{
	int meid;
	uint8 pkt_data[128];
	uint32 pkt_len;
};
#define	CFM_LB_SETTIMEOUT			0x9E
#define	CFM_LB_GETTIMEOUT			0x9F
struct Test_LBTimeout_st{
	int meid;
	uint32 time;
};


#define	CFM_LB_SETSNDIDTLVCONTAINED		0xA0	
#define	CFM_LB_GETSNDIDTLVCONTAINED		0xA1
#define	CFM_LB_SETDATATLVCONTAINED		0xA2
#define	CFM_LB_GETDATATLVCONTAINED		0xA3
#define	CFM_LB_SETORGSPCIFICTLVCONTAINED	0xA4
#define	CFM_LB_GETORGSPCIFICTLVCONTAINED	0xA5
struct Test_LBTLVContained_st{
	int meid;
	int contained;
};

#define	CFM_LB_SETDATATLV			0xA6
#define	CFM_LB_GETDATATLV			0xA7
struct Test_LBDataTLV_st{
	int meid;
	uint8 pkt_data[128];
	uint32 pkt_len;
};

#define CFM_GET_CHASSISMANAGEMENT 0xA8
struct Test_ChassisManagement_st
{
	uint8 chassis_ID_Length;
	uint8 chassis_ID_Subtype;
	uint8 chassis_ID[50];
	uint8 management_Address_Domain_Length;
	uint8 management_Address_Domain[50];
	uint8 management_Address_Length;
	uint8 management_Address[50];
};

//end
#endif//CFM_CMD_H_
