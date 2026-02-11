#ifndef FADE_IN_H
#define FADE_IN_H

#include <vector>
#include "command.h"

/*
 * Creates a fade in edit command pattern/object.
 */
class FadeIn: public Command {
    public:
        FadeIn(int start, int end)
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

            int length = endSample - startSample;

            // Compute a linear gain ramp going from 0.0 to 1.0.
            for (int i = 0; i < length; ++i) {
                float gain = static_cast<float>(i) / (length - 1);
                int idx = startSample + i;

                // Multiply samples by the newly computed gain ramp.

                // Audio
                track.getLeftSamples()[idx]  *= gain;
                track.getRightSamples()[idx] *= gain;
                // View
                waveform.getLeftSamples()[idx] *= gain;
                waveform.getRightSamples()[idx] *= gain;
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
        EditID editID() { return EditID::FADE_IN; }

    private:

        int startSample;
        int endSample;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // FADE_IN_H
