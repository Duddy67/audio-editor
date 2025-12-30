#include "dialog.h"

Dialog::Dialog(const std::string& title, int width, int height)
{
    window = new Fl_Window(width, height, title.c_str());
}

Dialog::~Dialog() = default;

void Dialog::init()
{
    buildDialogWindow();
    addDefaultButtons();
    window->end();
}

void Dialog::addDefaultButtons()
{
    int y = window->h() - BUTTON_HEIGHT - TINY_SPACE;

    okButton = new Fl_Button(window->w() - 2 * BUTTON_WIDTH - 2 * TINY_SPACE, y, BUTTON_WIDTH , BUTTON_HEIGHT, "OK");
    cancelButton = new Fl_Button(window->w() - BUTTON_WIDTH - TINY_SPACE, y, BUTTON_WIDTH, BUTTON_HEIGHT, "Cancel");

    okButton->callback([](Fl_Widget*, void* userdata) {
        static_cast<Dialog*>(userdata)->onOk();
    }, this);

    cancelButton->callback([](Fl_Widget*, void* userdata) {
        static_cast<Dialog*>(userdata)->onCancel();
    }, this);
}

void Dialog::onCancel()
{
    hide();
}

void Dialog::show()
{
    window->show();
}

void Dialog::hide()
{
    window->hide();
}

