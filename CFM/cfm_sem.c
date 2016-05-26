//#include <asm/semaphore.h>
#include <linux/fs.h> /*kmalloc*/
//#include <linux/mutex.h> /*larger than 2.6.16*/
#include "cfm_header.h"

/*static initial semaphore
DECLARE_MUTEX(name)
or 
DECLARE_MUTEX_LOCKED(name)
*/

cfm_lock_t cfm_sem_lock_create()
{
	cfm_lock_t lock;
	lock = kmalloc(sizeof(cfm_lock_st), GFP_KERNEL);
	if (lock == NULL){
		printk("fail to create lock\n");
		return NULL;
	}	
	sema_init(lock,1);
	
	return lock;
}
int cfm_sem_lock(cfm_lock_t lock)
{
	if (lock == NULL){
		return -1;
	}
	down(lock);
	return 0;
}
int cfm_sem_unlock(cfm_lock_t lock)
{
	if (lock == NULL){
		return -1;
	}
	up(lock);
	return 0;
}
int cfm_sem_lock_destroy(cfm_lock_t lock)
{
	if (lock == NULL){
		return -1;
	}
	kfree(lock);
	lock = NULL;
	return 0;
}
/*jone:20100512 for mutex*/
int cfm_mutex_init (cfm_mutex_t mut)
{
	if (mut == NULL){
		return -1;
	}
	mutex_init(mut);
	return  0;
}
int cfm_mutex_lock (cfm_mutex_t mut)
{
	if (mut == NULL){
		return -1;
	}
	mutex_lock (mut);
	return  0;
}
int cfm_mutex_trylock (cfm_mutex_t mut)
{
	if (mut== NULL){
		return -1;
	}
   return mutex_trylock (mut);
}
int	cfm_mutex_unlock (cfm_mutex_t mut)
{
	if (mut== NULL){
		return -1;
	}
	mutex_unlock (mut);
	return  0;
}


/*jone:20100517 for spinlock*/
int cfm_mutex_init_nosleep (cfm_spinlock_t mut)
{
	if (mut == NULL){
		return -1;
	}
	spin_lock_init(mut);
	return  0;
}
int cfm_mutex_lock_nosleep (cfm_spinlock_t mut)
{
	if (mut == NULL){
		return -1;
	}
	spin_lock (mut);
	return  0;
}

int	cfm_mutex_unlock_nosleep (cfm_spinlock_t mut)
{
	if (mut== NULL){
		return -1;
	}
	spin_unlock (mut);
	return  0;
}

