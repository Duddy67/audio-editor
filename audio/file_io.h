#ifndef FILE_IO_H
#define FILE_IO_H

#include <vector>

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
        std::atomic<bool> eof{false};
        OriginalFileFormat originalFileFormat;

        bool storeOriginalFileFormat(const char* filename);
        bool decode();

    public:
        FileIO(Buffer& b) : buffer(b) {}

        void load(const char *fileName);
        std::map<std::string, std::string> getOriginalFileFormat();
        bool isEndOfFile() const { return eof.load(); }
        void resetEndOfFile() { eof.store(false); }
        void save(const char* filename);
};

#endif // FILE_IO_H
