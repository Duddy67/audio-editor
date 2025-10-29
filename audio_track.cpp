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

    eof.store(false);

    // Fill buffer.
    for (int i = 0; i < frameCount; ++i) {
        // Increment the sample index (ie: ++).
        int idx = playbackSampleIndex.fetch_add(1);

        // End of audio file.
        if (idx >= totalFrames) {
            eof.store(true);
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

void AudioTrack::recordInto(const float* input, ma_uint32 frameCount, ma_uint32 captureChannels)
{
    std::lock_guard<std::mutex> lock(recordingMutex);

    // Check first if the track is recording.
    if (!recording.load()) {
        return;
    }

    // Store captured audio data (interleaved stereo)
    for (ma_uint32 i = 0; i < frameCount; ++i) {
        float left = 0.0f, right = 0.0f;

        if (captureChannels == 1) {
            left = input[i * 1 + 0];
            right = left; // duplicate mono -> stereo
        }
        // assume >=2: use first two channels
        else { 
            left = input[i * captureChannels + 0];
            right = input[i * captureChannels + 1];
        }

        // Check if we've reached the maximum recording length
        if (maxRecordingSamples > 0 && recordedLeft.size() >= static_cast<size_t>(maxRecordingSamples)) {
            recording.store(false);
            break;
        }

        // Left channel
        recordedLeft.push_back(left);
        // Right channel
        recordedRight.push_back(right);
    }
}

void AudioTrack::play() { playing.store(true); }
void AudioTrack::pause() { paused.store(true); }
void AudioTrack::unpause() { paused.store(false); }

void AudioTrack::stop()
{
    playing.store(false);

    if (recording.load()) {
        // Stop recording.
        recording.store(false);
        setRecordedData();
    }
}

void AudioTrack::record()
{
    std::lock_guard<std::mutex> lock(recordingMutex);

    recordingStartIndex = playbackSampleIndex.load();
    recording.store(true);
}

void AudioTrack::setRecordedData()
{
    std::lock_guard<std::mutex> lock(recordingMutex);

    if (leftSamples.size() == 0) {
        leftSamples = recordedLeft;
        rightSamples = recordedRight;
    }
    else {
        size_t endPlayback = leftSamples.size();
        int recordSampleIndex = recordingStartIndex.load();

        for (size_t i = 0; i < recordedLeft.size(); i++) {
            if (static_cast<size_t>(recordSampleIndex) < endPlayback) {
                // Replace the playback values with the newly recorded ones.
                leftSamples.at(recordSampleIndex) = recordedLeft[i];
                rightSamples.at(recordSampleIndex) = recordedRight[i];

                recordSampleIndex++;
            }
            else {
                // Add the extra recorded samples to the playback's.
                leftSamples.push_back(recordedLeft[i]);
                rightSamples.push_back(recordedRight[i]);
            }
        }
    }

    recordedLeft.clear();
    recordedRight.clear();

    totalFrames = leftSamples.size();
}

void AudioTrack::setNewTrack()
{
    stereo = engine.getDefaultOutputChannels() == 2;
}

/*
 * Loads a given audio file.
 */
void AudioTrack::loadFromFile(const char *filename)
{
    printf("Load audio file '%s'\n", filename); // Debog.
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
        throw std::runtime_error("Format: " + fileFormat + " not supported.");
    }

    // First store the original data file format.
    if (!storeOriginalFileFormat(filename)) {
        throw std::runtime_error("Failed to initialized temporary decoder.");
    }

    // Then initialize decoder with format conversion.
    ma_decoder_config decoderConfig = ma_decoder_config_init(engine.getDefaultOutputFormat(), engine.getDefaultOutputChannels(), engine.getDefaultOutputSampleRate());

    if (ma_decoder_init_file(filename, &decoderConfig, &decoder) != MA_SUCCESS) {
        throw std::runtime_error("Failed to initialize decoder with conversion.");
    }

    if (!decodeFile()) {
        throw std::runtime_error("Failed to decode file.");
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

void AudioTrack::renderWaveform(int x, int y, int w, int h) 
{
    waveform = std::make_unique<WaveformView>(x, y, w, h, *this);
    waveform->take_focus();    
    waveform->setStereoMode(isStereo());    
    waveform->setStereoSamples(getLeftSamples(), getRightSamples());
}

