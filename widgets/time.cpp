#include "time.h"


void Time::update(uint64_t sample) 
{
    TimePosition t {sample, sampleRate};
    currentSample = sample;
    char buffer[32];

    switch (timeFormat) {
      case TimeFormat::HH_MM_SS_SSS:
            snprintf(buffer, sizeof(buffer), "%02lu h %02lu m %02lu.%03lu s\n",
                   t.hours(),
                   t.minutes(),
                   t.secondsPart(),
                   t.milliseconds());
            break;

      case TimeFormat::MM_SS_SSS:
            snprintf(buffer, sizeof(buffer), "%02lu m %02lu.%03lu s\n",
                   t.minutes(),
                   t.secondsPart(),
                   t.milliseconds());
            break;

      case TimeFormat::SS_SSS:
            snprintf(buffer, sizeof(buffer), "%02lu.%03lu s\n",
                   t.secondsPart(),
                   t.milliseconds());
            break;
    }

    const char* result = buffer;
    copy_label(result);

    redraw();
}

void Time::createMenu()
{
    menu = new Fl_Menu_Button(0, 0, XLARGE_SPACE, SMALL_SPACE, "Time menu");
    menu->type(Fl_Menu_Button::POPUP3);

    // Create the time menu entries.
    menu->add("hours:minutes:seconds:millisesconds", 0, [](Fl_Widget*, void* data) {
                                Time* time = static_cast<Time*>(data);
                                // 
                                time->setFormat(TimeFormat::HH_MM_SS_SSS);
                            }, (void*) this);
    menu->add("minutes:seconds:millisesconds", 0, [](Fl_Widget*, void* data) {
                                Time* time = static_cast<Time*>(data);
                                // 
                                time->setFormat(TimeFormat::MM_SS_SSS);
                            }, (void*) this);
    menu->add("seconds:millisesconds", 0, [](Fl_Widget*, void* data) {
                                Time* time = static_cast<Time*>(data);
                                // 
                                time->setFormat(TimeFormat::SS_SSS);
                            }, (void*) this);
}

void Time::setFormat(TimeFormat format)
{
    timeFormat = format;
    update(currentSample);
}

int Time::handle(int event) 
{
    switch(event) {
        case FL_PUSH:
            if (Fl::event_button() == FL_RIGHT_MOUSE) {
                // Make sure the popup menu exists.
                if (menu == nullptr) {
                    createMenu();
                }
 
                // Show menu.
                menu->popup();

                return 1;
            }

            break;
    }

    return Fl_Box::handle(event);
}

