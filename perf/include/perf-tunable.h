/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include "perf-common.h"
#include <string>
#include <cstdint>

namespace perf {

// A single tunable perf control point (sysfs or property-backed /proc)
class PerfTunable {
public:
    PerfTunable(int32_t id, const NodeConfig& config);
    ~PerfTunable();

    bool Apply(int32_t value);  // set new value
    bool Reset();               // restore default

private:
    bool WriteInternal(const std::string& val);
    void ReadDefault();
    bool EnsureOpen();          // re-open on demand for dynamic nodes

    int32_t     mId;
    std::string mPath;
    std::string mProp;      // if set, use SetProperty / GetProperty
    std::string mDefaultVal;
    int         mFd = -1;   // open O_WRONLY if mProp empty
};

} // namespace perf
