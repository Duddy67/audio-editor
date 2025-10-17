#include "main.h"
#include <cstdlib>

void Application::initAudioSystem()
{
    // Create and initialize the audio engine object.
    audioEngine = new AudioEngine(this);

    auto config = loadConfig(CONFIG_FILENAME);
    // Get the backend name set in the config file.
    auto backend = config.backend;

    // Initialize backend.
    try {
        audioEngine->setBackend(backend.c_str());
        std::cout << "Current backend: " << audioEngine->currentBackend() << std::endl;
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Backend error: " << std::string(e.what()) << std::endl;
        return;
    }

    auto outputs = audioEngine->getOutputDevices();

    // Initialize devices.
    try {
        audioEngine->setOutputDevice(config.outputDevice.c_str());
        audioEngine->start();
        audioEngine->printAllDevices();
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Device error: " << std::string(e.what()) << std::endl;
        return;
    }

    if (backend == "") {
        config.backend = audioEngine->currentBackend();
        config.outputDevice = audioEngine->currentOutput();
    }

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

