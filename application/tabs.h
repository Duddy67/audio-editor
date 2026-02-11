#ifndef TABS_H
#define TABS_H

#include <FL/Fl_Tabs.H>

/*
 * Class that extends the Fl_Tabs widget in order to automatically transfer focus to the
 * child widget (ie: Waveform) within the tab, which is not performed natively by
 * the Fl_Tabs widget.
 */

class Tabs : public Fl_Tabs {
    public:
        Tabs(int X, int Y, int W, int H, const char* L = 0)
            : Fl_Tabs(X, Y, W, H, L) {}

        int handle(int event) override {
            int result = Fl_Tabs::handle(event);

            // After handling tab selection, focus on the current widget.
            if (event == FL_PUSH || event == FL_RELEASE) {
                Fl_Widget* current = value();

                if (current && Fl::focus() != current) {
                    // Transfer focus to child widget.
                    current->take_focus();
                }
            }

            return result;
        }
};

#endif // TABS_H
