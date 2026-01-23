#ifndef RENAMING_H
#define RENAMING_H

#include <FL/Fl_Text_Editor.H>
#include <string>
#include "dialog.h"


class RenamingDialog : public Dialog {
  private:
      Fl_Text_Editor* textEditor = nullptr;
      Fl_Text_Buffer* buffer = nullptr;

  public:
      RenamingDialog(int x, int y, int width, int height, const char* title);
      void hideScrollbar() { textEditor->scrollbar_size(-1); }
      void setInitialName(const char* name);
      const char* getNewName() { return buffer->text(); }

  protected:
      void buildDialog() override;
      void onOk() override;
};

#endif // RENAMING_H
