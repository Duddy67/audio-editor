#include "main.h"


void Application::createMenu()
{
    menu->add(MenuLabels[MenuItemID::FILE_SUB].c_str(), 0, 0, 0, FL_SUBMENU);
    menu->add(MenuLabels[MenuItemID::FILE_NEW].c_str(), FL_ALT + 'n', new_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::FILE_OPEN].c_str(), 0, open_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::FILE_SAVE].c_str(), 0, save_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::FILE_SAVE_AS].c_str(), 0, saveas_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::FILE_QUIT].c_str(), FL_CTRL + 'q',(Fl_Callback*) quit_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::EDIT_SUB].c_str(), 0, 0, 0, FL_SUBMENU);
    menu->add(MenuLabels[MenuItemID::EDIT_UNDO].c_str(), 0, [](Fl_Widget* w, void* userData) { 
                                      Application* app = static_cast<Application*>(userData);
                                      app->audioEdit(EditID::UNDO);
                                  }, (void*) this);
    menu->add(MenuLabels[MenuItemID::EDIT_REDO].c_str(), 0, [](Fl_Widget* w, void* userData) { 
                                      Application* app = static_cast<Application*>(userData);
                                      app->audioEdit(EditID::REDO);
                                  }, (void*) this);
    menu->add(MenuLabels[MenuItemID::EDIT_COPY].c_str(), FL_CTRL + 'c',0, 0, 0);
    menu->add(MenuLabels[MenuItemID::EDIT_PAST].c_str(), FL_CTRL + 'v',0, 0, FL_MENU_INACTIVE);
    menu->add(MenuLabels[MenuItemID::EDIT_CUT].c_str(), FL_CTRL + 'x',0, 0, 0);
    menu->add(MenuLabels[MenuItemID::EDIT_INSERT_MARKER].c_str(), 0, insert_marker_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::EDIT_SETTINGS].c_str(), 0, settings_cb, (void*) this);
    menu->add(MenuLabels[MenuItemID::PROCESS_SUB].c_str(), 0, 0, 0, FL_SUBMENU);
    menu->add(MenuLabels[MenuItemID::PROCESS_MUTE].c_str(), 0, [](Fl_Widget* w, void* userData) { 
                                      Application* app = static_cast<Application*>(userData);
                                      app->audioEdit(EditID::MUTE);
                                  }, (void*) this);
    menu->add(MenuLabels[MenuItemID::PROCESS_NORMALIZE].c_str(), 0,0, 0, 0);
    menu->add(MenuLabels[MenuItemID::PROCESS_VOLUME].c_str(), 0,0, 0, 0);
    menu->add(MenuLabels[MenuItemID::PROCESS_FADE_IN].c_str(), 0, [](Fl_Widget* w, void* userData) { 
                                      Application* app = static_cast<Application*>(userData);
                                      app->audioEdit(EditID::FADE_IN);
                                  }, (void*) this);
    menu->add(MenuLabels[MenuItemID::PROCESS_FADE_OUT].c_str(), 0, [](Fl_Widget* w, void* userData) { 
                                      Application* app = static_cast<Application*>(userData);
                                      app->audioEdit(EditID::FADE_OUT);
                                  }, (void*) this);
    menu->add("Help", 0, 0, 0, FL_SUBMENU);
    menu->add("Help/Index", 0, 0, 0, 0);
    menu->add("Help/About", 0, 0, 0, 0);
    // etc...

    return;
}


// "Open" the file
void Application::open(const char* filename)
{
    TrackOptions options;
    options.filepath = filename;

    try {
        addDocument(options);
        printf("Open: '%s'\n", filename);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Failed to add document: " << e.what() << std::endl;
    }
}

// 'Save' the file, create the file if it doesn't exist
// and save something in it.
void Application::save(const char* filename) {
    printf("Saving '%s'\n", filename);
    auto* document = (Document*)tabs->value();
    auto& track = document->getTrack();
    // Just save the file - native dialog already handled confirmation 
    // in case of same file name.
    track.save(filename);
}

int Application::isFileExist(const char* filename) {
    FILE* fp = fl_fopen(filename, "r");

    if (fp) {
        fclose(fp);
        return(1);
    }
    else {
        return(0);
    }
}

// Return an 'untitled' default pathname
const char* Application::untitledDefault()
{
    static char* filename = 0;

    if (!filename) {
        const char* home = getenv("HOME") ? getenv("HOME") : // Unix
        getenv("HOME_PATH") ? getenv("HOME_PATH") :          // Windows
        ".";                                                 // other

        filename = (char*)malloc(strlen(home) + 20);
        sprintf(filename, "%s/untitled.txt", home);
    }

    return(filename);
}

void Application::setSupportedFormats() 
{
    std::vector<std::string> formats = getEngine().getSupportedFormats();
    unsigned int size = formats.size();
    std::string supportedFormats = "";

    // Iterate through the extension array.
    for (unsigned int i = 0; i < size; i++) {
        // Leave out formats in uppercase as there are displayed anyway.
        if (!std::isupper(formats[i][1])) {
            // Store the supported formats.
            supportedFormats = supportedFormats + "*" + formats[i] + "\n";
        }
    }

    // Initialize the file chooser
    /*filter("Wav\t*.wav\n"
           "MP3\t*.mp3\n");*/
    fileChooser->filter(supportedFormats.c_str());
}

const std::string* Application::getMenuItemLabel(Fl_Menu_Item* item) const
{
    auto it = menuItemLabels.find(item);
    return it != menuItemLabels.end() ? &it->second : nullptr;
}

Fl_Menu_Item* Application::getMenuItem(MenuItemID menuItemID)
{
    switch (menuItemID) {
      case MenuItemID::EDIT_UNDO:
          return undoMenuItem;
        break;

      case MenuItemID::EDIT_REDO:
          return redoMenuItem;
        break;

      default:
         return nullptr;
    }
}

void Application::updateMenuItem(MenuItemID menuID, Action action, const std::string& label /*= ""*/)
{
    Fl_Menu_Item* item;

    if ((item = getMenuItem(menuID)) != nullptr) {
        switch (action) {
          case Action::ACTIVATE:
              item->activate();
            break;

          case Action::DEACTIVATE:
              item->deactivate();
            break;
          
          default:
              return;
        }

        if (!label.empty()) {
            // Store string to keep a Fl_Menu_Item valid pointer.
            // Note: Get only the substring after the slash. 
            setMenuItemLabel(getMenuItem(menuID), label.substr(label.find("/") + 1));
            item->label(getMenuItemLabel(item)->c_str());
        }
    }
}

/*
 * Maps the edit menu item to the according functions.
 */
void Application::audioEdit(EditID id)
{
    // Check first a tab (ie: document) is active.
    if (tabs->value()) {
        try {
            auto& track = getActiveDocument().getTrack();

            switch (id) {
                case EditID::MUTE:
                    onMute(track);
                    break;

                case EditID::FADE_IN:
                    onFadeIn(track);
                    break;

                case EditID::FADE_OUT:
                    onFadeOut(track);
                    break;

                case EditID::NORMALIZE:
                    break;

                case EditID::VOLUME:
                    break;

                case EditID::COPY:
                    break;

                case EditID::PAST:
                    break;

                case EditID::CUT:
                    break;

                case EditID::UNDO:
                    onUndo(track);
                    break;

                case EditID::REDO:
                    onRedo(track);
                    break;

                case EditID::NONE:
                    break;
            }
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to get track: " << e.what() << std::endl;
        }
    }
    else {
        std::cout << "No active document." << std::endl;
    }
}

//================== Callback functions called from menu  =========================

/*
 * Handle an 'Open' request from the menu.
 */
void Application::open_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    // Create the file chooser widget.
    if (app->fileChooser == nullptr) {
        app->fileChooser = new Fl_Native_File_Chooser();
        app->setSupportedFormats();
    }

    app->fileChooser->title("Open file");
    // Only picks files that exist.
    app->fileChooser->type(Fl_Native_File_Chooser::BROWSE_FILE);     

    switch (app->fileChooser->show()) {
        case -1:   // Error
            break;
        case 1:    // Cancel
            break;
        default:   // Choice
            app->open(app->fileChooser->filename());
            break;
    }
}

/*
 * Handle a 'Save' request from the menu.
 */
void Application::save_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    // Create the file chooser widget.
    if (app->fileChooser == nullptr) {
        app->fileChooser = new Fl_Native_File_Chooser();
        app->setSupportedFormats();
    }

    app->fileChooser->title("Save");
    // Need this if file doesn't exist yet.
    app->fileChooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);    
    // Enable native overwrite confirmation.
    app->fileChooser->options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);

    if (app->tabs->value()) {
        auto* document = (Document*)app->tabs->value();
        // Set the name of the file to save. 
        app->fileChooser->preset_file(document->getFileName().c_str());

        // If file already exists in the default directory just save it.
        if (app->isFileExist(app->fileChooser->filename())) {
            auto& track = document->getTrack();
            track.save(app->fileChooser->filename());
            // No need to open up the chooser's dialog.
            return;
        }

        switch (app->fileChooser->show()) {
            case -1:   // Error
                break;
            case 1:    // Cancel
                break;
            default:   // Choice
                app->save(app->fileChooser->filename());
                break;
        }
    }
    else {
        std::cout << "No file selected!" << std::endl;
        return;
    }
}

// Handle a 'Save as' request from the menu
void Application::saveas_cb(Fl_Widget* w, void* data)
{
    Application* app = (Application*) data;

    // Create the file chooser widget.
    if (app->fileChooser == nullptr) {
        app->fileChooser = new Fl_Native_File_Chooser();
        app->setSupportedFormats();
    }

    app->fileChooser->title("Save As");
    // Need this if file doesn't exist yet.
    app->fileChooser->type(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);    
    // Enable native overwrite confirmation.
    app->fileChooser->options(Fl_Native_File_Chooser::SAVEAS_CONFIRM);

    if (app->tabs->value()) {
        auto* document = (Document*)app->tabs->value();
        // Set the name of the file to save. 
        app->fileChooser->preset_file(document->getFileName().c_str());

        switch (app->fileChooser->show()) {
            case -1:   // Error
                break;
            case 1:    // Cancel
                break;
            default:   // Choice
                app->save(app->fileChooser->filename());
                break;
        }
    }
    else {
        std::cout << "No file selected!" << std::endl;
        return;
    }
}

