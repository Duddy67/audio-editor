#include "../main.h"

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

    if (app->newFileDlg == nullptr) {
        app->newFileDlg = new NewFileDialog(app->x() + MODAL_WND_POS, app->y() + MODAL_WND_POS, XLARGE_SPACE, LARGE_SPACE, "New File");
    }

    if (app->newFileDlg->runModal() == DIALOG_OK) {
        auto dialogOptions = app->newFileDlg->getOptions();
        TrackOptions options;
        options.stereo = dialogOptions.stereo;

        try {
            app->addDocument(options);
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to create new document: " << e.what() << std::endl;
        }
    }
}

void Application::settings_cb(Fl_Widget *w, void *data)
{
    Application* app = (Application*) data;

    if (app->settingsDlg == nullptr) {
        app->settingsDlg = new SettingsDialog(app->x() + MODAL_WND_POS, app->y() + MODAL_WND_POS,
                                              XLARGE_SPACE + MEDIUM_SPACE, LARGE_SPACE + MEDIUM_SPACE, "Settings", app);
    }

    if (app->settingsDlg->runModal() == DIALOG_OK) {
        auto config = app->loadConfig(CONFIG_FILENAME);
        config.backend = app->settingsDlg->getBackend().text();
        config.outputDevice = app->settingsDlg->getOutput().text();
        config.inputDevice = app->settingsDlg->getInput().text();
        app->saveConfig(config, CONFIG_FILENAME);
    }
}

void Application::update_vu_cb(void* data)
{
    Application* app = (Application*) data;

    float levelL = app->getEngine().getCurrentLevelL();
    float levelR = app->getEngine().getCurrentLevelR();
    float peakL = app->getEngine().getCurrentPeakL();
    float peakR = app->getEngine().getCurrentPeakR();

    // If playback has stopped, force levels and peaks to decay toward zero.
    if (!app->getActiveDocument().getTrack().isPlaying()) {
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

void Application::insert_marker_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveDocument().getTrack();
            track.getMarking().insertMarker(track.getCurrentSample());
            track.getWaveform().redraw();
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get track: " << e.what() << std::endl;
        }
    }
}

void Application::time_cb(void *data)
{
    Application* app = (Application*) data;

    if (app->tabs->value()) {
        try {
            auto& track = app->getActiveDocument().getTrack();

            if (track.isPlaying()) {
                track.updateTime();
                Fl::repeat_timeout(0.01, time_cb, data); 
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get track: " << e.what() << std::endl;
        }
    }

}

