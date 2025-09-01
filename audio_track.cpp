#include "main.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../libraries/miniaudio.h"


void AudioTrack::setId(unsigned int i)
{
    // Make sure ID is initialized only once.
    if (id > 0) {
        std::cout << "ID already initialized." << std::endl;
        return;
    }

    id = i;
}

/*
 * Fills the given output buffer with interleaved stereo samples.
 */
void AudioTrack::mixInto(float* output, int frameCount) 
{
    // Check first if the track is playing.
    if (!playing.load()) {
        return;
    } 

    // Fill buffer.
    for (int i = 0; i < frameCount; ++i) {
        // Increment the sample index (ie: ++).
        int idx = playbackSampleIndex.fetch_add(1);

        // End of audio file.
        if (idx >= totalFrames) {
            // Stop playback.
            playing.store(false);
            // Exit the loop / function.
            break;
        }

        // Left
        output[i * 2] += leftSamples[idx];   
        // Right
        output[i * 2 + 1] += rightSamples[idx];  
    }
}

void AudioTrack::play()  { playing.store(true); }
void AudioTrack::pause() { playing.store(false); }
void AudioTrack::stop()  { playing.store(false); playbackSampleIndex.store(0); }
void AudioTrack::seek(int frame) {
    if (frame >= 0 && frame < totalFrames)
        playbackSampleIndex.store(frame);
}

/*
 * Loads a given audio file.
 */
bool AudioTrack::loadFromFile(const char *filename)
{
    printf("Load audio file '%s'\n", filename);
    // First ensure the file format is supported.
    std::string fileFormat = std::filesystem::path(filename).extension();
    std::vector<std::string> supportedFormats = engine.getSupportedFormats();
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
        return false;
    }

    // First store the original data file format.
    if (!storeOriginalFileFormat(filename)) {
        std::cerr << "Failed to load audio file." << std::endl;
        return false;
    }

    // Then initialize decoder with format conversion.
    ma_decoder_config decoderConfig = ma_decoder_config_init(engine.getDefaultOutputFormat(), engine.getDefaultOutputChannels(), engine.getDefaultOutputSampleRate());

    if (ma_decoder_init_file(filename, &decoderConfig, &decoder) != MA_SUCCESS) {
        std::cerr << "Failed to initialize decoder with conversion." << std::endl;
        return false;
    }

    if (!decodeFile()) {
        std::cerr << "Failed to decode file." << std::endl;
        return false;
    }

    // Reset index.
    playbackSampleIndex.store(0, std::memory_order_relaxed);

    ma_decoder_uninit(&decoder);

    return true;
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

