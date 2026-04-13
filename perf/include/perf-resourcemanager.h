/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include "perf-common.h"
#include "perf-resourcereq.h"
#include "perf-tunable.h"
#include <unordered_map>
#include <memory>
#include <vector>
#include <cstdint>

namespace perf {

class ResourceManager {
public:
    ResourceManager();

    // Update state:
    // isRemove = false -> Apply/Update
    // isRemove = true  -> Release
    void UpdateState(int32_t handle,
                     const std::vector<ResourceReq>& resources,
                     bool isRemove);

private:
    void ResolveResource(int32_t id);

    // Resources -> (handle -> value)
    std::unordered_map<int32_t, std::unordered_map<int32_t, int32_t>> mActiveRequests;

    // ResourceID -> Tunable
    std::unordered_map<int32_t, std::unique_ptr<PerfTunable>> mTunables;
};

} // namespace perf
