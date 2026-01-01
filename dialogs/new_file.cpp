#include "new_file.h"

NewFileDialog::NewFileDialog(int x, int y, int width, int height, const std::string& title) 
  : Dialog(x, y, width, height, title.c_str())
{
    init();
}

void NewFileDialog::buildDialog()
{
    // Create the radio button options.
    stereo = new Fl_Radio_Round_Button(MODAL_WND_POS, MODAL_WND_POS, BUTTON_WIDTH, BUTTON_HEIGHT, "Stereo");
    mono = new Fl_Radio_Round_Button(MODAL_WND_POS, MODAL_WND_POS + BUTTON_HEIGHT, BUTTON_WIDTH, BUTTON_HEIGHT, "Mono");
    // Set stereo option as default.
    stereo->set();

    // Add the Ok/Cancel buttons.
    addDefaultButtons();
}

void NewFileDialog::onOk() {
    // Set the option values chosen by the user (ie: stereo/mono).
    options.stereo = stereo->value() ? true : false;

    Dialog::onOk();
}

