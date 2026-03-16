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

            // Store the initial selection.
            selection = {startSample, endSample};
        }

        void undo(Track& track) override
        {
            track.setClips(previousClips);
        }

        // Returns the edit command identifier.
        EditID editID() { return EditID::DELETE; }

    private:

        int startSample;
        int endSample;
        std::vector<Clip> previousClips;
};

#endif // DELETE_H
