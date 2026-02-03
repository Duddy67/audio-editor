#ifndef EDIT_COMMAND_H
#define EDIT_COMMAND_H


// Forward declaration.
class Track;
class Waveform;

class EditCommand {
    public:
        virtual ~EditCommand() = default;

        virtual void apply(Track& track) = 0;
        virtual void undo(Track& track) = 0;
};

#endif // EDIT_COMMAND_H
