//FlowID:VlanID
// 1:10
// 2:20
// ......
#ifndef CFM_TYPES_H_
#define CFM_TYPES_H_
#define CFM_EMULATE

#define ADDR_LEN 6
#define VLAN_TAG_LEN  4
#define ETHERNETHEAD_LEN 14
#define CFMHEADER_LEN 4
#define OPCODE_WITHOUT_VLAN  15

#define Outside 0
#define Inside 1

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long  uint32 ;

typedef enum
{
	UPMP=0x01,
	DOWNMP=0x02
}upDown_t;

typedef enum
{
	MIP=0x01,
	MEP=0x02
}MPType_t;

typedef enum{
        type_Reserved = 0x00,
        type_CCM = 0x01,
        type_LBR = 0x02,
        type_LBM = 0x03,
        type_LTR = 0x04,
        type_LTM = 0x05
}pkt_type_t;

typedef enum{
        LAN = 0x00,
        PON = 0x01
}BRIDGE_PORT_ID;

typedef enum{
        FLOW0 = 0x00,
        FLOW1 = 0x01,
        FLOW2 = 0x02,
        FLOW3 = 0x03
}FLOW_ID;

typedef enum{
        REPLY = 0X00,
        FORWARD = 0x01,
        SENDTO = 0x02
}PACKET_ACTION_ID;


#define uint16_to_uint8(dest ,src) {\
	*dest = (src>>8) & 0xFF;			\
	*(dest+1) = src & 0xFF;			\
}
#define uint32_to_uint8(dest, src) {\
	*dest = (src>>24) & 0xFF;		\
	*(dest+1) = (src>>16) & 0xFF;		\
	*(dest+2) = (src>>8) & 0xFF;		\
	*(dest+3) = src & 0xFF;			\
}
#define uint8_to_uint16(dest, src) {\
	dest = *src<<8;				\
	dest |= *(src+1);				\
}
#define uint8_to_uint32(dest, src) {\
	dest = *src<<24;				\
	dest |= *(src+1)<<16;				\
	dest |= *(src+2)<<8;				\
	dest |= *(src+3);				\
}
#endif//CFM_TYPES_H_
