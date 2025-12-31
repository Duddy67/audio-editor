#include "dialog.h"

Dialog::Dialog(int x, int y, int width, int height, const std::string& title)
{
    // Create the window that contains the dialog.
    window = new Fl_Window(x, y, width, height, title.c_str());
}

Dialog::~Dialog() = default;

/*
 * Function to be called in the derived class constructor.
 */
void Dialog::init() {
    // Call the function that must be implemented by the derived class.
    buildDialog();
    // Stop adding widgets to the window.
    window->end();
}

/*
 * Adds the Ok and Cancel default buttons to the window.
 */
void Dialog::addDefaultButtons() {
    int y = window->h() - BUTTON_HEIGHT- TINY_SPACE;

    okButton = new Fl_Button(window->w() - 2 * BUTTON_WIDTH - 2 * TINY_SPACE, y, BUTTON_WIDTH, BUTTON_HEIGHT, "OK");
    cancelButton = new Fl_Button(window->w() - BUTTON_WIDTH - TINY_SPACE, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel");

    // Set a callback function for each button.
    okButton->callback([](Fl_Widget*, void* userdata) {
        static_cast<Dialog*>(userdata)->onOk();
    }, this);

    cancelButton->callback([](Fl_Widget*, void* userdata) {
        static_cast<Dialog*>(userdata)->onCancel();
    }, this);
}

DialogResult Dialog::runModal()
{
    window->set_modal();   // make this window modal
    window->show();

    // Process events until the dialog is hidden
    while (window->shown()) {
        Fl::wait();  // handle FLTK events
    }

    //window->clear_modal(); // cleanup modal state
    return result;         // result could be OK, Cancel, etc.
}

void Dialog::onOk() {
    result = DIALOG_OK;
    hide();
}

void Dialog::onCancel() {
    result = DIALOG_CANCEL;
    hide();
}

void Dialog::show() {
    window->show();
}

void Dialog::hide() {
    window->hide();
}

