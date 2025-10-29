#include "main.h"

/*
 * Prevents the escape key to close the application. 
 */
void Application::noEscapeKey_cb(Fl_Widget* w, void* data)
{
    // If the escape key is pressed it's ignored.
    if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
        return;
    }

    // Close the application when the "close" button is clicked.
    exit(0);
}

void Application::quit_cb(Fl_Widget* w, void* data)
{
    exit(0);
}

void Application::new_cb(Fl_Widget *w, void *data)
{
    Application* app = (Application*) data;

    try {
        app->addDocument();
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Failed to create new document: " << e.what() << std::endl;
    }
}


void Application::play_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    // Check first a tab (ie: document) is active.
    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            auto& waveform = track.getWaveform();

            if (!track.isPlaying()) {
                if (track.isPaused() || track.isEndOfFile()) {
                    waveform.resetCursor();
                    track.resetEndOfFile();
                }

                track.play();
                Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
            }
            else {
                waveform.resetCursor();
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

void Application::stop_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            auto& waveform = track.getWaveform();

            if (track.isPlaying() || track.isRecording()) {
                bool stoppedRecording = track.isRecording();
                track.stop();
                //track.setPlaybackSampleIndex(0);
                if (stoppedRecording) {
                    //waveform.setStereoMode(track.isStereo());
                    waveform.setStereoSamples(track.getLeftSamples(), track.getRightSamples());
                }
            }

            track.unpause();
            waveform.resetCursor();
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

void Application::pause_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            auto& waveform = track.getWaveform();

            if (track.isPlaying()) {
                track.stop();
                track.pause();
            }
            else if (track.isPaused() && !track.isPlaying()) {
                // Resume from where playback paused
                int resumeSample = waveform.getPlaybackSample();
                track.setPlaybackSampleIndex(resumeSample);
                track.unpause();
                track.play();
                Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

void Application::record_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            auto& waveform = track.getWaveform();

            if (!track.isPlaying() && !track.isRecording()) {
                track.record();
                Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }

    }
}

