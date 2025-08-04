#ifndef AUDIO_H
#define AUDIO_H

#include <string>
#include <iostream>
#include <filesystem>
#include <atomic>
#include <vector>
#include <thread>
#include <bits/stdc++.h> // std::map
#include <time.h>
#include "../libraries/miniaudio.h"

// Forward declaration.
class Application;

// Structure used to manipulate some Audio class members through the data_callback function.
struct AudioCallbackData {
    std::vector<float> *pLeftSamples;
    std::vector<float> *pRightSamples;
    std::atomic<int> *pPlaybackSampleIndex;
    int *pTotalSamples;
    // Pointer to the owning class.
    class Audio* pInstance;
    Application* pApplication;
};


/*
 * The Audio class is a kind of interface allowing the application and the MiniAudio
 * library to communicate with each other.
 */
class Audio {
    private:
        // Structure that holds the device data.
        struct DeviceInfo {
            std::string name;
            ma_device_id id;
            bool isDefault;
        };

        struct OriginalFileFormat {
            std::string fileName;
            ma_uint32 outputChannels;
            ma_uint32 outputSampleRate;
            ma_format outputFormat;
        };

        ma_context context;
        ma_decoder decoder;
        ma_uint64 frameCount;
        Application* pApplication;
        AudioCallbackData callbackData;
        std::vector<float> leftSamples;
        std::vector<float> rightSamples;
        std::atomic<int> playbackSampleIndex{0};
        int totalSamples = 0;
        bool contextInit = false;
        bool decoderInit = false;
        bool outputDeviceInit = false;
        bool stereo = true;
        bool playing = false;
        bool paused = false;
        const ma_format defaultOutputFormat = ma_format_f32;
        const ma_uint32 defaultOutputChannels = 2;
        const ma_uint32 defaultOutputSampleRate = 44100;
        ma_device outputDevice;
        ma_device_id outputDeviceID = {0};
        OriginalFileFormat originalFileFormat;
        std::vector<DeviceInfo> getDevices(ma_device_type deviceType);
        std::vector<std::string> supportedFormats = {".wav", ".WAV",".mp3", ".MP3", ".flac", ".FLAC", ".ogg", ".OGG"};

        bool storeOriginalFileFormat(const char* filename);
        void uninit();
        bool initializeOutputDevice();
        bool decodeFile();


    public:
      Audio(Application *app);
      ~Audio();

      bool init(const std::vector<float>& left, const std::vector<float>& right, int rate);
      std::vector<DeviceInfo> getOutputDevices();
      std::vector<DeviceInfo> getInputDevices();
      void printAllDevices();
      void loadFile(const char *fileName);
      void start() { ma_device_start(&outputDevice); }
      void stop() { ma_device_stop(&outputDevice); }

      // Getters.

      std::map<std::string, std::string> getOriginalFileFormat();
      std::vector<std::string> getSupportedFormats() { return supportedFormats; }
      bool isContextInit() { return contextInit; }
      bool isStereo() { return stereo; }
      bool isPlaying() { return playing; }

      // Setters.

      void setOutputDevice(const char *deviceName);
};

#endif // AUDIO_H

