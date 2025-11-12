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

    // Make sure playback never tries to read buffers while
    // a recording session is in progress.
    if (recording.load()) {
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
    // Check first if the track is recording.
    if (!recording.load()) {
        return;
    }

    // Read current write index (ie: number of frames already stored).
    size_t writeIndex = recordedFrameCount.load(std::memory_order_relaxed);
    // Compute the possible extra buffer capacity needed.
    size_t needed = writeIndex + frameCount;

    if (needed > recordedLeft.size()) {
        size_t currentCapacity = recordedLeft.size();
        // Grow buffers dynamically (e.g. 1.5x rule)
        size_t newSize = std::max(needed, currentCapacity + (currentCapacity / 2) + (engine.getDefaultOutputSampleRate() * 10));
        recordedLeft.resize(newSize);
        recordedRight.resize(newSize);
        // Update with the new size.
        maxRecordedFrames = newSize;
    }

    // For safety: avoid overrunning the preallocated buffer.
    const size_t capacity = maxRecordedFrames;

    // Store captured samples in each frame (ie: Mono => 1 frame = 1 sample, Stereo => 1 frame = 2 samples). 
    for (ma_uint32 f = 0; f < frameCount && writeIndex < capacity; ++f) {
        if (captureChannels == 1) {
            // Mono.
            float sample = input[f]; 
            recordedLeft[writeIndex] = sample;
            recordedRight[writeIndex] = sample;
        }
        // captureChannels == 2: Read first two channels as stereo.
        else {
            // Input interleaved: L R L R...
            recordedLeft[writeIndex] = input[f * captureChannels + 0];
            recordedRight[writeIndex] = input[f * captureChannels + 1];
        }
        captureSampleIndex.fetch_add(1);

        // Check if we've reached the maximum recording length
        /*if (maxRecordingSamples > 0 && recordedLeft.size() >= static_cast<size_t>(maxRecordingSamples)) {
            recording.store(false);
            break;
        }*/

        ++writeIndex;
    }

    // Publish the new frame count only *after* writing (release semantics)
    recordedFrameCount.store(writeIndex, std::memory_order_release);
}

void AudioTrack::prepareRecording()
{
    // Compute capacity in frames (frames == samples per channel).
    maxRecordedFrames = INITIAL_BUFFER_SIZE * engine.getDefaultOutputSampleRate();

    // Allocate the pre-allocated buffers.
    recordedLeft.resize(maxRecordedFrames);
    recordedRight.resize(maxRecordedFrames);

    // Reset published count.
    recordedFrameCount.store(0, std::memory_order_release);
}

void AudioTrack::play() { playing.store(true); }
void AudioTrack::pause() { paused.store(true); }
void AudioTrack::unpause() { paused.store(false); }

void AudioTrack::stop()
{
    playing.store(false);

    if (recording.load()) {
        // Stop recording audio.
        recording.store(false);
        // Stop drawing waveform.
        waveform->stopLiveUpdate();
        setRecordedData();
    }
}

void AudioTrack::record()
{
    prepareRecording();
    // Synchronize indexes.
    recordingStartIndex = captureSampleIndex = playbackSampleIndex.load();
    // Start recording audio.
    recording.store(true);
    // Start drawing waveform.
    waveform->startLiveUpdate();
}

void AudioTrack::setRecordedData()
{
    // Protects both recordedLeft and recordedRight vectors from race conditions.
    std::lock_guard<std::mutex> lock(recordingMutex);

    // Number of valid recorded frames.
    size_t validFrames = recordedFrameCount.load(std::memory_order_acquire);

    // First recording.
    if (leftSamples.empty()) {
        // Just copy the valid part.
        leftSamples.assign(recordedLeft.begin(),  recordedLeft.begin()  + validFrames);
        rightSamples.assign(recordedRight.begin(), recordedRight.begin() + validFrames);
    }
    else {
        size_t endPlayback = leftSamples.size();
        int recordSampleIndex = recordingStartIndex.load();

        for (size_t i = 0; i < validFrames; ++i) {
            if (static_cast<size_t>(recordSampleIndex) < endPlayback) {
                // Replace the playback values with the newly recorded ones.
                leftSamples.at(recordSampleIndex) = recordedLeft[i];
                rightSamples.at(recordSampleIndex) = recordedRight[i];

                ++recordSampleIndex;
            }
            else {
                // Add the extra recorded samples to the playback's.
                leftSamples.push_back(recordedLeft[i]);
                rightSamples.push_back(recordedRight[i]);
            }
        }
    }

    totalFrames = leftSamples.size();
}

void AudioTrack::setNewTrack()
{
    newTrack = true;
    // Set the recording format (ie: mono/stereo).
    stereo = engine.getDefaultInputChannels() == 2;
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

    // Then initialize decoder with format conversion (except for output channels).
    ma_decoder_config decoderConfig = ma_decoder_config_init(engine.getDefaultOutputFormat(), originalFileFormat.outputChannels, engine.getDefaultOutputSampleRate());

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

