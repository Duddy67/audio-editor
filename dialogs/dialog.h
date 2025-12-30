#ifndef DIALOG_H
#define DIALOG_H
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <string>
#include "../constants.h"

class Dialog {

  public:
      Dialog(const std::string& title, int width, int height);
      virtual ~Dialog();

      virtual void show();
      virtual void hide();
      void init();

      virtual void onOk() = 0;
      virtual void onCancel();

  protected:
      Fl_Window* window = nullptr;
      Fl_Button* okButton = nullptr;
      Fl_Button* cancelButton = nullptr;

      void addDefaultButtons();
      virtual void buildDialogWindow() = 0;
};

#endif
