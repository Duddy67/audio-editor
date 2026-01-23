#ifndef MARKER_H
#define MARKER_H

#include <vector>
#include <iostream>
#include <cmath>
#include <FL/Fl_Box.H>
#include "../dialogs/renaming.h"
#include "../constants.h"

// Forward declaration.
class Marking;

class Marker : public Fl_Box {
        unsigned int id = 0;
        Marking& marking;
        int samplePosition = -1;
        bool dragging = false;
        int dragStartX;
        RenamingDialog* renamingDlg = nullptr;

        int getNewSamplePosition(int x);

    public:

        Marker(int X, int Y, int W, int H, const char* L, unsigned int i, Marking& m)
            : Fl_Box(X, Y, W, H, L), id(i), marking(m) 
        {
            color(FL_GREEN);
            box(FL_FLAT_BOX);
        }

        void setSamplePosition(int position) { samplePosition = position; }
        void alignX(int x);
        int handle(int event) override;
        unsigned int getId() { return id; }
        int getSamplePosition() { return samplePosition; }
        bool isDragging() const { return dragging; }
};

#endif // MARKER_H
