#include "renaming.h"
#include <iostream>

RenamingDialog::RenamingDialog(int x, int y, int width, int height, const char* title) 
  : Dialog(x, y, width, height, title)
{
    init();
}

void RenamingDialog::buildDialog()
{
    // Create and set up the text editor.
    textEditor = new Fl_Text_Editor(TINY_SPACE, TINY_SPACE, window->w() - (TINY_SPACE * 2), TEXT_SIZE + TINY_SPACE);
    buffer = new Fl_Text_Buffer();
    textEditor->buffer(buffer);

    // Add the Ok/Cancel buttons.
    addDefaultButtons();
}

void RenamingDialog::onOk() {

    Dialog::onOk();
}

void RenamingDialog::setInitialName(const char* name)
{
    buffer->text(name);
    textEditor->buffer(buffer);
}
