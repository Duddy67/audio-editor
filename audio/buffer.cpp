#include "buffer.h"

void Buffer::fillFromInterleaved(const float data, size_t frames);
{
    totalFrames = frames;

    if (isStereo()) {
        // Split into left/right channels
        for (int i = 0; i < totalFrames; ++i) {
            leftSamples.push_back(data[i * 2]);
            rightSamples.push_back(data[i * 2 + 1]);
        }
    }
    // Mono data
    else {
        leftSamples = std::vector<float>(data);
        // Mirror for playback
        rightSamples = leftSamples;
    }
}

