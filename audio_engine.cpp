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
        contextInitialized = true;
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

    outputDeviceInitialized = true;

    g_audioEngine = this;

    return true;
}

/*
 * Sets the output to the given device.
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
    if (outputDeviceInitialized) {
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

/*
 * Clears all audio ressources currently used by the application. 
 */
void AudioEngine::shutdown() {
    ma_device_uninit(&outputDevice);
    ma_context_uninit(&context);
    tracks.clear();
}

void AudioEngine::start() { ma_device_start(&outputDevice); }
void AudioEngine::stop()  { ma_device_stop(&outputDevice); }

unsigned int AudioEngine::addTrack(std::unique_ptr<AudioTrack> track)
{
    // Set a brand new id for this track.
    track->setId(trackId++);
    unsigned int id = track->getId();
    // Transfer ownership.
    tracks.push_back(std::move(track));

    return id;
}

AudioTrack& AudioEngine::getTrack(unsigned int id) 
{
    for (auto& t : tracks) {
        if (t->getId() == id) {
            return *t.get();
        }
    }

    throw std::runtime_error("Couldn't find track with id: " + id);
}

void AudioEngine::removeTrack(unsigned int id)
{
    auto it = std::find_if(tracks.begin(), tracks.end(),
    [id](const std::unique_ptr<AudioTrack>& t) { return t->getId() == id; });

    if (it != tracks.end()) {
        // unique_ptr destructor deletes the owned AudioTrack.
        tracks.erase(it); 
    }
    // No such track. Throw an exception in case some functions need the info.
    else {
        throw std::runtime_error("Couldn't find track with id: " + id);
    }
}

void AudioEngine::data_callback(ma_device* pDevice, void* output, const void* /*input*/, ma_uint32 frameCount) {
    float* out = static_cast<float*>(output);
    // Clear buffer (stereo) with silence (ie: 0.0f). 
    std::fill(out, out + frameCount * 2, 0.0f);  

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

    if (!contextInitialized) {
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
 * Displays both the input and output audio devices in the console.
 * Function used for debugging purpose.
 */
void AudioEngine::printAllDevices()
{
    if (!contextInitialized) {
        std::cerr << "Audio context not initialized." << std::endl;
        return;
    }

    auto outputDevices = getOutputDevices();
    auto inputDevices = getInputDevices();

    std::cout << "=== Available Audio Devices ===" << std::endl;

    std::cout << "\nOutput Devices:" << std::endl;
    for (size_t i = 0; i < outputDevices.size(); ++i) {
        std::cout << "  " << i + 1 << ": " << outputDevices[i].name;
        if (outputDevices[i].isDefault) {
            std::cout << " (default)";
        }
        std::cout << std::endl;
    }

    std::cout << "\nInput Devices:" << std::endl;
    for (size_t i = 0; i < inputDevices.size(); ++i) {
        std::cout << "  " << i + 1 << ": " << inputDevices[i].name;
        if (inputDevices[i].isDefault) {
            std::cout << " (default)";
        }
        std::cout << std::endl;
    }
}
