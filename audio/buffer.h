#ifndef BUFFER_H
#define BUFFER_H

#include <vector>

class Buffer {
    private:

        // The original file format. 
        struct Format {
            std::string fileName;
            ma_uint32 outputChannels;
            ma_uint32 outputSampleRate;
            ma_format outputFormat;
        };

        std::vector<float> leftSamples;
        std::vector<float> rightSamples;
        int totalFrames = 0;
        Format format;

    public:

        std::vector<float>& getLeftSamples() { return leftSamples; }
        std::vector<float>& getRightSamples() { return rightSamples; }
        bool isStereo() { return format.outputChannels == 2 ? true : false; }
        ma_uint32 getSampleRate() { return format.outputSampleRate; }
        void clear();
        void reserve(size_t frames);
        void fillFromInterleaved(const float data, size_t frames);
        Format& getFormat() { return format; }
        int getTotalFrames() { return totalFrames; }
};

#endif // BUFFER_H
