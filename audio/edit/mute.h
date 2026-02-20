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

        void apply(Buffer& buffer) override
        {
            // First, save the initial state of the buffer samples.
            backupLeft.assign(buffer.getLeftSamples().begin() + static_cast<size_t>(startSample),
                              buffer.getLeftSamples().begin() + static_cast<size_t>(endSample));
            backupRight.assign(buffer.getRightSamples().begin() + static_cast<size_t>(startSample),
                               buffer.getRightSamples().begin() + static_cast<size_t>(endSample));

            // Mute samples.
            for (int i = startSample; i < endSample; i++) {
                buffer.getLeftSamples()[i] = 0.0f;
                buffer.getRightSamples()[i] = 0.0f;
            }

            // Store the initial selection.
            selection = {startSample, endSample};
        }

        void undo(Buffer& buffer) override
        {
            // Restore the buffer samples to their initial state.
            std::copy(backupLeft.begin(), backupLeft.end(),
                      buffer.getLeftSamples().begin() + static_cast<size_t>(startSample));
            std::copy(backupRight.begin(), backupRight.end(),
                      buffer.getRightSamples().begin() + static_cast<size_t>(startSample));
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::MUTE; }
        const Selection getSelection() const { return selection; }

    private:

        int startSample;
        int endSample;
        Selection selection;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // MUTE_H
