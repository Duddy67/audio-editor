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

    ma_uint32 channels = captureChannels;
    ma_uint32 framesToWrite = frameCount;

    // Write to ring buffer.
    float* pDst;
    ma_pcm_rb_acquire_write(&captureRing, &framesToWrite, (void**)&pDst);

    if (framesToWrite > 0 && pDst != nullptr) {
        memcpy(pDst, input, framesToWrite * channels * sizeof(float));
        ma_pcm_rb_commit_write(&captureRing, framesToWrite);
    }
}

void AudioTrack::prepareRecording()
{
    ma_uint32 numChannels = isStereo() ? 2 : 1;
    // Compute capacity in frames (frames == samples per channel).
    ma_uint32 capacityFrames = INITIAL_BUFFER_SIZE * engine.getDefaultOutputSampleRate();

    // Initialize the PCM ring buffer directly (no config struct)
    ma_result result = ma_pcm_rb_init(
        ma_format_f32,
        numChannels,
        capacityFrames,
        nullptr,
        nullptr,
        &captureRing
    );

    if (result != MA_SUCCESS) {
        std::cout << "Failed to initialize ring buffer\n" << std::endl;
        return;
    }

    // Set the start of the recording to the actual position of the cursor.
    // ie: zero for the very first recording or wherever the cursor is 
    // positioned for the next recordings.
    captureWriteIndex.store(playbackSampleIndex.load());
    // Clear count.
    totalRecordedFrames.store(0, std::memory_order_release);
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
        //waveform->stopLiveUpdate();
        workerRunning.store(false);

        // Join worker thread
        if (workerThread.joinable()) {
            workerThread.join();
        }

        // Done using the ring buffer.
        ma_pcm_rb_uninit(&captureRing);
        // Return possible unused memory (allocated through "reserve") to the system.
        leftSamples.shrink_to_fit();
        rightSamples.shrink_to_fit();
    }
}

void AudioTrack::record()
{
    prepareRecording();
    // Start recording audio.
    recording.store(true);
    // Start drawing waveform.
    //waveform->startLiveUpdate();
    workerRunning.store(true);

    // Start worker thread
    workerThread = std::thread(&AudioTrack::workerThreadLoop, this);
}

void AudioTrack::drainAndMergeRingBuffer()
{
    const ma_uint32 numChannels = isStereo() ? 2 : 1;

    // How many frames are available in the PCM ring buffer ?
    ma_uint32 framesToRead = ma_pcm_rb_available_read(&captureRing);
    if (framesToRead == 0) {
        return;
    }

    // TEMP INTERLEAVED READ BUFFER
    std::vector<float> interleaved(framesToRead * numChannels);
    float* pSrc = nullptr;

    ma_pcm_rb_acquire_read(&captureRing, &framesToRead, (void**)&pSrc);
    if (framesToRead == 0 || !pSrc) {
        // Nothing valid to read.
        return; 
    }

    memcpy(interleaved.data(), pSrc, framesToRead * numChannels * sizeof(float));
    ma_pcm_rb_commit_read(&captureRing, framesToRead);

    // --- Deinterleave into temporary buffers ---
    std::vector<float> newLeft(framesToRead);
    std::vector<float> newRight(framesToRead);

    for (ma_uint32 i = 0; i < framesToRead; ++i) {
        newLeft[i]  = interleaved[i * numChannels + 0];
        newRight[i] = (numChannels > 1)
                        ? interleaved[i * numChannels + 1]
                        : interleaved[i * numChannels + 0];
    }

    // --- Merge (Punch-In Aware) ---
    size_t writeIndex = captureWriteIndex.load(std::memory_order_acquire);
    size_t oldLength  = leftSamples.size();
    size_t newWriteEnd = writeIndex + framesToRead;
    // 1 second for 44.1 kHz
    const size_t blockSize = engine.getDefaultOutputSampleRate();
    size_t required = newWriteEnd;

    // Reserve new required capacity beforehand to prevent multiple 
    // vector memory allocations causing audio glitches.
    if (required > leftSamples.capacity()) {
        size_t newCapacity = ((required / blockSize) + 1) * blockSize;
        leftSamples.reserve(newCapacity);
        rightSamples.reserve(newCapacity);
    }

    // Overwrite or append frame-by-frame.
    for (size_t i = 0; i < framesToRead; ++i) {

        size_t pos = writeIndex + i;

        if (pos < oldLength) {
            // Overwrite existing audio.
            leftSamples[pos]  = newLeft[i];
            rightSamples[pos] = newRight[i];
        }
        else {
            // Append new audio.
            leftSamples.push_back(newLeft[i]);
            rightSamples.push_back(newRight[i]);
        }
    }

    // --- Update write cursor to the end of newly written region ---
    captureWriteIndex.store(newWriteEnd, std::memory_order_release);

    // --- Update stats and GUI ---
    totalRecordedFrames.fetch_add(framesToRead, std::memory_order_release);
    totalFrames = leftSamples.size();
}

void AudioTrack::workerThreadLoop()
{
    // Lower priority to avoid starving GUI
    // (optional, platform-specific)
    // setLowPriority();

    while (workerRunning.load(std::memory_order_acquire)) {
        drainAndMergeRingBuffer();

        // Sleep 1–2 ms for smooth draining
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // One last drain after stop
    drainAndMergeRingBuffer();
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

