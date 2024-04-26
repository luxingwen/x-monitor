/*
 * @Author: CALM.WU
 * @Date: 2024-04-26 14:05:57
 * @Last Modified by: CALM.WU
 * @Last Modified time: 2024-04-26 16:33:57
 */

#ifndef __CW_MISC_H
#define __CW_MISC_H

#ifdef __KERNEL__

#define PRINT_CTX()                                                                                                          \
	do {                                                                                                                 \
		int PRINTCTX_SHOWHDR = 1;                                                                                    \
		char intr = '.';                                                                                             \
		if (!in_task()) {                                                                                            \
			if (in_irq() && in_softirq())                                                                        \
				intr = 'H'; /* hardirq occurred inside a softirq */                                          \
			else if (in_irq())                                                                                   \
				intr = 'h'; /* hardirq is running */                                                         \
			else if (in_softirq())                                                                               \
				intr = 's';                                                                                  \
		} else                                                                                                       \
			intr = '.';                                                                                          \
                                                                                                                             \
		if (PRINTCTX_SHOWHDR == 1)                                                                                   \
			pr_debug(                                                                                            \
				"CPU)  task_name:PID  | irqs,need-resched,hard/softirq,preempt-depth  /* func_name() */\n"); \
		pr_debug("%03d) %c%s%c:%d   |  "                                                                             \
			 "%c%c%c%u   "                                                                                       \
			 "/* %s() */\n",                                                                                     \
			 raw_smp_processor_id(), (!current->mm ? '[' : ' '),                                                 \
			 current->comm, (!current->mm ? ']' : ' '),                                                          \
			 current->pid, (irqs_disabled() ? 'd' : '.'),                                                        \
			 (need_resched() ? 'N' : '.'), intr,                                                                 \
			 (preempt_count() && 0xff), __func__);                                                               \
	} while (0)

#endif

#endif // __CW_MISC_H
