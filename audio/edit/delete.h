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

            // Store the initial selection.
            selection = {startSample, endSample};
        }

        void undo(Track& track) override
        {
            // Restore the track samples to their initial state.
            track.getLeftSamples().insert(track.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                          backupLeft.begin(), backupLeft.end());
            track.getRightSamples().insert(track.getRightSamples().begin() + static_cast<size_t>(startSample),
                                           backupRight.begin(), backupRight.end());
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::DELETE; }
        const Selection getSelection() const { return selection; }

    private:

        int startSample;
        int endSample;
        Selection selection;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // DELETE_H
