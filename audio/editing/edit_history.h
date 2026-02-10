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
            lastCmdApplied = cmd->editID();
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
            lastCmdApplied = cmd->editID();
            redoStack.push(std::move(cmd));
        }

        void redo(Track& track) {
            if (redoStack.empty()) {
                return;
            }

            auto cmd = std::move(redoStack.top());
            // Remove the command from the redo stack.
            redoStack.pop();
            cmd->apply(track);
            lastCmdApplied = cmd->editID();
            // Append the command to the undo stack.
            undoStack.push(std::move(cmd));
        }

        const EditID getLastUndo() { return !undoStack.empty() ? undoStack.top()->editID() : EditID::NONE; }
        const EditID getLastRedo() { return !redoStack.empty() ? redoStack.top()->editID() : EditID::NONE; }

    private:

        std::stack<std::unique_ptr<EditCommand>> undoStack;
        std::stack<std::unique_ptr<EditCommand>> redoStack;
        // To trace the last command applied.
        EditID lastCmdApplied = EditID::NONE;
};

#endif // EDIT_HISTORY_H

