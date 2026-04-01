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

    public:

        GUI(Track& t) : track(t) {}

        Waveform& getWaveform() { return *waveform.get(); }
        Marking& getMarking() { return *marking.get(); }
        void init(int x, int y, int w, int h);
};

#endif // GUI_H

