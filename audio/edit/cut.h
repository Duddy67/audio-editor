#ifndef CUT_H
#define CUT_H

#include <vector>
#include "command.h"

/*
 * Creates a mute edit command pattern/object.
 */
class Cut : public Command {
    public:
        Cut(int start, int end)
            : startSample(start), endSample(end) {}

        void apply(Track& track) override
        {
            // First, save the timeline state.
            previousClips = track.getClips();

            size_t removedLength = endSample - startSample;

            track.splitClip(startSample);
            track.splitClip(endSample);

            track.removeClips(startSample, endSample);

            // Close the gap
            for (Clip& clip : track.getClips()) {
                if (clip.getTimelineStart() >= static_cast<size_t>(endSample)) {
                    clip.setTimelineStart(clip.getTimelineStart() - removedLength);
                }
            }

            // Store the initial selection.
            selection = {startSample, endSample};
        }

        void undo(Track& track) override
        {
            track.setClips(previousClips);
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::CUT; }
        const Selection getSelection() const { return selection; }

    private:

        int startSample;
        int endSample;
        Selection selection;
        std::vector<Clip> previousClips;
};

#endif // CUT_H
