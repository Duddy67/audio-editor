#include "gui.h"
#include "track.h"


void GUI::init(int x, int y, int w, int h)
{
    marking = std::make_unique<Marking>(x, y, w, MARKING_AREA_HEIGHT);
    waveform = std::make_unique<Waveform>(x, y + MARKING_AREA_HEIGHT, w, h - MARKING_AREA_HEIGHT, track, *marking);
    waveform->take_focus();    
    waveform->initView();
}

