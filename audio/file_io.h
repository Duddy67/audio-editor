#ifndef FILE_IO_H
#define FILE_IO_H

#include <vector>
#include <bits/stdc++.h> // std::map
#include "buffer.h"

// Forward declaration.
class Engine;
struct Format;

class FileIO {
    private:

        ma_decoder decoder;
        bool decode(Buffer& buffer);

    public:

        void load(const char *fileName, Buffer& buffer, const Engine& engine);
        std::map<std::string, std::string> getOriginalFileFormat();
        void save(const char* filename, Buffer& buffer);
        bool setFormat(const char* filename, Format& format);
        void setNewFileFormat(Format& format, bool stereo, const Engine& engine);
};

#endif // FILE_IO_H
