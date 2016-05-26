#include "cfmConfig.h"
int cfmConfig_SetPeerMEPID(uint16* peerid, int meid)
{
	MEP_t temp;
	int i;
	printk("in cfmConfig_SetPeerMEPID\n");
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	memcpy(temp->MEPBasic.PeerMEPId, peerid, sizeof(temp->MEPBasic.PeerMEPId));
	printk("peerid:\n");
	for(i=0;i<12;i++){
		printk("%d ", temp->MEPBasic.PeerMEPId[i]);
	}
	printk("\n");
	printk("cfmConfig_SetPeerMEPID end\n");
	return 0;
}
int cfmConfig_GetPeerMEPID(uint16* peerid, int meid)
{
	MEP_t temp;
	int i;
	printk("in cfmConfig_GetPeerMEPID\n");
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	memcpy(peerid, temp->MEPBasic.PeerMEPId, sizeof(temp->MEPBasic.PeerMEPId));
	printk("peerid:\n");
	for(i=0;i<12;i++){
		printk("%d ", peerid[i]);
	}
	printk("\n");
	printk("cfmConfig_GetPeerMEPID end\n");
	return 0;
}


//Chassis ID Length
int cfmConfig_SetChassisIDLength(uint8 len)
{
	printk("in cfmConfig_SetChassisIDLength\n");
	if(gCfm == NULL){
		return -1;
	}
	gCfm->TLV.Sender_ID_TLV.chassis_ID_Length = len;
	printk("chassis_ID_Length:%d\n", gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	printk("cfmConfig_SetChassisIDLength end\n");
	return 0;
}
int cfmConfig_GetChassisIDLength(uint8* len)
{
	printk("in cfmConfig_GetChassisIDLength\n");
	if(gCfm == NULL){
		return -1;
	}
	*len = gCfm->TLV.Sender_ID_TLV.chassis_ID_Length;
	printk("chassis_ID_Length:%d\n", *len);
	printk("cfmConfig_GetChassisIDLength end\n");
	return 0;
}

// Chassis ID Subtype
int cfmConfig_SetChassisIDSubtype(uint8 subtype)
{
	printk("in cfmConfig_SetChassisIDSubtype\n");
	if(gCfm == NULL){
		return -1;
	}
	gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype = subtype;
	printk("chassis_ID_Subtype:%d\n", gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype);
	printk("cfmConfig_SetChassisIDSubtype end\n");
	return 0;
}
int cfmConfig_GetChassisIDSubtype(uint8* subtype)
{
	printk("in cfmConfig_GetChassisIDSubtype\n");
	if(gCfm == NULL){
		return -1;
	}
	*subtype = gCfm->TLV.Sender_ID_TLV.chassis_ID_Subtype;
	printk("chassis_ID_Subtype:%d\n", *subtype);
	printk("cfmConfig_GetChassisIDSubtype end\n");
	return 0;
}
//Chassis ID
int cfmConfig_SetChassisID(uint8* chassisId)
{
	printk("in cfmConfig_SetChassisID\n");
	if(gCfm == NULL){
		return -1;
	}
	memcpy(gCfm->TLV.Sender_ID_TLV.chassis_ID, chassisId, gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	print_hex_data(gCfm->TLV.Sender_ID_TLV.chassis_ID,  gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	printk("cfmConfig_SetChassisID end\n");
	return 0;
}
int cfmConfig_GetChassisID(uint8* chassisId)
{
	printk("in cfmConfig_GetChassisID\n");
	if(gCfm == NULL){
		return -1;
	}
	memcpy(chassisId, gCfm->TLV.Sender_ID_TLV.chassis_ID, gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	print_hex_data(chassisId,  gCfm->TLV.Sender_ID_TLV.chassis_ID_Length);
	printk("cfmConfig_GetChassisID end\n");
	return 0;
}
//Management Address Domain Length
int cfmConfig_SetMgmtAddrDomainLength(uint8 len)
{
	printk("in cfmConfig_SetMgmtAddrDomainLength\n");
	if(gCfm == NULL){
		return -1;
	}
	gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length = len;
	printk("management_Address_Domain_Length:%d\n", gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	printk("cfmConfig_SetMgmtAddrDomainLength end\n");

	return 0;
}
int cfmConfig_GetMgmtAddrDomainLength(uint8* len)
{
	printk("in cfmConfig_GetMgmtAddrDomainLength\n");
	if(gCfm == NULL){
		return -1;
	}
	*len = gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length;
	printk("management_Address_Domain_Length:%d\n", *len);
	printk("cfmConfig_GetMgmtAddrDomainLength end\n");
	
	return 0;
}
//Management Address Domain
int cfmConfig_SetMgmtAddrDomain(uint8* domain)
{
	printk("in cfmConfig_SetMgmtAddrDomain\n");
	if(gCfm == NULL){
		return -1;
	}
	memcpy(gCfm->TLV.Sender_ID_TLV.management_Address_Domain, domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	print_hex_data(gCfm->TLV.Sender_ID_TLV.management_Address_Domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	printk("cfmConfig_SetMgmtAddrDomain end\n");
	return 0;
}
int cfmConfig_GetMgmtAddrDomain(uint8* domain)
{
	printk("in cfmConfig_GetMgmtAddrDomain\n");
	if(gCfm == NULL){
		return -1;
	}
	memcpy(domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	print_hex_data(domain, gCfm->TLV.Sender_ID_TLV.management_Address_Domain_Length);
	printk("cfmConfig_GetMgmtAddrDomain end\n");
	return 0;
}
//Management Address Length
int cfmConfig_SetMgmtAddrLength(uint8 len)
{
	printk("in cfmConfig_SetMgmtAddrLength\n");
	if(gCfm == NULL){
		return -1;
	}
	gCfm->TLV.Sender_ID_TLV.management_Address_Length = len;
	printk("management_Address_Length:%d\n", gCfm->TLV.Sender_ID_TLV.management_Address_Length);
	printk("cfmConfig_SetMgmtAddrLength end\n");
	return 0;
}
int cfmConfig_GetMgmtAddrLength(uint8* len)
{
	printk("in cfmConfig_GetMgmtAddrLength\n");
	if(gCfm == NULL){
		return -1;
	}
	*len = gCfm->TLV.Sender_ID_TLV.management_Address_Length;
	printk("management_Address_Length:%d\n", *len);
	printk("cfmConfig_GetMgmtAddrLength end\n");
	return 0;
}
//Management Address
int cfmConfig_SetMgmtAddr (uint8* addr)
{
	printk("in cfmConfig_SetMgmtAddr\n");
	if(gCfm == NULL){
		return -1;
	}
	memcpy(gCfm->TLV.Sender_ID_TLV.management_Address, addr, gCfm->TLV.Sender_ID_TLV.management_Address_Length);
	print_hex_data(gCfm->TLV.Sender_ID_TLV.management_Address,  gCfm->TLV.Sender_ID_TLV.management_Address_Length);
	printk("cfmConfig_SetMgmtAddr end\n");
	return 0;
}
int cfmConfig_GetMgmtAddr (uint8* addr)
{
	printk("in cfmConfig_GetMgmtAddr\n");
	if(gCfm == NULL){
		return -1;
	}
	memcpy(addr, gCfm->TLV.Sender_ID_TLV.management_Address, gCfm->TLV.Sender_ID_TLV.management_Address_Length);
	print_hex_data(addr, gCfm->TLV.Sender_ID_TLV.management_Address_Length);
	printk("cfmConfig_GetMgmtAddr end\n");
	return 0;
}



//Organization-Specific TLV
int cfmConfig_SetOrgSpecificLength(uint16 len)
{
	if(gCfm == NULL){
		return -1;
	}
	if(len < 4){
		printk("len is smaller than 4\n");
		return -1;
	}
	gCfm->TLV.Organization_Specific_TLV.length = len;
	return 0;
}
int cfmConfig_GetOrgSpecificLength(uint16* len)
{
	if(gCfm == NULL){
		return -1;
	}
	*len = gCfm->TLV.Organization_Specific_TLV.length;
	if(*len < 4){
		printk("len is smaller than 4\n");
		return -1;
	}
	return 0;
}

int cfmConfig_SetOrgSpecificOUI(uint8 *oui)
{
	if(gCfm == NULL){
		return -1;
	}
	memcpy(gCfm->TLV.Organization_Specific_TLV.OUI, oui, 3);
	return 0;
}
int cfmConfig_GetOrgSpecificOUI(uint8* oui)
{
	if(gCfm == NULL){
		return -1;
	}
	memcpy(oui, gCfm->TLV.Organization_Specific_TLV.OUI, 3);
	return 0;
}

int cfmConfig_SetOrgSpecificSubtype(uint8 subtype)
{
	if(gCfm == NULL){
		return -1;
	}
	gCfm->TLV.Organization_Specific_TLV.sub_Type = subtype;
	return 0;
}
int cfmConfig_GetOrgSpecificSubtype (uint8* subtype)
{
	if(gCfm == NULL){
		return -1;
	}
	*subtype = gCfm->TLV.Organization_Specific_TLV.sub_Type;
	return 0;
}

int cfmConfig_SetOrgSpecificValue(uint8* value)
{
	int len;
	
	if(gCfm == NULL){
		return -1;
	}
	len = gCfm->TLV.Organization_Specific_TLV.length - 4;
	if(len <= 0){
		return -1;
	}
	memcpy(gCfm->TLV.Organization_Specific_TLV.value, value, len);
	return 0;
}
int cfmConfig_GetOrgSpecificValue(uint8* value)
{
	int len;
	
	if(gCfm == NULL){
		return -1;
	}
	len = gCfm->TLV.Organization_Specific_TLV.length - 4;
	if(len <= 0){
		return -1;
	}
	memcpy(value, gCfm->TLV.Organization_Specific_TLV.value, len);
	return 0;
}
//Fault alarm threshold
int cfmConfig_SetAlarmThreshold (int threshold, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	temp->MEPBasic.FaultAarmThreshold = threshold;
	return 0;
}
int cfmConfig_GetAlarmThreshold (int* threshold, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	*threshold = temp->MEPBasic.FaultAarmThreshold;
	return 0;
}
//CCP
//CCM Interval
int cfmConfig_SetCCMInterval(int interval, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->MEPBasic.ma->CCMinterval = interval;

	return 0;
}
int cfmConfig_GetCCMInterval(int* interval, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	*interval = temp->MEPBasic.ma->CCMinterval;

	return 0;
}
int cfmConfig_SetCCIenabled(int CCIenabled, int meid)
{
	MEP_t temp;
	printk("in cfmConfig_SetCCIenabled\n");
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(CCIenabled){
		temp->ccpm->CCIenabled = true;
		temp->MEPBasic.MEPControl = temp->MEPBasic.MEPControl || 0x02;
		printk("CCIenabled is true\n");
	}
	else{
		temp->ccpm->CCIenabled = false;
		temp->MEPBasic.MEPControl = temp->MEPBasic.MEPControl && 0xFD;

	}
	printk("cfmConfig_SetCCIenabled end\n");
	return 0;
}

int cfmConfig_GetLastRcvdErrorCCM (uint8* ccm, int *len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	memcpy(ccm, temp->MEPStatus.LastReceivedErrorCCM , 128);
	//*len = temp->errorCCMlastFailure.length;
	return 0;
}

int cfmConfig_GetLastRcvdXconCCM (uint8* ccm, int *len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	memcpy(ccm, temp->MEPStatus.LastReceivedXconCCM , 128);
	//*len = temp->xconCCMlastFailure.length;
	return 0;
}

int cfmConfig_GetOutOfSeqCCMCounts (int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	*counts = temp->MEPStatus.OutOfSequenceCCMsCount;
	return 0;
}
int cfmConfig_GetXmitCCMCounts (int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	*counts = temp->MEPStatus.CCMsTransmittedCount;
	return 0;
}

int cfmConfig_CreateCCMDatabaseNode(uint16 MEPid, int meid)
{
	MEP_t temp;
	struct MEPDatabase_st* node;
	printk(" in cfmConfig_CreateCCMDatabaseNode\n");
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	LIST_TRAVERSE(&temp->MEPCCMDatabase.CCMdatabase, node, list)
		{
			if(node->RMepId== MEPid)
				{
					printk("this MEPid already existed!\n");
					return -1;
				}
		}
	node = (struct MEPDatabase_st*)kmalloc(sizeof(struct MEPDatabase_st),GFP_KERNEL);
	if (node == NULL){
		printk("create new ccm databse node error!\n");
		return -1;
	}
	memset(node, 0, sizeof(struct MEPDatabase_st));
	node->RMepId = MEPid;

	LIST_INSERT_TAIL(&temp->MEPCCMDatabase.CCMdatabase, node, list);
	printk("cfmConfig_CreateCCMDatabaseNode end\n");
	return 0;
	
}

int cfmConfig_SetPortStatus(uint8  value, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->ccpm->Port_Status_TLV.port_status_value=value;
	return 0;
}
int cfmConfig_GetPortStatus(uint8* value,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*value=temp->ccpm->Port_Status_TLV.port_status_value;
	return 0;
}

int cfmConfig_SetInterfaceStatus(uint8  value, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->ccpm->Interface_Status_TLV.interface_status=value;
	return 0;
}
int cfmConfig_GetInterfaceStatus(uint8* value,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*value=temp->ccpm->Interface_Status_TLV.interface_status;
	return 0;
}
int cfmConfig_SetCCOrgSpePermission(int value ,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(value){
		temp->ccpm->org_spe_permission = true;
	}
	else{
		temp->ccpm->org_spe_permission = false;
	}
	return 0;
}
int cfmConfig_GetCCOrgSpePermission(int* value ,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if( temp->ccpm->org_spe_permission){
		*value = 1;
	}
	else{
		*value = 0;
	}

	return 0;
}
//LTP
int cfmConfig_LinkTrace_Start(uint8* targetAddr,int meid)
{
	MEP_t mp;
	int ret;
	mp = cfm_GetMEP(meid);
	if (mp == NULL)
	{
		printk("Get MEP result is NULL in linkTrace_start()\n");
		return -1;
	}
	ret = ltpm_start(targetAddr,mp);
	if (ret != 0){
		return -1;
	}
	
	return 0;
}
int cfmConfig_LinkTrace_GetResult(void* list, uint8* targetAddr, int meid)
{
	MEP_t mp;
	linkTraceList_t result_list;
	linkTraceNode_t tmp,pre,node;
	int i;
	struct link_list * trace_list=NULL;

	printk("in linkTrace_GetResult.\n");
	trace_list =(struct link_list*)list;
	if (trace_list == NULL || targetAddr ==NULL)
	{
		printk("bad parameters\n");
		return -1;
	}
	mp = cfm_GetMEP(meid);
	if (mp == NULL)
	{
		printk("Get MEP result is NULL in linkTrace_GetResult()\n");
		return -1;
	}
	result_list = mp->ltpm->result_list;
	for (i=0;i<mp->ltpm->result_list_size;i++)
	{
		if ( !memcmp(result_list[i].dest_mac,targetAddr,ADDR_LEN))
		{
			tmp = LIST_FIRST(&result_list[i].trace_list);
			while(tmp != NULL)
			{
				pre = tmp;
				tmp = LIST_NEXT(tmp,list);
				node = (linkTraceNode_t)kmalloc(sizeof(struct linkTraceNode_st), GFP_KERNEL);
				memset(node,0,sizeof(struct linkTraceNode_st));
				memcpy(node->mac_addr,pre->mac_addr, ADDR_LEN);
				node->TTL = pre->TTL;
				node->ingress_action = pre->ingress_action;
				memcpy(node->ingressMAC, pre->ingressMAC, ADDR_LEN);
				node->egress_action = pre->egress_action;
				memcpy(node->egressMAC, pre->egressMAC, ADDR_LEN);
				LIST_INSERT_TAIL(trace_list,node,list);
			}
			if(result_list[i].success_flag)
				return 1;
			else return 0;
		}
	}
	printk("leave get result.\n");
	return -2;    //没有发起过这样的请求

}

int cfmConfig_GetXmitLTRCounts (int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*counts = temp->ltpm->LTRcount;
	
	return 0;
}
int cfmConfig_GetXmitLTMCounts(int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*counts = temp->ltpm->LTMcount;
	
	return 0;
}
int cfmConfig_GetLTMreceivedCounts(int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*counts = temp->ltpm->LTMreceived;
	return 0;
}
int cfmConfig_GetLTRreceivedCounts(int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*counts = temp->ltpm->LTRreceived;
	return 0;
}

int cfmConfig_GetUnexpectedLTRCounts (int* counts, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*counts = temp->MEPStatus.UnexpectedLTRsCount;
	
	return 0;
}

int cfmConfig_GetNextLTTranID (uint32* ltid, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*ltid = temp->ltpm->nextLTMtransID;
	
	return 0;
}

int cfmConfig_SetNextLTTranID(uint32 nextId, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->ltpm->nextLTMtransID = nextId;
	return 0;
}
int cfmConfig_SetLTMTTL(uint8 ttl, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->ltpm->LTMttl = ttl;
	printk("LTMttl:%.2x\n", temp->ltpm->LTMttl);
	return 0;
}

int cfmConfig_SetLTMflags(uint8 flags, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->ltpm->flags = flags;       //useFDBonly
	printk("flags:%.2x\n", temp->ltpm->flags);
	return 0;
}

//Egress identifier
int cfmConfig_SetEgressID(uint8* gressid, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	memcpy(temp->ltpm->EgressID, gressid, 8);

	return 0;
}
int cfmConfig_GetEgressID(uint8* gressid, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	memcpy(gressid, temp->ltpm->EgressID, 8);

	return 0;
}
int cfmConfig_SetLTMSenderIDPermission(int flags, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(flags){
		temp->ltpm->LTM_SenderID_Permission = true;
	}
	else{
		temp->ltpm->LTM_SenderID_Permission = false;
	}
	return 0;
}
int cfmConfig_GetLTMSenderIDPermission(int* flags, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->ltpm->LTM_SenderID_Permission){
		*flags = 1;
	}
	else{
		*flags = 0;
	}
	return 0;
}
int cfmConfig_SetLTRSenderIDPermission(int flags, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(flags){
		temp->ltpm->LTR_SenderID_Permission = true;
	}
	else{
		temp->ltpm->LTR_SenderID_Permission = false;
	}

	return 0;
}

int cfmConfig_GetLTRSenderIDPermission(int* flags, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->ltpm->LTR_SenderID_Permission){
		*flags = 1;
	}
	else{
		*flags = 0;
	}
	return 0;
}

//LBP
int cfmConfig_Loopback_Start(int lbmstosend, uint8 *desAddr, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	
	lbpm_start(lbmstosend, desAddr, temp,MEP);
	
	return 0;
}

int cfmConfig_LoopBack_GetResult(int*result,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->lbpm->result){
		*result = 1;
	}
	else{
		*result = 0;
	}
	return 0;
}

int cfmConfig_SetLBAvaliable(int valid,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(valid){
		temp->lbpm->avaliable = true;
	}
	else{
		temp->lbpm->avaliable = false;
	}
	return 0;
}

int cfmConfig_GetLBAvaliable(int*valid,int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->lbpm->avaliable){
		*valid = 1;
	}
	else{
		*valid = 0;
	}
	return 0;
}

int cfmConfig_GetErrorRcvdLBRCounts(int*count, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*count = temp->lbpm->out_of_sequence_counter;
	return 0;
    
}

int cfmConfig_GetCorrectRcvdLBRCounts(int*count, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*count = temp->lbpm->in_sequence_counter;
	return 0;    
}

int cfmConfig_GetXmitLBMCounts(int*count, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*count = temp->lbpm->lbm_sent_counter;
 	return 0;
}

int cfmConfig_GetXmitLBRCounts(int*count, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*count = temp->lbpm->lbr_sent_counter;
	return 0;
}
int cfmConfig_GetRcvdLBMCounts(int*count, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*count = temp->lbpm->lbm_received_counter;
	return 0; 
}
int cfmConfig_GetNextLBTransID(int* lbid, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*lbid = temp->lbpm->nextLBMtransID;
	return 0;
}
int cfmConfig_GetLastRcvdLBM(uint8*pkt_data, uint32*pkt_len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	memcpy(pkt_data, temp->lbpm->lastRecvdLBM.data, temp->lbpm->lastRecvdLBM.length);
	*pkt_len = temp->lbpm->lastRecvdLBM.length;
	return 0;
}
int cfmConfig_GetLastRcvdLBR(uint8*pkt_data, uint32*pkt_len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	memcpy(pkt_data, temp->lbpm->lastRecvdLBR.data, temp->lbpm->lastRecvdLBR.length);
	*pkt_len = temp->lbpm->lastRecvdLBR.length;
	return 0;
}
int cfmConfig_GetLastSentLBM(uint8*pkt_data, uint32*pkt_len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	memcpy(pkt_data, temp->lbpm->lastSentLBM.data, temp->lbpm->lastSentLBM.length);
	*pkt_len = temp->lbpm->lastSentLBM.length;
	return 0;
}
int cfmConfig_GetLastSentLBR(uint8*pkt_data, uint32*pkt_len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	memcpy(pkt_data, temp->lbpm->lastSentLBR.data, temp->lbpm->lastSentLBR.length);
	*pkt_len = temp->lbpm->lastSentLBR.length;
	return 0;
}

int cfmConfig_SetLBTimeOut(uint32 time, int meid)
{
	MEP_t temp;
	printk("in cfmConfig_SetLBTimeOut\n");
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	temp->lbpm->timeout = time ;
	printk("timeout:%lu\n", temp->lbpm->timeout);
	printk("cfmConfig_SetLBTimeOut end\n");
	return 0;
}
int cfmConfig_GetLBTimeOut(uint32*time, int meid)
{
	MEP_t temp;
	printk("in cfmConfig_GetLBTimeOut\n");
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	*time = temp->lbpm->timeout;
	printk("timeout:%lu\n", *time);
	printk("cfmConfig_GetLBTimeOut\n");
	return 0;
}

int cfmConfig_SetSenderIdTLVContainedInLBM(int contained, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(contained){
		temp->lbpm->senderID_TLV_Contained = true;
	}
	else{
		temp->lbpm->senderID_TLV_Contained = false;
	}
	return 0;
    
}
int cfmConfig_GetSenderIdTLVContainedInLBM(int*contained, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->lbpm->senderID_TLV_Contained){
		*contained = 1;
	}
	else{
		*contained = 0;
	}
	return 0;
}
int cfmConfig_SetDataTLVContainedInLBM(int contained, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(contained){
		temp->lbpm->data_TLV_Contained = true;
	}
	else{
		temp->lbpm->data_TLV_Contained = false;
	}
	return 0;
}
int cfmConfig_GetDataTLVContainedInLBM(int*contained, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->lbpm->data_TLV_Contained){
		*contained = 1;
	}
	else{
		*contained = 0;
	}
	return 0;
}

int cfmConfig_SetOrgSpecificTLVContainedInLBM(int contained, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(contained){
		temp->lbpm->orgSpecificTLVContained = true;
	}
	else{
		temp->lbpm->orgSpecificTLVContained = false;
	}
	return 0;
}
int cfmConfig_GetOrgSpecificTLVContainedInLBM(int*contained, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	if(temp->lbpm->orgSpecificTLVContained){
		*contained = 1;
	}
	else{
		*contained = 0;
	}
	return 0;
}

int cfmConfig_SetDataTLVInLBM(uint8*pkt_data, uint32 pkt_len, int meid)
{
	MEP_t temp;

	if(pkt_data == NULL){
		return -1;
	}
	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}

	memcpy(temp->lbpm->Data_TLV.data, pkt_data, pkt_len);
	temp->lbpm->Data_TLV.length = pkt_len ;
	return 0;
}
int cfmConfig_GetDataTLVInLBM(uint8*pkt_data, uint32*pkt_len, int meid)
{
	MEP_t temp;

	temp = cfm_GetMEP(meid);
	if(temp == NULL){
		printk("cant find MP with meid:%d\n", meid);
		return -1;
	}
	memcpy(pkt_data, temp->lbpm->Data_TLV.data, temp->lbpm->Data_TLV.length);
	*pkt_len = temp->lbpm->Data_TLV.length;
	return 0;
}


