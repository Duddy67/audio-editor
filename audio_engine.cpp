#include "audio_engine.h"
#include "audio_track.h"
#include <iostream>

static AudioEngine* g_audioEngine = nullptr;

/*
 * Constructor
 */
AudioEngine::AudioEngine(Application* app) : pApplication(app) {
    ma_context_config config = ma_context_config_init();

    // Initialize the audio context.
    if (ma_context_init(NULL, 0, &config, &context) == MA_SUCCESS) {
        std::cerr << "Audio context initialized." << std::endl;
    }
}

/*
 * Destructor: Uninitializes all of the audio parameters before closing the app.
 */
AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initializeOutputDevice() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = defaultOutputFormat;
    config.playback.channels = defaultOutputChannels;
    config.sampleRate = defaultOutputSampleRate;
    config.dataCallback = data_callback;
    config.pUserData = this;

    if (ma_device_init(&context, &config, &outputDevice) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device\n";
        return false;
    }

    g_audioEngine = this;

    return true;
}

/*
 * Set the output to the given device.
 */
void AudioEngine::setOutputDevice(const char *deviceName)
{
    bool found = false;
    auto outputDevices = getOutputDevices();

    // Loop through the available devices.
    for (ma_uint32 i = 0; i < (ma_uint32) outputDevices.size(); ++i) {
        if (strcmp(outputDevices[i].name.c_str(), deviceName) == 0) {
            std::cout << "Found target device: " << outputDevices[i].name << std::endl;
            // Set the given device id.
            memcpy(&outputDeviceID, &outputDevices[i].id, sizeof(ma_device_id));
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Target device not found!" << std::endl;
        return;
    }

    // A device is already set.
    if (outputDevice) {
        // Stop playback.
        ma_device_stop(&outputDevice);
        // Release device resources.
        ma_device_uninit(&outputDevice);
    }

    if(!initializeOutputDevice()) {
        std::cerr << "Failed to initialize output device." << std::endl;
        return;
    }
}

void AudioEngine::shutdown() {
    ma_device_uninit(&outputDevice);
    ma_context_uninit(&context);
    tracks.clear();
}

void AudioEngine::start() { ma_device_start(&outputDevice); }
void AudioEngine::stop() { ma_device_stop(&outputDevice); }

void AudioEngine::addTrack(std::shared_ptr<AudioTrack> track) {
    tracks.push_back(track);
}

void AudioEngine::removeTrack(std::shared_ptr<AudioTrack> track) {
    tracks.erase(std::remove(tracks.begin(), tracks.end(), track), tracks.end());
}

void AudioEngine::data_callback(ma_device* pDevice, void* output, const void* /*input*/, ma_uint32 frameCount) {
    float* out = static_cast<float*>(output);
    std::fill(out, out + frameCount * 2, 0.0f);  // Clear buffer (stereo)

    AudioEngine* engine = static_cast<AudioEngine*>(pDevice->pUserData);

    for (auto& track : engine->tracks) {
        if (track->isPlaying()) {
            track->mixInto(out, frameCount);
        }
    }
}

/*
 * Gathers all the capture and playback device info into an array.
 */
std::vector<AudioEngine::DeviceInfo> AudioEngine::getDevices(ma_device_type deviceType) {
    // Create a device array.
    std::vector<DeviceInfo> devices;

    if (!context) {
        return devices;
    }

    // initialize some MiniAudio variables.
    ma_device_info* pDeviceInfos = nullptr;
    ma_uint32 deviceCount = 0;
    ma_result result;

    // Get playback or capture devices.
    if (deviceType == ma_device_type_playback || deviceType == ma_device_type_duplex) {
        result = ma_context_get_devices(&context, &pDeviceInfos, &deviceCount, nullptr, nullptr);
    }
    else {
        result = ma_context_get_devices(&context, nullptr, nullptr, &pDeviceInfos, &deviceCount);
    }

    if (result != MA_SUCCESS) {
        std::cerr << "Failed to retrieve devices." << std::endl;
        return devices;
    }

    // Store the device data in the array.
    for (ma_uint32 i = 0; i < deviceCount; ++i) {
        DeviceInfo info;
        info.name = pDeviceInfos[i].name;
        info.id = pDeviceInfos[i].id;
        info.isDefault = pDeviceInfos[i].isDefault;
        devices.push_back(info);
    }

    return devices;
}

/* Device getters. */

std::vector<AudioEngine::DeviceInfo> AudioEngine::getOutputDevices() {
    return getDevices(ma_device_type_playback);
}

std::vector<AudioEngine::DeviceInfo> AudioEngine::getInputDevices() {
    return getDevices(ma_device_type_capture);
}

/*
 * Set the output to the given device.
 */
void AudioTrack::setOutputDevice(const char *deviceName)
{
    bool found = false;
    auto outputDevices = getOutputDevices();

    // Loop through the available devices.
    for (ma_uint32 i = 0; i < (ma_uint32) outputDevices.size(); ++i) {
        if (strcmp(outputDevices[i].name.c_str(), deviceName) == 0) {
            std::cout << "Found target device: " << outputDevices[i].name << std::endl;
            // Set the given device id.
            memcpy(&outputDeviceID, &outputDevices[i].id, sizeof(ma_device_id));
            found = true;
            break;
        }
    }

    if (!found) {
        std::cerr << "Target device not found!" << std::endl;
        return;
    }

    // Stop playback.
    ma_device_stop(&outputDevice);
    // Release device resources.
    ma_device_uninit(&outputDevice);

    if(!initializeOutputDevice()) {
        std::cerr << "Failed to initialize output device." << std::endl;
        return;
    }
}
