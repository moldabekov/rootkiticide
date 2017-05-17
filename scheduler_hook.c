/**
 * @file scheduler_hook.c
 * @author Mikhail Klementyev <jollheef@riseup.net>
 * @date May 2017
 * @brief scheduler hooks for revealing hidden process
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/perf_event.h>
#include <linux/delay.h>

#include "rootkiticide.h"

static struct perf_event * __percpu *try_to_wake_up_hbp;

static atomic_t try_to_wake_up_handler_usage = ATOMIC_INIT(0);
static void try_to_wake_up_handler(struct perf_event *bp,
				   struct perf_sample_data *data,
				   struct pt_regs *regs)
{
	atomic_inc(&try_to_wake_up_handler_usage);
	ulong err = log_process();
	WARN_ON(err);
	atomic_dec(&try_to_wake_up_handler_usage);
}

int scheduler_hook_init(void)
{
	/* Set hardware breakpoint on try_to_wake_up */
	try_to_wake_up_hbp = hbp_on_exec("try_to_wake_up",
					 try_to_wake_up_handler);
	if (IS_ERR(try_to_wake_up_hbp))
		return PTR_ERR(try_to_wake_up_hbp);

	return 0;
}

void scheduler_hook_cleanup(void)
{
	while (atomic_read(&try_to_wake_up_handler_usage))
		msleep_interruptible(100);

	hbp_clear(try_to_wake_up_hbp);
}
