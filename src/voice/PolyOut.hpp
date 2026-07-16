// Lightweight helpers for building and assigning poly voice targets
#pragma once
#include <vector>
#include <cmath>

namespace stx { namespace poly {

// Build target note CVs (V/oct) from semitone intervals for the requested voice count.
// Produces ascending voicings relative to the first interval (treated as chord root).
// If harmonyMode is true, push voices up by octaves and add fifths to odd voices to widen.
inline void buildTargetsFromIntervals(const std::vector<float>& intervalsSemitones,
                                      int voiceCount,
                                      bool harmonyMode,
                                      std::vector<float>& out) {
    out.clear();
    out.reserve(voiceCount);

    if (voiceCount <= 0) return;

    // Handle empty chord defensively
    if (intervalsSemitones.empty()) {
        out.assign(voiceCount, 0.f);
        return;
    }

    // Build semitone values ensuring ascending order relative to the root (first interval)
    float lastSemi = 0.f;
    for (int voice = 0; voice < voiceCount; ++voice) {
        int idx = voice % (int)intervalsSemitones.size();
        float semi = intervalsSemitones[idx];
        if (voice == 0) {
            // First voice: take as-is (defines our base/root reference)
            lastSemi = semi;
        } else {
            // Ensure ascending order by lifting into higher octaves as needed
            while (semi <= lastSemi) semi += 12.f;
            lastSemi = semi;
        }

        // Apply harmony widening if requested
        if (harmonyMode) {
            semi += 12.f;                 // +1 octave
            if (voice % 2 == 1) semi += 7.f; // add fifth on odd voices
        }

        out.push_back(semi / 12.f); // convert to V/oct, root at 0V
    }
}

// Assign all target notes to voices 0-5. Simple direct assignment for polyphonic chords.
// last[6] holds last CV per channel (V/oct). Returns assigned vector with 6 elements.
inline void assignNearest(const std::vector<float>& targets,
                          const float last[6],
                          int voiceCount,
                          std::vector<float>& assigned) {
    assigned.assign(6, 0.f);  // Always create 6 elements, initialize to 0V
    
    // Simple direct assignment: use all targets, cycling through them for 6 voices
    for (int v = 0; v < 6; ++v) {
        if (!targets.empty()) {
            int targetIdx = v % targets.size();
            float target = targets[targetIdx];
            
            // Apply octave wrapping to minimize jumps from last CV value
            float lastCV = last[v];
            float bestCV = target;
            float bestDist = std::fabs(target - lastCV);
            
            // Try octave shifts to find the closest version
            for (int octShift = -2; octShift <= 2; ++octShift) {
                float candidate = target + octShift; // shift by octaves in V/oct
                float dist = std::fabs(candidate - lastCV);
                if (dist < bestDist) {
                    bestDist = dist;
                    bestCV = candidate;
                }
            }
            
            assigned[v] = bestCV;
        }
        // If no targets, voice stays at 0V (silent)
    }
}

}} // namespace stx::poly
