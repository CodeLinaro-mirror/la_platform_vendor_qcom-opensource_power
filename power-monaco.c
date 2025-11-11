/*
 * Copyright (c) 2015-2021, The Linux Foundation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * *    * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of The Linux Foundation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 * Copyright (c) 2025 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#define LOG_NIDEBUG 0

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdlib.h>

#define LOG_TAG "QTI PowerHAL"
#include <utils/Log.h>
#include <hardware/hardware.h>
#include <hardware/power.h>

#include "utils.h"
#include "metadata-defs.h"
#include "hint-data.h"
#include "performance.h"
#include "power-common.h"

#define SLATERSB_ENABLE_PATH "/sys/devices/platform/soc/soc:qcom,slate-rsb/enable"

#define MAX_RETRY 10

/* Declare function before use */
void interaction(int duration, int num_args, int opt_list[]);

int power_hint_override(power_hint_t hint, void *data)
{
	int enabled = 0;
	int duration = 0;

	if (data != NULL)
		enabled = *(int *)data;

	ALOGI("Hint received, power_hint: %d, enabled: %d", hint, enabled);

	switch(hint) {
		case POWER_HINT_INTERACTION:
			if(enabled > 0) {
				int resources[] = {0x40800000, 0x360, 0x41000000, 0x2, 0x40CA4000, 0x2};
				duration = enabled;
				interaction(duration, sizeof(resources)/sizeof(resources[0]), resources);
			}
			break;
		case POWER_HINT_LOW_POWER:
		case POWER_HINT_LAUNCH:
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

int  set_interactive_override(int on)
{
     char *s = on? "1" : "0";
     int rc = -1;

     char prop_value[PROPERTY_VALUE_MAX] = {'\0'};
     int aon_len = property_get("ro.vendor.qc_aon_presence", prop_value, NULL);
     ALOGD("aon_len: %d, ro.vendor.qc_aon_presence: %d \n", aon_len, atoi(prop_value));
     if (atoi(prop_value) != 0) {
        rc = sysfs_write(SLATERSB_ENABLE_PATH, s);
        if(rc != 0)
            ALOGE("RSB command is not processed\n");
     }
     return HINT_HANDLED;
}
