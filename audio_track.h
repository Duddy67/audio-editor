#ifndef AUDIO_TRACK_H
#define AUDIO_TRACK_H

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
    class AudioTrack* pInstance;
    Application* pApplication;
};


/*
 * The AudioTrack class is a kind of interface allowing the application and the MiniAudio
 * library to communicate with each other.
 */
class AudioTrack {
    private:
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
        int totalFrames = 0;
        bool stereo = true;
        std::atomic<bool> playing{false};
        bool paused = false;
        OriginalFileFormat originalFileFormat;

        bool storeOriginalFileFormat(const char* filename);
        void uninit();
        bool decodeFile();


    public:
      AudioTrack(Application *app);
      ~AudioTrack();

      void loadFromFile(const char *fileName);
      void start() { ma_device_start(&outputDevice); }
      void stop() { ma_device_stop(&outputDevice); }
      void mixInto(float* output, int frameCount);

      // Getters.

      std::map<std::string, std::string> getOriginalFileFormat();
      bool isStereo() { return stereo; }
      bool isPlaying() const { return playing.load(); }
      bool isPaused() const { return paused; }
};

#endif // AUDIO_TRACK_H

