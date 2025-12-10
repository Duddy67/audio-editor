#include "audio_engine.h"
#include "audio_track.h"
#include <iostream>

/*
 * Destructor: Uninitializes all of the audio parameters before closing the app.
 */
AudioEngine::~AudioEngine() {
    // Clears all audio ressources currently used by the application. 
    uninitOutput();
    uninitContext();
    tracks.clear();
}

/*
 * Checks the given backend is available.
 */
bool AudioEngine::isBackendAvailable(ma_backend backend) {
    ma_context context;
    ma_context_config config = ma_context_config_init();

    if (ma_context_init(&backend, 1, &config, &context) == MA_SUCCESS) {
        ma_context_uninit(&context);
        return true;
    }

    return false;
}

/*
 * Returns the name of the given backend handled by MiniAudio.
 */
std::string AudioEngine::backendToString(ma_backend backend)
{
    switch (backend) {
        case ma_backend_pulseaudio:
            return "PulseAudio";
        case ma_backend_jack:
            return "JACK";
        case ma_backend_alsa:
            return "ALSA";
        case ma_backend_null:
            return "Null (no backend)";
        default:
            return "Unknown";
    }
}

/*
 * Initializes the output device.
 */
void AudioEngine::initializeOutputDevice() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.pDeviceID = &outputDeviceID;
    config.playback.format = defaultOutputFormat;
    config.playback.channels = defaultOutputChannels;
    config.sampleRate = defaultOutputSampleRate;
    config.dataCallback = data_callback;
    config.pUserData = this;

    if (ma_device_init(&context, &config, &outputDevice) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize playback device.");
    }

    std::cout << "Output device initialized." << std::endl;
    outputDeviceInitialized = true;
}

void AudioEngine::initializeInputDevice()
{
    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.capture.pDeviceID = &inputDeviceID;
    config.capture.format = ma_format_f32;
    config.capture.channels = 2;
    config.sampleRate = defaultOutputSampleRate;
    config.dataCallback = data_callback;
    config.pUserData = this;

    if (ma_device_init(nullptr, &config, &inputDevice) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize capture device.");
    }

    std::cout << "Input device initialized." << std::endl;
    inputDeviceInitialized = true;
}

void AudioEngine::initializeDuplexDevice()
{
    ma_device_config config = ma_device_config_init(ma_device_type_duplex);
    config.sampleRate       = 44100;
    config.playback.format  = ma_format_f32;
    config.playback.channels= 2;
    config.playback.pDeviceID = &duplexDeviceID;
    config.capture.format   = ma_format_f32;
    config.capture.channels = 2;
    config.capture.pDeviceID  = &duplexDeviceID;
    config.dataCallback     = data_callback;
    config.pUserData        = this;

    if (ma_device_init(&context, &config, &duplexDevice) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize duplex device.");
    }

    duplexDeviceInitialized = true;
}

bool AudioEngine::isDeviceDuplex(const char *name)
{
    auto duplexDevices = getDuplexDevices();

    if (duplexDevices.size() == 0) {
        return false;
    }

    // Loop through the available backends.
    for (ma_uint32 i = 0; i < (ma_uint32) duplexDevices.size(); ++i) {
        // Check for the target backend.
        if (strcmp(duplexDevices[i].name.c_str(), name) == 0) {
            return true;
        }
    }

    return false;
}

/*
 * Sets the given (or default) backend.
 */
void AudioEngine::setBackend(const char *name)
{
    auto backends = getBackends();

    // First make sure at least one backend is available.
    if (backends.size() == 0) {
        throw std::runtime_error("No backend available !");
    }

    if (contextInitialized) {
        // Clear devices first.
        uninitOutput();
        uninitInput();
        uninitDuplex();
        // Then clear context.
        uninitContext();
    }

    int index = 0;

    // Loop through the available backends.
    for (ma_uint32 i = 0; i < (ma_uint32) backends.size(); ++i) {
        // Check for the target backend.
        if (strcmp(backends[i].name.c_str(), name) == 0) {
            index = i;
            break;
        }

        // In case of no target backend found, the index of the 
        // backend chosen by default by the system will be used.
        if (backends[i].isDefault) {
            index = i;
        }
    }

    ma_context_config config = ma_context_config_init();

    // Initialize the audio context with the given (or default) backend.
    if (ma_context_init(&backends[index].backend, backends.size(), &config, &context) == MA_SUCCESS) {
        contextInitialized = true;
        std::cout << "Audio context initialized." << std::endl;
    }
    else {
        throw std::runtime_error("Failed to initialize backend");
    }
}

/*
 * Sets the given (or default) duplex device.
 */
void AudioEngine::setDuplexDevice(const char *name)
{
    auto duplexDevices = getDuplexDevices();

    // Check for available devices.
    if (duplexDevices.size() == 0) {
        throw std::runtime_error("No duplex device found!");
    }

    // A device is already set.
    if (duplexDeviceInitialized) {
        // Clear current device.
        uninitDuplex();
    }

    int index = 0;

    if (name != nullptr) {
        // Loop through the available devices.
        for (ma_uint32 i = 0; i < (ma_uint32) duplexDevices.size(); ++i) {
            // Check for the target device.
            if (strcmp(duplexDevices[i].name.c_str(), name) == 0) {
                std::cout << "Found Duplex Device: " << duplexDevices[i].name << std::endl;
                index = i;
                break;
            }

            // In case of no target device found, the index of the 
            // device chosen by default by the system will be used.
            if (duplexDevices[i].isDefault) {
                index = i;
            }
        }
    }

    // Set the given (or default) device id.
    memcpy(&duplexDeviceID, &duplexDevices[index].id, sizeof(ma_device_id));

    // Let initializeDuplexDevice throw if it fails.
    initializeDuplexDevice();
}

/*
 * Sets the given (or default) output device.
 */
void AudioEngine::setOutputDevice(const char *name)
{
    auto outputDevices = getOutputDevices();

    // Check for available devices.
    if (outputDevices.size() == 0) {
        throw std::runtime_error("No output device found!");
    }

    // A device is already set.
    if (outputDeviceInitialized) {
        // Clear current device.
        uninitOutput();
    }

    int index = 0;

    if (name != nullptr) {
        // Loop through the available devices.
        for (ma_uint32 i = 0; i < (ma_uint32) outputDevices.size(); ++i) {
            // Check for the target device.
            if (strcmp(outputDevices[i].name.c_str(), name) == 0) {
                index = i;
                break;
            }

            // In case of no target device found, the index of the 
            // device chosen by default by the system will be used.
            if (outputDevices[i].isDefault) {
                index = i;
            }
        }
    }

    // Set the given (or default) device id.
    memcpy(&outputDeviceID, &outputDevices[index].id, sizeof(ma_device_id));

    // Let initializeOutputDevice throw if it fails.
    initializeOutputDevice();
}

/*
 * Sets the input to the given device.
 */
void AudioEngine::setInputDevice(const char *name)
{
    auto inputDevices = getInputDevices();

    // Check for available devices.
    if (inputDevices.size() == 0) {
        throw std::runtime_error("No input device found!");
    }

    // A device is already set.
    if (inputDeviceInitialized) {
        // Clear current device.
        uninitInput();
    }

    int index = 0;

    if (name != nullptr) {
        // Loop through the available devices.
        for (ma_uint32 i = 0; i < (ma_uint32) inputDevices.size(); ++i) {
            // Check for the target device.
            if (strcmp(inputDevices[i].name.c_str(), name) == 0) {
                index = i;
                break;
            }

            // In case of no target device found, the index of the 
            // device chosen by default by the system will be used.
            if (inputDevices[i].isDefault) {
                index = i;
            }
        }
    }

    // Set the given (or default) device id.
    memcpy(&inputDeviceID, &inputDevices[index].id, sizeof(ma_device_id));

    // Let initializeInputDevice throw if it fails.
    initializeInputDevice();
}

/*
 * Uninitializes current context.
 */
void AudioEngine::uninitContext() {
    if (contextInitialized) {
        ma_context_uninit(&context);
        contextInitialized = false;
    }
}

/*
 * Uninitializes current output device.
 */
void AudioEngine::uninitOutput() {
    if (outputDeviceInitialized) {
        stopPlayback();
        ma_device_uninit(&outputDevice);
        outputDeviceInitialized = false;
    }
}

/*
 * Uninitializes current input device.
 */
void AudioEngine::uninitInput() {
    if (inputDeviceInitialized) {
        stopCapture();
        ma_device_uninit(&inputDevice);
        inputDeviceInitialized = false;
    }
}

/*
 * Uninitializes current input device.
 */
void AudioEngine::uninitDuplex() {
    if (duplexDeviceInitialized) {
        stopDuplex();
        ma_device_uninit(&duplexDevice);
        duplexDeviceInitialized = false;
    }
}

void AudioEngine::startPlayback() { ma_device_start(&outputDevice); }
void AudioEngine::stopPlayback()  { ma_device_stop(&outputDevice); }
void AudioEngine::startCapture() { ma_device_start(&inputDevice); }
void AudioEngine::stopCapture()  { ma_device_stop(&inputDevice); }
void AudioEngine::startDuplex() { ma_device_start(&duplexDevice); }
void AudioEngine::stopDuplex()  { ma_device_stop(&duplexDevice); }

/*
 * Adds a new track to the track list.
 */
unsigned int AudioEngine::addTrack(std::unique_ptr<AudioTrack> track)
{
    // Set a brand new id for this track.
    track->setId(trackId++);
    unsigned int id = track->getId();
    // Transfer ownership.
    tracks.push_back(std::move(track));

    return id;
}

/*
 * Returns a track by its id.
 */
AudioTrack& AudioEngine::getTrack(unsigned int id) 
{
    for (auto& t : tracks) {
        if (t->getId() == id) {
            return *t.get();
        }
    }

    throw std::runtime_error("Couldn't find track with id: " + id);
}

/*
 * Removes a track from the track list by its id.
 */
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

/*
 * Callback function used by MiniAudio to feed audio data to devices.
 */
void AudioEngine::data_callback(ma_device* pDevice, void* output, const void* input, ma_uint32 frameCount) {
    AudioEngine* engine = static_cast<AudioEngine*>(pDevice->pUserData);

    // First check there are tracks.
    if (engine->tracks.size() == 0) {
        return;
    }

    // Handle playback (output).
    if (output != nullptr) {
        float* out = static_cast<float*>(output);
        // Clear buffer (stereo) with silence (ie: 0.0f). 
        std::fill(out, out + frameCount * 2, 0.0f);  

        // Dispatch data among playing tracks.
        for (auto& track : engine->tracks) {
            if (track->isPlaying()) {
                track->mixInto(out, frameCount);
            }
        }

        engine->setCurrentLevel(out, frameCount);
    }

    // Handle capture (input)
    if (input != nullptr) {
        const float* in = static_cast<const float*>(input);

        // Use the actual device capture channels.
        ma_uint32 captureChannels = pDevice->capture.channels;
        // Fallback guard in case the device reports nonsense.
        if (captureChannels == 0 || captureChannels > 8) {
            captureChannels = 2;
        }

        for (auto& track : engine->tracks) {
            if (track->isRecording()) {
                track->recordInto(in, frameCount, captureChannels);
            }
        }
    }
}

void AudioEngine::setCurrentLevel(const float* out, const ma_uint32 frameCount)
{
    // --- Compute RMS level for the mixed output ---
    double sumL = 0.0;
    double sumR = 0.0;

    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float left = out[i * 2];
        float right = out[i * 2 + 1];
        sumL += left * left;
        sumR += right * right;
    }

    float rmsL = sqrt(sumL / frameCount);
    float rmsR = sqrt(sumR / frameCount);

    // Convert to dB (optional, for realism)
    float dBL = 20.0f * log10f(rmsL + 1e-6f);
    float dBR = 20.0f * log10f(rmsR + 1e-6f);

    // Normalize from -60dB..0dB to 0..1 range
    auto normalizeDB = [](float dB) {
        return std::clamp((dB + 60.0f) / 60.0f, 0.0f, 1.0f);
    };

    float normL = normalizeDB(dBL);
    float normR = normalizeDB(dBR);

    // Optional: Apply smoothing to avoid flicker
    const float smoothing = 0.6f;
    currentLevelL = smoothing * currentLevelL.load() + (1.0f - smoothing) * normL;
    currentLevelR = smoothing * currentLevelR.load() + (1.0f - smoothing) * normR;
}

/*
 * Collects and returns all the available backends on the system.
 */
std::vector<AudioEngine::BackendInfo> AudioEngine::getBackends() 
{
    std::vector<BackendInfo> backends;
    std::vector<ma_backend> candidates = {
        ma_backend_pulseaudio,
        ma_backend_jack,
        ma_backend_alsa
    };

    for (auto backend : candidates) {
        if (isBackendAvailable(backend)) {
            BackendInfo info;
            info.name = backendToString(backend);
            info.backend = backend;
            backends.push_back(info);
        }
    }

    return backends;
}

/*
 * Gathers all the capture and playback device info into an array.
 */
std::vector<AudioEngine::DeviceInfo> AudioEngine::getDevices(ma_device_type deviceType)
{
    // Create a device array.
    std::vector<DeviceInfo> devices;

    if (!contextInitialized) {
        return devices;
    }

    // initialize some MiniAudio variables.
    ma_device_info* pDeviceInfos = nullptr;
    ma_uint32 deviceCount = 0;
    ma_result result;

    // Get playback, capture or duplex devices.
    if (deviceType == ma_device_type_playback) {
        result = ma_context_get_devices(&context, &pDeviceInfos, &deviceCount, nullptr, nullptr);
    }
    else if (deviceType == ma_device_type_capture) {
        result = ma_context_get_devices(&context, nullptr, nullptr, &pDeviceInfos, &deviceCount);
    }
    else if (deviceType == ma_device_type_duplex) {
        // Duplex = intersection of playback & capture devices (same hardware)
        ma_device_info *pPlaybackInfos = nullptr, *pCaptureInfos = nullptr;
        ma_uint32 playbackCount = 0, captureCount = 0;

        result = ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount);
        if (result != MA_SUCCESS) {
            std::cerr << "Failed to retrieve devices." << std::endl;
            return devices;
        }

        // Find matching names/IDs between playback and capture
        for (ma_uint32 i = 0; i < playbackCount; ++i) {
            for (ma_uint32 j = 0; j < captureCount; ++j) {
                if (strcmp(pPlaybackInfos[i].name, pCaptureInfos[j].name) == 0) {
                    DeviceInfo info;
                    info.name = pPlaybackInfos[i].name;
                    info.id = pPlaybackInfos[i].id;
                    info.isDefault = pPlaybackInfos[i].isDefault && pCaptureInfos[j].isDefault;
                    devices.push_back(info);
                }
            }
        }

        return devices;
    }
    // Invalid type.
    else {
        return devices; 
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

std::vector<AudioEngine::DeviceInfo> AudioEngine::getDuplexDevices() {
    return getDevices(ma_device_type_duplex);
}

/*
 * Returns the name of the current backend.
 */
std::string AudioEngine::currentBackend()
{
    if (!contextInitialized) {
        return backendToString(ma_backend_null);
    }

    ma_backend backend = context.backend;

    return backendToString(backend);
}

std::string AudioEngine::currentOutput()
{
    return outputDevice.playback.name;
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
    auto duplexDevices = getDuplexDevices();

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

    std::cout << "\nDuplex Devices:" << std::endl;
    for (size_t i = 0; i < duplexDevices.size(); ++i) {
        std::cout << "  " << i + 1 << ": " << duplexDevices[i].name;
        if (duplexDevices[i].isDefault) {
            std::cout << " (default)";
        }
        std::cout << std::endl;
    }
}
