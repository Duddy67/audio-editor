#ifndef AUDIO_SETTINGS_H
#define AUDIO_SETTINGS_H
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Choice.H>


class AudioSettings : public Fl_Window 
{
    public:
        Fl_Button* saveBtn;
        Fl_Button* cancelBtn;
        Fl_Choice* input;
        Fl_Choice* output;

        AudioSettings(int x, int y, int w, int h, const char* title = 0) : Fl_Window(x, y, w, h, title)
        {
            saveBtn = new Fl_Button(10, 150, 80, 40, "Save");
            cancelBtn = new Fl_Button(110, 150, 80, 40, "Cancel");
            output = new Fl_Choice(80,10,300,25,"Output:");
            input = new Fl_Choice(80,50,300,25,"Input:");

            end();
            set_modal();
            fullscreen_off();
            show();
        }

        // Getters.
        Fl_Button* getSaveButton()const { return saveBtn; }
        Fl_Button* getCancelButton()const { return cancelBtn; }
        ma_device getOutputDevice() { return outputDevice; }
};

#endif

