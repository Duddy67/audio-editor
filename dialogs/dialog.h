#ifndef DIALOG_H
#define DIALOG_H

#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <string>
#include "../constants.h"

enum DialogResult {DIALOG_CANCEL, DIALOG_OK};

// Abstract class meant to be used as basic widget by dialog windows.
class Dialog {
  public:
      Dialog(int x, int y, int width, int height, const std::string& title);
      virtual ~Dialog();

      void init();
      DialogResult runModal();
      // Show/hide the dialog
      virtual void show();
      virtual void hide();

      // Base lifecycle hooks
      virtual void onOk();      
      virtual void onCancel();      // Optional override

  protected:
      Fl_Window* window = nullptr;
      Fl_Button* okButton = nullptr;
      Fl_Button* cancelButton = nullptr;
      DialogResult result = DIALOG_CANCEL;

      // Helper to position common buttons
      void addDefaultButtons();
      // To be implemented by derived classes
      virtual void buildDialog() = 0; 
      Fl_Window& getWindow() const { return *window; } 
};

#endif
