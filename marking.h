#ifndef MARKING_H
#define MARKING_H

#include <filesystem>
#include <vector>
#include <iostream>
#include <FL/Fl_Box.H>


struct Marker {
    std::string name;
    unsigned int id = 0;
    int position = -1;
    //Fl_Box* label = nullptr;
};

class Marking : public Fl_Box {
        std::vector<Marker> markers;  

        unsigned int getNewMarkerId();

    public:

        Marking(int X, int Y, int W, int H)
            : Fl_Box(X, Y, W, H, 0) 
        {
            Fl_Box* label = new Fl_Box(X + 10, Y + 10, 100, 20, "label");
            label->color(FL_GREEN);
            label->box(FL_DOWN_BOX);
            box(FL_DOWN_BOX);
        }

        std::vector<Marker> getMarkers() { return markers; }
        void insertMarker(int position);
        void deleteMarker(unsigned int id);
};

#endif // MARKING_H
