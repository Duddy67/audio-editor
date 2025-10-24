#include "main.h"
#include <cstdlib>

void Application::initBackend()
{
    auto backends = getAudioEngine().getBackends();
    auto config = loadConfig(CONFIG_FILENAME);
    unsigned int index = 0;

    // Check for first starting.
    if (config.backend == "") {
        // Search for JACK or the backend set by the system by default.
        // If nothing found, the first backend on the list will be used (ie: index 0).
        for (size_t i = 0; i < backends.size(); ++i) {
            // If JACK is available, use it.
            if (backends[i].name == "JACK") {
                index = i;
                break;
            }

            if (backends[i].isDefault) {
                index = i;
            }
        }

        // Update setting.
        config.backend = backends[index].name;
        saveConfig(config, CONFIG_FILENAME);
    }

    // Initialize backend.
    getAudioEngine().setBackend(config.backend.c_str());
}

void Application::initDevices()
{
    auto config = loadConfig(CONFIG_FILENAME);
    unsigned int index = 0;

    // Check for first starting.
    if (config.outputDevice == "") {
        // Privilege duplex devices if available.
        auto duplexDevices = getAudioEngine().getDuplexDevices();

        if (duplexDevices.size() != 0) {
            // Search for the device set by the system by default.
            // If no found, the first device on the list (ie: index 0) will be used.
            for (size_t i = 0; i < duplexDevices.size(); ++i) {
                if (duplexDevices[i].isDefault) {
                    index = i;
                }
            }

            config.outputDevice = duplexDevices[index].name;
            config.inputDevice = duplexDevices[index].name;
        }
        else {
            auto outputDevices = getAudioEngine().getOutputDevices();
            for (size_t i = 0; i < outputDevices.size(); ++i) {
                if (outputDevices[i].isDefault) {
                    index = i;
                }
            }

            config.outputDevice = outputDevices[index].name;

            auto inputDevices = getAudioEngine().getInputDevices();
            index = 0;
            for (size_t i = 0; i < inputDevices.size(); ++i) {
                if (inputDevices[i].isDefault) {
                    index = i;
                }
            }

            config.inputDevice = inputDevices[index].name;
        }

        saveConfig(config, CONFIG_FILENAME);
    }

    // Check first if the selected device is duplex. 
    // Note: Don't use PulseAudio duplex device as it malfunctions (hashed sound, stuttering...).
    if (getAudioEngine().isDeviceDuplex(config.outputDevice.c_str()) && getAudioEngine().currentBackend() != "PulseAudio") {
        getAudioEngine().setDuplexDevice(config.outputDevice.c_str());
        getAudioEngine().startDuplex();
    }
    // If no duplex device, fall back on standard output/input devices.
    else {
        getAudioEngine().setOutputDevice(config.outputDevice.c_str());
        getAudioEngine().startPlayback();
        getAudioEngine().setInputDevice(config.inputDevice.c_str());
        getAudioEngine().startCapture();
    }
}

void Application::initAudioSystem()
{
    // Create and initialize the audio engine object.
    audioEngine = new AudioEngine(this);

    try {
        initBackend();
        std::cout << "Current backend: " << audioEngine->currentBackend() << std::endl;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Backend error: " << std::string(e.what()) << std::endl;
        return;
    }

    // Get the initialized config file.
    auto config = loadConfig(CONFIG_FILENAME);

    // Initialize devices.
    try {
        initDevices();
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Device error: " << std::string(e.what()) << std::endl;
        return;
    }

    audioEngine->printAllDevices();

    std::cout << "=== Audio system initialized ===" << std::endl;
}

void Application::saveConfig(const AppConfig& config, const std::string& filename)
{
    json j;
    j["backend"] = config.backend;
    j["outputDevice"] = config.outputDevice;
    j["inputDevice"] = config.inputDevice;
    //j["volume"] = config.volume;

    std::ofstream file(filename);
    file << j.dump(4); // Pretty print with 4 spaces indentation
    std::cout << "Configuration saved to " << filename << std::endl;

}

Application::AppConfig Application::loadConfig(const std::string& filename)
{
    AppConfig config;
    std::ifstream file(filename);

    // If no config file is found, create it.
    if (!file.is_open()) {
        config.backend = "";
        config.outputDevice = "";
        config.inputDevice = "";
        //config.volume = "0";
        this->saveConfig(config, filename);
        return config;
    }

    try {
        json j;
        file >> j;

        config.backend = j.value("backend", "");
        config.outputDevice = j.value("outputDevice", "");
        config.inputDevice = j.value("inputDevice", "");
        //config.volume = j.value("volume", "0");
    }
    catch (const json::exception& e) {
        setMessage("Error parsing config: " + std::string(e.what()));
    }

    return config;
}

/*
 * Adds a new document (ie: audio track + waveform) to edit. 
 */
void Application::addDocument(const char *filepath)
{
    // Height of tab label area.
    const int tabBarHeight = SMALL_SPACE; 

    // Create the group at the correct position relative to the tabs widget
    tabs->begin();

    auto doc = new Document(
        // Same x as tabs.
        tabs->x(),                
        // Push down for tab bar.
        tabs->y() + tabBarHeight, 
        tabs->w(),
        tabs->h() - tabBarHeight,
        *audioEngine,
        filepath
    );

    // Link the new tab to the tab closure callback function.
    doc->when(FL_WHEN_CLOSED);

    doc->callback([](Fl_Widget* w, void* userdata) {
        auto* app = static_cast<Application*>(userdata);
        app->removeDocument(static_cast<Document*>(w));
    }, this);

    std::string filename = std::filesystem::path(filepath).filename().string();
    // SMALL_SPACE + MEDIUM_SPACE = label max width.
    std::string label = truncateText(filename, SMALL_SPACE + MEDIUM_SPACE, FL_HELVETICA, 12); 

    // Add a tiny space on the left between the close button and the label.
    label = " " + label;
    // Pad with spaces so FLTK makes the tab ~ (SMALL_SPACE * 2) + MEDIUM_SPACE wide
    while (fl_width(label.c_str()) < (SMALL_SPACE * 2) + MEDIUM_SPACE) {
        label += " ";
    }

    // Safe string copy
    doc->copy_label(label.c_str()); 
    tabs->end();

    // It's the first document of the list.
    if (documents.size() == 0) {
        tabs->show();
        // Keep tab height constant.
        tabs->resizable(doc);
    }

    // Keep track of this document
    // documents now owns data (ie: move).
    documents.push_back(doc);

    // Switch to the newly added tab
    tabs->value(doc);

    // Force FLTK to recalc layout so the first tab displays properly
    tabs->init_sizes();
}

std::string Application::truncateText(const std::string &text, int maxWidth, int font, int size) {
    fl_font(font, size);

    int w, h;
    fl_measure(text.c_str(), w, h, 0);

    if (w <= maxWidth) {
        // Fits fine.
        return text; 
    }

    std::string truncated = text;
    // Loop through the string until it fits the given width.
    while (!truncated.empty()) {
        // Erase the last character of the string.
        truncated.pop_back();
        // Add 3 dots at the end of the string.
        std::string test = truncated + "...";
        fl_measure(test.c_str(), w, h, 0);

        // Check new string's width.
        if (w <= maxWidth) {
            return test;
        }
    }

    return "...";
}

void Application::removeDocument(Document* document)
{
    auto it = std::find(documents.begin(), documents.end(), document);

    if (it != documents.end()) {
        // Remove the track from the track list. 
        (*it)->removeTrack();

        // Check the document widget is FLTK parented.
        // If not, memory must be freed here.
        if (!(*it)->parent()) {
            delete *it;
        }

        // Remove the document from tabs
        tabs->remove(*it);
        // then from the vector.
        documents.erase(it);

        // No document left in the tab list.
        if (documents.size() == 0) {
            tabs->hide();
        }
    }
}

void Application::removeDocuments()
{

}

/*
 * Returns the track corresponding to the active document (ie: tab).
 */
AudioTrack& Application::getActiveTrack()
{
    auto tabs = getTabs();
    auto document = static_cast<Document*>(tabs->value());

    return document->getTrack();
}

void Application::setMessage(std::string message)
{
    this->message = message;
}

void Application::displayFileInfo(std::map<std::string, std::string> info)
{
    // Clear the previous display.
    fileInfo->value("");
    // Concatenate data to display.
    std::string concat = "File name: " + info["fileName"] + "\n";
    concat = concat + "Channels: " + info["outputChannels"] + "\n";
    concat = concat + "Sample rate: " + info["outputSampleRate"] + "\n";
    concat = concat + "Format: " + info["outputFormat"];
    // Display info.
    fileInfo->value(concat.c_str());
}

