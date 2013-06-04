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
#include <linux/moduleparam.h>
#include <linux/cpu.h>
#include <linux/workqueue.h>
#include <linux/sched.h>
#include <linux/rq_stats.h>
#include <linux/cpufreq.h>

#include <mach/cpufreq.h>

#include "../mach-msm/acpuclock.h"

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
#define ONLINE_SAMPLING_PERIODS		3
#define OFFLINE_SAMPLING_PERIODS	6
#define SAMPLING_MULTIPLIER			1

/*
 * Load defines:
 *
 * ENABLE is the load which is required to enable CPU1
 * DISABLE is the load at which CPU1 is disabled
 */
#define ENABLE_LOAD_THRESHOLD		40
#define DISABLE_LOAD_THRESHOLD		25
#define DEFAULT_SUSPEND_FREQ 		702000

static struct delayed_work hotplug_decision_work;

static struct workqueue_struct *wq;

static unsigned int suspend_frequency = DEFAULT_SUSPEND_FREQ;
static unsigned int online_sample = 1;
static unsigned int offline_sample = 1;
static unsigned int online_sampling_periods = ONLINE_SAMPLING_PERIODS;
static unsigned int offline_sampling_periods = OFFLINE_SAMPLING_PERIODS;
static unsigned int sampling_multiplier = SAMPLING_MULTIPLIER;

static unsigned int disable_load = DISABLE_LOAD_THRESHOLD;
static unsigned int enable_load = ENABLE_LOAD_THRESHOLD;	

static void __cpuinit hotplug_decision_work_fn(struct work_struct *work)
{
	unsigned int avg_running;
	unsigned int online_cpus, available_cpus, current_freq;

	online_cpus = num_online_cpus();
	available_cpus = CPUS_AVAILABLE;
	
	avg_running = report_load_at_max_freq();

#if DEBUG
	pr_info("average_running is: %d\n", avg_running);
#endif

	if ((avg_running >= enable_load) && (online_cpus < available_cpus)) {
		if (online_sample >= online_sampling_periods) {		
			cpu_up(1);
			//pr_info("auto_hotplug: CPU1 up, avg running: %d\n", avg_running);
		} else {
			online_sample++;
#if DEBUG
	pr_info("auto_hotplug: online sample%d: %d\n", online_sample, avg_running);
#endif
		}
		offline_sample = 1;
	} else if (avg_running <= (disable_load << 1) && (online_cpus == available_cpus)) {
		if (offline_sample >= offline_sampling_periods) {
			//pr_info("auto_hotplug: CPU1 down, avg running: %d\n", avg_running);
			cpu_down(1);
		} else {
			offline_sample++;
#if DEBUG
	pr_info("auto_hotplug: offline sample%d: %d\n", offline_sample, avg_running);
#endif
		}
		online_sample = 1;
	}
	queue_delayed_work_on(0, wq, &hotplug_decision_work, msecs_to_jiffies(HZ) 
							* sampling_multiplier);
}

static ssize_t show_enable_load(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", enable_load);
}

static ssize_t store_enable_load(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	enable_load = val;
	return count;
}

static struct global_attr enable_load_attr = __ATTR(enable_load, 0644,
		show_enable_load, store_enable_load);
		
static ssize_t show_disable_load(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", disable_load);
}

static ssize_t store_disable_load(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	disable_load = val;
	return count;
}

static struct global_attr disable_load_attr = __ATTR(disable_load, 0644,
		show_disable_load, store_disable_load);

static ssize_t show_online_samples(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", online_sampling_periods);
}

static ssize_t store_online_samples(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	online_sampling_periods = val;
	return count;
}

static struct global_attr online_samples_attr = __ATTR(online_samples, 0644,
		show_online_samples, store_online_samples);

static ssize_t show_offline_samples(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", offline_sampling_periods);
}

static ssize_t store_offline_samples(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	offline_sampling_periods = val;
	return count;
}

static struct global_attr offline_samples_attr = __ATTR(offline_samples, 0644,
		show_offline_samples, store_offline_samples);
		
static ssize_t show_sampling_multiplier(struct kobject *kobj,
				     struct attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", sampling_multiplier);
}

static ssize_t store_sampling_multiplier(struct kobject *kobj,
				  struct attribute *attr, const char *buf,
				  size_t count)
{
	int ret;
	long unsigned int val;

	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;
	sampling_multiplier = val;
	return count;
}

static struct global_attr sampling_multiplier_attr = __ATTR(sampling_multiplier, 0644,
		show_sampling_multiplier, store_sampling_multiplier);

static struct attribute *auto_hotplug_attributes[] = {
	&enable_load_attr.attr,
	&disable_load_attr.attr,
	&online_samples_attr.attr,
	&offline_samples_attr.attr,
	&sampling_multiplier_attr.attr,
	NULL,
};

static struct attribute_group auto_hotplug_attr_group = {
	.attrs = auto_hotplug_attributes,
	.name = "auto_hotplug",
};

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
    msm_cpufreq_set_freq_limits(0, CONFIG_MSM_CPU_FREQ_MIN, 
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
	
	queue_delayed_work(wq, &hotplug_decision_work, HZ * 2);
}

static struct early_suspend auto_hotplug_suspend = {
	.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1,
	.suspend = auto_hotplug_early_suspend,
	.resume = auto_hotplug_late_resume,
};
#endif /* CONFIG_HAS_EARLYSUSPEND */

static int __init auto_hotplug_init(void)
{
	int rc;
	
	pr_info("auto_hotplug: Original: v0.220 by _thalamus\n");
	pr_info("auto_hotplug: New: v1.1 by InstigatorX\n");
	pr_info("auto_hotplug: %d CPUs detected\n", CPUS_AVAILABLE);

	wq = create_freezable_workqueue("auto_hotplug_workqueue");
    
    if (!wq)
        return -ENOMEM;
        
    rc = sysfs_create_group(cpufreq_global_kobject,
				&auto_hotplug_attr_group);
        
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
