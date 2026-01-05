#include "settings.h"
#include "../main.h"

SettingsDialog::SettingsDialog(int x, int y, int width, int height, const char* title, Application* app) 
  : Dialog(x, y, width, height, title), pApplication(app)
{
    init();
}

/*
 * Create the setting's backend and device drop down lists.
 */
void SettingsDialog::buildDialog()
{
    // Drop down list height.
    int height = (TINY_SPACE * 2) + MICRO_SPACE;

    backend = new Fl_Choice(SMALL_SPACE, TINY_SPACE * 3, XLARGE_SPACE, height, "Backend");
    output = new Fl_Choice(SMALL_SPACE, (TINY_SPACE * 2) * 4, XLARGE_SPACE, height, "Output");
    input = new Fl_Choice(SMALL_SPACE, (TINY_SPACE * 2) * 6 + TINY_SPACE, XLARGE_SPACE, height, "Input");
    // Align labels.
    backend->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
    output->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);
    input->align(FL_ALIGN_TOP | FL_ALIGN_LEFT);

    backend->callback([](Fl_Widget*, void* userdata) {
        static_cast<SettingsDialog*>(userdata)->onChangeBackend();
    }, this);

    output->callback([](Fl_Widget*, void* userdata) {
        static_cast<SettingsDialog*>(userdata)->onChangeOutput();
    }, this);

    if (!pApplication->getAudioEngine().isContextInitialized()) {
        std::cerr << "Failed to initialize audio system." << std::endl;
    }
    else {
        buildBackends();
        buildDevices();
    }

    // Add the Ok/Cancel buttons.
    addDefaultButtons();
}

void SettingsDialog::onButtonsCreated() 
{
    // Change the OK button's label.
    okButton->label("Save");   
}

void SettingsDialog::onOk()
{
    save();
    Dialog::onOk();
}

void SettingsDialog::onCancel()
{
    cancel();
    Dialog::onCancel();
}

void SettingsDialog::buildBackends()
{
    // Get the required variables.
    auto backends = pApplication->getAudioEngine().getBackends();
    auto config = pApplication->loadConfig(CONFIG_FILENAME);

    for (size_t i = 0; i < backends.size(); ++i) {
        backend->add(backends[i].name.c_str());

        if (config.backend.compare(backends[i].name.c_str()) == 0) {
            backend->value(i);
        }
    }
}

void SettingsDialog::buildDevices()
{
    auto config = pApplication->loadConfig(CONFIG_FILENAME);

    auto outputDevices = pApplication->getAudioEngine().getOutputDevices();
    int value = 100, defaultDevice = 0;
    std::string escapedName = ""; 

    for (size_t i = 0; i < outputDevices.size(); ++i) {
        // Create an option for the device.
        escapedName = pApplication->escapeMenuText(outputDevices[i].name);
        output->add(escapedName.c_str());

        if (outputDevices[i].isDefault) {
            defaultDevice = i;
        }

        if (config.outputDevice.compare(outputDevices[i].name.c_str()) == 0) {
            value = i;
        }
    }

    value = (value != 100) ? value : defaultDevice;
    output->value(value);

    auto inputDevices = pApplication->getAudioEngine().getInputDevices();
    value = 100, defaultDevice = 0;

    for (size_t i = 0; i < inputDevices.size(); ++i) {
        // Create an option for the device.
        escapedName = pApplication->escapeMenuText(inputDevices[i].name);
        input->add(escapedName.c_str());

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
void SettingsDialog::onChangeBackend() 
{
    //Application* pApplication = (Application*) data;

    try {
        // Reset backend and devices.
        pApplication->getAudioEngine().setBackend(backend->text());
        pApplication->getAudioEngine().setOutputDevice(output->text());
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Backend choice error: " << std::string(e.what()) << std::endl;
        return;
    }

    // Delete the previous device options.
    output->clear();
    input->clear();
    // Rebuild the device options.
    buildDevices();
}

/*
 * The output option has been changed.
 */
void SettingsDialog::onChangeOutput() 
{
    try {
        // Reset the output device.
        pApplication->getAudioEngine().setOutputDevice(output->text());
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Output choice error: " << std::string(e.what()) << std::endl;
        return;
    }

    // Delete the previous output options.
    output->clear();
    // Rebuild the device options.
    buildDevices();

}

void SettingsDialog::save() 
{
    // Save the new settings in the config file.
    auto config = pApplication->loadConfig(CONFIG_FILENAME);
    config.backend = backend->text();
    config.outputDevice = output->text();
    config.inputDevice = input->text();
    pApplication->saveConfig(config, CONFIG_FILENAME);
}

void SettingsDialog::cancel() 
{
    // The backend or devices have been changed.
    // Reset it all to the initial settings set in the config file.
    if (backend->changed() || output->changed()) {
        try {
            pApplication->getAudioEngine().setBackend(backend->text());
            pApplication->getAudioEngine().setOutputDevice(output->text());
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Backend choice error: " << std::string(e.what()) << std::endl;
            return;
        }

        // Delete the previous device options.
        output->clear();
        input->clear();
        // Rebuild the device options.
        buildDevices();
    }
}

