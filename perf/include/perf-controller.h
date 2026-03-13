/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * perf-controller.h
 * Public C Interface for Performance HAL
 */
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// =========================================================
// Client Handles (Unique ID per use case)
// =========================================================
typedef enum {
    PERF_CLIENT_APP_LAUNCH        = 1,
    PERF_CLIENT_INTERACTION_MODE  = 2,
    PERF_CLIENT_INTERACTION_BOOST = 3,
    // Add more as needed
    PERF_CLIENT_MAX
} PerfClientHandle;

// =========================================================
/* Resource IDs (Must match NODE_DB in perf-common.cpp) */
// =========================================================

// CPU Frequency
#define PERF_RES_CLUSTER_0_MIN_FREQ     1000
#define PERF_RES_CLUSTER_1_MIN_FREQ     1001

// Core Control
#define PERF_RES_MIN_CPUS               1005

// Scheduler (WALT window ticks)
#define PERF_RES_SCHED_WINDOW           1008

// GPU / Graphics
#define PERF_RES_GPU_MIN_PWR            1009

// =========================================================
// API Functions
// =========================================================

/**
 * perfBoostAcq
 * Acquire a performance boost.
 *
 * @param handle    Unique ID for this client
 * @param duration  Duration in ms (0 = persistent until perfBoostRel)
 * @param num_args  Total size of resources array (must be even: ID+Value pairs)
 * @param resources Array of {ResourceID, Value}
 *
 * @return 0 on success, -1 on failure.
 */
int32_t perfBoostAcq(int32_t handle, uint32_t duration, int32_t num_args, int32_t resources[]);

/**
 * perfBoostRel
 * Release a previously acquired boost.
 *
 * @param handle    The unique ID used in perfBoostAcq
 *
 * @return 0 on success, -1 on failure.
 */
int32_t perfBoostRel(int32_t handle);

#ifdef __cplusplus
}
#endif
