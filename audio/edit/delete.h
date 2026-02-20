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

        void apply(Buffer& buffer) override
        {
            // First, save the initial state of the buffer samples.
            backupLeft.assign(buffer.getLeftSamples().begin() + static_cast<size_t>(startSample),
                              buffer.getLeftSamples().begin() + static_cast<size_t>(endSample));
            backupRight.assign(buffer.getRightSamples().begin() + static_cast<size_t>(startSample),
                               buffer.getRightSamples().begin() + static_cast<size_t>(endSample));

            // Delete the selected samples.
            buffer.getLeftSamples().erase(buffer.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                         buffer.getLeftSamples().begin() + static_cast<size_t>(endSample));
            buffer.getRightSamples().erase(buffer.getRightSamples().begin() + static_cast<size_t>(startSample),
                                          buffer.getRightSamples().begin() + static_cast<size_t>(endSample));

            // Store the initial selection.
            selection = {startSample, endSample};
        }

        void undo(Buffer& buffer) override
        {
            // Restore the buffer samples to their initial state.
            buffer.getLeftSamples().insert(buffer.getLeftSamples().begin() + static_cast<size_t>(startSample),
                                          backupLeft.begin(), backupLeft.end());
            buffer.getRightSamples().insert(buffer.getRightSamples().begin() + static_cast<size_t>(startSample),
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
