#include "buffer.h"

void Buffer::fillFromInterleaved(const std::vector<float>& data, size_t frames)
{
    if (isStereo()) {
        // Split into left/right channels
        for (size_t i = 0; i < frames; ++i) {
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

void Buffer::interleaveSamples(std::vector<float>& interleaved)
{
    // Interleave the samples
    interleaved.clear();
    size_t frameCount = leftSamples.size();
    interleaved.resize(frameCount * 2);

    for (size_t i = 0; i < frameCount; ++i) {
        interleaved[i * 2 + 0] = leftSamples[i];
        interleaved[i * 2 + 1] = rightSamples[i];
    }
}

void Buffer::clear()
{
    leftSamples.clear();
    rightSamples.clear();
}

void Buffer::reserve(size_t frames)
{
    leftSamples.reserve(frames);
    rightSamples.reserve(frames);
}
