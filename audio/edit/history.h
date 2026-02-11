#ifndef HISTORY_H
#define HISTORY_H

#include <vector>
#include "command.h"

// Forward declaration.
class Track;

// Use namespaces as History is a common word and might be used by other classes.
namespace audio {
    namespace edit {

        /*
         * Class through which edit commands are performed and stored
         * as history through the undo and redo stacks.
         */
        class History {
            public:

                void apply(std::unique_ptr<Command> cmd, Track& track) {
                    cmd->apply(track);
                    lastCmdApplied = cmd->editID();
                    // Append the command to the undo stack.
                    undoStack.push(std::move(cmd));
                    // Initialize (or empty) the redo stack. 
                    redoStack = {};
                }

                void undo(Track& track) {
                    if (undoStack.empty()) {
                        return;
                    }

                    auto cmd = std::move(undoStack.top());
                    // Remove the command from the undo stack.
                    undoStack.pop();
                    cmd->undo(track);
                    lastCmdApplied = cmd->editID();
                    // Append the command to the redo stack.
                    redoStack.push(std::move(cmd));
                }

                void redo(Track& track) {
                    if (redoStack.empty()) {
                        return;
                    }

                    auto cmd = std::move(redoStack.top());
                    // Remove the command from the redo stack.
                    redoStack.pop();
                    // Apply the command again.
                    cmd->apply(track);
                    lastCmdApplied = cmd->editID();
                    // Append the command to the undo stack.
                    undoStack.push(std::move(cmd));
                }

                const EditID getLastUndo() { return !undoStack.empty() ? undoStack.top()->editID() : EditID::NONE; }
                const EditID getLastRedo() { return !redoStack.empty() ? redoStack.top()->editID() : EditID::NONE; }

            private:

                std::stack<std::unique_ptr<Command>> undoStack;
                std::stack<std::unique_ptr<Command>> redoStack;
                // To trace the last command applied.
                EditID lastCmdApplied = EditID::NONE;
        };
    }
}

#endif // HISTORY_H

