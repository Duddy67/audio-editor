#include "clip.h"


float Clip::processSample(float inputSample, size_t timelineIndex) const
{
    float result = inputSample;

    size_t localIndex = timelineIndex - timelineStart;

    // Check for edits:

    if (hasMute && localIndex < muteLength) {
        result = 0.0f;
    }

    if (hasFadeIn && localIndex < fadeInLength) {
        float factor = static_cast<float>(localIndex) / static_cast<float>(fadeInLength);
        result *= factor;
    }

    if (hasFadeOut && localIndex < fadeOutLength) {
        float factor = 1.0f - static_cast<float>(localIndex) / static_cast<float>(fadeOutLength);
        result *= factor;
    }

    return result;
}
