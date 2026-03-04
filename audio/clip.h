#ifndef CLIP_H
#define CLIP_H

#include <vector>
//#include <bits/stdc++.h> // std::map
#include "buffer.h"

// Forward declaration.
//class Engine;
//struct Format;

class Clip {
    private:

        std::shared_ptr<Buffer> source;
        size_t sourceStart = 0;
        size_t sourceEnd = 0;
        size_t timelineStart = 0;

    public:

        Clip(std::shared_ptr<Buffer> src) : source(std::move(src)) {
            sourceEnd = source->getTotalFrames();
        }

        size_t getLength() const { return sourceEnd - sourceStart; }
        std::shared_ptr<Buffer> getSource() const { return source; }
        size_t getSourceStart() const { return sourceStart; }
        size_t getSourceEnd() const { return sourceEnd; }
        size_t getTimelineStart() const { return timelineStart; }
        float processSample(float inputSample, size_t timelineIndex) const;
};

#endif // CLIP_H
