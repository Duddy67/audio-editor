#ifndef DIALOG_WND_H
#define DIALOG_WND_H
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>


class DialogWindow : public Fl_Window 
{
    public:
        Fl_Button* okBtn;
        Fl_Button* cancelBtn;

        DialogWindow(int x, int y, int w, int h, const char* title = 0) : Fl_Window(x, y, w, h, title)
        {
            okBtn = new Fl_Button(10, 150, 80, 40, "Ok");
            cancelBtn = new Fl_Button(110, 150, 80, 40, "Cancel");

            end();
            set_modal();
            fullscreen_off();
            show();
        }

        // Getters.
        Fl_Button* getOkButton()const { return okBtn; }
        Fl_Button* getCancelButton()const { return cancelBtn; }

};

#endif
