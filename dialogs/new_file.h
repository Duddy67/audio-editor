#ifndef NEW_FILE_H
#define NEW_FILE_H

#include <FL/Fl_Radio_Round_Button.H>
#include <string>
#include "dialog.h"


class NewFileDialog : public Dialog {
  public:
      NewFileDialog(int x, int y, int width, int height, const std::string& title);
      int runModalNewFile();

  protected:
      void buildDialog() override;

  private:
      Fl_Radio_Round_Button* stereo = nullptr;
      Fl_Radio_Round_Button* mono = nullptr;
};

#endif
