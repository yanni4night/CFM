/******************************************************************************
**                                                                          **
** Copyright (c) 2007 Browan Communications Inc.                            **
** All rights reserved.                                                     **
**                                                                          **
******************************************************************************

Version History:
----------------

Version     : 1.0
Date        : April 20, 2010
Revised By  : jone.li
Description : Original Version
version     : 1.1
Date        : MAY 14, 2010
Revised By  : jone.li
Description : change the timer precision to 1-10ms

******************************************************************************
***/

#include <linux/fs.h> /*kmalloc*/
#include "cfm_header.h"
static unsigned int calculate_min_precision(void);

static unsigned int calculate_min_precision()
{
	unsigned int ms;
	ms = 1000/HZ;
	return ms;
}
/*
	mdelay > 10ms  (better)
*/
cfm_timer_t cfm_timer_create(void (*fun)(unsigned long),int mdelay,long data)
{
	cfm_timer_t timer;
	unsigned int delta;
	unsigned int precision;
	if (fun == NULL){
		return NULL;
	}
	timer = kmalloc(sizeof(cfm_timer_st), GFP_KERNEL);
	if (timer == NULL){
		printk("fail to create timer\n");
		return NULL;
	}
	/*jone 20100514: caculate the clock tick number*/
	precision = calculate_min_precision();
	if (precision == 0){
		precision = 1; /*compensate to 1ms*/
	}
	delta = mdelay/precision;
	if (delta == 0){ /*compensate to 1 tick*/
		delta = 1;
	}

	printk("timer delta :%d tick precision: %d ms\n",delta,precision);
	init_timer(timer); 
	timer->function = fun; 
	timer->data = data;
	timer->expires = jiffies+delta;
	
	add_timer(timer); 
	return timer;
}
int cfm_timer_continue(cfm_timer_t timer,int mdelay)
{
	unsigned int delta;
	unsigned int precision;
	if (timer == NULL){
		printk("timer is null\n");
		return -1;
	}
	/*jone 20100514: caculate the clock tick number*/
	precision = calculate_min_precision();
	if (precision == 0){
		precision = 1; /*compensate to 1ms*/
	}
	delta = mdelay/precision;
	if (delta == 0){ /*compensate to 1 tick*/
		delta = 1;
	}
	mod_timer(timer,jiffies+delta);
	return 0;
}
int cfm_timer_destroy(cfm_timer_t timer)
{
	if (timer == NULL){
		printk("timer is null\n");
		return -1;
	}
	del_timer(timer);
	kfree(timer);
	timer = NULL;
	return 0;
}
