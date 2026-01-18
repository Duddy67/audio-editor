#include "marking.h"
#include "../audio_track.h"

/*
 * Realigns label horizontally. 
 */
void Marker::xAlign(int x) 
{
    position(marking.x() + x, marking.y() + 20);
    redraw();
}

int Marker::getNewSamplePosition(int newX)
{
    int sample = marking.getWaveform().getScrollOffset() + static_cast<int>(newX / marking.getWaveform().getZoomLevel());
    // Clamp within sample range
    sample = std::clamp(sample, 0, (int)marking.getWaveform().getTrack().getLeftSamples().size() - 1);

std::cout << "newX" << newX << std::endl; // For debog purpose
    if (sample != samplePosition) {
        samplePosition = sample;
    }

    return samplePosition;
}

int Marker::handle(int event) {

    switch(event) {
        case FL_PUSH:
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                dragging = true;
                dragX = Fl::event_x();
            //int newX = x() + (Fl::event_x() - dragX) - TAB_BORDER_THICKNESS;
            //std::cout << "dragX: " << dragX << " newX" << newX << std::endl; // For debog purpose
                return 1;
            }

            break;

        case FL_DRAG:
            if (dragging) {
                int newX = x() + (Fl::event_x() - dragX);
                int minX = marking.x();
                int maxX = marking.x() + marking.w() - w();
                newX = std::max(minX, std::min(newX, maxX));
                position(newX, y());
                dragX = Fl::event_x();
                //dragY = Fl::event_y();
                marking.redraw();
            std::cout << "newX: " << newX << std::endl; // For debog purpose
                //samplePosition = getNewSamplePosition(newX);
                /*getNewSamplePosition(newX);
                marking.getWaveform().redraw();*/

                return 1;
            }

            break;
      
        case FL_RELEASE:
            if (dragging && Fl::event_button() == FL_LEFT_MOUSE) {
                dragging = false;

                return 1;
            }

            break;
    }

    return Fl_Box::handle(event);
}
