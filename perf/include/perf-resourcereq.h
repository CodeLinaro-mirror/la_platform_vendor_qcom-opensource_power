/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <cstdint>

namespace perf {

// Thin DTO for "ID -> value"
class ResourceReq {
public:
    int32_t id   = -1; // must exist in NODE_DB
    int32_t value = 0;

    static inline ResourceReq make(int32_t i, int32_t v) { return ResourceReq{i, v}; }
};

} // namespace perf
