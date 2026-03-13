/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "perf-tunable.h"
#include <android-base/properties.h>
#include <fcntl.h>
#include <unistd.h>
#include <fstream>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include "perf-log.h"

namespace perf {

PerfTunable::PerfTunable(int32_t id, const NodeConfig& cfg)
    : mId(id), mPath(cfg.path), mProp(cfg.prop) {
    ReadDefault();
    EnsureOpen(); // try once; if dynamic node, later writes retry
}

PerfTunable::~PerfTunable() {
    if (mFd >= 0) close(mFd);
}

void PerfTunable::ReadDefault() {
    if (!mProp.empty()) {
        mDefaultVal = android::base::GetProperty(mProp, "");
        return;
    }
    std::ifstream f(mPath);
    if (f.is_open()) std::getline(f, mDefaultVal);
}

bool PerfTunable::EnsureOpen() {
    if (!mProp.empty()) return true;       // property-backed: no FD
    if (mFd >= 0) return true;

    mFd = TEMP_FAILURE_RETRY(open(mPath.c_str(), O_WRONLY | O_CLOEXEC));
    if (mFd < 0) {
        // Node may be dynamic; caller can retry later
        PERF_W("Open failed for %s: %s", mPath.c_str(), strerror(errno));
        return false;
    }
    return true;
}

bool PerfTunable::WriteInternal(const std::string& val) {
    // Strategy A: Property
    if (!mProp.empty()) {
        bool ok = android::base::SetProperty(mProp, val);
        if (!ok) PERF_W("Open failed for %s: %s", mPath.c_str(), strerror(errno));
        return ok;
    }

    // Strategy B: sysfs FD (stack buffer to avoid heap on newline)
    if (!EnsureOpen()) return false;

    constexpr size_t kBufCap = 64;
    char buf[kBufCap];
    const char* data = nullptr;
    size_t len = 0;

    if (!val.empty() && val.back() == '\n') {
        data = val.c_str();
        len  = val.size();
    } else {
        int n = snprintf(buf, kBufCap, "%s\n", val.c_str());
        if (n >= 0 && static_cast<size_t>(n) < kBufCap) {
            data = buf;
            len  = static_cast<size_t>(n);
        } else {
            PERF_E("Formatting overflow for %s", mPath.c_str());
            return false;
        }
    }

    (void)lseek(mFd, 0, SEEK_SET);

    size_t written = 0;
    while (written < len) {
        ssize_t ret = TEMP_FAILURE_RETRY(write(mFd, data + written, len - written));
        if (ret < 0) {
            PERF_E("Formatting overflow for %s", mPath.c_str());
            close(mFd);
            mFd = -1;
            return false;
        }
        written += static_cast<size_t>(ret);
    }
    return true;
}

bool PerfTunable::Apply(int32_t value) {
    // sysfs path: format integer with newline using stack buffer
    if (mProp.empty()) {
        constexpr size_t kBufCap = 32;
        char buf[kBufCap];
        int n = snprintf(buf, kBufCap, "%d\n", value);
        if (n > 0 && static_cast<size_t>(n) < kBufCap) {
            if (!EnsureOpen()) return false;
            (void)lseek(mFd, 0, SEEK_SET);
            size_t written = 0;
            while (written < static_cast<size_t>(n)) {
                ssize_t ret = TEMP_FAILURE_RETRY(write(mFd, buf + written, n - written));
                if (ret < 0) {
                    PERF_E("Write failed %s: %s", mPath.c_str(), strerror(errno));
                    close(mFd);
                    mFd = -1;
                    return false;
                }
                written += static_cast<size_t>(ret);
            }
            PERF_V("Applied %d to %s", value, mPath.c_str());
            return true;
        }
        // Fallback to string path if formatting exceeds buffer
    }

    // Property-backed or overflow fallback
    std::string valStr = std::to_string(value);
    if (!WriteInternal(valStr)) {
        PERF_E("Failed to apply %s to nodeId=%d (%s)", valStr.c_str(), mId, (mProp.empty() ? mPath.c_str() : mProp.c_str()));

        return false;
    }
    PERF_V("Applied %s to %s",\
       valStr.c_str(), (mProp.empty() ? mPath.c_str() : mProp.c_str()));
    return true;
}

bool PerfTunable::Reset() {
    if (mDefaultVal.empty()) {
        PERF_V("Reset no-op: no default recorded for %s", (mProp.empty() ? mPath.c_str() : mProp.c_str()));

        return true;
    }
    if (!WriteInternal(mDefaultVal)) {
        PERF_E("Failed to reset (%s) to %s", (mProp.empty() ? mPath.c_str() : mProp.c_str()), mDefaultVal.c_str());

        return false;
    }

    PERF_V("Restored %s to %s", (mProp.empty() ? mPath.c_str() : mProp.c_str()), mDefaultVal.c_str());

    return true;
}

} // namespace perf
