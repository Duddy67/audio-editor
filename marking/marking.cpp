#include "marking.h"
#include "../audio/track.h"


void Marking::init(Waveform* w)
{
    if (pWaveform == nullptr) {
        pWaveform = w;
    }
}

unsigned int Marking::getNewMarkerId()
{
    unsigned int highestId = 0;

    for (size_t i = 0; i < markers.size(); i++) {
        highestId = markers[i]->getId() > highestId ? markers[i]->getId() : highestId;
    }

    return highestId + 1;
}

void Marking::insertMarker(int samplePosition)
{
    unsigned int newId = getNewMarkerId();
    Marker* marker = new Marker(0, 0, MARKER_WIDTH, MARKER_HEIGHT, 0, newId, *this);

    std::string label = "Mark " + std::to_string(newId);
    marker->copy_label(label.c_str());
    marker->setSamplePosition(samplePosition);
    float x = (samplePosition - pWaveform->getScrollOffset()) * pWaveform->getZoomLevel();
    marker->alignX((int) x);
    // Add the new marker to the parent widget.
    add(marker);
    // Store the new marker.
    markers.push_back(marker);

    redraw();
}

/*
 * Deletes a marker by the given id.
 */
void Marking::deleteMarker(unsigned int id)
{
  for (size_t i = 0; i < markers.size(); i++) {
      if (id == markers[i]->getId()) {
          // Remove the marker from the parent widget.
          remove(markers[i]);
          // Schedules the marker widget for deletion at the next call to the event loop.
          Fl::delete_widget(markers[i]);
          // Remove the marker from the marker list.
          markers.erase(markers.begin() + i);

          redraw();
          pWaveform->redraw();

          return;
      }
  }
}
