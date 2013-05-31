/* Copyright (c) 2012, Will Tisdale <willtisdale@gmail.com>. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

/*
 * Generic auto hotplug driver for ARM SoCs. Targeted at current generation
 * SoCs with dual and quad core applications processors.
 * Automatically hotplugs online and offline CPUs based on system load.
 * It is also capable of immediately onlining a core based on an external
 * event by calling void hotplug_boostpulse(void)
 *
 * Not recommended for use with OMAP4460 due to the potential for lockups
 * whilst hotplugging.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>

#include <mach/cpufreq.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

/*
 * Enable debug output to dump the average
 * calculations and ring buffer array values
 * WARNING: Enabling this causes a ton of overhead
 *
 * FIXME: Turn it into debugfs stats (somehow)
 * because currently it is a sack of shit.
 */
#define DEBUG 0

#define CPUS_AVAILABLE		num_possible_cpus()
/*
 * SAMPLING_PERIODS * MIN_SAMPLING_RATE is the minimum
 * load history which will be averaged
 */
#define ONLINE_SAMPLING_PERIODS		1
#define OFFLINE_SAMPLING_PERIODS	3

/*
 * Load defines:
 *
 * ENABLE is the load which is required to enable CPU1
 * DISABLE is the load at which CPU1 is disabled
 */
#define ENABLE_LOAD_THRESHOLD		350
#define DISABLE_LOAD_THRESHOLD		150
#define DEFAULT_SUSPEND_FREQ 		702000
#define SAMPLING_RATE				130

static struct delayed_work hotplug_decision_work;

static struct workqueue_struct *wq;

static unsigned int suspend_frequency = DEFAULT_SUSPEND_FREQ;
static unsigned int sampling_rate = SAMPLING_RATE;
static unsigned int online_sample = 0;
static unsigned int offline_sample = 0;
static unsigned int online_sampling_periods = ONLINE_SAMPLING_PERIODS;
static unsigned int offline_sampling_periods = OFFLINE_SAMPLING_PERIODS;

static void __cpuinit hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned int disable_load, enable_load, iowait_avg, avg_running = 0;
	unsigned int online_cpus, available_cpus;
#if DEBUG
	unsigned int k;
#endif

	online_cpus = num_online_cpus();
	available_cpus = CPUS_AVAILABLE;
	disable_load = DISABLE_LOAD_THRESHOLD;
	enable_load = ENABLE_LOAD_THRESHOLD;	
	
	sched_get_nr_running_avg(&avg_running, &iowait_avg);

#if DEBUG
	pr_info("average_running is: %d\n", avg_running);
#endif

	if ((avg_running >= enable_load) && (online_cpus < available_cpus)) {
#if DEBUG
	pr_info("auto_hotplug: online_sample %d\n", online_sample);
#endif
		if (online_sample >= online_sampling_periods) {
			pr_info("auto_hotplug: Onlining CPU1, avg running: %d\n", avg_running);
			cpu_up(1);
			online_sample--;
			offline_sample = 0;
		}
		online_sample++;
	} else if (avg_running <= disable_load && (online_cpus == available_cpus)) {
#if DEBUG
	pr_info("auto_hotplug: offline_sample %d\n", offline_sample);
#endif
		if (offline_sample >= offline_sampling_periods) {
			pr_info("auto_hotplug: Offlining CPU1, avg running: %d\n", avg_running);
			cpu_down(1);
			offline_sample--;
			online_sample = 0;
		}
		offline_sample++;
	}

	queue_delayed_work(wq, &hotplug_decision_work, msecs_to_jiffies(sampling_rate));

}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void auto_hotplug_early_suspend(struct early_suspend *handler)
{
	pr_info("auto_hotplug: early suspend handler\n");

	/* Cancel all scheduled delayed work to avoid races */
	cancel_delayed_work_sync(&hotplug_decision_work);
	flush_workqueue(wq);
	
	pr_info("auto_hotplug: Offlining CPUs for early suspend\n");
	cpu_down(1);
	
	/* limit max frequency */
    msm_cpufreq_set_freq_limits(0, MSM_CPUFREQ_NO_LIMIT, 
            suspend_frequency);
    pr_info("Cpulimit: Early suspend - limit max frequency to: %dMHz\n",
            suspend_frequency/1000);
}

static void __cpuinit auto_hotplug_late_resume(struct early_suspend *handler)
{
	pr_info("auto_hotplug: late resume handler\n");
	
	/* restore max frequency */
    msm_cpufreq_set_freq_limits(0, MSM_CPUFREQ_NO_LIMIT, MSM_CPUFREQ_NO_LIMIT);
    pr_info("Cpulimit: Late resume - restore max frequency.\n");

	cpu_up(1);
	
	queue_delayed_work(wq, &hotplug_decision_work, 0);
}

static struct early_suspend auto_hotplug_suspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = auto_hotplug_early_suspend,
	.resume = auto_hotplug_late_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int __init auto_hotplug_init(void)
{
	pr_info("auto_hotplug: v0.220 by _thalamus\n");
	pr_info("auto_hotplug: %d CPUs detected\n", CPUS_AVAILABLE);

	wq = alloc_workqueue("auto_hotplug_workqueue",
                         WQ_UNBOUND | WQ_RESCUER | WQ_FREEZABLE, 1);
    
    if (!wq)
        return -ENOMEM;
        
	/*
	 * Give the system time to boot before fiddling with hotplugging.
	 */
	
	INIT_DELAYED_WORK(&hotplug_decision_work, hotplug_decision_work_fn);
	queue_delayed_work(wq, &hotplug_decision_work, HZ * 20);

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&auto_hotplug_suspend);
#endif
	return 0;
}
late_initcall(auto_hotplug_init);
