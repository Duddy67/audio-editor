#ifndef COMMAND_H
#define COMMAND_H

// Forward declaration.
//class Track;

/*
 * Abstract class all audio edit commands (mute, normalize, fade in...) are built from. 
 */
class Command {
    public:
        virtual ~Command() = default;

        virtual void apply(Buffer& buffer) = 0;
        virtual void undo(Buffer& buffer) = 0;
        virtual EditID editID() = 0;
        virtual const Selection getSelection() const = 0;
};

#endif // COMMAND_H
