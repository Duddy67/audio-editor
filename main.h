#ifndef MAIN_H
#define MAIN_H
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Light_Button.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <errno.h>
#include <cstdlib>
#include "dialog_wnd.h"
#include "tabs.h"
#include "audio_engine.h"
#include "vu_meter.h"
#include "document.h"
#include "audio_settings.h"
#include "dialogs/new_file.h"
#include "constants.h"
#include "../libraries/json.hpp"

using json = nlohmann::json;

class Application : public Fl_Double_Window 
{
    std::vector<Document*> documents;
    Fl_Menu_Bar* menu = nullptr;
    Fl_Menu_Item* menuItem = nullptr;
    Fl_Group* toolbar = nullptr;
    Fl_Multiline_Output* fileInfo = nullptr;
    Fl_Button* playBtn = nullptr;
    Fl_Button* stopBtn = nullptr;
    Fl_Button* pauseBtn = nullptr;
    Fl_Button* recordBtn = nullptr;
    Fl_Light_Button* loopBtn = nullptr;
    DialogWindow* dialogWnd = nullptr;
    NewFileDialog* newFileDlg = nullptr;
    Fl_Native_File_Chooser* fileChooser = nullptr;
    Fl_Group* vuMeters = nullptr;
    VuMeter* vuMeterL = nullptr;
    VuMeter* vuMeterR = nullptr;
    AudioSettings* audioSettings = nullptr;
    AudioEngine* audioEngine = nullptr;
    Tabs* tabs = nullptr;
    std::string message;
    // The number of new documents in tabs.
    unsigned int newDocuments = 0;
    bool loop = false;


    struct AppConfig {
        std::string backend;
        std::string outputDevice;
        std::string inputDevice;
        //std::string volume;
    };

    void initBackend();
    void initDevices();

    public:

        Application(int w, int h, const char* l, int argc, char* argv[]);
        ~Application() {
            delete audioSettings;
            delete audioEngine;
        }

        void createMenu();
        void open(const char* filename);
        void save(const char* filename);
        const char* untitledDefault();
        int isFileExist(const char* filename);
        void saveConfig(const AppConfig& config, const std::string& filename);
        // Function to load configuration from file
        AppConfig loadConfig(const std::string& filename);
        std::string getMessage() { return message; }
        void setMessage(std::string message);
        void displayFileInfo(std::map<std::string, std::string> info);
        void addDocument(const char *name = nullptr);
        void removeDocument(Document* document);
        void removeDocuments();
        std::string truncateText(const std::string &text, int maxWidth, int font, int size);
        size_t getNbDocuments() { return documents.size(); }
        void hideTabs() { tabs->hide(); }
        AudioTrack& getActiveTrack();
        void initAudioSystem();
        AudioEngine& getAudioEngine() { return *audioEngine; }
        VuMeter& getVuMeterL() const { return *vuMeterL; }
        VuMeter& getVuMeterR() const { return *vuMeterR; }
        std::string escapeMenuText(const std::string& input);
        Fl_Button& getButton(const char* name);
        void documentHasChanged(unsigned int trackId);
        unsigned int checkChangedDocuments();
        Document& getDocumentByTrackId(unsigned int trackId);
        void setSupportedFormats();
        void startVuMeters();
        void playTrack(AudioTrack& track);
        void stopTrack(AudioTrack& track);
        void pauseTrack(AudioTrack& track);
        void recordTrack(AudioTrack& track);
        bool isLooped() const { return loop; }
        int handle(int event) override;

        Fl_Tabs* getTabs() const { return tabs; }

        // Call back functions.
        static void quit_cb(Fl_Widget* w, void* data);
        static void noEscapeKey_cb(Fl_Widget* w, void* data);
        static void dialog_cb(Fl_Widget* w, void* data);
        static void file_chooser_cb(Fl_Widget *w, void *data);
        static void audio_settings_cb(Fl_Widget *w, void *data);
        static void open_cb(Fl_Widget* w, void* data);
        static void new_cb(Fl_Widget* w, void* data);
        static void save_cb(Fl_Widget* w, void* data);
        static void saveas_cb(Fl_Widget* w, void* data);
        static void cancel_audio_settings_cb(Fl_Widget *w, void *data);
        static void save_audio_settings_cb(Fl_Widget *w, void *data);
        static void backend_choice_cb(Fl_Widget *w, void *data);
        static void output_choice_cb(Fl_Widget *w, void *data);
        static void ok_cb(Fl_Widget* w, void* data);
        static void cancel_cb(Fl_Widget* w, void* data);
        static void playButton_cb(Fl_Widget* w, void* data);
        static void stopButton_cb(Fl_Widget* w, void* data);
        static void pauseButton_cb(Fl_Widget* w, void* data);
        static void recordButton_cb(Fl_Widget* w, void* data);
        static void loopButton_cb(Fl_Widget* w, void* data);
        static void update_vu_cb(void* data);
};

#endif
