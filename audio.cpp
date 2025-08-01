#include "main.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../libraries/miniaudio.h"


/*
 * Constructor
 */
Audio::Audio(Application* app) : pApplication(app), contextInit(false) {
    ma_context_config config = ma_context_config_init();

    // Initialize the audio context.
    if (ma_context_init(NULL, 0, &config, &context) == MA_SUCCESS) {
        contextInit = true;
        std::cerr << "Audio context initialized." << std::endl;
    }

    // Set the callbackData parameters used in the MiniAudio callback function (ie: data_callback).
    callbackData.pApplication = app;
    // Set pointers to the private members needed in the data_callback function.
    callbackData.pDecoder = &decoder;
    callbackData.pLeftSamples = &leftSamples;
    callbackData.pRightSamples = &rightSamples;
    callbackData.pPlaybackSampleIndex = &playbackSampleIndex;
    callbackData.pTotalSamples = &totalSamples;
    // Store pointer to this Audio instance.
    callbackData.pInstance = this;
}

/*
 * Destructor: Uninitializes all of the audio parameters before closing the app.
 */
Audio::~Audio() {
    uninit();

    if (contextInit) {
        ma_context_uninit(&context);
        std::cerr << "Audio context uninitialized." << std::endl;
    }
}

/*
 * Uninitializes both output device and decoder parameters.
 */
void Audio::uninit()
{
    if (outputDeviceInit) {
        ma_device_uninit(&outputDevice);
    }

    if (decoderInit) {
        ma_decoder_uninit(&decoder);
    }
}

void data_callback(ma_device* pDevice, void* output, const void*, ma_uint32 frameCount) {
    AudioCallbackData* self = (AudioCallbackData*)pDevice->pUserData;

    if (self == nullptr || self->pDecoder == nullptr) {
        return;
    }

    float* out = static_cast<float*>(output);

    int remaining = *self->pTotalSamples - self->pPlaybackSampleIndex->load(std::memory_order_relaxed);
    int framesToCopy = std::min((int)frameCount, remaining);

    // Fill buffer with interleaved stereo samples
    for (int i = 0; i < framesToCopy; ++i) {
        // Increment the sample index (ie: ++).
        int idx = self->pPlaybackSampleIndex->fetch_add(1, std::memory_order_relaxed);
        // Left
        out[i * 2] = (*self->pLeftSamples)[idx];
        // Right
        out[i * 2 + 1] = (*self->pRightSamples)[idx];
    }

    // Fill remaining frames with silence
    for (int i = framesToCopy; i < (int)frameCount; ++i) {
        out[i * 2] = 0.0f;
        out[i * 2 + 1] = 0.0f;
    }
}

/*
 * Initializes and starts the output device selected by the user.
 */
bool Audio::initializeOutputDevice()
{
    // Reset the cursor position.
    //cursor.store(0, std::memory_order_relaxed);

    // Configure device parameters.
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.pDeviceID = &outputDeviceID;
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &callbackData;

    // Initialize and start device
    if (ma_device_init(&context, &deviceConfig, &outputDevice) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device." << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    ma_result result = ma_device_start(&outputDevice);

    if (result != MA_SUCCESS) {
        printf("Failed to get sound length.\n");
        uninit();
        return false;
    }

    return true;
}

/*
 * Gathers all the capture and playback device info into an array.
 */
std::vector<Audio::DeviceInfo> Audio::getDevices(ma_device_type deviceType) {
    // Create a device array.
    std::vector<DeviceInfo> devices;
    
    if (!contextInit) {
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

std::vector<Audio::DeviceInfo> Audio::getOutputDevices() {
    return getDevices(ma_device_type_playback);
}

std::vector<Audio::DeviceInfo> Audio::getInputDevices() {
    return getDevices(ma_device_type_capture);
}

/*
 * Set the output to the given device.
 */
void Audio::setOutputDevice(const char *deviceName)
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

    // An audio file has been loaded.
    if (decoderInit) {
        // Stop playback.
        ma_device_stop(&outputDevice);
        // Release device resources.
        ma_device_uninit(&outputDevice);

        if(!initializeOutputDevice()) {
            std::cerr << "Failed to initialize output device." << std::endl;
            return;
        }
    }
}

/*
 * Probes the original file format and store its data.
 */
bool Audio::storeOriginalFileFormat(const char* filename)
{
    // Initialize a temporary decoder without any config data (ie: NULL).
    ma_decoder decoderProbe;

    if (ma_decoder_init_file(filename, NULL, &decoderProbe) != MA_SUCCESS) {
        ma_decoder_uninit(&decoderProbe);
        return false;
    }

    // Retrieve data about the original file format.
    originalFileFormat.fileName = filename;
    originalFileFormat.outputChannels = decoderProbe.outputChannels;
    originalFileFormat.outputSampleRate = decoderProbe.outputSampleRate;
    originalFileFormat.outputFormat = decoderProbe.outputFormat;

    // Done probing
    ma_decoder_uninit(&decoderProbe);

    return true;
}

/*
 * Loads a given audio file.
 */
void Audio::loadFile(const char *filename)
{
    printf("Load audio file '%s'\n", filename);
    // First ensure the file format is supported.
    std::string fileFormat = std::filesystem::path(filename).extension();
    unsigned int size = supportedFormats.size();
    bool supported = false;

    // Check wether the format of the given file is supported
    for (unsigned int i = 0; i < size; i++) {
        if (fileFormat.compare(supportedFormats[i]) == 0) {
            supported = true;
            break;
        }
    }

    if (!supported) {
        std::cerr << "Format: " << fileFormat << " not supported." << std::endl;
        return;
    }

    // Check for a possible file previously loaded.
    if (decoderInit) {
        // Ensure no more callbacks are running.
        ma_device_stop(&outputDevice);
        uninit();
        decoderInit = false;
    }

    // First store the original data file format.
    if (!storeOriginalFileFormat(filename)) {
        std::cerr << "Failed to load audio file." << std::endl;
        return;
    }

    // Then initialize decoder with format conversion.
    ma_decoder_config decoderConfig = ma_decoder_config_init(defaultOutputFormat, defaultOutputChannels, defaultOutputSampleRate);

    if (ma_decoder_init_file(filename, &decoderConfig, &decoder) != MA_SUCCESS) {
        std::cerr << "Failed to initialize decoder with conversion." << std::endl;
        return;
    }

    decoderInit = true;

    if(!initializeOutputDevice()) {
        std::cerr << "Failed to initialize output device." << std::endl;
        return;
    }
}

/*
 * Displays both the input and output audio devices in the console.
 * Function used for debugging purpose.
 */
void Audio::printAllDevices()
{
    if (!contextInit) {
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

