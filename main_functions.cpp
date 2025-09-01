#include "main.h"
#include <cstdlib>


void Application::saveConfig(const AppConfig& config, const std::string& filename)
{
    json j;
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
        config.outputDevice = "none";
        config.inputDevice = "none";
        //config.volume = "0";
        this->saveConfig(config, filename);
        return config;
    }

    try {
        json j;
        file >> j;

        config.outputDevice = j.value("outputDevice", "none");
        config.inputDevice = j.value("inputDevice", "none");
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
    // First of all, load the audio file.
    auto data = std::make_unique<DocumentData>(*audioEngine);
    if (!data->loadAudioFile(filepath)) {
        std::cerr << "Failed to load file." << std::endl;
        return;
    }

    // Height of tab label area.
    const int tabBarHeight = SMALL_SPACE; 

    // Create the group at the correct position relative to the tabs widget
    tabs->begin();

    auto view = new DocumentView(
        // Same x as tabs.
        tabs->x(),                
        // Push down for tab bar.
        tabs->y() + tabBarHeight, 
        tabs->w(),
        tabs->h() - tabBarHeight,
        data.get()
    );

    // Link the new tab to the tab closure callback function.
    view->when(FL_WHEN_CLOSED);
    view->callback([](Fl_Widget* w, void* userdata) {
        auto* app = static_cast<Application*>(userdata);
        app->removeDocument(static_cast<DocumentView*>(w));
    }, this);

    view->renderWaveform(data->getTrack());

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
    view->copy_label(label.c_str()); 
    tabs->end();

    // It's the first document of the list.
    if (documents.size() == 0) {
        tabs->show();
        // Keep tab height constant.
        tabs->resizable(view);
    }

    // Keep track of this document
    // Store both data + view.
    // documents now owns data (ie: move).
    documents.push_back({std::move(data), view});

    // Switch to the newly added tab
    tabs->value(view);

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

void Application::removeDocument(DocumentView* view)
{
    // Find the entry
    auto it = std::find_if(documents.begin(), documents.end(),
        [view](const DocumentEntry& entry) {
            return entry.view == view;
        }
    );

    if (it != documents.end()) {
        // 1) Delete the view (FLTK takes care if parented properly)
        delete it->view;

        // 2) Model is destroyed automatically when unique_ptr goes out of scope
        documents.erase(it);
    }
}

/*
 * Returns the track corresponding to the active document (ie: tab).
 */
AudioTrack& Application::getActiveTrack()
{
    auto tabs = getTabs();
    auto view = static_cast<DocumentView*>(tabs->value());
    auto data = static_cast<DocumentData*>(view->getData());

    return data->getTrack();
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

