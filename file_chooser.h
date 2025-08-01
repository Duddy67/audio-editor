#ifndef FILE_CHOOSER_H
#define FILE_CHOOSER_H
#include <stdlib.h>     // exit,malloc
#include <string>
#include <vector>
#include <cctype>
#include <iostream>
#include <FL/Fl_Native_File_Chooser.H>

class FileChooser : public Fl_Native_File_Chooser
{
    // Return an 'untitled' default pathname
    const char* untitled_default() 
    {
        static char *filename = 0;

        if (!filename) {
            const char *home = fl_getenv("HOME") ? fl_getenv("HOME") :            // unix
                               fl_getenv("HOME_PATH") ? fl_getenv("HOME_PATH") :  // windows
                               ".";                                               // other

            filename = (char*) malloc(strlen(home) + 20);
            sprintf(filename, "%s/untitled.txt", home);
        }

        return (filename);
    }

    public:

    FileChooser(std::vector<std::string> formats) {
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
        filter(supportedFormats.c_str());
        preset_file(this->untitled_default());
    }

    // 'Open' the file
    void open(const char *filename)
    {
        printf("Open '%s'\n", filename);
    }
};

#endif
