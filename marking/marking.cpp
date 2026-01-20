#include "marking.h"
#include "../audio_track.h"


void Marking::init(WaveformView* w)
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
    // Add the new marker widget to the parent marking widget.
    this->add(marker);
    // Store the new marker.
    markers.push_back(marker);

    this->redraw();
}
