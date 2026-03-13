#ifndef CLIP_H
#define CLIP_H

#include <vector>
//#include <bits/stdc++.h> // std::map
#include <memory>
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

        // Processing.
        bool hasFadeIn = false;
        size_t fadeInLength = 0;
        bool hasFadeOut = false;
        size_t fadeOutLength = 0;
        bool hasMute = false;
        size_t muteLength = 0;

    public:

        Clip(std::shared_ptr<Buffer> src) : source(std::move(src)) {
            sourceEnd = source->getTotalFrames();
        }

        std::shared_ptr<Buffer> getSource() const { return source; }
        float processSample(float inputSample, size_t timelineIndex) const;

        size_t getLength() const { return sourceEnd - sourceStart; }
        size_t getSourceStart() const { return sourceStart; }
        size_t getSourceEnd() const { return sourceEnd; }
        size_t getTimelineStart() const { return timelineStart; }

        void setTimelineStart(size_t position) { timelineStart = position; }
        void setSourceStart(size_t position) { sourceStart = position; }
        void setLength(size_t length) { sourceEnd = sourceStart + length; }
        void setFadeIn(bool state) { hasFadeIn = state; }
        void setFadeOut(bool state) { hasFadeOut = state; }
        void setMute(bool state) { hasMute = state; }
        void setFadeInLength(size_t length) { fadeInLength = length; }
        void setFadeOutLength(size_t length) { fadeOutLength = length; }
        void setMuteLength(size_t length) { muteLength = length; }
};

#endif // CLIP_H
