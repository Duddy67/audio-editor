#ifndef FILE_IO_H
#define FILE_IO_H

#include <vector>
#include <bits/stdc++.h> // std::map
#include "buffer.h"
#include "clip.h"

// Forward declaration.
class Engine;
struct Format;

class FileIO {
    private:

        ma_decoder decoder;
        bool decode(std::vector<Clip>& clips, Format format);

    public:

        void load(const char *fileName, std::vector<Clip>& clips, const Engine& engine);
        std::map<std::string, std::string> getOriginalFileFormat();
        void save(const char* filename, Buffer& buffer);
        bool setFormat(const char* filename, Format& format);
};

#endif // FILE_IO_H
