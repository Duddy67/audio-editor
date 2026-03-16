#ifndef PASTE_H
#define PASTE_H

#include <vector>
#include "command.h"

/*
 * Creates a mute edit command pattern/object.
 */
class Paste : public Command {
    public:
        Paste(size_t pos)
            : position(pos) {}

        void apply(Track& track) override
        {
            // First, save the current timeline state.
            previousClips = track.getClips();
            // Ensure clip boundary.
            track.splitClip(position);

            auto& clipboard = track.getClipboard();
            size_t clipboardLength = 0;

            // Compute clipboard duration.
            for (const Clip& clip : clipboard) {
                size_t end = clip.getTimelineStart() + clip.getLength();
                clipboardLength = std::max(clipboardLength, end);
            }

            // Shift existing clips to create space on the time line.
            for (Clip& clip : track.getClips()) {
                if (clip.getTimelineStart() >= position) {
                    clip.setTimelineStart(
                        clip.getTimelineStart() + clipboardLength
                    );
                }
            }

            // Insert clipboard clips.
            for (const Clip& clip : clipboard) {
                Clip newClip = clip;
                newClip.setTimelineStart(position + clip.getTimelineStart());
                track.insertClip(newClip, newClip.getTimelineStart());
            }
        }

        void undo(Track& track) override
        {
            track.setClips(previousClips);
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::PASTE; }

    private:

        size_t position;
        std::vector<Clip> previousClips;
};

#endif // PASTE_H
