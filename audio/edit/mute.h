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
            // First, save the timeline state.
            previousClips = track.getClips();

            track.splitClip(startSample);
            track.splitClip(endSample);

            for (Clip& clip : track.getClips()) {
                if (clip.getTimelineStart() >= static_cast<size_t>(startSample) && clip.getTimelineStart() < static_cast<size_t>(endSample)) {
                    clip.setMute(true);
                    clip.setMuteLength(clip.getLength());
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
        EditID editID() { return EditID::MUTE; }
        const Selection getSelection() const { return selection; }

    private:

        int startSample;
        int endSample;
        Selection selection;
        std::vector<Clip> previousClips;
};

#endif // MUTE_H
