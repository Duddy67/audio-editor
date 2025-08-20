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
    auto track = app->audioEngine->getTrack(0);
    track->play();
}

void Application::stop_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;
    auto track = app->audioEngine->getTrack(0);
    track->stop();
}

void Application::pause_cb(Fl_Widget* w, void* data)
{
    //Application* app = (Application*) data;
}

void Application::close_document_cb(Fl_Widget* w, void* data)
{
    // Cast back the generic Fl_Widget pointer to the type initially passed (ie: Fl_Group*).
    Fl_Group* g = static_cast<Fl_Group*>(w);
    Application* app = (Application*) data;

    // 1. Ask user if they want to save changes
    // 2. Release Track/Waveform/etc objects
    // 3. Remove from your documents vector
    app->removeDocument(g);

    auto parent = g->parent();
    parent->remove(g);

    // No more document in the application.
    if (app->getNbDocuments() == 0) {
        app->hideTabs();
    }
}

