#include "../main.h"
#include <cstdlib>

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

/*
 * Utility function which escapes characters considered as special by FLTK (ie: / & _).
 */
std::string Application::escapeMenuText(const std::string& input) {
    std::string result;

    for (char c : input) {
        if (c == '/' || c == '&' || c == '_') {
            result += '\\';  // FLTK uses backslash for escaping
            result += c;
        }
        else {
            result += c;
        }
    }

    return result;
}

/*
 * Returns the reference of the given button.
 */
Fl_Button& Application::getButton(const char* name)
{
    if (strcmp(name, "record") == 0) {
        return *recordBtn;
    }
    else if (strcmp(name, "play") == 0) {
        return *playBtn;
    }
    else if (strcmp(name, "pause") == 0) {
        return *pauseBtn;
    }
    else if (strcmp(name, "loop") == 0) {
        return *loopBtn;
    }

    throw std::runtime_error("Couldn't find button: ");
}

void Application::startVuMeters()
{
    getVuMeterL().resetDecayTimer();
    getVuMeterR().resetDecayTimer();
    // Launch vu-meter timer.
    Fl::add_timeout(0.05, update_vu_cb, this);
}

int Application::handle(int event) {
    switch (event) {
        case FL_KEYDOWN: {
            //int key = Fl::event_key();

            // Set here possible key handling.

            return 0;
        }

        default:
            return Fl_Double_Window::handle(event);
    }
}

