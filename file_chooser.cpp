#include "main.h"

void Application::file_chooser_cb(Fl_Widget *w, void *data)
{
    Application* app = (Application*) data;

    if (app->fileChooser == 0) {
        app->fileChooser = new FileChooser(app->audioEngine->getSupportedFormats());
    }

    switch (app->fileChooser->show()) {
        case -1:   // Error
            break;
        case 1:    // Cancel
            break;
        default:   // Choice
            app->fileChooser->preset_file(app->fileChooser->filename());
            /*auto track = std::make_shared<AudioTrack>(app->audioEngine);

            if (!track->loadFromFile(app->fileChooser->filename())) {
                std::cerr << "Failed to load file." << std::endl;
                break;
            }

            app->audioEngine->addTrack(track);*/
            app->addDocument(app->fileChooser->filename());
            //app->getTabs()->show();

            break;
    }
}


