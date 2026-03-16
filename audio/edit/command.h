#ifndef COMMAND_H
#define COMMAND_H

// Forward declaration.
//class Track;

/*
 * Abstract class all audio edit commands (mute, normalize, fade in...) are built from. 
 */
class Command {
    protected:  // Make it accessible to derived classes
        Selection selection;

    public:
        virtual ~Command() = default;

        virtual void apply(Track& track) = 0;
        virtual void undo(Track& track) = 0;
        virtual EditID editID() = 0;
        // Non-virtual getter since all commands behave the same
        const Selection getSelection() const { return selection; }
};

#endif // COMMAND_H
