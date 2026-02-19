#ifndef GUI_H
#define GUI_H

#include <vector>
#include <bits/stdc++.h> // std::map
#include "buffer.h"
#include "../view/waveform.h"
#include "../marking/marking.h"

// Forward declarations.
class Track;
class Waveform;
class Marking;


class GUI {
    private:

        Track& track;
        std::unique_ptr<Waveform> waveform;  
        std::unique_ptr<Marking> marking;  
        std::atomic<size_t> dirtyStart{SIZE_MAX};
        std::atomic<size_t> dirtyEnd{0};

    public:

        GUI(Track& t) : track(t) {}

        Waveform& getWaveform() { return *waveform.get(); }
        Marking& getMarking() { return *marking.get(); }
        void init(int x, int y, int w, int h);
        bool getNewSamplesCopy(std::vector<float>& leftCopy, std::vector<float>& rightCopy, size_t& newStartIndex, size_t& newCount);
        std::atomic<size_t>& getDirtyStart() { return dirtyStart; }
        std::atomic<size_t>& getDirtyEnd() { return dirtyEnd; }
};

#endif // GUI_H

