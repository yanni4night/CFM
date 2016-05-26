#include "cfm_protocol.h"

int highestDefectPri_get(MEP_t mep)
{
	if(mep == NULL)
		{
			printk("wrong mep in xmitFaultAlarm()!!\n");
			return -1;
		}
	else if (mep->xconCCMdefect==true)
			return 5;
	else if(mep->errorCCMdefect==true)
			return 4;
	else if(mep->ccpm->someRMEPCCMdefect==true)
			return 3;
	else if (mep->ccpm->someMACstatusDefect==true)
			return 2;
	else if(mep->ccpm->someRDIdefect ==true)
			return 1;
	else return -1;
	
	
}

int xmitFaultAlarm(uint16 MEPid,uint8 MDlevel/*,uint8 MAid[]*/,int MEPprimaryVID, uint8 fngPriority/*,uint8 alarm_to_mac[]*/)
{
	//The format and method of transmission of the Fault Alarm is not specified in this standard.
	
	printk("**********************FNG alarm info start************************\n");
	printk("MEPID:%d , MDlevel:%d , MEPprimaryVID:%d , FNGpriotiy:%d\n",MEPid,MDlevel,MEPprimaryVID,fngPriority);
	printk("**********************FNG alarm info ends************************\n");
	return 0;
}

int fault_notification_generator(MEP_t mep)
{
	if(mep == NULL)
		{
			printk("wrong mep in fault_notification_generator() !!\n");
			return -1;
		}
	mep->ccpm->highestDefectPri= highestDefectPri_get(mep);
	mep->MAdefectIndication=(mep->errorCCMdefect || mep->xconCCMdefect || mep->ccpm->someRDIdefect|| mep->ccpm->someRMEPCCMdefect || mep->ccpm->someMACstatusDefect);

	if(mep->ccpm->defect_reported == true)
	{
			
			if(mep->MAdefectIndication == false)
			{
				mep->ccpm->FNGwhile = mep->ccpm->interval_count;
				mep->ccpm->fngResetTime_on=true;
				mep->ccpm->defect_reported = false;
				return 0;
			}
			if((mep->MAdefectIndication == true)&&(mep->ccpm->highestDefectPri > mep->ccpm->fngPriority))
				{
					
					mep->ccpm->fngPriority = mep->ccpm->highestDefectPri;
					xmitFaultAlarm(mep->MEPBasic.MEPId,mep->MEPBasic.ma->MDPointer->MDLevel,mep->MEPBasic.PrimaryVlan,mep->ccpm->fngPriority);
					return 0;
				}
	}


	
else if(mep->ccpm->fngResetTime_on== true)//now no defect wait to disappear,annouce a new defect 
	{														//mep->ccpm->fngResetTime/mep->ccpm->CCIwhile
		if((mep->ccpm->interval_count - mep->ccpm->FNGwhile >=(mep->MEPBasic.AlarmClearSoakTime/mep->ccpm->CCIwhile))&&(mep->MAdefectIndication==false))
			{
				mep->ccpm->fngResetTime_on=false;
				return 0;		
			}
		else if(mep->MAdefectIndication == true)
			{
				mep->ccpm->fngResetTime_on = false;
				mep->ccpm->fngAlarmTime_on = true;
				mep->ccpm->FNGwhile = mep->ccpm->interval_count;
				return 0;
			}
	}
	else if(mep->ccpm->fngAlarmTime_on == true)
	{													//mep->ccpm->fngAlarmTime
		if((mep->ccpm->interval_count - mep->ccpm->FNGwhile >= (mep->MEPBasic.AlarmDeclarationSoakTime/mep->ccpm->CCIwhile))&&(mep->MAdefectIndication == true))
		{
			mep->ccpm->fngAlarmTime_on = false;
			//mep->ccpm->FNGwhile = mep->ccpm->interval_count;
			mep->ccpm->fngResetTime_on = false;
			mep->ccpm->defect_reported = true;
			return xmitFaultAlarm(mep->MEPBasic.MEPId,mep->MEPBasic.ma->MDPointer->MDLevel,mep->MEPBasic.PrimaryVlan ,mep->ccpm->fngPriority);
		}
		else if(mep->MAdefectIndication == false)
			{
				mep->ccpm->fngAlarmTime_on = false;
				mep->ccpm->fngResetTime_on = false;
				mep->ccpm->defect_reported = false;
			}
	}
	else if(mep->MAdefectIndication == true)//with a defect on ,start a timer to wait it to stabilize
		{
			mep->ccpm->fngAlarmTime_on=true;
			mep->ccpm->fngResetTime_on = false;
			mep->ccpm->defect_reported = false;
			mep->ccpm->FNGwhile = mep->ccpm->interval_count;
		}

	else if(mep->MAdefectIndication == false)
		{
				mep->ccpm->fngAlarmTime_on=false;
				mep->ccpm->fngResetTime_on=false;
				mep->ccpm->defect_reported = false;
		}
	return 0;
	
}




