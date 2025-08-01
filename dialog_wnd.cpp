#include "main.h"

void Application::dialog_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    if (app->dialogWnd == 0) {
        app->dialogWnd = new DialogWindow(app->x() + MODAL_WND_POS, app->y() + MODAL_WND_POS, 400, 200, "Dialog Window");
        app->dialogWnd->getOkButton()->callback(ok_cb, app);
        app->dialogWnd->getCancelButton()->callback(cancel_cb, app);
    }

    app->dialogWnd->show();
}

void Application::ok_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    app->label("Ok button clicked");
    app->dialogWnd->hide();
}

void Application::cancel_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    app->label("Cancel button clicked");
    app->dialogWnd->hide();
}

