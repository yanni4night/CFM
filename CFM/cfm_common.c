
/******************************************************************************
**                                                                          **
** Copyright (c) 2007 Browan Communications Inc.                            **
** All rights reserved.                                                     **
**                                                                          **
******************************************************************************

Version History:
----------------

Version     : 1.0
Date        : June 10, 2009
Revised By  : Jone.Li
Description : Define some common functions
******************************************************************************
***/
#include <linux/kernel.h>
#include <linux/fs.h>
#include "cfm_protocol.h"
#include "cfm_header.h"

void print_hex_data(char* data,int length)
{
	int num =0;
	while(num < length){
		printk("%2.2x ",(unsigned char)data[num]);
		if (((num+1) %16) == 0){
			printk("\n");
		}
		num ++;
	}
	printk("\n");	
}

CFMHeader_t generateCFMHeader(int  mdLevel, pkt_type_t pduType, uint8 flags)
{
	CFMHeader_t header;

	header = (CFMHeader_t)alloc(sizeof(struct CFMHeader_st));
	if(header == NULL){
		printk("fail to kmalloc cfm header\n");
		return NULL;
	}
	memset(header, 0, sizeof(struct CFMHeader_st));
	header->mdLevel_version = (mdLevel<<5)&0xFF;
	switch(pduType)
	{
        case type_CCM :
		header->opCode=0x01 ;
		header->firstTLVOffset=70;
		break ;
	case type_LBR :
		header->opCode=0x02 ;
		header->firstTLVOffset=4 ;
		break ;
	case type_LBM :
		header->opCode=0x03 ;
		header->firstTLVOffset=4 ;
		break ;
	case type_LTR :
		header->opCode=0x04 ;
		header->firstTLVOffset=6 ;
		break ;
	case type_LTM :
		header->opCode=0x05 ;
		header->firstTLVOffset=17 ;
		break ;
	default:
		break;
	}
	header->flags = flags;
    
	return header ;
}



