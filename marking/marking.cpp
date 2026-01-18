#include "marking.h"


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
    Marker* marker = new Marker(0, 0, MARKER_LABEL_WIDTH, 20, "Label", getNewMarkerId(), *this);
    marker->setSamplePosition(samplePosition);
    // Add the new marker widget to the parent marking widget.
    this->add(marker);
    // Store the new marker.
    markers.push_back(marker);

    this->redraw();
}
