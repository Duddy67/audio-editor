#include "main.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../libraries/miniaudio.h"


/*
 * Constructor
 */
AudioTrack::AudioTrack(Application* app) : pApplication(app), contextInit(false) {
    ma_context_config config = ma_context_config_init();

    // Initialize the audio context.
    if (ma_context_init(NULL, 0, &config, &context) == MA_SUCCESS) {
        contextInit = true;
        std::cerr << "Audio context initialized." << std::endl;
    }

    // Set the callbackData parameters used in the MiniAudio callback function (ie: data_callback).
    callbackData.pApplication = app;
    // Set pointers to the private members needed in the data_callback function.
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
AudioTrack::~AudioTrack() {
    uninit();

    if (contextInit) {
        ma_context_uninit(&context);
        std::cerr << "Audio context uninitialized." << std::endl;
    }
}

/*
 * Uninitializes both output device and decoder parameters.
 */
void AudioTrack::uninit()
{
    if (outputDeviceInit) {
        ma_device_uninit(&outputDevice);
    }

    if (decoderInit) {
        ma_decoder_uninit(&decoder);
    }
}

/*
 * Initializes the output device selected by the user.
 */
bool AudioTrack::initializeOutputDevice()
{
    // Configure device parameters.
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.pDeviceID = &outputDeviceID;
    deviceConfig.playback.format = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate = decoder.outputSampleRate;
    deviceConfig.dataCallback = data_callback;
    deviceConfig.pUserData = &callbackData;

    // Initialize device
    if (ma_device_init(&context, &deviceConfig, &outputDevice) != MA_SUCCESS) {
        std::cerr << "Failed to initialize playback device." << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    return true;
}

/*
 * Fills the given output buffer with interleaved stereo samples.
 */
void AudioTrack::mixInto(float* output, int frameCount) {
    // Check first if the track is playing.
    if (!playing.load()) {
        return;
    } 

    // Fill buffer.
    for (int i = 0; i < frameCount; ++i) {
        // Increment the sample index (ie: ++).
        int idx = playbackIndex.fetch_add(1);

        // End of audio file.
        if (idx >= totalFrames) {
            // Stop playback.
            playing.store(false);
            break;
        }

        // Left
        output[i * 2] += leftSamples[idx];   
        // Right
        output[i * 2 + 1] += rightSamples[idx];  
    }
}

/*
 * Probes the original file format and store its data.
 */
bool AudioTrack::storeOriginalFileFormat(const char* filename)
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
void AudioTrack::loadFromFile(const char *filename)
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

    if (!decodeFile()) {
        std::cerr << "Failed to decode file." << std::endl;
        return;
    }

    // Reset index.
    playbackSampleIndex.store(0, std::memory_order_relaxed);

    ma_decoder_uninit(&decoder);
}

/*
 * Decode the entire file manually to playback straight from memory (ie: no streaming).
 */
bool AudioTrack::decodeFile()
{
    frameCount = 0;

    if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) != MA_SUCCESS) {
        std::cerr << "Failed to get length" << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    // Create a array/buffer to hold the total number of samples (not frames!):
    // nb frames * nb channels = total nb samples
    std::vector<float> tempData(static_cast<size_t>(frameCount * decoder.outputChannels));

    ma_uint64 framesRead = 0;
    if (ma_decoder_read_pcm_frames(&decoder, tempData.data(), frameCount, &framesRead) != MA_SUCCESS) {
        std::cerr << "Failed to read PCM frames" << std::endl;
        ma_decoder_uninit(&decoder);
        return false;
    }

    // Check whether the file is stereo.
    stereo = decoder.outputChannels == 2;
    totalFrames = static_cast<int>(framesRead);

    if (stereo) {
        // Split into left/right channels
        leftSamples.clear();  rightSamples.clear();

        for (int i = 0; i < totalFrames; ++i) {
            leftSamples.push_back(tempData[i * 2]);
            rightSamples.push_back(tempData[i * 2 + 1]);
        }
    }
    // Mono data
    else {
        leftSamples = std::vector<float>(tempData);
        // Mirror for playback
        rightSamples = leftSamples;
    }

    return true;
}

/*
 * Displays both the input and output audio devices in the console.
 * Function used for debugging purpose.
 */
void AudioTrack::printAllDevices()
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

