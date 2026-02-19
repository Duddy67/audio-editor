#include "gui.h"
#include "track.h"


void GUI::init(int x, int y, int w, int h)
{
    marking = std::make_unique<Marking>(x, y, w, MARKING_AREA_HEIGHT);
    waveform = std::make_unique<Waveform>(x, y + MARKING_AREA_HEIGHT, w, h - MARKING_AREA_HEIGHT, track, *marking);
    waveform->take_focus();    
    waveform->setStereoMode(track.getBuffer().isStereo());    
    waveform->setStereoSamples(track.getBuffer().getLeftSamples(), track.getBuffer().getRightSamples());
}

// Safe method that copies only new data
bool GUI::getNewSamplesCopy(std::vector<float>& leftCopy, std::vector<float>& rightCopy, size_t& newStartIndex, size_t& newCount)
{
    auto& newDataAvailable = track.getNewDataAvailableFlag();
    if (!newDataAvailable.load(std::memory_order_acquire)) {
        return false;
    }

    // Atomically grab and reset dirty range
    auto& leftSamples = track.getBuffer().getLeftSamples();
    auto& rightSamples = track.getBuffer().getRightSamples();
    size_t start = dirtyStart.exchange(SIZE_MAX, std::memory_order_acq_rel);
    size_t end = dirtyEnd.exchange(0, std::memory_order_acq_rel);
    // Inform Track that more new data can now be handled.
    newDataAvailable.store(false, std::memory_order_release);

    if (start == SIZE_MAX || end <= start || end > leftSamples.size()) {
        return false;
    }

    newStartIndex = start;
    newCount = end - start;

    // Copy or overwrite a new chunk of recorded data.
    leftCopy.assign(leftSamples.begin() + start, leftSamples.begin() + end);
    rightCopy.assign(rightSamples.begin() + start, rightSamples.begin() + end);

    return true;
}
