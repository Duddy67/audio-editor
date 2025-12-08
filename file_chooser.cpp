#include "main.h"

/*void Application::file_chooser_cb(Fl_Widget *w, void *data)
{
    Application* app = (Application*) data;

    if (app->fileChooser == nullptr) {
        app->fileChooser = new FileChooser(app->audioEngine->getSupportedFormats());
    }

    switch (app->fileChooser->show()) {
        case -1:   // Error
            break;
        case 1:    // Cancel
            break;
        default:   // Choice
            app->fileChooser->preset_file(app->fileChooser->filename());

            try {
                app->addDocument(app->fileChooser->filename());
            }
            catch (const std::runtime_error& e) {
                std::cerr << "Failed to add document: " << e.what() << std::endl;
            }

            break;
    }
}*/

