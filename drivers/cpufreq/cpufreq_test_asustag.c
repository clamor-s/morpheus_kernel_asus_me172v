/*
 *  linux/drivers/cpufreq/cpufreq_performance.c
 *
 *  Copyright (C) 2002 - 2003 Dominik Brodowski <linux@brodo.de>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cpufreq.h>
#include <linux/init.h>


struct cpu_test_info_s {
        int cpu;
        int cpufreq_index;
        int sampling_rate;
        struct cpufreq_policy *policy;
        struct cpufreq_frequency_table *p_table;
	struct delayed_work work;
	struct mutex timer_mutex;
};

DEFINE_PER_CPU(struct cpu_test_info_s, cpufreq_test_info);

static void do_cpufreq_timer(struct work_struct *work)
{
	struct cpu_test_info_s *this_cpu_test_info =
		container_of(work, struct cpu_test_info_s, work.work);
	unsigned int cpu = this_cpu_test_info->cpu;
	struct cpufreq_frequency_table *table=NULL;
	int delay = usecs_to_jiffies(this_cpu_test_info->sampling_rate);
        struct cpufreq_policy *policy = this_cpu_test_info->policy;
        int i=0;
        unsigned int freq;

	mutex_lock(&this_cpu_test_info->timer_mutex);

        table = cpufreq_frequency_get_table(policy->cpu);


        for (i=this_cpu_test_info->cpufreq_index+1; i != this_cpu_test_info->cpufreq_index; i++) {
                freq = table[i].frequency;

                if (freq == CPUFREQ_TABLE_END) {
                        i=-1; //later will be zero.
                        continue;
                }
                if (freq == CPUFREQ_ENTRY_INVALID)
                        continue;

                if ((freq < policy->min) || (freq > policy->max))
                        continue;

                break;
        }
        if (i == this_cpu_test_info->cpufreq_index)
                pr_info("WARNING: Can't find next transision step. Only one table entry for dvfs ??\n");

        this_cpu_test_info->cpufreq_index = i;

        pr_debug("[%s] Change to %dkHz\n", __func__, table[i].frequency);
        __cpufreq_driver_target(policy, table[i].frequency, CPUFREQ_RELATION_H);

	schedule_delayed_work_on(cpu, &this_cpu_test_info->work, delay);

	mutex_unlock(&this_cpu_test_info->timer_mutex);
}

static inline void cpufreq_timer_init(struct cpu_test_info_s *cpu_test_info)
{
	/* We want all CPUs to do sampling nearly on same jiffy */
	int delay = usecs_to_jiffies(cpu_test_info->sampling_rate);

	INIT_DELAYED_WORK_DEFERRABLE(&cpu_test_info->work, do_cpufreq_timer);
	schedule_delayed_work_on(cpu_test_info->cpu, &cpu_test_info->work, delay);
}

static inline void cpufreq_timer_exit(struct cpu_test_info_s *cpu_test_info)
{
	cancel_delayed_work_sync(&cpu_test_info->work);
}

static int cpufreq_governor_test(struct cpufreq_policy *policy,
					unsigned int event)
{
	struct cpufreq_frequency_table *table=NULL;
        struct cpu_test_info_s *this_cpu_test_info = NULL;
        int i=0;
        int ret=0;
        unsigned int latency=0;

        if (!policy) 
                return -EINVAL;

        pr_debug("[%s]: event %d\n", __func__, event);

        this_cpu_test_info = &per_cpu(cpufreq_test_info, policy->cpu);

	switch (event) {
	case CPUFREQ_GOV_START:
                //get the current frequency index
                table = cpufreq_frequency_get_table(policy->cpu);
                if (!table)
                        return -EINVAL;

                /* policy latency is in nS. Convert it to uS first */
                latency = policy->cpuinfo.transition_latency / 1000;
                if (latency == 0)
                        latency = 1;

                this_cpu_test_info->cpufreq_index = 0;
                this_cpu_test_info->sampling_rate = latency;
                this_cpu_test_info->cpu = policy->cpu;
                this_cpu_test_info->policy = policy;
		mutex_init(&this_cpu_test_info->timer_mutex);

                pr_debug("[%s] cpu: %d, latency: %d, idx: %d\n", 
                        __func__, policy->cpu, latency, this_cpu_test_info->cpufreq_index
                );

		cpufreq_timer_init(this_cpu_test_info);
                
                break;

        case CPUFREQ_GOV_STOP:
		cpufreq_timer_exit(this_cpu_test_info);
		mutex_destroy(&this_cpu_test_info->timer_mutex);

                break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_cpu_test_info->timer_mutex);

		if (policy->max < policy->cpuinfo.max_freq) {
                        pr_debug("[%s] Change to %dkHz\n", __func__, table[i].frequency);
			__cpufreq_driver_target(this_cpu_test_info->policy,
				this_cpu_test_info->policy->cpuinfo.max_freq, CPUFREQ_RELATION_H);
                }

		mutex_unlock(&this_cpu_test_info->timer_mutex);
		break;
	default:
		break;
	}
	return ret;
}

#ifdef CONFIG_CPU_FREQ_GOV_PERFORMANCE_MODULE
static
#endif
struct cpufreq_governor cpufreq_gov_test= {
	.name		= "asus_test",
	.governor	= cpufreq_governor_test,
	.owner		= THIS_MODULE,
};


static int __init cpufreq_gov_test_init(void)
{
	return cpufreq_register_governor(&cpufreq_gov_test);
}


static void __exit cpufreq_gov_test_exit(void)
{
	cpufreq_unregister_governor(&cpufreq_gov_test);
}


MODULE_AUTHOR("Wade Chen, Wade1_Chen@asus.com");
MODULE_DESCRIPTION("CPUfreq policy governor 'asus_test' for ASUS testing");
MODULE_LICENSE("GPL");

fs_initcall(cpufreq_gov_test_init);
module_exit(cpufreq_gov_test_exit);
