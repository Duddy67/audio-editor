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
    // Height of tab label area.
    const int tabBarHeight = SMALL_SPACE; 

    // Create the group at the correct position relative to the tabs widget
    tabs->begin();

    Fl_Group *g = new Fl_Group(
        // Same x as tabs.
        tabs->x(),                
        // Push down for tab bar.
        tabs->y() + tabBarHeight, 
        tabs->w(),
        tabs->h() - tabBarHeight,
        nullptr
    );

    // Link the new tab to the tab closure callback function.
    g->when(FL_WHEN_CLOSED);
    g->callback(close_document_cb, this);

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
    g->copy_label(label.c_str()); 

    // Add content to the tab
    g->begin();
    Fl_Box *b = new Fl_Box(
        g->x() + 10,
        g->y() + 10,
        g->w() - 20,
        g->h() - 20
    );

    b->box(FL_DOWN_BOX);
    b->color(FL_WHITE);
    b->copy_label(filename.c_str()); 
    b->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    // Only the content box resizes
    g->resizable(b); 
    g->end();
    tabs->end();

    // It's the first document of the list.
    if (documents.size() == 0) {
        tabs->show();
        // Keep tab height constant.
        tabs->resizable(g);
    }

    // Keep track of this document
    documents.push_back(g);

    // Switch to the newly added tab
    tabs->value(g);

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

