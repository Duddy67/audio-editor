#ifndef FADE_OUT_H
#define FADE_OUT_H

#include <vector>
#include "command.h"

/*
 * Creates a fade out edit command pattern/object.
 */
class FadeOut: public Command {
    public:
        FadeOut(int start, int end)
            : startSample(start), endSample(end) {}

        void apply(Buffer& buffer) override
        {
            // First, save the initial state of the buffer samples.
            backupLeft.assign(buffer.getLeftSamples().begin() + static_cast<size_t>(startSample),
                              buffer.getLeftSamples().begin() + static_cast<size_t>(endSample));
            backupRight.assign(buffer.getRightSamples().begin() + static_cast<size_t>(startSample),
                               buffer.getRightSamples().begin() + static_cast<size_t>(endSample));

            int length = endSample - startSample;

            // Compute a linear gain ramp going from 1.0 to 0.0.
            for (int i = 0; i < length; ++i) {
                float gain = 1.0f - static_cast<float>(i) / (length - 1);
                int idx = startSample + i;

                // Multiply samples by the newly computed gain ramp.
                buffer.getLeftSamples()[idx]  *= gain;
                buffer.getRightSamples()[idx] *= gain;
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
        EditID editID() { return EditID::FADE_OUT; }
        const Selection getSelection() const { return selection; }

    private:

        int startSample;
        int endSample;
        Selection selection;
        std::vector<float> backupLeft;
        std::vector<float> backupRight;
};

#endif // FADE_OUt_H
