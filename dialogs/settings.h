#ifndef SETTINGS_H
#define SETTINGS_H

#include <FL/Fl_Choice.H>
#include <string>
#include "dialog.h"

// Forward declarations.
class Application;

class SettingsDialog : public Dialog
{
  public:

      SettingsDialog(int x, int y, int width, int height, const char* title, Application* app);
      Fl_Choice& getBackend() const { return *backend; }
      Fl_Choice& getInput() const { return *input; }
      Fl_Choice& getOutput() const { return *output; }

  private:
      Application* pApplication;
      Fl_Choice* backend = nullptr;
      Fl_Choice* input = nullptr;
      Fl_Choice* output = nullptr;

      void buildBackends();
      void buildDevices();
      void onChangeBackend();
      void onChangeOutput();
      void onButtonsCreated() override;
      void save();
      void cancel();

  protected:
      void buildDialog() override;
      void onOk() override;
      void onCancel() override;
};

#endif


