#ifndef CFM_TLV_H_
#define CFM_TLV_H_
#include "cfm_types.h"

#define  EGRESS_IDENTIFIER_LEN 8


typedef enum
{
	type_Sender_ID_TLV=0x01,
	type_Port_status_TLV=0x02,
	type_Data_TLV=0x03,
	type_Interface_Status_TLV=0x04,
	type_Reply_Ingress_TLV=0x05,
	type_Reply_Egress_TLV=0x06,
	type_LTM_Egress_Identifier_TLV=0x07,
	type_LTR_Egress_Identifier_TLV=0x08,
	type_Organization_Specific_TLV=0x1F
}TLV_type;

/*****Define Sender ID TLV*****/
typedef enum{
	None = 0x01,
	Chassis = 0x02,
	Manage = 0x03,
	ChassisManage = 0x04
}SenderIdPermission_valve;

typedef struct Sender_ID_TLV_st * Sender_ID_TLV_t;
struct Sender_ID_TLV_st
{
	uint8 type ;
	uint16 length ;
	uint8 chassis_ID_Length;
	uint8 chassis_ID_Subtype;
	uint8 chassis_ID[50];
	uint8 management_Address_Domain_Length;
	uint8 management_Address_Domain[50];
	uint8 management_Address_Length;
	uint8 management_Address[50];
};

/********Define Port Status TLV*******/
typedef enum {
	psBlocked = 0x01,
	psUp = 0x02
}PortStatus_Value;

typedef struct Port_Status_TLV_st * Port_Status_TLV_t;
struct Port_Status_TLV_st
{
	uint8 type;     
	uint16 length;
	PortStatus_Value port_status_value;
};

/***********Define Data TLV************/
typedef struct Data_TLV_st * Data_TLV_t;
struct Data_TLV_st
{
	uint8 type;
	uint16 length;
	uint8  data[256];
};

/********Define Interface Status TLV***********/
typedef enum {
	isUp=0x01,
	isDown=0x02,
	isTesting=0x03,
	isUnknown=0x04,
	isDormant=0x05,
	isNotPresent=0x06,
	isLowerLayerDown=0x07
}Interface_Status;

typedef struct Interface_Status_TLV_st * Interface_Status_TLV_t;
struct Interface_Status_TLV_st
{
	uint8  type; 
	uint16 length;
	Interface_Status interface_status;
};

/*************Define Reply Ingress TLV****************/
typedef enum
{
	IngOK=0x01,
	IngDown=0x02,
	IngBlocked=0x03,
	IngVID=0x04
}Ingress_Action;

typedef enum
{
	Interface_alias=0x01,
	Port_component=0x02,
	MAC_address=0x03,
	Network_address=0x04,
	Interface_name=0x05,
	Agent_circuit_ID=0x06,
	Locally_assigned=0x07
}Port_ID_Subtype;

typedef struct Reply_Ingress_TLV_st *Reply_Ingress_TLV_t;
struct Reply_Ingress_TLV_st
{
	uint8 type;
	uint16 length;
	Ingress_Action ingress_Action;
	uint8 ingress_MAC_Address[ADDR_LEN];
	uint8 ingress_Port_ID_Length;
	Port_ID_Subtype ingress_Port_ID_Subtype;
	uint8 * ingress_Port_ID;
};

/*************Define Reply Egress TLV********** ******/
typedef enum
{
	EgrOK=0x01,
	EgrDown=0x02,
	EgrBlocked=0x03,
	EgrVID=0x04
}Egress_Action;

typedef struct Reply_Egress_TLV_st * Reply_Egress_TLV_t;
struct Reply_Egress_TLV_st
{
	uint8 type;
	uint16 length;
	Egress_Action egress_Action;
	uint8 egress_MAC_Address[ADDR_LEN];
	uint8 egress_Port_ID_Length;
	Port_ID_Subtype egress_Port_ID_Subtype;
	uint8 * egress_Port_ID;
};

/*************Define LTM Egress Identifier TLV**************/
typedef struct LTM_Egress_Identifier_TLV_st*  LTM_Egress_Identifier_TLV_t;
struct LTM_Egress_Identifier_TLV_st
{
	uint8 type;
	uint16 length;
	uint8 egress_Identifier[EGRESS_IDENTIFIER_LEN];
};

/*************Define LTR Egress Identifier TLV**************/
typedef struct LTR_Egress_Identifier_TLV_st * LTR_Egress_Identifier_TLV_t;
struct LTR_Egress_Identifier_TLV_st
{
	uint8 type;
	uint16 length;
	uint8 last_Egress_Identifier[EGRESS_IDENTIFIER_LEN];
	uint8 next_Egress_Identifier[EGRESS_IDENTIFIER_LEN];
};

/****************Define Organization-Specific TLV*****************/
typedef struct Organization_Specific_TLV_st * Organization_Specific_TLV_t;
struct Organization_Specific_TLV_st
{
	uint8 type;
	uint16 length;
	uint8 OUI[3];
	uint8 sub_Type;
	uint8 value[128];
};
//define struct TLV_st
typedef struct TLV_st * TLV_t;
struct TLV_st{
	struct Sender_ID_TLV_st Sender_ID_TLV;
	struct Organization_Specific_TLV_st Organization_Specific_TLV;
};
#endif//CFM_TLV_H_
