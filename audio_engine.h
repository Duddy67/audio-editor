#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <iostream>
#include <vector>
#include <memory>
#include <atomic>
#include "../libraries/miniaudio.h"

// Forward declarations.
class AudioTrack;
class Application;

class AudioEngine {
        // Structure that holds the backend data.
        struct BackendInfo {
            std::string name;
            ma_backend backend;
            bool isDefault;
        };

        // Structure that holds the device data.
        struct DeviceInfo {
            std::string name;
            ma_device_id id;
            bool isDefault;
        };

        Application* pApplication;
        ma_context context;
        ma_device outputDevice;
        ma_device inputDevice;
        ma_device duplexDevice;
        ma_device_id outputDeviceID = {0};
        ma_device_id inputDeviceID = {0};
        ma_device_id duplexDeviceID = {0};
        // Flags to keep track of object state.
        bool contextInitialized = false;
        bool outputDeviceInitialized = false;
        bool inputDeviceInitialized = false;
        bool duplexDeviceInitialized = false;
        // Multiple loaded tracks
        std::vector<std::unique_ptr<AudioTrack>> tracks;  
        // The unique id assigned to each track.
        unsigned int trackId = 1;
        const ma_format defaultOutputFormat = ma_format_f32;
        const ma_uint32 defaultOutputSampleRate = 44100;
        std::vector<std::string> supportedFormats = {".wav", ".WAV",".mp3", ".MP3", ".flac", ".FLAC", ".ogg", ".OGG"};
        // Used with vu-meters.
        std::atomic<float> currentLevelL {0.0f};
        std::atomic<float> currentLevelR {0.0f};
        std::atomic<float> currentPeakL {0.0f};
        std::atomic<float> currentPeakR {0.0f};

        std::vector<DeviceInfo> getDevices(ma_device_type deviceType);
        static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frameCount);
        void initializeOutputDevice();
        void initializeInputDevice();
        void initializeDuplexDevice();
        bool isBackendAvailable(ma_backend backend);
        std::string backendToString(ma_backend backend);
        void setCurrentLevel(const float* out, const ma_uint32 frameCount);

    public:
        AudioEngine(Application* app) : pApplication(app) {} 
        ~AudioEngine();

        void printAllDevices();
        std::string currentBackend();
        std::string currentOutput();
        void uninitContext();
        void uninitOutput();
        void uninitInput();
        void uninitDuplex();
        unsigned int addTrack(std::unique_ptr<AudioTrack> track);
        void removeTrack(unsigned int id);
        void startPlayback();
        void stopPlayback();
        void startCapture();
        void stopCapture();
        void startDuplex();
        void stopDuplex();
        size_t numberOfTracks() { return tracks.size(); }
        bool isDeviceDuplex(const char *name);

        // Getters.
        std::vector<BackendInfo> getBackends();
        std::vector<DeviceInfo> getOutputDevices();
        std::vector<DeviceInfo> getInputDevices();
        std::vector<DeviceInfo> getDuplexDevices();
        std::vector<std::string> getSupportedFormats() { return supportedFormats; }
        bool isContextInitialized() { return contextInitialized; }
        ma_format getDefaultOutputFormat() { return defaultOutputFormat; }
        ma_uint32 getDefaultOutputSampleRate() { return defaultOutputSampleRate; }
        float getCurrentLevelL() const { return currentLevelL.load(); }
        float getCurrentLevelR() const { return currentLevelR.load(); }
        float getCurrentPeakL() const { return currentPeakL.load(); }
        float getCurrentPeakR() const { return currentPeakR.load(); }
        AudioTrack& getTrack(unsigned int id);
        Application& getApplication() const { return *pApplication; }

      // Setters.
      void setBackend(const char *name);
      void setOutputDevice(const char *name = nullptr);
      void setInputDevice(const char *name = nullptr);
      void setDuplexDevice(const char *name = nullptr);
};

#endif // AUDIO_ENGINE_H
