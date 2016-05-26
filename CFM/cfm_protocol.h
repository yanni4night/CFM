#ifndef CFM_PROTOCOL_H_
#define CFM_PROTOCOL_H_

#include "cfm_types.h"
#include "cfm_lists.h"
#include "cfm_header.h"
#include "cfm_tlv.h"
/*end*/

extern struct cfm_st *gCfm;

#define MAX_CFM_LEN   1600
/*start : protocol data type define*/


typedef struct dataStream_st* dataStream_t;
struct dataStream_st
{
	uint8 data[MAX_CFM_LEN];
	uint32 length;
};
/*********************************************************************************/
#if 0
typedef enum Fault_Notification_Generator_State
{
	Fault_Notification_Generator_State_Reset=0x01,
	Fault_Notification_Generator_State_Defect=0x02,
	Fault_Notification_Generator_State_Report_Defect=0x03,
	Fault_Notification_Generator_State_Defect_Reported=0x04,
	Fault_Notification_Generator_State_Defect_Clearing=0x05
};

typedef enum Highest_Priority_Defect_Observed
{
	Highest_Priority_Defect_Observed_No_Defect_Observed=0x00,
	Highest_Priority_Defect_Observed_Received_CCM_RDI_Set=0x01,
	Highest_Priority_Defect_Observed_Received_CCM_PortStatus_or_InterfaceStatus_Error=0x02,
	Highest_Priority_Defect_Observed_No_CCMs_Received_for_3p5_Interval=0x03,
	Highest_Priority_Defect_Observed_Received_Invalid_CCMs_for_3p5_Interval=0x04,
	Highest_Priority_Defect_Observed_Received_CCMs_from_Other_MA=0x05
};

typedef enum Current_Defects
{
	Current_Defects_Other_MEP_Transmitting_RDI=0x01,
	Current_Defects_Portstatus_or_Interfacestatus_Indicating_Error=0x02,
	Current_Defects_CCMs_Not_Received_for_3p5_Interval_from_Remote_MEPs=0x04,
	Current_Defects_Erroneous_CCMs_Received_for_3p5_Interval_from_Remote_MEPs=0x08,
	Current_Defects_CCMs_Received_for_3p5_Interval_from_MEP_Not_Configured_into_Current_MA=0x10
	
};

typedef enum Sender_ID_Permission
{
	Sender_ID_Permission_None=0x01,
	Sender_ID_Permission_Chassis=0x02,
	Sender_ID_Permission_Manage=0x03,
	Sender_ID_Permission_ChassisManage=0x04
};

typedef enum Detected_Configuration_Error
{
	Detected_Configuration_Error_CFM_Leak=0x01,
	Detected_Configuration_Error_Conflicting_VIDs=0x02,
	Detected_Configuration_Error_Excessive_Levels=0x03,
	Detected_Configuration_Error_Overlapped_Levels=0x04
};

typedef enum L2_Type
{
	Layer2_Type_MAC_Bridge_Service_Profile=0x00,
	Layer2_Type_Dot1p_Mapper=0x01
};

typedef enum MEP_Control
{
	MEP_Control_MEP_Generates_CCMs=0x02,
	MEP_Control_Enable_Yp1731_Server_MEP_Function=0x04,
	MEP_Control_Enable_Generation_of_Ethernet_AIS=0x08,
	MEP_Control_Up_MEP_Facing_toward_Core_of_Bridge=0x10	
};

typedef  enum Administrative_State
{
	Administrative_State_LOCK=0x01,
	Administrative_State_UNLOCK=0x00
};

typedef enum Table_Control
{
	Table_Control_Add_Record_to_Table=0x01,
	Table_Control_Delete_Record_from_Table=0x02,
	Table_Control_Clear_All_Entries_from_Table=0x03
};
#endif
  enum Detected_Configuration_Error
 {
	 Detected_Configuration_Error_CFM_Leak=0x01,
	 Detected_Configuration_Error_Conflicting_VIDs=0x02,
	 Detected_Configuration_Error_Excessive_Levels=0x03,
	 Detected_Configuration_Error_Overlapped_Levels=0x04
 };

 enum MD_Name_Format
{
	MD_Name_Format_None=0x01,
	MD_Name_Format_DNS_Like=0x02,
	MD_Name_Format_MAC_Addr_and_UINT=0x03,
	MD_Name_Format_Character_String =0x04,
	MD_Name_Format_ICC_Based =0x05
};

 enum Short_MA_Name_Format
{
	Short_MA_Name_Format_Primary_VID=0x01,
	Short_MA_Name_Format_Character_String =0x02,
	Short_MA_Name_Format_Two_Byte_Integer=0x03,
	Short_MA_Name_Format_VPN_ID=0x04
};

 enum MHF_Creation
{
	MHF_Creation_None=0x01,
	MHF_Creation_Default=0x02,
	MHF_Creation_Explicit=0x03
};

typedef struct MAC_Bridge_Service_Profile_st * MAC_Bridge_Service_Profile_t;
struct MAC_Bridge_Service_Profile_st
{
	uint16 meid;
	bool spanning_Tree_Ind;
	bool learning_Ind;
	bool port_Bridging_Ind;
	uint16 priority;
	uint16 maxAge;//6-40
	uint16 helloTime;		
	uint16 Forward_delay;//4
	bool unknown_MAC_Address_Discard;
	uint8 MAC_Learning_Depth;
};

typedef struct MAC_Bridge_Port_Configuration_Data_st * MAC_Bridge_Port_Configuration_Data_t;
struct MAC_Bridge_Port_Configuration_Data_st
{
	uint16 meid;
	MAC_Bridge_Service_Profile_t MAC_Bridge_Service_Profile;
	uint8 port_Num;
	uint8 tpType;
	uint16 tpPointer;
	uint16 port_Priority;
	uint16 port_Path_Cost;
	bool port_Spanning_Tree_Ind;
	bool LAN_FCS_Ind;
	uint8 port_MAC_Address[6];
	uint16 outbound_TD_Pointer;
	uint16 inbound_TD_Pointer;
};
typedef struct Layer2_Entity_st*Layer2_Entity_t;
struct Layer2_Entity_st
{
	uint16 port;
	uint8 MACAddr[ADDR_LEN];
};
struct pdu_st{
	uint8 data[MAX_CFM_LEN];
	int len;
};


typedef struct Test_Chassis_st * Test_Chassis_t;
struct Test_Chassis_st 
{
	uint8 chassis_ID_Length;
	uint8 chassis_ID_Subtype;
	uint8 chassis_ID_1[25];
	uint8 chassis_ID_2[25];
	uint8 management_Address_Domain_Length;
	uint8 management_Address_Domain_1[25];
	uint8 management_Address_Domain_2[25];
	uint8 management_Address_Length;
	uint8 management_Address_1[25];
	uint8 management_Address_2[25];
};

/*********************************************************************************/


struct MEPBasic_st{
	struct MEP_st *mep;
	uint16 meid;
	uint16 *L2EntityPointer;
	uint8 L2_type;
	struct MA_st *ma;
	uint16 MEPId;
	uint8 MEPControl;
	uint16 PrimaryVlan;
	uint8 AdministrativeState;
	uint8 CCM_LTM_priority;
	uint8 EgressId[8];
	uint16 PeerMEPId[12];
	uint8  ETH_AIS_control;
	uint8 FaultAarmThreshold;
	uint16 AlarmDeclarationSoakTime;//default 2.5 s
	uint16 AlarmClearSoakTime;//default 10 s
	
};
struct MEPStatus_st{
	struct MEP_st *mep;
	uint16 meid;
	uint8 MACAddress[ADDR_LEN];
	uint8 FaultNotificationGeneratorState;
	uint8 HighestPriorityDefectObserved;
	uint8 CurrentDefects;
	uint8 LastReceivedErrorCCM[128];
	uint8 LastReceivedXconCCM[128];
	uint32 OutOfSequenceCCMsCount;
	uint32 CCMsTransmittedCount;
	uint32 UnexpectedLTRsCount;
	uint32 LBRsTransmittedCount;
	uint32 NextLBTransId;
	uint32 NextLTTransId;
};

struct MEPDatabase_st
{
	LIST_ENTRY(MEPDatabase_st) list;  //必须加上
	uint16 RMepId;
	uint8 RMepState;
	uint32 failed_ok_time;
	uint8 MAID[48];
	uint8 MACAddr[ADDR_LEN];
	bool RDI;
	int count;
	uint32 expectedSequence;
	uint8   PortStatus;
	uint8   InterfaceStatus;
	struct Sender_ID_TLV_st 	    sender_ID_TLV;
};
LIST_HEAD(MEPDatabase_list, MEPDatabase_st);
struct MEPCCMDatabase_st{
	uint16 meid;
	LIST_DECARE(MEPDatabase_list)  CCMdatabase;
};
typedef struct MEP_st *MEP_t;
struct MEP_st{
	LIST_ENTRY(MEP_st) list; 
	int meid;
	struct MEPBasic_st MEPBasic;
	struct MEPStatus_st MEPStatus;
	struct MEPCCMDatabase_st MEPCCMDatabase;
	uint16 srcPortId;
	uint8 Direction;
	uint16 FlowId;
	bool MAdefectIndication;
	bool rMEPCCMdefect;
	PortStatus_Value rMEPlastPortState;
	Interface_Status rMEPlastInterfaceStatus;
	struct Sender_ID_TLV_st rMEPlastSenderId;
	bool rCCMreceived;
	bool rMEPportStatusDefect;
	bool rMEPinterfaceStatusDefect;
	bool errorCCMreceived;
	bool errorCCMdefect;
	bool xconCCMreceived;
	bool xconCCMdefect;	
	struct ccpm_st *ccpm;
	struct ltpm_st *ltpm;
	struct lbpm_st *lbpm;
};

typedef struct MA_st *MA_t;
struct MA_st{
	LIST_ENTRY(MA_st) list;
	uint16 meid;
	struct MD_st *MDPointer;
	uint8 ShortMAnameFormat;
	uint8 ShortMAnameLength;
	uint8 ShortMAname[50];
	uint8 CCMinterval;
	uint16 AssociatedVLANs[12];
	uint8 MHFCreation;
	uint8 SenderIDPermission;
};

struct MIPdatabase_st
{	
	LIST_ENTRY(MIPdatabase_st) list; 
	uint32 VID;	
	uint8 source_address[ADDR_LEN];	
	uint8 port_number;
};

LIST_HEAD(MIPdatabase_list,MIPdatabase_st);

typedef struct MIP_st * MIP_t;
struct MIP_st
{
	LIST_ENTRY(MIP_st) list;
	int meid;
	MA_t ma;
	uint8 MACAddr[ADDR_LEN];
	uint16 srcPortId;
	uint16 VlanId;
	uint8 Direction;
	uint8 MDLevel;
	bool MIPdatabase_enabled;
	struct  MHF_ccr_st* MHF_ccr;


	
	struct ltpm_st *ltpm;
	struct lbpm_st *lbpm;
};

typedef struct MD_st *MD_t;
struct MD_st{
	LIST_ENTRY(MD_st) list;
	uint16 meid;
	uint8 MDLevel;
	uint8 MDnameFormat;
	uint8 MDnameLength;
	uint8 MDname[50];
	uint8 MHFCreation;
	uint8 SenderIDPermission;
	int ma_num;
};

LIST_HEAD(MD_list, MD_st);
struct md_info_st{
	int meid;
	LIST_DECARE(MD_list) md_list;
	int md_num;
};
struct MPStatusEntry_st{
	LIST_ENTRY(MPStatusEntry_st) list;
	uint16 PortId;
	uint8 Level;
	uint8 Direction;
	uint16 VlanId;
	uint16 MD;//struct MD_st *MD;
	uint16 MA;//struct MA_st *MA;
	uint16 MEPId;
	uint8 MAC[ADDR_LEN];
};

struct ConfigurationErrorEntry_st{
	LIST_ENTRY(ConfigurationErrorEntry_st) list;
	uint16 VlanId;
	uint16 PortId;
	uint8 DetectedConfigurationError;
};
LIST_HEAD(MPStatus_list, MPStatusEntry_st);
LIST_HEAD(ConfigurationError_list, ConfigurationErrorEntry_st);

struct cfm_stack_st{
	uint16 meid;
	uint8 L2_type;
	LIST_DECARE(MPStatus_list) mpStatus_list;
	int MPStatusEntry_num;
	LIST_DECARE(ConfigurationError_list) configurationError_list;
	int ConfigurationErrorEntry_num;	
};

struct DefaultMDLevelEntry_st{
	LIST_ENTRY(DefaultMDLevelEntry_st) list;
	uint16 PrimaryVlanId;
	uint8 TableControl;
	uint8 Status;
	uint8 Level;
	uint8 MHFCreation;
	uint8 SenderIDPermission;
	uint16 AssociatedVLANs[11];
};
LIST_HEAD(DefaultMDLevel_list, DefaultMDLevelEntry_st);
struct DefaultMDLevel_st{
	uint16 meid;
	uint8 L2_type;
	uint8 CatchallLevel;
	uint8 CatchallMHFCreation;
	uint8 CatchallSenderIDPermission;
	LIST_DECARE(DefaultMDLevel_list) defaultMDLevel_list;
	int defaultMDLevelEntry_num;
};


LIST_HEAD(MA_list, MA_st);
struct ma_info_st{
	LIST_DECARE(MA_list) ma_list;
	int num;
};
LIST_HEAD(MIP_list, MIP_st);
struct mip_info_st{
	LIST_DECARE(MIP_list) mip_list;
	LIST_DECARE(MIPdatabase_list) MIPdatabase; //struct MIPdatabse_list MIPdatabse

	int num;
};
LIST_HEAD(MEP_list, MEP_st);
struct mep_info_st{
	LIST_DECARE(MEP_list) mep_list;
	int num;
};

#ifdef CFM_EMULATE
typedef struct FilteringDatabaseMAc_st *FilteringDatabaseMAc_t;
struct FilteringDatabaseMAc_st{
	LIST_ENTRY(FilteringDatabaseMAc_st) list;
	uint8 DestMAC[ADDR_LEN];
};
LIST_HEAD(FilteringDatabaseMac_list, FilteringDatabaseMAc_st);
typedef struct FilteringDatabaseVlan_st *FilteringDatabaseVlan_t;
struct FilteringDatabaseVlan_st{
	LIST_ENTRY(FilteringDatabaseVlan_st) list;
	uint16 VlanId;
	LIST_DECARE(FilteringDatabaseMac_list) FilteringDatabaseMac_list;
	
};
LIST_HEAD(FilteringDatabaseVlan_list, FilteringDatabaseVlan_st);
typedef struct FilteringDatabase_st* FilteringDatabase_t;
struct FilteringDatabase_st
{
	LIST_ENTRY(FilteringDatabase_st) list;
	uint16 PortID;
	uint8 MacAddr[ADDR_LEN];
	LIST_DECARE(FilteringDatabaseVlan_list) FilteringDatabaseVlan_list;
};
LIST_HEAD(FilteringDatabase_list, FilteringDatabase_st);
#endif

typedef struct cfm_st * cfm_t;
struct cfm_st{
	struct md_info_st md_info;
	struct cfm_stack_st cfm_stack;
	struct DefaultMDLevel_st DefaultMDLevel;
	
	struct ma_info_st ma_info;
	struct mep_info_st mep_info;
	struct mip_info_st mip_info;
	
	struct queue_st *recv_queue;
	struct queue_st *send_queue;
#ifdef CFM_EMULATE
	LIST_DECARE(FilteringDatabase_list) FilteringDatabase_list;
#endif
	struct TLV_st TLV;
};

typedef struct CFMHeader_st * CFMHeader_t;
struct CFMHeader_st{
	uint8 mdLevel_version ;
	uint8 opCode ;
	uint8 flags ;
	uint8 firstTLVOffset ;
};


/******************************\
			LBPM
\******************************/
typedef struct lbpm_st * lbpm_t;
struct lbpm_st{
	void* mp;
	int mpFlag;
	uint32 LBMsToSend ;
	uint32 nextLBMtransID ;
	uint32 expectedLBRtransID ;
	bool LBIactive ;
	bool xmitReady ;
	bool LBRreceived ;
	bool LBMreceived ;
	uint8 destination[ADDR_LEN];
	uint32 in_sequence_counter ;
	uint32 out_of_sequence_counter ;
	uint32 lbm_sent_counter;
	uint32 lbm_received_counter;
	uint32 lbr_sent_counter;
	uint8 LBIWhile ;
	struct dataStream_st lastSentLBR ;
	struct dataStream_st lastSentLBM ; 
	struct dataStream_st lastRecvdLBR ; 
	struct dataStream_st lastRecvdLBM ; 
	cfm_timer_t timer ;
    bool result;
    bool avaliable;
	bool senderID_TLV_Contained;
	bool data_TLV_Contained;
	bool orgSpecificTLVContained;
	uint32 timeout;//ms
	struct Data_TLV_st Data_TLV;
};

typedef struct lbmpdu_st * lbmpdu_t;
struct lbmpdu_st
{
    CFMHeader_t header ;
    uint32 ltransId ;
    uint8 endTLVOffset ;
};


/******************************\
			CCPM
\******************************/


struct MHF_ccr_st			//MHF CCR variables
{
	bool MHFrecvdCCM;
	struct pdu_st MHFCCMPDU;
};


 enum  defect_priority
{
	xconCCMdefect_pri        = 0x05,
	errorCCMdefect_pri       = 0x04,
	someRMEPCCMdefect_pri    = 0x03,
	someMACstatusDefect_pri  = 0x02,
	someRDIdefect_pri        = 0x01,
	noDefect_pri             = 0x00
};

/*ccm database*/
/*
struct database_st
{
	LIST_ENTRY(database_st) list;  //必须加上 
	uint16 MEPID;
	uint8 MAID[48];
	uint8 MACAddr[ADDR_LEN];
	bool RDI;
	int count;
	uint32 expectedSequence;
	struct Port_Status_TLV_st        port_status_TLV;
	struct Interface_Status_TLV_st interface_status_TLV;
	struct Sender_ID_TLV_st 	    sender_ID_TLV;
};
LIST_HEAD(database_list, database_st); 
*/
typedef struct ccpm_st* ccpm_t;
struct ccpm_st{
//	LIST_DECARE(database_list)  CCMdatabase;
	MEP_t mep;
	int CCIwhile;
	//uint8  CCMinterval;
	uint8 errorCCMwhile ;
	uint8 xconCCMwhile;
	bool   CCIenabled;
	uint32 CCIsentCCMs;
	bool   MACstatusChanged;
	///////////////////////////end of CCI variables
	bool   CCMreceivedEqual;
	struct pdu_st CCMequalPDU;
	bool CCMreceivedLow ;
	struct pdu_st CCMlowPDU ;
	uint8  recvdMacAddress[ADDR_LEN];
	bool recvdRDI ;
	uint8 recvdInterval ;
  	PortStatus_Value recvdPortState;
	Interface_Status recvdInterfaceStatus;
	struct Sender_ID_TLV_st recvdSenderId ;
	struct dataStream_st recvdFrame;
	uint32 CCMsequenceErrors;
	uint32 interval_count;
	cfm_timer_t timer ;
	
	uint32 out_of_sequence_ccms;
	bool someRMEPCCMdefect;
	bool someMACstatusDefect;
	bool someRDIdefect;
	
	uint32 fngAlarmTime;
	uint32 fngResetTime;
	uint32 FNGwhile;
	enum defect_priority fngPriority;
	enum defect_priority highestDefectPri;
	bool fngAlarmTime_on;
	bool fngResetTime_on;
	bool defect_reported;
	uint8 fault_alarm_address[ADDR_LEN];
	//////////////////////////end of fng  variables
	//struct MHF_ccr_st* MHF_ccr;// MHF CCR structure 
	//TLV
	struct Port_Status_TLV_st Port_Status_TLV;
	struct Interface_Status_TLV_st Interface_Status_TLV;
	//config
	uint16 peerid[12];
	bool org_spe_permission;
};
#if 1
struct ccmpdu_st
{
	struct CFMHeader_st header;
	unsigned int      seqNumber;
	uint16 MEPid;
	uint8  MAid[48];//will be defined as MAID_0 or MAID_1 structure,lenth=48bytes
	unsigned  int undefined1;//0;
	unsigned  int undefined2;//0;
	unsigned  int undefined3;//0;
	unsigned  int undefined4;//0;//Defined by ITU-T Y.1731,16bytes,0
	unsigned char	*optionalCCMTLV;
	unsigned char     endTLV;//=0;//usually 0
};
#endif

/******************************\
			LTPM
\******************************/
//单个 reply 链表节点   存储LTM或LTR
typedef struct ltmReplyListNode_st* ltmReplyListNode_t;
struct ltmReplyListNode_st
{
	LIST_ENTRY(ltmReplyListNode_st) list;
	uint8  node[MAX_CFM_LEN];         //存储内存地址
	uint32 length;			//存储消息长度
};
LIST_HEAD(ltmr_list, ltmReplyListNode_st);
//整个链表上的 单一节点
typedef struct ltemReplyList_st * ltemReplyList_t;
struct ltemReplyList_st
{
	uint32 TransID;
	uint8 dest_mac[ADDR_LEN];
	bool  is_time_out;
	bool  success_flag;
	cfm_timer_t timer;
	LIST_DECARE(ltmr_list) ltmr_list;   //存储LTM和LTR
};
//存储最终路径结果的节点
typedef struct linkTraceNode_st *linkTraceNode_t;
struct linkTraceNode_st
{
	LIST_ENTRY(linkTraceNode_st) list;
	uint8 mac_addr[ADDR_LEN];    
	uint8 TTL;
	uint8 ingress_action;
	uint8 ingressMAC[ADDR_LEN];
	uint8 egress_action;
	uint8 egressMAC[ADDR_LEN];
};
LIST_HEAD(trace_list,linkTraceNode_st);

//存储最终路径的链表
typedef struct linkTraceList_st * linkTraceList_t;
struct linkTraceList_st
{
	uint8 dest_mac[ADDR_LEN];
	bool success_flag;
	LIST_DECARE(trace_list) trace_list;
};
typedef struct ltpm_st * ltpm_t;
struct ltpm_st{
	void* mp;
	uint8  mptype;
	uint32   nextLTMtransID;
	ltemReplyList_t  reply_list;    //一维数组 节点存储链表 
	uint16 reply_list_size;
	uint16 index;
	
	linkTraceList_t result_list;      //一维数组，存储最终的路径 
	uint16 result_list_size;
	uint16 result_index;

	uint32  LTRreceived;    
	uint32  LTMreceived;
	
	uint32 LTMcount;      //主动发送的个数
	uint32 LTRcount;      //主动发送
	uint32 unexpectLTR;
	uint8 LTMttl;
	uint8 flags;     //LTM Common Header中的flags
	bool LTM_SenderID_Permission;
	bool LTR_SenderID_Permission;
	//LTR 状态机
	struct ltr_state_machine_st * ltr_machine;
	//TLV
//	struct Reply_Ingress_TLV_st Reply_Ingress_TLV;
//	struct Reply_Egress_TLV_st Reply_Egress_TLV;
	uint8 EgressID[8];
};

struct timeout_param
{
	ltemReplyList_t list;
	MEP_t mep;
};

typedef struct ltmpdu_st *ltmpdu_t;
struct ltmpdu_st{
	struct CFMHeader_st * header;
	uint32 ltmTransID;
	uint8 ltmTtl;
	uint8 oriAddr[ADDR_LEN];
	uint8 tarAddr[ADDR_LEN];
	uint8 endTlv;
};

typedef struct ltrpdu_st * ltrpdu_t;
struct ltrpdu_st
{
	CFMHeader_t header;
	uint32 ltrTransId;
	uint8  repTtl;
	uint8  relAction;
	uint8  endTlv;
};
//状态机状态
typedef enum {
	RT_IDLE, 
	RT_WAITING,
	RT_TRANSMITTING
}LTR_STATE;

typedef struct ltrListNode_st* ltrListNode_t;
struct ltrListNode_st
{
	LIST_ENTRY(ltrListNode_st) list;
	uint8 node[MAX_CFM_LEN];
	uint32 length;
	uint16 srcPortId;
	uint32 srcFlowId;
	int srcDirection;
};
LIST_HEAD(ltr_list,ltrListNode_st);

typedef struct ltr_state_machine_st *ltr_state_machine_t;
struct ltr_state_machine_st 
{
	cfm_spinlock_st lock;
	LTR_STATE ltr_state;
	LIST_DECARE(ltr_list) ltr_list;
	int nPendingLTRs;
	int timer_delay;
	cfm_timer_t ltr_timer;
	ltpm_t ltpm;
};
LIST_HEAD(link_list,linkTraceNode_st);



/******************************\
			Datatype end
\******************************/
/******************************\
			CFM and MP API
\******************************/
cfm_t cfm_create(void);
int cfm_destroy(cfm_t cfm);
int cfmSupervisory_Stop(cfm_t cfm);
int cfmSupervisory_GetChassisManagement(void * result);
/*Default MD Level*/
int cfmSupervisory_GetCFMDefaultCatchall(uint8 event, uint8 *value8, uint8 *value16);
int cfmSupervisory_SetCFMDefaultCatchall(uint8 event, uint8 *value8);
int cfmSupervisory_GetCFMDefaultTable(void*param);
int cfmSupervisory_SetCFMDefaultTable(uint16 *vidList, uint8 event, uint8 value);
/*CFM Stack*/
int cfm_AddCFMMPStatus(struct MPStatusEntry_st *MPStatusEntry);
int cfm_DeleteCFMMPStatus(uint16 PortId, uint8 Level, uint8 Direction, uint16 VlanId);
int cfmSupervisory_GetCFMMPStatus(void *param);
int cfmSupervisory_GetCFMConfigurationError(void *param);
int cfmSupervisory_GetCFMStack(void *pstack);
/*MD*/
int cfmSupervisory_GetMDList(void * MDList);
int cfmSupervisory_CreateMD(uint16 *meid, uint8 MDLevel, uint8 MDnameFormat, uint8 MDnameLength, uint8 *MDname);
int cfmSupervisory_DeleteMD(uint16 meid);
int cfmSupervisory_GetMD(void *result);
int cfmSupervisory_SetMD(uint16 meid, uint8 event, uint8 value, uint8 *MDname);
struct MD_st * cfm_GetMD(uint16 meid);
/*MA*/
int cfmSupervisory_CreateMA(uint16 *meid, uint16 MDId, uint8 ShortMAFormat, uint8 ShortMAnameLength, uint8 *ShortMAname, uint16 *AssociatedVLANs);
int cfmSupervisory_DeleteMA(uint16 meid);
int cfmSupervisory_GetMA(void *result);
int cfmSupervisory_SetMA(uint16 meid, uint8 event, uint8 value,  uint16 md, void *arg);
struct MA_st * cfm_GetMA(uint16 meid);
/*MEP*/
int cfmSupervisory_CreateMEP(uint16 *meid, uint16 MAid, uint16 MEPId, uint8 Direction, Layer2_Entity_t layer2Entity, uint16 FlowID);
int cfmSupervisory_DeleteMEP(uint16 meid);
int cfmSupervisory_GetMEP(uint16 meid, void *result);
int cfmSupervisory_SetMEP(uint16 meid, uint8 event, uint8 value8, uint16 value16 );
struct MEP_st * cfm_GetMEP(uint16 meid);
/*MIP*/
int UpdateMIPS(void);
/**/
void * cfm_GetNextMP(cfm_t cfm, uint16 VlanId, uint16 PortID, uint8 srcDirection, uint8 Action, void * mp, int *MPFlag);

 CFMHeader_t generateCFMHeader(int  mdLevel,pkt_type_t pduType,uint8 flags);
/******************************\
			LBPM API
\******************************/
lbpm_t lbpm_init(void * mp,int mpFlag);
int lbpm_destroy(lbpm_t lbpm);
void lbpm_start(uint8 lbmstosend,uint8 destination_address[ADDR_LEN],void * mp,int mpFlag);//Modified 11:41 07-22
int lbr_process(uint8*pkt_data,uint32 pkt_len,uint16 srcPortId,uint32 srcFlowId,int flags,void * mp,int mpFlag);//Modified 11:27 07-22
int lbm_process(uint8*pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int flags,void * mp,int mpFlag, uint8* result, int result_size, int* result_len, uint8 srcDirection);
void*alloc(uint32 size);
int isGroupAddr(uint8* addr);

/******************************\
			CCPM API
\******************************/
ccpm_t ccpm_init(MEP_t mp);
int ccpm_destroy(ccpm_t ccpm);
int ccpm_start(ccpm_t ccpm);
int ccm_process(uint8 *buffer, uint32 len, uint32 bridge_id, uint32 flow_id, int flag,MEP_t mp);
int MHFprocessCCM(uint8 * buffer, uint32 len,uint32 bridge_id, uint32 flow_id, int flag,MIP_t mip );
struct MHF_ccr_st * MHF_CCR_init(void);
int MHF_CCR_destroy(struct MHF_ccr_st * ccpm);

int MIPdatabase_destroy(cfm_t cfm);
int MIPdatabase_query(uint8* MACaddr,int vlan_id, uint16 *PortID);
/******************************\
			FNG API
\******************************/
int fault_notification_generator(MEP_t mp);
int highestDefectPri_get(MEP_t mp);
int xmitFaultAlarm(uint16 MEPid,uint8 MDlevel/*,uint8 MAid[]*/,int MEPprimaryVID, uint8 fngPriority/*,uint8 alarm_to_mac[]*/);

/******************************\
			LTPM API
\******************************/
ltpm_t ltpm_init(void* mp,uint8 mptype);
int ltpm_destroy(ltpm_t ltpm);
int ltpm_start(uint8 *tarAddr,MEP_t mep);
int ltr_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId, int flags, MEP_t mep);
int ltm_process(uint8 * pkt_data, uint32 pkt_len, uint16 srcPortId,uint32 srcFlowId, int flags, void* mp,uint8 mptype, uint8* result, int result_size, int* result_len, uint8 srcDirection);

/******************************\
			 API end
\******************************/
#endif//CFM_PROTOCOL_H_

