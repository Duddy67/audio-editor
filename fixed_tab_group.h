#include <FL/Fl_Group.H>

/*
 * Used with Fl_Tabs widgets to prevent the tab bar area
 * to get resized by default.
 */
class FixedTabGroup : public Fl_Group {
    int fixed_y;
public:
    FixedTabGroup(int X, int Y, int W, int H, const char *L = 0)
        : Fl_Group(X, Y, W, H, L), fixed_y(Y) {}

    void resize(int X, int Y, int W, int H) override {
        // Ignore Y changes to keep tab bar height fixed
        Fl_Group::resize(X, fixed_y, W, H + (Y - fixed_y));
    }
};

