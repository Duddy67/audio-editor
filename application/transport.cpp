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
    auto& waveform = track.getWaveform();

    // Cannot play while recording.
    if (track.isRecording()) {
        return;
    }

    if (!track.isPlaying()) {
        if (track.isPaused() || track.isEndOfFile() || waveform.selection()) {
            waveform.resetCursor();
            track.resetEndOfFile();
        }

        track.play();

        getButton("record").deactivate();
        startVuMeters();
        // Launch cursor timer.
        Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
        //
        Fl::add_timeout(0.01, time_cb, this); 
    }
    else {
        waveform.resetCursor();
    }
}

void Application::onStop(Track& track)
{
    auto& waveform = track.getWaveform();

    if (track.isPlaying() || track.isRecording()) {
        bool stoppedRecording = track.isRecording();
        track.stop();

        if (stoppedRecording) {
            waveform.setStereoSamples(track.getLeftSamples(), track.getRightSamples());
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
    auto& waveform = track.getWaveform();

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
        Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
    }
}

void Application::onRecord(Track& track)
{
    auto& waveform = track.getWaveform();

    // Check the app can record.
    if (!track.isPlaying() && !track.isRecording()) {
        track.record();
        getButton("play").deactivate();
        Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
    }
}

void Application::onLoop()
{
    // Toggle the loop flag.
    loop = loop ? false : true;
}

