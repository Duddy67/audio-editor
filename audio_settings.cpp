#include "main.h"

/*
 * Create the setting modal window and its input/output device drop down lists.
 */
void Application::audio_settings_cb(Fl_Widget *w, void *data)
{
    Application* app = (Application*) data;

    // Build the modal window.
    if (app->audioSettings == 0) {
        app->audioSettings = new AudioSettings(app->x() + MODAL_WND_POS, app->y() + MODAL_WND_POS, 400, 200, "Audio Settings");
        app->audioSettings->getSaveButton()->callback(save_audio_settings_cb, app);
        app->audioSettings->getCancelButton()->callback(cancel_audio_settings_cb, app);
    }

    if (!app->audioEngine->isContextInitialized()) {
        std::cerr << "Failed to initialize audio system." << std::endl;
    }
    else {
        // Get the required variables.
        auto outputDevices = app->audioEngine->getOutputDevices();
        auto inputDevices = app->audioEngine->getInputDevices();
        AppConfig config = app->loadConfig(CONFIG_FILENAME);

        int defaultSelec = 0, selection = 0;
        bool found = false;

        for (size_t i = 0; i < outputDevices.size(); ++i) {
            // Create an option for the device.
            app->audioSettings->output->add(outputDevices[i].name.c_str());

            // Check for selection.
            if (outputDevices[i].isDefault) {
                defaultSelec = i;
            }
            else if (config.outputDevice.compare(outputDevices[i].name.c_str()) == 0) {
                selection = i;
                found = true;
            }
        }

        // Set the device selection.
        selection = (found) ? selection : defaultSelec;
        app->audioSettings->output->value(selection);

        // Reset selection variables.
        defaultSelec = 0, selection = 0;
        found = false;

        for (size_t i = 0; i < inputDevices.size(); ++i) {
            // Create an option for the device.
            app->audioSettings->input->add(inputDevices[i].name.c_str());

            // Check for selection.
            if (inputDevices[i].isDefault) {
                defaultSelec = i;
            }
            else if (config.inputDevice.compare(inputDevices[i].name.c_str()) == 0) {
                selection = i;
                found = true;
            }
        }

        // Set the device selection.
        selection = (found) ? selection : defaultSelec;
        app->audioSettings->input->value(selection);
    }

    app->audioSettings->show();
}

void Application::save_audio_settings_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    app->label(app->audioSettings->output->text());

    // Save the new settings in the config file.
    AppConfig config = app->loadConfig(CONFIG_FILENAME);
    config.outputDevice = app->audioSettings->output->text();
    config.inputDevice = app->audioSettings->input->text();
    app->saveConfig(config, CONFIG_FILENAME);
    // Update the newly selected playback device. 
    app->audioEngine->setOutputDevice(config.outputDevice.c_str());

    app->audioSettings->hide();
}

void Application::cancel_audio_settings_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    app->label("Cancel button clicked");
    app->audioSettings->hide();
}
