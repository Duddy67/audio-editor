#include "main.h"
#include "audio/edit/mute.h"
#include "audio/edit/fade_in.h"
#include "audio/edit/fade_out.h"
#include <cstdlib>

void Application::initBackend()
{
    auto backends = getEngine().getBackends();
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
    getEngine().setBackend(config.backend.c_str());
}

void Application::initDevices()
{
    auto config = loadConfig(CONFIG_FILENAME);
    unsigned int index = 0;

    // Check for first starting.
    if (config.outputDevice == "") {
        // Privilege duplex devices if available.
        auto duplexDevices = getEngine().getDuplexDevices();

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
            auto outputDevices = getEngine().getOutputDevices();
            for (size_t i = 0; i < outputDevices.size(); ++i) {
                if (outputDevices[i].isDefault) {
                    index = i;
                }
            }

            config.outputDevice = outputDevices[index].name;

            auto inputDevices = getEngine().getInputDevices();
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
    if (config.outputDevice.compare(config.inputDevice) == 0 && getEngine().isDeviceDuplex(config.outputDevice.c_str())) {
        getEngine().setDuplexDevice(config.outputDevice.c_str());
        getEngine().startDuplex();
    }
    // If no duplex device, fall back on standard output/input devices.
    else {
        getEngine().setOutputDevice(config.outputDevice.c_str());
        getEngine().startPlayback();
        getEngine().setInputDevice(config.inputDevice.c_str());
        getEngine().startCapture();
    }
}

void Application::initAudioSystem()
{
    // Create and initialize the audio engine object.
    engine = new Engine(this);

    try {
        initBackend();
        std::cout << "Current backend: " << engine->currentBackend() << std::endl;
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

    // Set sample rate for time computing.
    time->setSampleRate(engine->getDefaultOutputSampleRate());

    //engine->printAllDevices(); // For debug purpose.
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
void Application::addDocument(TrackOptions options)
{
    // Height of tab label area.
    const int tabBarHeight = SMALL_SPACE; 

    // Create the group at the correct position relative to the tabs widget
    tabs->begin();

    auto doc = new Document(
        // Same coordinates as tabs.
        tabs->x(),                
        // Push down for tab bar.
        tabs->y() + tabBarHeight, 
        tabs->w(),
        tabs->h() - tabBarHeight,
        *engine,
        options
    );

    // Link the new tab to the tab closure callback function.
    doc->when(FL_WHEN_CLOSED);

    doc->callback([](Fl_Widget* w, void* userdata) {
        auto* app = static_cast<Application*>(userdata);
        app->removeDocument(static_cast<Document*>(w));
    }, this);

    std::string filename;

    if (options.filepath != nullptr) {
        //filename = std::filesystem::path(filepath).filename().string();
        filename = doc->getFileName();
    }
    else {
        newDocuments++;
        // Create new document as .wav file by default.
        filename = "New document " + std::to_string(newDocuments) + ".wav";
        doc->setFileName(filename.c_str());
    }

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
 * Returns the active document (ie: tab).
 */
Document& Application::getActiveDocument()
{
    auto tabs = getTabs();
    auto document = static_cast<Document*>(tabs->value());

    return *document;
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

Document& Application::getDocumentByTrackId(unsigned int trackId)
{
    for (size_t i = 0; i < documents.size(); i++) {
        if (documents[i]->getTrackId() == trackId) {
            // 
            return *documents[i];
        }
    }

    throw std::runtime_error("Couldn't find document: ");
}

void Application::documentHasChanged(unsigned int trackId)
{
    for (size_t i = 0; i < documents.size(); i++) {
        if (documents[i]->getTrackId() == trackId) {
            // Mark the given document as "changed".
            documents[i]->hasChanged();

            return;
        }
    }
}

unsigned int Application::checkChangedDocuments()
{
    unsigned int changedDocuments = 0;

    for (size_t i = 0; i < documents.size(); i++) {
        if (documents[i]->isChanged()) {
            changedDocuments++;
        }
    }

    return changedDocuments;
}

void Application::startVuMeters()
{
    getVuMeterL().resetDecayTimer();
    getVuMeterR().resetDecayTimer();
    // Launch vu-meter timer.
    Fl::add_timeout(0.05, update_vu_cb, this);
}

void Application::playTrack(Track& track)
{
    auto& waveform = track.getWaveform();

    // Cannot play while recording.
    if (track.isRecording()) {
        return;
    }

    if (!track.isPlaying()) {
        if (track.isPaused() || track.isEndOfFile() || waveform.selection()) {
            waveform.resetCursor();
            track.resetEndOfFile();
        }

        track.play();

        getButton("record").deactivate();
        startVuMeters();
        // Launch cursor timer.
        Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
        //
        Fl::add_timeout(0.01, time_cb, this); 
    }
    else {
        waveform.resetCursor();
    }
}

void Application::stopTrack(Track& track)
{
    auto& waveform = track.getWaveform();

    if (track.isPlaying() || track.isRecording()) {
        bool stoppedRecording = track.isRecording();
        track.stop();

        if (stoppedRecording) {
            waveform.setStereoSamples(track.getLeftSamples(), track.getRightSamples());
            getButton("play").activate();
        }
        else {
            getButton("record").activate();
        }
    }

    track.unpause();
    waveform.resetCursor();
}

void Application::pauseTrack(Track& track)
{
    auto& waveform = track.getWaveform();

    if (track.isPlaying()) {
        track.stop();
        track.pause();
    }
    else if (track.isPaused() && !track.isPlaying()) {
        // Resume from where playback paused
        int resumeSample = waveform.getCursorSamplePosition();
        track.setPlaybackSampleIndex(resumeSample);
        track.unpause();
        track.play();
        Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
    }
}

void Application::recordTrack(Track& track)
{
    auto& waveform = track.getWaveform();

    // Check the app can record.
    if (!track.isPlaying() && !track.isRecording()) {
        track.record();
        getButton("play").deactivate();
        Fl::add_timeout(0.016, waveform.update_cursor_timer_cb, &track);
    }
}

const Selection Application::getSelection(Track& track)
{
    // Get the current selection.
    auto& waveform = track.getWaveform();
    int start = waveform.getSelectionStartSample();
    int end = waveform.getSelectionEndSample();
    int totalSamples = static_cast<int>(track.getLeftSamples().size());

    // Make sure selection is valid.
    if (start >= end || start > totalSamples || end > totalSamples) {
        Selection selection = {0, 0};
        return selection; 
    }

    Selection selection = {start, end};
    return selection;
}

void Application::onUndo(Track& track)
{
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.undo(track);
    auto& waveform = track.getWaveform();
    waveform.redraw();
    std::string label = "";

    // No more edit command left.
    if (audioHistory.getLastUndo() == EditID::NONE) {
        label = MenuLabels[MenuItemID::EDIT_UNDO];
        updateMenuItem(MenuItemID::EDIT_UNDO, Action::DEACTIVATE, label);
    }
    else {
        label = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[audioHistory.getLastUndo()]; 
        updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, label);
    }

    label = MenuLabels[MenuItemID::EDIT_REDO] + " " + EditLabels[audioHistory.getLastRedo()]; 
    updateMenuItem(MenuItemID::EDIT_REDO, Action::ACTIVATE, label);
}

void Application::onRedo(Track& track)
{
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.redo(track);
    auto& waveform = track.getWaveform();
    waveform.redraw();
    std::string label = "";

    if (audioHistory.getLastRedo() == EditID::NONE) {
        label = MenuLabels[MenuItemID::EDIT_REDO];
        updateMenuItem(MenuItemID::EDIT_REDO, Action::DEACTIVATE, label);
    }
    else {
        label = MenuLabels[MenuItemID::EDIT_REDO] + " " + EditLabels[audioHistory.getLastRedo()]; 
        updateMenuItem(MenuItemID::EDIT_REDO, Action::ACTIVATE, label);
    }

    label = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[audioHistory.getLastUndo()]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, label);
}

///// Editing /////

void Application::onMute(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start >= selection.end) {
        return; 
    }

    auto muteCmd = std::make_unique<Mute>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.apply(std::move(muteCmd), track);
    track.getWaveform().redraw();

    //
    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::MUTE]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
}

void Application::onFadeIn(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start == 0 && selection.end == 0) {
        return; 
    }

    auto fadeInCmd = std::make_unique<FadeIn>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.apply(std::move(fadeInCmd), track);
    track.getWaveform().redraw();

    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::FADE_IN]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
}

void Application::onFadeOut(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start >= selection.end) {
        return; 
    }

    // Create a new fade out command process.
    auto fadeOutCmd = std::make_unique<FadeOut>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    // Apply the command.
    audioHistory.apply(std::move(fadeOutCmd), track);
    track.getWaveform().redraw();

    // Update the Undo menu item accordingly.
    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::FADE_OUT]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
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
