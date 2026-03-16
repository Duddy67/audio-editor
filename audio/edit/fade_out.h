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

        void apply(Track& track) override
        {
            // First, save the timeline state.
            previousClips = track.getClips();

            track.splitClip(startSample);
            track.splitClip(endSample);

            for (Clip& clip : track.getClips()) {
                if (clip.getTimelineStart() >= static_cast<size_t>(startSample) && clip.getTimelineStart() < static_cast<size_t>(endSample)) {
                    clip.setFadeOut(true);
                    clip.setFadeOutLength(clip.getLength());
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
        EditID editID() { return EditID::FADE_OUT; }

    private:

        int startSample;
        int endSample;
        std::vector<Clip> previousClips;
};

#endif // FADE_OUt_H
