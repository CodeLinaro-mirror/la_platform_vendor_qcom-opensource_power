/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NIDEBUG 0
#define LOG_TAG "QTI PowerHAL"

#include <log/log.h>
#include <cutils/properties.h>
#include <hardware/power.h>
#include <string.h>
#include <errno.h>

#include "utils.h"
#include "power-common.h"

#include "perf-controller.h"

#define RSB_ENABLE_PATH "/sys/class/rsb/enable"
#define RSB_OFFLOAD_PROP "persist.vendor.rsb_offload"

/*Interaction Handles*/
#define POWERHINT_LOW_POWER 20
#define POWERHINT_LAUNCH 30
#define POWERHINT_INTERACTION_BOOST 40
#define POWERHINT_INTERACTION_MODE_ENABLED 50
#define POWERHINT_INTERACTION_MODE_DISABLED 60

/* Declare function before use */
void interaction(int duration, int num_args, int opt_list[]);

/*
 * power_hint_override:
 */
int power_hint_override(power_hint_t hint, void *data)
{
	int enabled = 0;
	int duration = 0;
	static int low_power_hint_acquired;

	if (data != NULL)
		enabled = *(int *)data;

	ALOGI("Hint received, power_hint: %d, enabled: %d", hint, enabled);

	switch(hint) {
		case POWER_HINT_INTERACTION:
			/* Interaction Hint for setBoost*/
			if(enabled > 1) {
                                int32_t resources[] = {PERF_RES_CLUSTER_0_MIN_FREQ, 0x14CD00, 
                                                        PERF_RES_MIN_CPUS, 0x2, 
                                                        PERF_RES_SCHED_WINDOW, 0x2}; 
				duration = enabled;
                                (void)perfBoostAcq(PERF_CLIENT_INTERACTION_BOOST, duration,  sizeof(resources) / sizeof(resources[0]), resources);
			}
			/* Interaction Hint for setMode*/
			else {
				if(enabled == 1) {
					int32_t resources[] = {PERF_RES_CLUSTER_0_MIN_FREQ, 0x704E0};
                                       (void)perfBoostAcq(PERF_CLIENT_INTERACTION_MODE, 0,  sizeof(resources) / sizeof(resources[0]), resources);
				} else {
                                       (void)perfBoostRel(PERF_CLIENT_INTERACTION_MODE);  
                                        uint32_t resources[] = {0x40804000, 0x5C6, 0x40804200, 0x10C, 0x41000200, 0x0, 0x41004200, 0x0};
					duration = 2000;
					interaction_with_handle(POWERHINT_INTERACTION_MODE_DISABLED, duration, sizeof(resources)/sizeof(resources[0]), resources);
				}
			}
			break;
		case POWER_HINT_LOW_POWER:
			if(enabled) {
				if(!low_power_hint_acquired) {
					int resources[] = {0x40804000, 0x5C6, 0x40804200, 0x10C, 0x41000200, 0x0, 0x41004200, 0x0};
					perform_hint_action(POWERHINT_LOW_POWER, resources, sizeof(resources)/sizeof(resources[0]));
					low_power_hint_acquired = 1;
				} else {
					ALOGI("Low power hint acquire failed, power_hint:%d, already acquired", hint);
				}
			} else {
				if (low_power_hint_acquired) {
					undo_hint_action(POWERHINT_LOW_POWER);
					low_power_hint_acquired = 0;
				}
				else {
					ALOGI("Low power hint release failed, power_hint:%d, already released", hint);
				}
			}
			break;
		case POWER_HINT_LAUNCH:
			if(enabled) {
				int resources[] = {0x40804000, 0x7A6, 0x40804200, 0x840, 0x41000200, 0x0, 0x41004200, 0x1,
                                                   0x40C00000, 0x1};
				duration = 1000;
				interaction_with_handle(POWERHINT_LAUNCH, duration, sizeof(resources)/sizeof(resources[0]), resources);
                                int32_t resourcesperf[] = {PERF_RES_CLUSTER_0_MIN_FREQ, 0x1DE200,
                                                        PERF_RES_CLUSTER_1_MIN_FREQ, 0x203A00,
                                                        PERF_RES_MIN_CPUS, 0x04,
                                                        PERF_RES_SCHED_WINDOW, 0x02,
                                                        PERF_RES_GPU_MIN_PWR, 0x0};
                                (void)perfBoostAcq(PERF_CLIENT_APP_LAUNCH, 0,  sizeof(resourcesperf) / sizeof(resourcesperf[0]), resourcesperf);
			} else {
                               (void)perfBoostRel(PERF_CLIENT_APP_LAUNCH);
                        }
			break;
		case POWER_HINT_VSYNC:
		case POWER_HINT_SUSTAINED_PERFORMANCE:
		case POWER_HINT_VR_MODE:
		case POWER_HINT_DISABLE_TOUCH:
		default:
			ALOGD("Power Hint: %d Not Supported", hint);
			break;
	}

    return HINT_HANDLED;
}

/*
 * set_interactive_override:
 * Toggle RSB enable node only when offload is disabled via persist.vendor.rsb_offload=no_offload.
 * on: 1 = interactive, 0 = non-interactive/doze.
 */
int set_interactive_override(int on)
{
	int rc = 0;
	const char *s = on ? "1" : "0";
	char prop_value[PROPERTY_VALUE_MAX] = {0};
	property_get(RSB_OFFLOAD_PROP, prop_value, "");

	if (strcmp(prop_value, "no_offload") == 0) {
		rc = sysfs_write(RSB_ENABLE_PATH, s);
		if (rc != 0) {
			ALOGE("RSB write(%s -> %s) failed rc=%d (%s)", RSB_ENABLE_PATH, s, rc, strerror(errno));
		} else {
			ALOGD("RSB set to %s", s);
		}
	} else {
		ALOGD("RSB offload prop='%s'; skipping sysfs write", prop_value);
	}

	return HINT_HANDLED;
}
