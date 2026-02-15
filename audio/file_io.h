#ifndef FILE_IO_H
#define FILE_IO_H

#include <vector>
#include <bits/stdc++.h> // std::map
#include "buffer.h"

// Forward declarations.
class Buffer;
class Track;

class FileIO {
    private:

        struct OriginalFileFormat {
            std::string fileName;
            ma_uint32 outputChannels;
            ma_uint32 outputSampleRate;
            ma_format outputFormat;
        };

        Buffer& buffer;
        ma_decoder decoder;
        OriginalFileFormat originalFileFormat;

        bool storeOriginalFileFormat(const char* filename);
        bool decode();

    public:
        FileIO(Buffer& b) : buffer(b) {}

        void load(const char *fileName, Track& track);
        bool decode(Track& track);
        std::map<std::string, std::string> getOriginalFileFormat();
        void save(const char* filename, Track& track);
        bool setFormat(const char* filename, Format& format);
        void setNewFileFormat(Format& format, bool stereo, Track& track);
};

#endif // FILE_IO_H
