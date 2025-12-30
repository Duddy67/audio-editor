#ifndef NEW_FILE_H
#define NEW_FILE_H

#include <FL/Fl_Round_Button.H>
#include "dialog.h"

class NewFileDialog : public Dialog
{
  public:
      NewFileDialog();

  private:
      Fl_Round_Button* mono = nullptr;
      Fl_Round_Button* stereo = nullptr;

};

#endif
