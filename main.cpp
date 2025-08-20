#include "main.h"

Application::Application(int w, int h, const char *l, int argc, char *argv[]) : Fl_Double_Window(w, h, l)
{
    box(FL_DOWN_BOX);
    color((Fl_Color) FL_INACTIVE_COLOR);

    // Create and build the menu.
    menu = new Fl_Menu_Bar(0, 0, w, SMALL_SPACE);
    menu->box(FL_FLAT_BOX);
    createMenu();
    menu->textsize(TEXT_SIZE);
    menuItem = (Fl_Menu_Item *)menu->find_item("Edit/&Toolbar");
    menuItem->clear();

    toolbar = new Fl_Group(0, SMALL_SPACE, w, SMALL_SPACE + (TINY_SPACE * 2));
        toolbar->box(FL_FLAT_BOX);
        // Create buttons.
        playBtn = new Fl_Button(TINY_SPACE, SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "Play");
        stopBtn = new Fl_Button((TINY_SPACE * 2) + MEDIUM_SPACE, SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "Stop");
        pauseBtn = new Fl_Button((TINY_SPACE * 3) + (MEDIUM_SPACE * 2), SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "Pause");

        playBtn->callback(play_cb, this);
        stopBtn->callback(stop_cb, this);
        pauseBtn->callback(pause_cb, this);

        // Disable keyboard focus on buttons
        playBtn->clear_visible_focus();
        stopBtn->clear_visible_focus();
        pauseBtn->clear_visible_focus();
    toolbar->end();

    // Create tabs container
    /*tabs = new ClosableTabs(0, (SMALL_SPACE * 2) + (TINY_SPACE * 2), w, h - SMALL_SPACE);   

    tabs->set_close_callback([](Fl_Widget *tab, void *userdata) {
        Application *app = static_cast<Application*>(userdata);
        app->closeDocument(static_cast<Fl_Group*>(tab));
    }, this);*/

    tabs = new Fl_Tabs(0, (SMALL_SPACE * 2) + (TINY_SPACE * 2), w, h - SMALL_SPACE);   
    tabs->end();
    tabs->hide();

    // Make the window resizable via the tabs widget.
    resizable(tabs);
    // Prevent toolbar from being resized.
    toolbar->resizable(nullptr); 
    // Stop adding children to this window.
    end();
    show();

    this->callback(noEscapeKey_cb, this);

    AppConfig config = loadConfig(CONFIG_FILENAME);

    // Create and initialize the Audio object.
    this->audioEngine = new AudioEngine(this);

    if (!this->audioEngine->isContextInitialized()) {
        setMessage("Failed to initialize audio system.");
        this->dialog_cb(this->dialogWnd, this);
    }

    audioEngine->setOutputDevice(config.outputDevice.c_str());
    audioEngine->start();
    //audio->printAllDevices();
}


int main(int argc, char *argv[])
{
    Application app(1200, 800, "Audio Editor", argc, argv);
    return Fl::run();
}
