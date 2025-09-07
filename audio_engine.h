#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <iostream>
#include <vector>
#include <memory>
#include "../libraries/miniaudio.h"

// Forward declarations.
class AudioTrack;
class Application;

class AudioEngine {
        // Structure that holds the device data.
        struct DeviceInfo {
            std::string name;
            ma_device_id id;
            bool isDefault;
        };

        Application* pApplication;
        ma_context context;
        ma_device outputDevice;
        ma_device_id outputDeviceID = {0};
        // Flags to keep track of object state.
        bool contextInitialized = false;
        bool outputDeviceInitialized = false;
        // Multiple loaded tracks
        std::vector<std::unique_ptr<AudioTrack>> tracks;  
        // The unique id assigned to each track.
        unsigned int trackId = 1;
        const ma_format defaultOutputFormat = ma_format_f32;
        const ma_uint32 defaultOutputChannels = 2;
        const ma_uint32 defaultOutputSampleRate = 44100;
        std::vector<std::string> supportedFormats = {".wav", ".WAV",".mp3", ".MP3", ".flac", ".FLAC", ".ogg", ".OGG"};

        std::vector<DeviceInfo> getDevices(ma_device_type deviceType);
        static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount);

    public:
        AudioEngine(Application *app);
        ~AudioEngine();

        bool initializeOutputDevice();
        void printAllDevices();
        void shutdown();
        unsigned int addTrack(std::unique_ptr<AudioTrack> track);
        void removeTrack(unsigned int id);
        void start();
        void stop();
        size_t numberOfTracks() { return tracks.size(); }

        // Getters.
        std::vector<DeviceInfo> getOutputDevices();
        std::vector<DeviceInfo> getInputDevices();
        std::vector<std::string> getSupportedFormats() { return supportedFormats; }
        bool isContextInitialized() { return contextInitialized; }
        ma_format getDefaultOutputFormat() { return defaultOutputFormat; }
        ma_uint32 getDefaultOutputChannels() { return defaultOutputChannels; }
        ma_uint32 getDefaultOutputSampleRate() { return defaultOutputSampleRate; }
        AudioTrack& getTrack(unsigned int id);

      // Setters.
      void setOutputDevice(const char *deviceName);
};

#endif // AUDIO_ENGINE_H
