#ifndef BUFFER_H
#define BUFFER_H

#include "../../libraries/miniaudio.h"
#include <vector>
#include <string>


// The original audio file format. 
struct Format {
    std::string fileName;
    ma_uint32 outputChannels;
    ma_uint32 outputSampleRate;
    ma_format outputFormat;
};

class Buffer {
    private:

        std::vector<float> leftSamples;
        std::vector<float> rightSamples;
        Format format;

    public:

        std::vector<float>& getLeftSamples() { return leftSamples; }
        std::vector<float>& getRightSamples() { return rightSamples; }

        bool isStereo() { return format.outputChannels == 2 ? true : false; }
        ma_uint32 getSampleRate() { return format.outputSampleRate; }
        void clear();
        void reserve(size_t frames);
        void resize(size_t frames);
        void fillFromInterleaved(const std::vector<float>& data, size_t frames);
        Format getFormat() { return format; }
        void setFormat(Format f) { format = f; }
        size_t getTotalFrames() const { return leftSamples.size(); }
        void interleaveSamples(std::vector<float>& interleaved);
        void setSamples(const std::vector<float>& left, const std::vector<float>& right);
};

#endif // BUFFER_H
