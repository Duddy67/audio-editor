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
};

class Marking : public Fl_Box {
        int xPos, yPos, width, height;
        std::vector<std::unique_ptr<Marker>> markers;  

        unsigned int getNewMarkerId();

    public:

        Marking(int X, int Y, int W, int H)
            : Fl_Box(X, Y, W, H, 0) 
        {
            // Compute tab area.
            xPos = X + 10;
            yPos = Y + 10;
            width = W - 20;
            height = H - 100;

            box(FL_DOWN_BOX);
        }

        std::vector<Marker> getMarkers();
        void insertMarker(int position);
};

#endif // MARKING_H
