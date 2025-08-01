#include "main.h"

/*
 * Prevents the escape key to close the application. 
 */
void Application::noEscapeKey_cb(Fl_Widget* w, void* data)
{
    // If the escape key is pressed it's ignored.
    if (Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape) {
        return;
    }

    // Close the application when the "close" button is clicked.
    exit(0);
}

void Application::quit_cb(Fl_Widget* w, void* data)
{
    exit(0);
}

void Application::play_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
}

void Application::stop_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
}

void Application::pause_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
}
