/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "perf-controller.h"
#include "perf-server.h"
#include "perf-common.h"
#include "perf-resourcereq.h"
#include <vector>
#include "perf-log.h"

extern "C" {

int32_t perfBoostAcq(int32_t handle, uint32_t duration, int32_t num_args, int32_t resources[]) {
    if (handle < 1 || num_args < 2 || (num_args % 2) != 0) {
        PERF_E("perfBoostAcq: Invalid args (handle=%d, num_args=%d)", handle, num_args);
        return -1;
    }

    std::vector<perf::ResourceReq> args;
    args.reserve(static_cast<size_t>(num_args) / 2);

    for (int i = 0; i < num_args; i += 2) {
        const int32_t id  = resources[i];
        const int32_t val = resources[i+1];

        if (perf::NODE_DB.find(id) == perf::NODE_DB.end()) {
            PERF_W("perfBoostAcq: Unknown resource id %d ignored", id);
            continue;
        }
        args.push_back(perf::ResourceReq::make(id, val));
    }

    if (args.empty()) return -1;

    bool success = perf::PerfServer::GetInstance().SubmitAcquire(handle, duration, std::move(args));
    if(success==0) PERF_I("perfBoostAcq: handle=%d, duration=%u, pairs=%d", handle, duration, num_args/2);
 
    return success ? 0 : -1;
}

int32_t perfBoostRel(int32_t handle) {
    if (handle < 1) return -1;
    bool success = perf::PerfServer::GetInstance().SubmitRelease(handle);
    return success ? 0 : -1;
}

} // extern "C"
