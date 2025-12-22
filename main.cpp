#include "main.h"

/*
 * Application's constructor.
 * Build the UI part of the application (windows, buttons...) through FLTK.   
 */
Application::Application(int w, int h, const char *l, int argc, char *argv[]) : Fl_Double_Window(w, h, l)
{
    box(FL_DOWN_BOX);
    color((Fl_Color) FL_INACTIVE_COLOR);

    // Create and build the menu.
    menu = new Fl_Menu_Bar(0, 0, w, SMALL_SPACE);
    menu->box(FL_THIN_UP_BOX);
    createMenu();
    menu->textsize(TEXT_SIZE);
    menuItem = (Fl_Menu_Item *)menu->find_item("Edit/&Toolbar");
    menuItem->clear();

    toolbar = new Fl_Group(0, SMALL_SPACE, w, SMALL_SPACE + (TINY_SPACE * 2));
        toolbar->box(FL_FLAT_BOX);
        // Create buttons.
        playBtn = new Fl_Button(TINY_SPACE, SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "@>");
        stopBtn = new Fl_Button((TINY_SPACE * 2) + MEDIUM_SPACE, SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "@square");
        pauseBtn = new Fl_Button((TINY_SPACE * 3) + (MEDIUM_SPACE * 2), SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "@||");
        recordBtn = new Fl_Button((TINY_SPACE * 4) + (MEDIUM_SPACE * 3), SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "@circle");
        loopBtn = new Fl_Light_Button((TINY_SPACE * 5) + (MEDIUM_SPACE * 4), SMALL_SPACE + TINY_SPACE, MEDIUM_SPACE, SMALL_SPACE, "@reload");
        loopBtn->selection_color(FL_GREEN);
        // Set the loop button shortcut to the L key (ie: numeric code = 108).
        loopBtn->shortcut(108);

        playBtn->callback(playButton_cb, this);
        stopBtn->callback(stopButton_cb, this);
        pauseBtn->callback(pauseButton_cb, this);
        recordBtn->callback(recordButton_cb, this);
        loopBtn->callback(loopButton_cb, this);

        // Disable keyboard focus on buttons
        playBtn->clear_visible_focus();
        stopBtn->clear_visible_focus();
        pauseBtn->clear_visible_focus();
        recordBtn->clear_visible_focus();
        loopBtn->clear_visible_focus();

        // Create the vu-meters container.
        vuMeters = new Fl_Group((TINY_SPACE * 6) + (MEDIUM_SPACE * 5), SMALL_SPACE + MICRO_SPACE, XLARGE_SPACE + (TINY_SPACE * 2), SMALL_SPACE + MICRO_SPACE);
            vuMeters->box(FL_UP_BOX);
            // Create stereo vu-meters.
            vuMeterL = new VuMeter((TINY_SPACE * 7) + (MEDIUM_SPACE * 5), SMALL_SPACE + TINY_SPACE + MICRO_SPACE, XLARGE_SPACE, TINY_SPACE);
            vuMeterR = new VuMeter((TINY_SPACE * 7) + (MEDIUM_SPACE * 5), SMALL_SPACE + (TINY_SPACE * 2) + TINY_SPACE, XLARGE_SPACE, TINY_SPACE);
            vuMeterL->type(FL_HORIZONTAL);
            vuMeterR->type(FL_HORIZONTAL);
        vuMeters->end();
    toolbar->end();

    // Create tabs container
    tabs = new Tabs(0, (SMALL_SPACE * 2) + (TINY_SPACE * 2), w, h - SMALL_SPACE);   
    tabs->end();
    tabs->hide();

    // Make the window resizable via the tabs widget.
    resizable(tabs);
    // Prevent toolbar (and its children) from being resized.
    toolbar->resizable(nullptr); 
    // Stop adding children to this window.
    end();
    show();

    this->callback(noEscapeKey_cb, this);
}


int main(int argc, char *argv[])
{
    Application app(1200, 800, "Audio Editor", argc, argv);
    app.initAudioSystem();

    return Fl::run();
}
