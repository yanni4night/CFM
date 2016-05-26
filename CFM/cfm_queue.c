/******************************************************************************
**                                                                          **
** Copyright (c) 2007 Browan Communications Inc.                            **
** All rights reserved.                                                     **
**                                                                          **
******************************************************************************

Version History:
----------------

Version     : 1.0
Date        : MAY 4, 2010
Revised By  : hbz
Description : Original Version
version     : 1.1
Date        : MAY 17, 2010
Revised By  : jone.li
Description : use spinlock to avoid sleep action in context of timer or irq
******************************************************************************
***/

#include <linux/fs.h>
#include "cfm_header.h"

queue_t queue_create(int size)
{
	queue_t	queue;
	int i;
	queue_node_t temp;
	queue = (queue_t)kmalloc(sizeof(struct queue_st), GFP_KERNEL); 
	if(queue == NULL){
		printk("error kmalloc queue\n");
		return NULL;
	}
	memset(queue, 0, sizeof(struct queue_st));
	cfm_mutex_init_nosleep(&queue->lock);
	for(i=0;i<size;i++){
		
		temp = (queue_node_t)kmalloc(sizeof(struct queue_node_st), GFP_KERNEL);
		if(temp == NULL){
			printk("error kmalloc node\n");
			goto destroy;
		}
		memset(temp, 0, sizeof(struct queue_node_st));
		queue->size++;
		
		cfm_mutex_init_nosleep(&temp->lock);
		
		if(i ==0 ){
			queue->head = temp;
			queue->last = temp;
		}
		else{
			queue->last->next = temp;
			temp->prev = queue->last;
			queue->last = temp;
		}
		temp = NULL;		
	}

	queue->last->next = queue->head;
	queue->head->prev = queue->last;
	
	if(queue&&(queue->size == size)){
		return queue;
	}
	else{
		goto destroy;
	}
	destroy:
	if(queue){
		if(queue->size){
			for(i=0;i<queue->size;i++){
				temp = queue->head;
				queue->head = queue->head->next;
				
				kfree(temp);
				temp = NULL;
			}
		}
		kfree(queue);
		queue = NULL;
	}
	return NULL;
}

int queue_destroy(queue_t queue)
{
	queue_node_t temp;
	
	if(queue == NULL){
		printk("queue is NULL\n");
		return -1;
	}

	cfm_mutex_lock_nosleep (&queue->lock);
	
	while(queue->size){
		temp = queue->head;
		queue->head = queue->head->next;
		kfree(temp);
		temp = NULL;
		queue->size--;
	}
	
	cfm_mutex_unlock_nosleep(&queue->lock);

	kfree(queue);
	queue = NULL;
	return 0;
}

int queue_push(queue_t queue, unsigned char * buf, int len, uint16 srcPortId, uint32 srcFlowId)
{
	queue_node_t temp;

	if((queue == NULL)||(queue->size == 0)){
		printk("queue is NULL, no queue can be used\n");
		return -1;
	}
	
	if(queue_is_full(queue)){
		printk("queue is full\n");
		return -1;
	}
	
	if((buf == NULL)||(len == 0)){
		printk("no data can be push\n");
		return 0;
	}

	temp = queue->last->next;
	
	cfm_mutex_lock_nosleep(&temp->lock);
	memcpy(temp->data, buf, sizeof(temp->data));
	temp->len = len;
	temp->flag = 1;
	temp->srcPortId = srcPortId;
	temp->srcFlowId = srcFlowId;
	cfm_mutex_unlock_nosleep(&temp->lock);

	cfm_mutex_lock_nosleep(&queue->lock);
	queue->last = temp;
	
	cfm_mutex_unlock_nosleep(&queue->lock);
	return 0;
}

int queue_pull(queue_t queue, unsigned char * buf, int buflen, uint16 * srcPortId, uint32 *srcFlowId)
{
	int len;
	queue_node_t temp;
	int ret;

	if((queue == NULL)||(queue->size == 0)){
		printk("no queue available\n");
		return -1;
	}

	ret = queue_is_empty(queue);
	if(ret == -1){
		printk("no queue available\n");
		return -1;
	}
	else if(ret == 1){
		return 0;
	}

	temp = queue->head;
	
	cfm_mutex_lock_nosleep(&temp->lock);
	memcpy(buf, temp->data, buflen);
	len = temp->len;
	temp->flag = 0;
	if(srcPortId != NULL)
		*srcPortId = temp->srcPortId;
	if(srcFlowId != NULL)
		*srcFlowId = temp->srcFlowId;
	cfm_mutex_unlock_nosleep(&temp->lock);

	/*debug data*/
/*
printk("in queue_pull\n");
print_hex_data(buf,len);	
*/
	cfm_mutex_lock_nosleep(&queue->lock);
	queue->head = queue->head->next;

	cfm_mutex_unlock_nosleep(&queue->lock);

	return len;	
}

int queue_get_size(queue_t queue)
{
	if(queue == NULL){
		printk("queue is NULL\n");
		return -1;
	}
	
	return queue->size;
}
int queue_get_count(queue_t queue)
{
	int count = 0;
	int i;
	queue_node_t temp;

	if(queue == NULL){
		printk("queue is NULL\n");
		return -1;
	}

	temp = queue->head;
	for(i=0;i<queue->size;i++)
	{
		if(temp->flag == 1){
			count++;
		}
		temp = temp->next;
	}
	return count;
}

int queue_is_empty(queue_t queue)
{
	int count;
	if(queue == NULL){
		printk("queue is NULL\n");
		return -1;
	}
	count = queue_get_count(queue);
	if(count){
		return 0;
	}
	else{
		return 1;
	}
}

int queue_is_full(queue_t queue)
{
	int count;
	if(queue == NULL){
		printk("queue is NULL\n");
		return -1;
	}

	count = queue_get_count(queue);
	if(count == queue->size){
		return 1;
	}
	else{
		return 0;
	}
}
