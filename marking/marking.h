#ifndef MARKING_H
#define MARKING_H

#include <filesystem>
#include <vector>
#include <iostream>
#include <FL/Fl_Group.H>
#include "marker.h"

// Forward declaration.
class WaveformView;

class Marking : public Fl_Group {
        std::vector<Marker*> markers;  
        WaveformView* pWaveform = nullptr;  

        unsigned int getNewMarkerId();

    public:

        Marking(int X, int Y, int W, int H)
            : Fl_Group(X, Y, W, H) 
        {
            box(FL_DOWN_BOX);
            end();
        }

        void init(WaveformView* w);
        WaveformView& getWaveform() { return *pWaveform; }
        // Markers can be read but not owned (ie: modified).
        const std::vector<Marker*>& getMarkers() const { return markers; }
        void insertMarker(int samplePosition);
        void deleteMarker(unsigned int id);
};

#endif // MARKING_H
