/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "perf-server.h"
#include "perf-log.h"
#include <android-base/properties.h>
#include "perf-controller.h"

namespace perf {

PerfServer& PerfServer::GetInstance() {
    static PerfServer instance;
    return instance;
}

PerfServer::PerfServer() {
    mRunning = true;
    mWorkerThread = std::thread(&PerfServer::WorkerLoop, this);
    PERF_I("PerfServer: Worker thread started."); 
    StartPostBootObserver();
}

PerfServer::~PerfServer() {
    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        mRunning = false;
        // Push EXIT to unblock wait if needed
        auto msg = std::make_shared<PerfMessage>();
        msg->type = PerfMessage::Type::EXIT;
        mQueue.push(msg);
    }
    mCondition.notify_all();

    if (mPostBootThread.joinable()) mPostBootThread.join();
    if (mWorkerThread.joinable()) mWorkerThread.join();
    PERF_I("PerfServer: Worker thread stoped.");
}

void PerfServer::StartPostBootObserver() {
    mPostBootThread = std::thread(&PerfServer::PostBootThreadMain, this);
}

bool PerfServer::SubmitAcquire(int32_t handle, uint32_t durationMs, std::vector<ResourceReq> args) {
    size_t nres = args.size();
    auto msg = std::make_shared<PerfMessage>();
    msg->type = PerfMessage::Type::ACQUIRE;
    msg->handle = handle;
    msg->durationMs = durationMs;
    msg->resources = std::move(args);
    PERF_I("SubmitAcquire(handle=%d, dur=%u, nres=%zu)", handle, durationMs, nres);
    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (mQueue.size() >= 200) {
            PERF_E("PerfServer: Queue full, dropping ACQUIRE (handle=%d)", handle);
            return false;
        }
        mQueue.push(msg);
    }
    mCondition.notify_one();
    return true;
}

bool PerfServer::SubmitRelease(int32_t handle) {
    auto msg = std::make_shared<PerfMessage>();
    msg->type = PerfMessage::Type::RELEASE;
    msg->handle = handle;
    PERF_I("SubmitRelease(handle=%d)", handle);
    {
        std::lock_guard<std::mutex> lock(mQueueMutex);
        if (mQueue.size() >= 200) {
            PERF_E("PerfServer: Queue full, dropping RELEASE (handle=%d)", handle);
            return false;
        }
        mQueue.push(msg);
    }
    mCondition.notify_one();
    return true;
}

void PerfServer::WorkerLoop() {
    while (true) {
        std::shared_ptr<PerfMessage> msg;
        // Compute next wake-up target (nearest deadline), if any
        TimePoint nextWake = TimePoint::max();
        {
            std::unique_lock<std::mutex> lock(mQueueMutex);
            if (!mDeadlines.empty()){
                nextWake = mDeadlines.begin()->first;
            }
            if (nextWake == TimePoint::max()) {
                mCondition.wait(lock, [this] { return !mQueue.empty() || !mRunning; });
            } else {
                mCondition.wait_until(lock, nextWake, [this] { return !mQueue.empty() || !mRunning; });
            }
            if (!mRunning && mQueue.empty()) break;
            if (!mQueue.empty()) {
                msg = std::move(mQueue.front());
                mQueue.pop();
            }
        }

        // Handle timed expirations (timeout) if woke up due to deadline
        if (!msg && nextWake != TimePoint::max()) {
            const auto now = Clock::now();
            while (!mDeadlines.empty()) {
                auto it = mDeadlines.begin();
                if (it->first > now) break;

                const int32_t handle = it->second;
                mDeadlines.erase(it);
                mDeadlineIdx.erase(handle);

                auto freed = mRequestMgr.Remove(handle);
                mResourceManager.UpdateState(handle, freed, true);
            }
            continue;
        }

        if (msg) ProcessMessage(msg);
    }
}

void PerfServer::ProcessMessage(const std::shared_ptr<PerfMessage>& msg) {
    if (!msg) return;

    if (msg->type == PerfMessage::Type::EXIT) {
        // Nothing else to do; loop will exit when queue drains
        return;
    }

    if (msg->type == PerfMessage::Type::ACQUIRE) {
        // Store data
        mRequestMgr.AddOrReplace(msg->handle, msg->durationMs, msg->resources);

        // Apply without removing
        mResourceManager.UpdateState(msg->handle, mRequestMgr.GetResources(msg->handle), false);

        // Deadline scheduling
        if (msg->durationMs > 0) {
            if (auto dIt = mDeadlineIdx.find(msg->handle); dIt != mDeadlineIdx.end()) {
                mDeadlines.erase(dIt->second);
                mDeadlineIdx.erase(dIt);
            }
            const auto tp = Clock::now() + std::chrono::milliseconds(msg->durationMs);
            auto it = mDeadlines.insert({tp, msg->handle});
            mDeadlineIdx[msg->handle] = it;
        } else {
            // Cancel any previous deadline for this handle
            if (auto dIt = mDeadlineIdx.find(msg->handle); dIt != mDeadlineIdx.end()) {
                mDeadlines.erase(dIt->second);
                mDeadlineIdx.erase(dIt);
            }
        }
    }
    else if (msg->type == PerfMessage::Type::RELEASE) {
        // Cancel deadline
        if (auto dIt = mDeadlineIdx.find(msg->handle); dIt != mDeadlineIdx.end()) {
            mDeadlines.erase(dIt->second);
            mDeadlineIdx.erase(dIt);
        }

        // Remove request and release exact resources
        auto freed = mRequestMgr.Remove(msg->handle);
        mResourceManager.UpdateState(msg->handle, freed, true);
    }
}

void PerfServer::PostBootThreadMain() {
// Prevent double execution logic
    bool expected = false;
    if (!mPostBootBoostDone.compare_exchange_strong(expected, true)) return;

    using namespace std::chrono_literals;
    
    // Total wait limit: 5 minutes
    const auto deadline = std::chrono::steady_clock::now() + 5min;

    while (mRunning) {
        // Check if timed out globally
        if (std::chrono::steady_clock::now() > deadline) {
            PERF_W("PostBoot: Timed out waiting for vendor.post_boot.parsed");
            return;
        }

        // Wait for property with a SHORT timeout (e.g., 5 second)
        // This allows us to check 'mRunning' every second so we don't hang shutdown.
        if (android::base::WaitForProperty("vendor.post_boot.parsed", "1", 5s)) {
            PERF_I("PostBoot: Property detected! Triggering boost.");
            
            std::vector<ResourceReq> args;
            args.reserve(1);
            args.push_back(ResourceReq::make(PERF_RES_CLUSTER_0_MIN_FREQ, 0x704E0));

            // Use dedicated handle if available, or INTERACTION_BOOST
            (void)SubmitAcquire(PERF_CLIENT_INTERACTION_MODE, 0, std::move(args));
            return;
        }
        
        // Property not seen yet, loop again to check mRunning...
    }
}

} // namespace perf
