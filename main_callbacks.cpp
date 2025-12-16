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


void Application::playButton_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    // Check first a tab (ie: document) is active.
    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            app->playTrack(track);
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

void Application::stopButton_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            app->stopTrack(track);
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

void Application::pauseButton_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            app->pauseTrack(track);
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get active track: " << e.what() << std::endl;
        }
    }
}

void Application::recordButton_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveTrack();
            app->recordTrack(track);
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
