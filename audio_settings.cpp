#include "main.h"

/*
 * Create the setting modal window and its backend and device drop down lists.
 */
void Application::audio_settings_cb(Fl_Widget *w, void *data)
{
    Application* app = (Application*) data;

    // Build the modal audio settings window.
    if (app->audioSettings == nullptr) {
        app->audioSettings = new AudioSettings(app->x() + MODAL_WND_POS, app->y() + MODAL_WND_POS, 400, 200, "Audio Settings");
        app->audioSettings->getSaveButton()->callback(save_audio_settings_cb, app);
        app->audioSettings->getCancelButton()->callback(cancel_audio_settings_cb, app);
        app->audioSettings->getBackendChoice()->callback(backend_choice_cb, app);
    }

    if (!app->audioEngine->isContextInitialized()) {
        std::cerr << "Failed to initialize audio system." << std::endl;
    }
    else {
        // Get the required variables.
        auto backends = app->audioEngine->getBackends();
        auto config = app->loadConfig(CONFIG_FILENAME);

        for (size_t i = 0; i < backends.size(); ++i) {
            app->audioSettings->backend->add(backends[i].name.c_str());

            if (config.backend.compare(backends[i].name.c_str()) == 0) {
                app->audioSettings->backend->value(i);
            }
        }

        app->audioSettings->buildDevices(data);
    }

    app->audioSettings->show();
}

void AudioSettings::buildDevices(void* data)
{
    Application* app = (Application*) data;
    auto config = app->loadConfig(CONFIG_FILENAME);

    auto outputDevices = app->getAudioEngine().getOutputDevices();
    int value = 100, defaultDevice = 0;

    for (size_t i = 0; i < outputDevices.size(); ++i) {
        // Create an option for the device.
        output->add(outputDevices[i].name.c_str());

        if (outputDevices[i].isDefault) {
            defaultDevice = i;
        }

        if (config.outputDevice.compare(outputDevices[i].name.c_str()) == 0) {
            value = i;
        }
    }

    value = (value != 100) ? value : defaultDevice;
    output->value(value);

    auto inputDevices = app->getAudioEngine().getInputDevices();
    value = 100, defaultDevice = 0;

    for (size_t i = 0; i < inputDevices.size(); ++i) {
        // Create an option for the device.
        input->add(inputDevices[i].name.c_str());

        if (inputDevices[i].isDefault) {
            defaultDevice = i;
        }

        if (config.inputDevice.compare(inputDevices[i].name.c_str()) == 0) {
            value = i;
        }
    }

    value = (value != 100) ? value : defaultDevice;
    input->value(value);
}

/*
 * The backend option has been changed.
 */
void Application::backend_choice_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    try {
        // Reset backend and devices.
        app->getAudioEngine().setBackend(app->audioSettings->backend->text());
        app->getAudioEngine().setOutputDevice(app->audioSettings->output->text());
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Backend choice error: " << std::string(e.what()) << std::endl;
        return;
    }

    // Delete the previous device options.
    app->audioSettings->output->clear();
    app->audioSettings->input->clear();
    // Rebuild the device options.
    app->audioSettings->buildDevices(data);
}

/*
 * The output option has been changed.
 */
void Application::output_choice_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    try {
        // Reset the output device.
        app->getAudioEngine().setOutputDevice(app->audioSettings->output->text());
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Output choice error: " << std::string(e.what()) << std::endl;
        return;
    }

    // Delete the previous output options.
    app->audioSettings->output->clear();
    // Rebuild the device options.
    app->audioSettings->buildDevices(data);
}

void Application::save_audio_settings_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    // Save the new settings in the config file.
    auto config = app->loadConfig(CONFIG_FILENAME);
    config.backend = app->audioSettings->backend->text();
    config.outputDevice = app->audioSettings->output->text();
    config.inputDevice = app->audioSettings->input->text();
    app->saveConfig(config, CONFIG_FILENAME);

    app->audioSettings->hide();
}

void Application::cancel_audio_settings_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    // The backend or devices have been changed.
    // Reset it all to the initial settings set in the config file.
    if (app->audioSettings->backend->changed() || app->audioSettings->output->changed()) {
        try {
            app->getAudioEngine().setBackend(app->audioSettings->backend->text());
            app->getAudioEngine().setOutputDevice(app->audioSettings->output->text());
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Backend choice error: " << std::string(e.what()) << std::endl;
            return;
        }

        // Delete the previous device options.
        app->audioSettings->output->clear();
        app->audioSettings->input->clear();
        // Rebuild the device options.
        app->audioSettings->buildDevices(data);
    }

    app->audioSettings->hide();
}

