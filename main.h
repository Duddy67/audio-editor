#ifndef MAIN_H
#define MAIN_H
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Multiline_Output.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Group.H>
#include <errno.h>
#include <cstdlib>
#include "dialog_wnd.h"
#include "file_chooser.h"
#include "audio.h"
#include "audio_settings.h"
#include "../libraries/json.hpp"
#define CONFIG_FILENAME "config.json"
#define HEIGHT_MENUBAR 24
#define MODAL_WND_POS 20
#define TINY_SPACE 10
#define SMALL_SPACE 40
#define MEDIUM_SPACE 80
#define LARGE_SPACE 160
#define XLARGE_SPACE 320
#define TEXT_SIZE 13

using json = nlohmann::json;

class Application : public Fl_Double_Window 
{
    Fl_Menu_Bar* menu;
    Fl_Menu_Item* menuItem;
    Fl_Group* toolbar;
    Fl_Multiline_Output *fileInfo;
    Fl_Button* playBtn;
    Fl_Button* stopBtn;
    Fl_Button* pauseBtn;
    DialogWindow* dialogWnd = 0;
    FileChooser *fileChooser = 0;
    AudioSettings *audioSettings = 0;
    Audio *audio = 0;
    std::string message;

    struct AppConfig {
        std::string outputDevice;
        std::string inputDevice;
        //std::string volume;
    };

    public:

        Application(int w, int h, const char* l, int argc, char* argv[]);

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
        void dispayFileInfo(std::map<std::string, std::string> info);

        // Call back functions.
        static void quit_cb(Fl_Widget* w, void* data);
        static void noEscapeKey_cb(Fl_Widget* w, void* data);
        static void dialog_cb(Fl_Widget* w, void* data);
        static void file_chooser_cb(Fl_Widget *w, void *data);
        static void audio_settings_cb(Fl_Widget *w, void *data);
        static void open_cb(Fl_Widget* w, void* data);
        static void save_cb(Fl_Widget* w, void* data);
        static void saveas_cb(Fl_Widget* w, void* data);
        static void cancel_audio_settings_cb(Fl_Widget *w, void *data);
        static void save_audio_settings_cb(Fl_Widget *w, void *data);
        static void ok_cb(Fl_Widget* w, void* data);
        static void cancel_cb(Fl_Widget* w, void* data);
        static void play_cb(Fl_Widget* w, void* data);
        static void stop_cb(Fl_Widget* w, void* data);
        static void pause_cb(Fl_Widget* w, void* data);
};

#endif
