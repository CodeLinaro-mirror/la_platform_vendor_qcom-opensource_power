/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include "perf-common.h"
#include "perf-resourcereq.h"
#include "perf-resourcemanager.h"
#include "perf-requestmanager.h"
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <memory>
#include <atomic>
#include <map>
#include <unordered_map>
#include <chrono>
#include <cstdint>

namespace perf {

struct PerfMessage {
    enum class Type { ACQUIRE, RELEASE, EXIT };
    Type type{};
    int32_t handle{};
    uint32_t durationMs{};
    std::vector<ResourceReq> resources;
};

class PerfServer {
public:
    static PerfServer& GetInstance();

    PerfServer(const PerfServer&) = delete;
    PerfServer& operator=(const PerfServer&) = delete;
    PerfServer(PerfServer&&) = delete;
    PerfServer& operator=(PerfServer&&) = delete;

    bool SubmitAcquire(int32_t handle, uint32_t durationMs, std::vector<ResourceReq> args);
    bool SubmitRelease(int32_t handle);

private:
    PerfServer();
    ~PerfServer();

    void WorkerLoop();
    void ProcessMessage(const std::shared_ptr<PerfMessage>& msg);

    void StartPostBootObserver();
    void PostBootThreadMain();
    std::thread mPostBootThread;
    std::atomic<bool> mPostBootBoostDone{false};

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    std::atomic<bool> mRunning{false};
    std::thread mWorkerThread;

    std::queue<std::shared_ptr<PerfMessage>> mQueue;
    std::mutex mQueueMutex;
    std::condition_variable mCondition;

    // Deadline handling
    std::multimap<TimePoint, int32_t> mDeadlines; // time -> handle
    std::unordered_map<int32_t, std::multimap<TimePoint,int32_t>::iterator> mDeadlineIdx; // handle -> iterator

    // Managers
    ResourceManager mResourceManager;
    RequestManager  mRequestMgr;
};

} // namespace perf
