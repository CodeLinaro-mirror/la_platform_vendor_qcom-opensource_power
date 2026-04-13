/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// perf-log.h
#pragma once

// ==============================
// MASTER PERF LOG SWITCH
// ==============================
// 1 = enable all perf logs
// 0 = compile out all perf logs
#ifndef PERF_LOG_ENABLE
#define PERF_LOG_ENABLE 0
#endif

#if PERF_LOG_ENABLE

// Force verbose logs compiled in for these sources only
#ifndef LOG_NDEBUG
#define LOG_NDEBUG 0
#endif

#ifndef LOG_TAG
#define LOG_TAG "perf"
#endif

#include <log/log.h>

#define PERF_V(...)  ALOGV(__VA_ARGS__)
#define PERF_D(...)  ALOGD(__VA_ARGS__)
#define PERF_I(...)  ALOGI(__VA_ARGS__)
#define PERF_W(...)  ALOGW(__VA_ARGS__)
#define PERF_E(...)  ALOGE(__VA_ARGS__)

#else  // PERF_LOG_ENABLE == 0

// ==============================
// LOGGING COMPLETELY DISABLED
// ==============================
// All macros compile to nothing (zero runtime cost)
#define PERF_V(...)
#define PERF_D(...)
#define PERF_I(...)
#define PERF_W(...)
#define PERF_E(...)

#endif
