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

    Application* app = (Application*) data;

    unsigned int changedDocuments = app->checkChangedDocuments();

    if (changedDocuments > 0) {
        std::cout << changedDocuments << " changed document(s) !" << std::endl;
    }

    // Close the application when the "close" button is clicked.
    exit(0);
}

void Application::quit_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    unsigned int changedDocuments = app->checkChangedDocuments();

    if (changedDocuments > 0) {
        std::cout << changedDocuments << " changed document(s) !" << std::endl;
    }

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

            // Cannot play while recording.
            if (track.isRecording()) {
                return;
            }

            if (!track.isPlaying()) {
                if (track.isPaused() || track.isEndOfFile()) {
                    waveform.resetCursor();
                    track.resetEndOfFile();
                }

                track.play();

                app->getButton("record").deactivate();
                app->getVuMeterL().resetDecayTimer();
                app->getVuMeterR().resetDecayTimer();

                // Launch needed timers.
                Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
                Fl::add_timeout(0.05, update_vu_cb, app);
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

                if (stoppedRecording) {
                    waveform.setStereoSamples(track.getLeftSamples(), track.getRightSamples());
                    app->getButton("play").activate();
                }
                else {
                    app->getButton("record").activate();
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

            // Check the app can record.
            if (!track.isPlaying() && !track.isRecording()) {
                track.record();
                app->getButton("play").deactivate();
                Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }

    }
}

void Application::update_vu_cb(void* data)
{
    Application* app = (Application*) data;

    float levelL = app->getAudioEngine().getCurrentLevelL();
    float levelR = app->getAudioEngine().getCurrentLevelR();
    float peakL = app->getAudioEngine().getCurrentPeakL();
    float peakR = app->getAudioEngine().getCurrentPeakR();

    // If playback has stopped, force levels and peaks to decay toward zero.
    if (!app->getActiveTrack().isPlaying()) {
        levelL *= 0.9f;
        levelR *= 0.9f;
        // Accumulate vu-meters decay time.
        app->getVuMeterL().decayTimer();
        app->getVuMeterR().decayTimer();

        // After ~1 second, stop updating completely.
        if (app->getVuMeterL().getVuDecayTimer() >= VU_METER_DECAY_TIME && levelL < 0.01f && levelR < 0.01f) {
            app->getVuMeterL().setLevel(0.0f, 0.0f);
            app->getVuMeterR().setLevel(0.0f, 0.0f);

            // Don't repeat timeout anymore.
            return; 
        }
    }

    app->getVuMeterL().setLevel(levelL, peakL);
    app->getVuMeterR().setLevel(levelR, peakR);

    Fl::repeat_timeout(0.05, update_vu_cb, data); // 20 FPS
}
