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
            // First, save the timeline state.
            previousClips = track.getClips();

            track.splitClip(startSample);
            track.splitClip(endSample);

            for (Clip& clip : track.getClips()) {
                if (clip.getTimelineStart() >= static_cast<size_t>(startSample) && clip.getTimelineStart() < static_cast<size_t>(endSample)) {
                    clip.setFadeIn(true);
                    clip.setFadeInLength(clip.getLength());
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
        EditID editID() { return EditID::FADE_IN; }

    private:

        int startSample;
        int endSample;
        std::vector<Clip> previousClips;
};

#endif // FADE_IN_H
