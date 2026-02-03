#ifndef EDIT_HISTORY_H
#define EDIT_HISTORY_H

#include <vector>
#include "edit_command.h"

// Forward declaration.
class Track;

class EditHistory {
    public:

        void apply(std::unique_ptr<EditCommand> cmd, Track& track) {
            cmd->apply(track);
            undoStack.push(std::move(cmd));
            redoStack = {};
        }

        void undo(Track& track) {
            if (undoStack.empty()) {
                return;
            }

            auto cmd = std::move(undoStack.top());
            undoStack.pop();
            cmd->undo(track);
            redoStack.push(std::move(cmd));
        }

    private:

        std::stack<std::unique_ptr<EditCommand>> undoStack;
        std::stack<std::unique_ptr<EditCommand>> redoStack;
};

#endif // EDIT_HISTORY_H

