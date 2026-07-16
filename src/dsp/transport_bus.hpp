#pragma once
#include <atomic>
#include <cstdint>

// Cable-free transport link between Shapetaker modules.
//
// A small lock-free registry of transport "channels". A leader module
// (Transmutation) acquires a channel keyed by its module ID and publishes its
// transport every process() call; follower modules (Tetrarch) look the channel
// up by the saved module ID and read it. All fields are plain atomics, no
// locks, safe to touch from the audio thread on both sides.
//
// Publishing contract (leader):
//   - running:    any sequence currently playing
//   - stepBpm:    effective leader steps-per-minute (incl. multiplier)
//   - stepPhase:  0..1 fractional position inside the current leader step
//   - stepCount:  increments once per leader step (wraps naturally)
//   - resetToken: increments when the leader is hard-reset
//
// Follower usage: derive a monotonically increasing leader position
// (stepCount + stepPhase) and fire local steps as it crosses multiples of the
// local step ratio — drift-free, like an external clock but with fractional
// resolution and no cable.

namespace shapetaker {
namespace sync {

struct TransportChannel {
    std::atomic<int64_t> moduleId;
    std::atomic<bool> running;
    std::atomic<float> stepBpm;
    std::atomic<float> stepPhase;
    std::atomic<uint32_t> stepCount;
    std::atomic<uint32_t> resetToken;

    TransportChannel()
        : moduleId(-1), running(false), stepBpm(120.f),
          stepPhase(0.f), stepCount(0), resetToken(0) {}
};

static const int MAX_TRANSPORT_CHANNELS = 8;

// Function-local static in an inline function: one shared instance across all
// translation units (C++11 vague linkage), initialized thread-safely.
inline TransportChannel* transportChannels() {
    static TransportChannel channels[MAX_TRANSPORT_CHANNELS];
    return channels;
}

inline TransportChannel* findTransport(int64_t moduleId) {
    if (moduleId < 0) {
        return nullptr;
    }
    TransportChannel* ch = transportChannels();
    for (int i = 0; i < MAX_TRANSPORT_CHANNELS; ++i) {
        if (ch[i].moduleId.load(std::memory_order_acquire) == moduleId) {
            return &ch[i];
        }
    }
    return nullptr;
}

inline TransportChannel* acquireTransport(int64_t moduleId) {
    if (moduleId < 0) {
        return nullptr;
    }
    if (TransportChannel* existing = findTransport(moduleId)) {
        return existing;
    }
    TransportChannel* ch = transportChannels();
    for (int i = 0; i < MAX_TRANSPORT_CHANNELS; ++i) {
        int64_t expected = -1;
        if (ch[i].moduleId.compare_exchange_strong(expected, moduleId,
                                                   std::memory_order_acq_rel)) {
            ch[i].running.store(false);
            ch[i].stepBpm.store(120.f);
            ch[i].stepPhase.store(0.f);
            ch[i].stepCount.store(0);
            ch[i].resetToken.store(0);
            return &ch[i];
        }
    }
    return nullptr; // registry full (8 leaders is plenty)
}

inline void releaseTransport(int64_t moduleId) {
    if (TransportChannel* ch = findTransport(moduleId)) {
        ch->running.store(false);
        ch->moduleId.store(-1, std::memory_order_release);
    }
}

} // namespace sync
} // namespace shapetaker
