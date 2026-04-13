/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "perf-common.h"

namespace perf {

// Definition (allocated only once here)
const std::unordered_map<int32_t, NodeConfig> NODE_DB = {
    // ID    Path                                                       Property (if needed)                            Policy
    {1000, {"/sys/devices/system/cpu/cpufreq/policy0/scaling_min_freq", "",                                             ArbitratorPolicy::MAX_VALUE}},
    {1001, {"/sys/devices/system/cpu/cpufreq/policy4/scaling_min_freq", "",                                             ArbitratorPolicy::MAX_VALUE}},
    {1005, {"/sys/devices/system/cpu/cpu0/core_ctl/min_cpus",           "",                                             ArbitratorPolicy::MAX_VALUE}},
    {1008, {"/proc/sys/walt/sched_ravg_window_nr_ticks",                "vendor.power.walt.sched_ravg_window_nr_ticks", ArbitratorPolicy::MAX_VALUE}},
    {1009, {"/sys/class/kgsl/kgsl-3d0/min_pwrlevel",                    "",                                             ArbitratorPolicy::MIN_VALUE}}, // lower index = higher perf
};

} // namespace perf
