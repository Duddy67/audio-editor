#ifndef FILE_IO_H
#define FILE_IO_H

#include <vector>
#include <bits/stdc++.h> // std::map
#include "buffer.h"

// Forward declaration.
class Track;
struct Format;

class FileIO {
    private:

        Track& track;
        ma_decoder decoder;
        bool decode();

    public:

        FileIO(Track& t) : track(t) {}

        void load(const char *fileName);
        std::map<std::string, std::string> getOriginalFileFormat();
        void save(const char* filename);
        bool setFormat(const char* filename, Format& format);
        void setNewFileFormat(Format& format, bool stereo);
};

#endif // FILE_IO_H
