/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "perf-resourcemanager.h"
#include <algorithm>
#include "perf-log.h" 

namespace perf {

ResourceManager::ResourceManager() {
    for (const auto& kv : NODE_DB) {
        const int32_t id = kv.first;
        const NodeConfig& cfg = kv.second;
        mTunables[id] = std::make_unique<PerfTunable>(id, cfg);
    }
}

void ResourceManager::UpdateState(int32_t handle,
                                  const std::vector<ResourceReq>& resources,
                                  bool isRemove) {
    // Special Case: Release ALL resources for this handle (empty vector)
    if (isRemove && resources.empty()) {
        for (auto& [resId, reqMap] : mActiveRequests) {
            if (reqMap.erase(handle) > 0) {
                ResolveResource(resId);
            }
        }
        return;
    }

    for (const auto& r : resources) {
        const int32_t id  = r.id;
        const int32_t val = r.value;

        if (mTunables.find(id) == mTunables.end()) {
            PERF_W("ResourceManager: Unknown resource ID %d", id);
            continue;
        }

        if (isRemove) mActiveRequests[id].erase(handle);
        else          mActiveRequests[id][handle] = val;

        ResolveResource(id);
    }
}

void ResourceManager::ResolveResource(int32_t id) {
    auto tIt = mTunables.find(id);
    if (tIt == mTunables.end()) return;

    auto& reqMap = mActiveRequests[id];
    if (reqMap.empty()) {
        (void)tIt->second->Reset();
        return;
    }

    // Determine policy from NODE_DB (default MAX)
    ArbitratorPolicy pol = ArbitratorPolicy::MAX_VALUE;
    if (auto cfgIt = NODE_DB.find(id); cfgIt != NODE_DB.end()) {
        pol = cfgIt->second.policy;
    }

    // Pick winner in one pass
    auto it = reqMap.begin();
    int32_t winner = it->second;
    ++it;
    for (; it != reqMap.end(); ++it) {
        winner = (pol == ArbitratorPolicy::MAX_VALUE)
                    ? std::max(winner, it->second)
                    : std::min(winner, it->second);
    }

    PERF_V("ResolveResource(%d): policy=%s, active=%zu", id, (pol == ArbitratorPolicy::MAX_VALUE ? "MAX" : "MIN"), reqMap.size());

    (void)tIt->second->Apply(winner);
}

} // namespace perf
