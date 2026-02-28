#include "../main.h"

/*
 * Maps the transport clicked to the according functions.
 */
void Application::onTransport(TransportID id)
{
    // Check first a tab (ie: document) is active.
    if (tabs->value()) {
        try {
            auto& track = getActiveDocument().getTrack();

            switch (id) {
                case TransportID::PLAY:
                    onPlay(track);
                    break;

                case TransportID::STOP:
                    onStop(track);
                    break;

                case TransportID::PAUSE:
                    onPause(track);
                    break;

                case TransportID::RECORD:
                    onRecord(track);
                    break;

                case TransportID::LOOP:
                    onLoop();
                    break;
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get track: " << e.what() << std::endl;
        }
    }
    else {
        std::cout << "No active document." << std::endl;
    }
}

void Application::onPlay(Track& track)
{
    // Cannot play while recording.
    if (track.isRecording()) {
        return;
    }

    if (!track.isPlaying()) {
        track.play();

        getButton("record").deactivate();
        startVuMeters();

        // Launch timer that updates cursor and time.
        Fl::add_timeout(TIMER_CALLBACK_VALUE, gui_cb, &track); 
    }
}

void Application::onStop(Track& track)
{
    auto& waveform = track.getGUI().getWaveform();

    if (track.isPlaying() || track.isRecording()) {
        bool stoppedRecording = track.isRecording();
        track.stop();

        if (stoppedRecording) {
            waveform.redraw();
            getButton("play").activate();
        }
        else {
            getButton("record").activate();
        }
    }

    track.unpause();
    waveform.resetCursor();
}

void Application::onPause(Track& track)
{
    auto& waveform = track.getGUI().getWaveform();

    if (track.isPlaying()) {
        track.stop();
        track.pause();
    }
    else if (track.isPaused() && !track.isPlaying()) {
        // Resume from where playback paused
        int resumeSample = waveform.getCursorSamplePosition();
        track.setPlaybackSampleIndex(resumeSample);
        track.unpause();
        track.play();
        Fl::add_timeout(TIMER_CALLBACK_VALUE, gui_cb, &track); 
    }
}

void Application::onRecord(Track& track)
{
    // Check the app can record.
    if (!track.isPlaying() && !track.isRecording()) {
        track.record();
        getButton("play").deactivate();
        Fl::add_timeout(TIMER_CALLBACK_VALUE, gui_cb, &track); 
    }
}

void Application::onLoop()
{
    // Toggle the loop flag.
    loop = loop ? false : true;
}

