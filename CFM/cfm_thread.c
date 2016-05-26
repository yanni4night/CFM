#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <asm/system.h>
#include <asm/processor.h>
#include <asm/signal.h>
#include <linux/completion.h>       // for DECLARE_COMPLETION()
#include <linux/sched.h>            // for daemonize() and set_current_state() 
#include <linux/delay.h>            // mdelay()

int cfm_create_thread(int(*fun)(void*),void*arg,int flags)
{
    /*int kernel_thread(int (*fn)(void *), void *arg, unsigned long flags)*/
    int handle;
    handle = kernel_thread(fun, arg,flags);
    return handle;
}
void cfm_destroy_thread(int handle)
{
	if (handle > 0){
    	kill_proc(handle, SIGTERM, 1);
	}
}
