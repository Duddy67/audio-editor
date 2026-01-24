#include "../audio_track.h" // Holds waveform.h
#include "marking.h"

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

void Marker::createMenu()
{
    menu = new Fl_Menu_Button(0, 0, XLARGE_SPACE, SMALL_SPACE, "Marker menu");
    menu->type(Fl_Menu_Button::POPUP3);
    // Create the "delete" menu entry.
    menu->add("Delete", 0, [](Fl_Widget*, void* data) {
                                Marker* marker = static_cast<Marker*>(data);
                                // Delete this marker through parent widget (ie: Marking widget).
                                marker->marking.deleteMarker(marker->id);
                            }, (void*) this);
}

int Marker::handle(int event) {

    switch(event) {
        case FL_PUSH:
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                // Check for double click.
                if (Fl::event_clicks() > 0) {
                    // Create the dialog if it doesn't exist yet.
                    if (renamingDlg == nullptr) {
                        renamingDlg = new RenamingDialog(x(), y(), XLARGE_SPACE, SMALL_SPACE + MEDIUM_SPACE, "Rename marker");
                        renamingDlg->hideScrollbar();
                    }

                    // Set the current name.
                    renamingDlg->setInitialName(label());

                    if (renamingDlg->runModal() == DIALOG_OK) {
                        // Get back the new name as label. 
                        label(renamingDlg->getNewName());
                    }

                    return 1;
                }

                dragging = true;
                dragStartX = Fl::event_x();

                return 1;
            }

            if (Fl::event_button() == FL_RIGHT_MOUSE) {
                // Make sure the popup menu exists.
                if (menu == nullptr) {
                    createMenu();
                }
 
                // Show menu.
                menu->popup();

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
