#include "cfm_header.h"
#include "cfm_protocol.h"

#ifdef CFM_EMULATE
uint8 MAC_Addr[6][6] = {
					{0x00,0x0C,0x29,0x9F,0x98,0xD7}, //Be changed depend
					{0x00,0x0c,0x29,0x94,0x76,0xA2}, 
					{0x00,0x0c,0x29,0x94,0x76,0xAC}, 
					{0x00,0x0c,0x29,0x85,0x2B,0xF6}, 
					{0x00,0x0c,0x29,0x85,0x2B,0xEC}, 
					{0x00,0x0C,0x29,0xED,0xAF,0xDD}
};
int FilteringDatabase_init(void* tcfm)
{
	cfm_t cfm = (cfm_t)tcfm;
	if(cfm == NULL){
		return -1;
	}
	LIST_HEAD_INIT(&cfm->FilteringDatabase_list);
#if 1//Need to update depend on your network
	FilteringDatabase_AddEntry(cfm, 20, MAC_Addr[2], 1);//confirm the hostmac of port 1
	FilteringDatabase_AddEntry(cfm, 20, MAC_Addr[1], 2);//confirm the hostmac of port 2
	
	FilteringDatabase_AddEntry(cfm, 20, MAC_Addr[0], 1);
	FilteringDatabase_AddEntry(cfm, 20, MAC_Addr[3], 2);
	FilteringDatabase_AddEntry(cfm, 20, MAC_Addr[4], 2);
	FilteringDatabase_AddEntry(cfm, 20, MAC_Addr[5], 2);
#endif
	

	return 0;
}

int FilteringDatabase_destroy(void* tcfm)
{
	FilteringDatabase_t temp, pre;
	FilteringDatabaseVlan_t tvlan, pvlan;
	FilteringDatabaseMAc_t tmac, pmac;
	cfm_t cfm = (cfm_t)tcfm;

	if(cfm == NULL){
		return -1;
	}
	temp = LIST_FIRST(&cfm->FilteringDatabase_list);
	while(NULL != temp){
		pre = temp;
		temp = LIST_NEXT(temp, list);
		tvlan = LIST_FIRST(&pre->FilteringDatabaseVlan_list);
		while(NULL != tvlan){
			pvlan = tvlan;
			tvlan = LIST_NEXT(tvlan, list);
			tmac = LIST_FIRST(&pvlan->FilteringDatabaseMac_list);
			while(NULL != tmac){
				pmac = tmac;
				tmac = LIST_NEXT(tmac, list);
				kfree(pmac);
				pmac = NULL;
			}
			LIST_HEAD_DESTROY(&pvlan->FilteringDatabaseMac_list);
			kfree(pvlan);
			pvlan = NULL;
		}
		LIST_HEAD_DESTROY(&pre->FilteringDatabaseVlan_list);
		kfree(pre);
		pre = NULL;	
	}
	LIST_HEAD_DESTROY(&cfm->FilteringDatabase_list);	

	return 0;
}

int FilteringDatabase_AddEntry(void* tcfm, uint16 VlanID, uint8 *DestMAC, uint16 PortID)
{
	FilteringDatabase_t entry;
	FilteringDatabaseVlan_t ventry;
	FilteringDatabaseMAc_t mentry;
	
	cfm_t cfm = (cfm_t)tcfm;
	if(cfm == NULL){
		printk("cfm is NULL\n");
		return -1;
	}

	LIST_TRAVERSE(&cfm->FilteringDatabase_list,entry,list){
		if(entry->PortID == PortID){
			break;
		}
	}

	if(entry == NULL){
		entry = (FilteringDatabase_t)kmalloc(sizeof(struct FilteringDatabase_st), GFP_KERNEL);
		if (entry == NULL)
		{
			printk("kmalloc failed in FilteringDatabase_AddEntry()\n");
			return -1;
		}
		memset(entry, 0, sizeof(struct FilteringDatabase_st));
		entry->PortID = PortID;
		memcpy(entry->MacAddr,DestMAC,ADDR_LEN);
		LIST_HEAD_INIT(&entry->FilteringDatabaseVlan_list);
		LIST_INSERT_TAIL(&cfm->FilteringDatabase_list, entry, list);
	}

	LIST_TRAVERSE(&entry->FilteringDatabaseVlan_list, ventry, list){
		if(ventry->VlanId == VlanID){
			break;
		}
	}
	if(ventry == NULL){
		ventry = (FilteringDatabaseVlan_t)kmalloc(sizeof(struct FilteringDatabaseVlan_st), GFP_KERNEL);
		if(ventry == NULL){
			return -1;
		}
		memset(ventry, 0, sizeof(struct FilteringDatabaseVlan_st));
		ventry->VlanId = VlanID;
		LIST_HEAD_INIT(&ventry->FilteringDatabaseMac_list);
		LIST_INSERT_TAIL(&entry->FilteringDatabaseVlan_list, ventry, list);
	}

	LIST_TRAVERSE(&ventry->FilteringDatabaseMac_list, mentry, list){
		if(memcmp(mentry->DestMAC, DestMAC, ADDR_LEN) == 0){
			break;
		}
	}
	if(mentry == NULL){
		mentry = (FilteringDatabaseMAc_t)kmalloc(sizeof(struct FilteringDatabaseMAc_st), GFP_KERNEL);
		if(mentry == NULL){
			return -1;
		}
		memset(mentry, 0, sizeof(struct FilteringDatabaseMAc_st));
		memcpy(mentry->DestMAC, DestMAC, ADDR_LEN);
		LIST_INSERT_TAIL(&ventry->FilteringDatabaseMac_list, mentry, list);
	}

	return 0;
}

int FilteringDatabase_query(uint8 *DestMAC, int VlanID,uint16* PortID)
{
	FilteringDatabase_t entry;
	FilteringDatabaseVlan_t ventry;
	FilteringDatabaseMAc_t mentry;
	
	if(gCfm == NULL){
		return -1;
	}
	LIST_TRAVERSE(&gCfm->FilteringDatabase_list,entry,list)
	{
		LIST_TRAVERSE(&entry->FilteringDatabaseVlan_list, ventry, list){
			LIST_TRAVERSE(&ventry->FilteringDatabaseMac_list, mentry, list){
				if ((memcmp(DestMAC, mentry->DestMAC, ADDR_LEN) == 0) && (ventry->VlanId== VlanID))
				{
					*PortID = entry->PortID;
					return 0;
				}
			}
		}

	}
	
	return -1;	
}
#endif//CFM_EMULATE

