#ifndef COPY_H
#define COPY_H

#include <vector>
#include "command.h"

/*
 * Creates a mute edit command pattern/object.
 */
class Copy : public Command {
    public:
        Copy(int start, int end)
            : startSample(start), endSample(end) {}

        void apply(Track& track) override
        {
            // First, save the current timeline state.
            previousClips = track.getClips();

            track.splitClip(startSample);
            track.splitClip(endSample);
            // Copy the cut clip region.
            track.copy(startSample, endSample);

            // Store the initial selection.
            selection = {startSample, endSample};
        }

        void undo(Track& track) override
        {
            track.setClips(previousClips);
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::CUT; }

    private:

        int startSample;
        int endSample;
        std::vector<Clip> previousClips;
};

#endif // COPY_H
