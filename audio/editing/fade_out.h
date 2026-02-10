#ifndef FADE_OUT_H
#define FADE_OUT_H

#include <vector>
#include "edit_command.h"

class FadeOut: public EditCommand {
    public:
        FadeOut(int start, int end)
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

            for (int i = 0; i < length; ++i) {
                float gain = 1.0f - static_cast<float>(i) / (length - 1);
                int idx = startSample + i;

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
        EditID editID() { return EditID::FADE_OUT; }

    private:

        int startSample;
        int endSample;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // FADE_OUt_H
