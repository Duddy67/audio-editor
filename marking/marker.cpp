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
                // Calculate new position.
                // Fl::event_x() gives current mouse X in screen coordinates.
                // x() is current widget X in screen coordinates.
                // (Fl::event_x() - dragStartX) = how much mouse moved since start.
                int newX = x() + (Fl::event_x() - dragStartX);

                // ----- Constrain movement to parent bounds -----
                int minX = marking.x();
                int maxX = marking.x() + marking.w() - w();
                // Clamp newX between minX and maxX.
                newX = std::max(minX, std::min(newX, maxX));

                // Waveform area width is smaller than main window width. 
                if (newX > (int)marking.getWaveform().getLastDrawnX() + (int)TAB_BORDER_THICKNESS) {
                    // Prevent dragging marker beyond the waveform width.
                    return 1;
                }

                // Update positions.
                position(newX, y());
                samplePosition = getNewSamplePosition(newX);

                // Update dragStart for next FL_DRAG event.
                // This makes movement relative to last position, not initial.
                dragStartX = Fl::event_x();

                // Refresh widgets (ie: schedule widget for repainting).
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
