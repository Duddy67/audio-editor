#include "marking.h"
#include "../audio_track.h"

/*
 * Realigns the marker horizontally. 
 */
void Marker::alignX(int x) 
{
    position(marking.x() + x, marking.y() + (MARKING_AREA_HEIGHT - MARKER_HEIGHT));
}

int Marker::getNewSamplePosition(int newX)
{
    // Get the new sample position out of the new x value, the scroll offset and the zoom level. 
    int samplePos = marking.getWaveform().getScrollOffset() + static_cast<int>((newX - TAB_BORDER_THICKNESS) / marking.getWaveform().getZoomLevel());
    // Clamp within sample range
    samplePos = std::clamp(samplePos, 0, (int)marking.getWaveform().getTrack().getLeftSamples().size() - 1);

    return samplePos;
}

int Marker::handle(int event) {

    switch(event) {
        case FL_PUSH:
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                dragging = true;
                dragStartX = Fl::event_x();

                return 1;
            }

            break;

        case FL_DRAG:
            if (dragging) {
                int newX = x() + (Fl::event_x() - dragStartX);
                int minX = marking.x();
                int maxX = marking.x() + marking.w() - w();
                newX = std::max(minX, std::min(newX, maxX));

                // Update positions.
                position(newX, y());
                dragStartX = Fl::event_x();
                samplePosition = getNewSamplePosition(newX);

                // Update widgets.
                marking.redraw();
                marking.getWaveform().redraw();

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
