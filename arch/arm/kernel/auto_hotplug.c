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
#define SAMPLING_PERIODS	10
#define INDEX_MAX_VALUE		(SAMPLING_PERIODS - 1)

/*
 * Load defines:
 *
 * ENABLE is the load which is required to enable CPU1
 * DISABLE is the load at which CPU1 is disabled
 */
#define ENABLE_LOAD_THRESHOLD		350
#define DISABLE_LOAD_THRESHOLD		150
#define DEFAULT_SUSPEND_FREQ 		702000

static struct delayed_work hotplug_decision_work;

static struct workqueue_struct *wq;

static unsigned int history[SAMPLING_PERIODS];
static unsigned int index;

static unsigned int suspend_frequency = DEFAULT_SUSPEND_FREQ;

static void __cpuinit hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned int running, disable_load, enable_load, iowait_avg, avg_running = 0;
	unsigned int online_cpus, available_cpus, i, j;
#if DEBUG
	unsigned int k;
#endif

	online_cpus = num_online_cpus();
	available_cpus = CPUS_AVAILABLE;
	disable_load = DISABLE_LOAD_THRESHOLD;
	enable_load = ENABLE_LOAD_THRESHOLD;
	
	sched_get_nr_running_avg(&running, &iowait_avg);

	history[index] = running;

#if DEBUG
	pr_info("online_cpus is: %d\n", online_cpus);
	pr_info("enable_load is: %d\n", enable_load);
	pr_info("disable_load is: %d\n", disable_load);
	pr_info("index is: %d\n", index);
	pr_info("running is: %d\n", running);
#endif

	/*
	 * Use a circular buffer to calculate the average load
	 * over the sampling periods.
	 * This will absorb load spikes of short duration where
	 * we don't want additional cores to be onlined because
	 * the cpufreq driver should take care of those load spikes.
	 */
	for (i = 0, j = index; i < SAMPLING_PERIODS; i++, j--) {
		avg_running += history[j];
		if (unlikely(j == 0))
			j = INDEX_MAX_VALUE;
	}

	/*
	 * If we are at the end of the buffer, return to the beginning.
	 */
	if (unlikely(index++ == INDEX_MAX_VALUE))
		index = 0;

#if DEBUG
	pr_info("array contents: ");
	for (k = 0; k < SAMPLING_PERIODS; k++) {
		 pr_info("%d: %d\t",k, history[k]);
	}
	pr_info("\n");
	pr_info("avg_running before division: %d\n", avg_running);
#endif

	avg_running = avg_running / SAMPLING_PERIODS;

#if DEBUG
	pr_info("average_running is: %d\n", avg_running);
#endif

	if ((avg_running >= enable_load) && (online_cpus < available_cpus)) {
		pr_info("auto_hotplug: Onlining CPU1, avg running: %d\n", avg_running);
		cpu_up(1);
	} else if (avg_running <= disable_load && (online_cpus == available_cpus)) {
		pr_info("auto_hotplug: Offlining CPU1, avg running: %d\n", avg_running);
		cpu_down(1);
	}

	queue_delayed_work_on(0, wq, &hotplug_decision_work, msecs_to_jiffies(HZ));

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
	
	queue_delayed_work_on(0, wq, &hotplug_decision_work, msecs_to_jiffies(HZ));
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
	queue_delayed_work_on(0, wq, &hotplug_decision_work, HZ * 20);

#ifdef CONFIG_HAS_EARLYSUSPEND
	register_early_suspend(&auto_hotplug_suspend);
#endif
	return 0;
}
late_initcall(auto_hotplug_init);
