#ifndef DELETE_H
#define DELETE_H

#include <vector>
#include "command.h"

/*
 * Creates a delete edit command pattern/object.
 */
class Delete : public Command {
    public:
        Delete(int start, int end)
            : startSample(start), endSample(end) {}

        void apply(Track& track) override
        {
            // First, save the initial state of the track samples.
            backupLeft.assign(track.getLeftSamples().begin() + static_cast<size_t>(startSample),
                              track.getLeftSamples().begin() + static_cast<size_t>(endSample));
            backupRight.assign(track.getRightSamples().begin() + static_cast<size_t>(startSample),
                               track.getRightSamples().begin() + static_cast<size_t>(endSample));

            // Delete the selected samples.
            track.getLeftSamples().erase(track.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                         track.getLeftSamples().begin() + static_cast<size_t>(endSample));
            track.getRightSamples().erase(track.getRightSamples().begin() + static_cast<size_t>(startSample),
                                          track.getRightSamples().begin() + static_cast<size_t>(endSample));

            // The sample vectors used to draw audio waveforms has to be modified as well.
            auto& waveform = track.getWaveform();
            
            waveform.getLeftSamples().erase(waveform.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                            waveform.getLeftSamples().begin() + static_cast<size_t>(endSample));
            waveform.getRightSamples().erase(waveform.getRightSamples().begin() + static_cast<size_t>(startSample),
                                             waveform.getRightSamples().begin() + static_cast<size_t>(endSample));
        }

        void undo(Track& track) override
        {
            // Restore the track samples to their initial state.
            track.getLeftSamples().insert(track.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                          backupLeft.begin(), backupLeft.end());
            track.getRightSamples().insert(track.getRightSamples().begin() + static_cast<size_t>(startSample),
                                           backupRight.begin(), backupRight.end());

            // The sample vectors used to draw audio waveforms has to be restored as well.
            auto& waveform = track.getWaveform();

            waveform.getLeftSamples().insert(waveform.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                             backupLeft.begin(), backupLeft.end());
            waveform.getRightSamples().insert(waveform.getRightSamples().begin() + static_cast<size_t>(startSample),
                                              backupRight.begin(), backupRight.end());

            // Restore the selection as well.
            waveform.setSelectionStartSample(startSample);
            waveform.setSelectionEndSample(endSample);
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::DELETE; }

    private:

        int startSample;
        int endSample;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // DELETE_H
