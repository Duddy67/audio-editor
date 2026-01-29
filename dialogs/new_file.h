#ifndef NEW_FILE_H
#define NEW_FILE_H

#include <FL/Fl_Radio_Round_Button.H>
#include <string>
#include "dialog.h"


class NewFileDialog : public Dialog {
  private:
      Fl_Radio_Round_Button* stereo = nullptr;
      Fl_Radio_Round_Button* mono = nullptr;

      struct NewFileOptions {
          bool stereo = true;
      };

      NewFileOptions options;

  public:
      NewFileDialog(int x, int y, int width, int height, const char* title);
      NewFileOptions getOptions() const { return options; }

  protected:
      void buildDialog() override;
      void onOk() override;
};

#endif // NEW_FILE_H
