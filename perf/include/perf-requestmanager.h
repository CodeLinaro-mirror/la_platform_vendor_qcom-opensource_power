/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include "perf-resourcereq.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace perf {

class RequestManager {
public:
    struct RequestData {
        int32_t handle{};
        uint32_t duration{}; // ms
        std::vector<ResourceReq> resources;
    };

    void AddOrReplace(int32_t handle, uint32_t duration, std::vector<ResourceReq> resources);
    std::vector<ResourceReq> Remove(int32_t handle);

    bool Exists(int32_t handle) const {
        return mRequests.find(handle) != mRequests.end();
    }

    uint32_t GetDuration(int32_t handle) const {
        auto it = mRequests.find(handle);
        return (it == mRequests.end()) ? 0u : it->second.duration;
    }

    // Peek resources without removing (safe const ref; returns empty if missing)
    const std::vector<ResourceReq>& GetResources(int32_t handle) const;

private:
    std::unordered_map<int32_t, RequestData> mRequests;
};

} // namespace perf
