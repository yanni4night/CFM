#ifndef CFM_HEADER_H_
#define CFM_HEADER_H_

#include <asm/semaphore.h>
#include <linux/timer.h>
#include <linux/if_ether.h>
#include <linux/mutex.h> /*larger than 2.6.16*/
#include <linux/time.h>
#include <linux/spinlock.h>
#include "cfm_types.h"
/*
info = kmalloc(sizeof(struct async_struct), GFP_KERNEL);
kfree(info);
*/
typedef int  cfm_thread_t;

typedef struct semaphore   cfm_lock_st;
typedef cfm_lock_st * cfm_lock_t;

typedef struct mutex cfm_mutex_st;
typedef cfm_mutex_st * cfm_mutex_t;

typedef spinlock_t cfm_spinlock_st; /*spinlock_t my_lock = SPIN_LOCK_UNLOCKED;*/
typedef cfm_spinlock_st * cfm_spinlock_t;

typedef struct timer_list  cfm_timer_st;
typedef cfm_timer_st * cfm_timer_t;

/*queue structs and functions*/
#define QUEUE_MAXSIZE     20
#define QUEUE_NODE_MAXLEN       1600
typedef struct queue_node_st * queue_node_t;
struct queue_node_st{
	uint8 data[QUEUE_NODE_MAXLEN];
	int len;
	uint16 srcPortId;
	uint32 srcFlowId;
	int flag;
	cfm_spinlock_st lock;
	struct queue_node_st * next;
	struct queue_node_st * prev;
};

typedef struct queue_st * queue_t;
struct queue_st{
        queue_node_t head;         //update by queue_pull
        queue_node_t last;          //update by queue_push
        int size;                            //the number of nodes in the queue
        //int count;                         //the number of nodes in use
        cfm_spinlock_st lock;
};

/*queue*/
queue_t queue_create(int size);
int queue_destroy(queue_t queue);
int queue_push(queue_t queue, unsigned char * buf, int len, uint16 srcPortId, uint32 srcFlowId);
int queue_pull(queue_t queue, unsigned char *buf, int buflen, uint16 * srcPortId, uint32 *srcFlowId);
int queue_get_size(queue_t queue);
int queue_get_count(queue_t queue);
int queue_is_empty(queue_t queue);
int queue_is_full(queue_t queue);

/*demo function*/
extern int cfm_core_init(void);
extern int cfm_core_destroy(void);
/*thread*/
extern int cfm_create_thread(int(*fun)(void*),void*arg,int flags);
extern void cfm_destroy_thread(int handle);
/*lock*/
extern cfm_lock_t cfm_sem_lock_create(void);
extern int cfm_sem_lock(cfm_lock_t lock);
extern int cfm_sem_unlock(cfm_lock_t lock);
extern int cfm_sem_lock_destroy(cfm_lock_t lock);

extern int cfm_mutex_init (cfm_mutex_t mut);
extern int cfm_mutex_lock (cfm_mutex_t mut);
extern int cfm_mutex_trylock (cfm_mutex_t mut);
extern int cfm_mutex_unlock (cfm_mutex_t mut);

extern int cfm_mutex_init_nosleep (cfm_spinlock_t mut);
extern int cfm_mutex_lock_nosleep (cfm_spinlock_t mut);
extern int cfm_mutex_unlock_nosleep (cfm_spinlock_t mut);
/*timer*/
extern cfm_timer_t cfm_timer_create(void (*fun)(unsigned long),int mdelay,long data);
extern int cfm_timer_continue(cfm_timer_t timer,int mdelay);
extern int cfm_timer_destroy(cfm_timer_t timer);
/*time*/
struct timeval cfm_tv_now(void);
int cfm_tv_cmp(struct timeval a, struct timeval b);
int cfm_tv_eq(struct timeval a, struct timeval b);
int cfm_tvdiff_ms(struct timeval end, struct timeval start);
/*common*/
void print_hex_data(char* data,int length);
/*packet API*/
int cfmPacket_xmit(uint8 *buf, uint32 len, uint16 srcPortId, uint32 srcFlowId, uint32 action_id);
int cfmPacket_receive (uint8* pkt_data, uint32 pkt_len, uint16 srcPortId, uint32 srcFlowId);
int cfm_send(uint8* pkt_data,int pkt_len,uint16 srcPortId,uint32 srcFlowId,void* tmp, int MPFlag, uint8 srcDirection, uint8 Action);
#ifdef CFM_EMULATE
/*filter database*/
int FilteringDatabase_init(void* cfm);
int FilteringDatabase_destroy(void* cfm);
int FilteringDatabase_AddEntry(void* cfm, uint16 VlanID, uint8 *DestMAC, uint16 PortID);
int FilteringDatabase_query(uint8 *DestMAC, int VlanID,uint16* PortID);
#endif//CFM_EMULATE
#endif
