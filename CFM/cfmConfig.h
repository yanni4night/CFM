#ifndef CFM_CONFIG_H_
#define CFM_CONFIG_H_
#include "cfm_header.h"
#include "cfm_protocol.h"

//some public

int cfmConfig_SetPeerMEPID(uint16* peerid, int meid);
int cfmConfig_GetPeerMEPID(uint16* peerid, int meid);

//SendId TLV

int cfmConfig_SetChassisIDLength(uint8 len);
int cfmConfig_GetChassisIDLength(uint8* len);

int cfmConfig_SetChassisIDSubtype(uint8 subtype);
int cfmConfig_GetChassisIDSubtype(uint8* subtype);

int cfmConfig_SetChassisID(uint8*chassisId);
int cfmConfig_GetChassisID(uint8*chassisId);

int cfmConfig_SetMgmtAddrDomainLength(uint8 len);
int cfmConfig_GetMgmtAddrDomainLength(uint8*len);

int cfmConfig_SetMgmtAddrDomain(uint8* domain);
int cfmConfig_GetMgmtAddrDomain(uint8* domain);

int cfmConfig_SetMgmtAddrLength(uint8 len);
int cfmConfig_GetMgmtAddrLength(uint8*len);

int cfmConfig_SetMgmtAddr (uint8*addr);
int cfmConfig_GetMgmtAddr (uint8*addr);


//Organization-Specific TLV
int cfmConfig_SetOrgSpecificLength(uint16 len);
int cfmConfig_GetOrgSpecificLength(uint16* len);

int cfmConfig_SetOrgSpecificOUI(uint8 *oui);
int cfmConfig_GetOrgSpecificOUI(uint8* oui);

int cfmConfig_SetOrgSpecificSubtype(uint8 subtype);
int cfmConfig_GetOrgSpecificSubtype (uint8* subtype);

int cfmConfig_SetOrgSpecificValue(uint8* value);
int cfmConfig_GetOrgSpecificValue(uint8* value);

//Alarm
int cfmConfig_SetAlarmThreshold (int threshold, int meid);
int cfmConfig_GetAlarmThreshold (int* threshold, int meid);

//CCP
int cfmConfig_SetCCMInterval(int interval, int meid);
int cfmConfig_GetCCMInterval(int* interval, int meid);
int cfmConfig_SetCCIenabled(int CCIenabled, int meid);
int cfmConfig_GetLastRcvdErrorCCM (uint8* ccm, int *len, int meid);
int cfmConfig_GetLastRcvdXconCCM (uint8* ccm, int *len, int meid);
int cfmConfig_GetOutOfSeqCCMCounts (int* counts, int meid);
int cfmConfig_GetXmitCCMCounts (int* counts, int meid);
int cfmConfig_CreateCCMDatabaseNode(uint16 MEPid, int meid);
int cfmConfig_GetPortStatus(uint8* value,int meid);
int cfmConfig_SetPortStatus(uint8  value, int meid);
int cfmConfig_GetInterfaceStatus(uint8* value,int meid);
int cfmConfig_SetInterfaceStatus(uint8  value, int meid);
int cfmConfig_SetCCOrgSpePermission(int value ,int meid);
int cfmConfig_GetCCOrgSpePermission(int* value ,int meid);

//LTP
int cfmConfig_LinkTrace_Start(uint8* targetAddr,int meid);
int cfmConfig_LinkTrace_GetResult(void* list, uint8* targetAddr, int meid);
int cfmConfig_GetUnexpectedLTRCounts (int* counts, int meid);
int cfmConfig_GetXmitLTRCounts (int* counts, int meid);
int cfmConfig_GetXmitLTMCounts(int* counts, int meid);
int cfmConfig_GetNextLTTranID (uint32* ltid, int meid);
int cfmConfig_SetNextLTTranID(uint32 nextId, int meid);
int cfmConfig_SetLTMTTL(uint8 ttl, int meid);
int cfmConfig_SetLTMflags(uint8 flags, int meid);
int cfmConfig_GetLTMreceivedCounts(int* counts, int meid);
int cfmConfig_GetLTRreceivedCounts(int* counts, int meid);
int cfmConfig_SetEgressID(uint8* gressid, int meid);
int cfmConfig_GetEgressID(uint8* gressid, int meid);
int cfmConfig_SetLTMSenderIDPermission(int flags, int meid);
int cfmConfig_GetLTMSenderIDPermission(int* flags, int meid);
int cfmConfig_SetLTRSenderIDPermission(int flags, int meid);
int cfmConfig_GetLTRSenderIDPermission(int* flags, int meid);

//LBP
int cfmConfig_Loopback_Start(int lbmstosend,uint8 *desAddr,int meid);
int cfmConfig_LoopBack_GetResult(int*result,int meid);
int cfmConfig_SetLBAvaliable(int valid,int meid);
int cfmConfig_GetLBAvaliable(int*valid,int meid);
int cfmConfig_GetErrorRcvdLBRCounts(int*count, int meid);
int cfmConfig_GetCorrectRcvdLBRCounts(int*count, int meid);
int cfmConfig_GetXmitLBMCounts(int*count, int meid);
int cfmConfig_GetXmitLBRCounts(int*count, int meid);
int cfmConfig_GetRcvdLBMCounts(int*count, int meid);
int cfmConfig_GetNextLBTransID(int* lbid, int meid);
int cfmConfig_GetLastRcvdLBM(uint8*pkt_data, uint32*pkt_len, int meid);
int cfmConfig_GetLastRcvdLBR(uint8*pkt_data, uint32*pkt_len, int meid);
int cfmConfig_GetLastSentLBM(uint8*pkt_data, uint32*pkt_len, int meid);
int cfmConfig_GetLastSentLBR(uint8*pkt_data, uint32*pkt_len, int meid);
int cfmConfig_SetLBTimeOut(uint32 time,int meid);
int cfmConfig_GetLBTimeOut(uint32*time,int meid);

int cfmConfig_SetSenderIdTLVContainedInLBM(int contained, int meid);
int cfmConfig_GetSenderIdTLVContainedInLBM(int*contained, int meid);
int cfmConfig_SetDataTLVContainedInLBM(int contained, int meid);
int cfmConfig_GetDataTLVContainedInLBM(int*contained, int meid);
int cfmConfig_SetDataTLVInLBM(uint8*pkt_data, uint32 pkt_len, int meid);
int cfmConfig_GetDataTLVInLBM(uint8*pkt_data, uint32*pkt_len, int meid);
int cfmConfig_SetOrgSpecificTLVContainedInLBM(int contained, int meid);
int cfmConfig_GetOrgSpecificTLVContainedInLBM(int*contained, int meid);


#endif//CFM_CONFIG_H_
