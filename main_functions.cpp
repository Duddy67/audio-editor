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

void Application::addDocument(const char *name)
{
    // Height of tab label area.
    const int tabBarHeight = SMALL_SPACE; 

    // Create the group at the correct position relative to the tabs widget
    tabs->begin();

    FixedTabGroup *g = new FixedTabGroup(
        tabs->x(),                // same x as tabs
        tabs->y() + tabBarHeight, // push down for tab bar
        tabs->w(),
        tabs->h() - tabBarHeight,
        nullptr
    );

    g->copy_label(name); // Safe string copy

    // Add content to the tab
    g->begin();
    Fl_Box *b = new Fl_Box(
        g->x() + 10,
        g->y() + 10,
        g->w() - 20,
        g->h() - 20
    );

    b->box(FL_UP_BOX);
    b->copy_label(name); // Safe string copy
    b->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);
    // Only the content box resizes
    g->resizable(b); 
    g->end();
    tabs->end();

    if (documents.size() == 0) {
        tabs->show();
    }

    // Keep track of this document
    documents.push_back(g);

    // Switch to the newly added tab
    tabs->value(g);

    // Force FLTK to recalc layout so the first tab displays properly
    tabs->init_sizes();
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

