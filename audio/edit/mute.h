#ifndef MUTE_H
#define MUTE_H

#include <vector>
#include "command.h"

/*
 * Creates a mute edit command pattern/object.
 */
class Mute : public Command {
    public:
        Mute(int start, int end)
            : startSample(start), endSample(end) {}

        void apply(Track& track) override
        {
            // First, save the initial state of the track samples.
            backupLeft.assign(track.getLeftSamples().begin() + static_cast<size_t>(startSample),
                              track.getLeftSamples().begin() + static_cast<size_t>(endSample));
            backupRight.assign(track.getRightSamples().begin() + static_cast<size_t>(startSample),
                               track.getRightSamples().begin() + static_cast<size_t>(endSample));

            // The sample vectors used to draw audio waveforms has to be modified as well.
            auto& waveform = track.getWaveform();

            // Mute samples.
            for (int i = startSample; i < endSample; i++) {
                // Audio
                track.getLeftSamples()[i] = 0.0f;
                track.getRightSamples()[i] = 0.0f;
                // View
                waveform.getLeftSamples()[i] = 0.0f;
                waveform.getRightSamples()[i] = 0.0f;
            }
        }

        void undo(Track& track) override
        {
            // Restore the track samples to their initial state.
            std::copy(backupLeft.begin(), backupLeft.end(),
                      track.getLeftSamples().begin() + static_cast<size_t>(startSample));
            std::copy(backupRight.begin(), backupRight.end(),
                      track.getRightSamples().begin() + static_cast<size_t>(startSample));

            // The sample vectors used to draw audio waveforms has to be restored as well.
            auto& waveform = track.getWaveform();

            std::copy(backupLeft.begin(), backupLeft.end(),
                      waveform.getLeftSamples().begin() + static_cast<size_t>(startSample));
            std::copy(backupRight.begin(), backupRight.end(),
                      waveform.getRightSamples().begin() + static_cast<size_t>(startSample));

            // Restore the selection as well.
            waveform.setSelectionStartSample(startSample);
            waveform.setSelectionEndSample(endSample);
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::MUTE; }

    private:

        int startSample;
        int endSample;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // MUTE_H
