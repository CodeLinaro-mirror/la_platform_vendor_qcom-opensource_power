/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <string>
#include <unordered_map>
#include <cstdint>

namespace perf {

// Aggregation Policies
enum class ArbitratorPolicy {
    MAX_VALUE, // Winner is largest value (e.g., CPU min freq)
    MIN_VALUE  // Winner is smallest value (e.g., latency, pwrlevel index)
};

// Configuration
struct NodeConfig {
    std::string path;     // sysfs or /proc path
    std::string prop;     // optional Android property alias (for /proc or property-backed)
    ArbitratorPolicy policy;
};

// Declaration only (definition in perf-common.cpp)
extern const std::unordered_map<int32_t, NodeConfig> NODE_DB;

} // namespace perf
