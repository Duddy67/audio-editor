#include "buffer.h"

void Buffer::fillFromInterleaved(std::vector<float>& data, size_t frames)
{
    totalFrames = frames;

    if (isStereo()) {
        // Split into left/right channels
        for (int i = 0; i < totalFrames; ++i) {
            //leftSamples[i] = data[i * 2];
            //rightSamples[i] = data[i * 2 + 1];
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

std::vector<float>& Buffer::interleaveSamples()
{
    // Interleave the samples
    size_t frameCount = leftSamples.size();
    interleaved.resize(frameCount * 2);

    for (size_t i = 0; i < frameCount; ++i) {
        interleaved[i * 2 + 0] = leftSamples[i];
        interleaved[i * 2 + 1] = rightSamples[i];
    }

    return interleaved;
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
