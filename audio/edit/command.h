#ifndef COMMAND_H
#define COMMAND_H


// Forward declaration.
class Track;
class Waveform;

/*
 * Abstract class all audio edit commands (mute, normalize, fade in...) are built from. 
 */
class Command {
    public:
        virtual ~Command() = default;

        virtual void apply(Track& track) = 0;
        virtual void undo(Track& track) = 0;
        virtual EditID editID() = 0;
};

#endif // COMMAND_H
