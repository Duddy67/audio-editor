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

void Application::play_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    // Check first a tab (ie: document) is active.
    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            auto& waveform = track.getWaveform();

            if (!track.isPlaying()) {
                if (track.isPaused()) {
                    waveform.resetCursor();
                }
                else {
                    waveform.setPlaybackSample(0);
                }

                track.play();
                Fl::add_timeout(0.016, update_cursor_timer_cb, &track);
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

            if (track.isPlaying()) {
                track.stop();
                track.setPlaybackSampleIndex(0);
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
                Fl::add_timeout(0.016, update_cursor_timer_cb, &track);
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

// ---- Timer Callback ----
void Application::update_cursor_timer_cb(void* userdata) {
    auto& track = *(AudioTrack*)userdata;  // Dereference to get reference
    auto& waveform = track.getWaveform();
    // Reads from atomic.
    int sample = track.currentSample();
    waveform.setPlaybackSample(sample);

    // --- Smart auto-scroll ---
    // Auto-scroll the view if cursor gets near right edge

    // pixels from right edge
    int margin = 30;
    float zoom = waveform.getZoomLevel();
    int viewWidth = waveform.w();
    int cursorX = static_cast<int>((sample - waveform.getScrollOffset()) * zoom);

    if (cursorX > viewWidth - margin) {
        int newOffset = sample - static_cast<int>((viewWidth - margin) / zoom);
        waveform.setScrollOffset(newOffset);
    }

    // Assuming left and right channels are the same length.
    int totalSamples = static_cast<int>(track.getLeftSamples().size());

    if (sample < totalSamples && track.isPlaying()) {
        // ~60 FPS
        Fl::repeat_timeout(0.016, update_cursor_timer_cb, &track);
    }
}
