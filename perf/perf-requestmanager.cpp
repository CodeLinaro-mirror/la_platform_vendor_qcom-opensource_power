/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "perf-requestmanager.h"

namespace perf {

void RequestManager::AddOrReplace(int32_t handle, uint32_t duration,
                                  std::vector<ResourceReq> resources) {
    mRequests[handle] = RequestData{handle, duration, std::move(resources)};
}

std::vector<ResourceReq> RequestManager::Remove(int32_t handle) {
    auto it = mRequests.find(handle);
    if (it == mRequests.end()) return {};
    auto resources = std::move(it->second.resources);
    mRequests.erase(it);
    return resources;
}

const std::vector<ResourceReq>& RequestManager::GetResources(int32_t handle) const {
    static const std::vector<ResourceReq> kEmpty;
    auto it = mRequests.find(handle);
    if (it == mRequests.end()) return kEmpty;
    return it->second.resources;
}

} // namespace perf
