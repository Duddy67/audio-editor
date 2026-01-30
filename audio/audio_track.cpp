#include "../main.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../../libraries/miniaudio.h"


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
        int idx = playbackSampleIndex.fetch_add(1, std::memory_order_relaxed);

        // End of audio file.
        if (idx >= totalFrames) {
            if (getApplication().isLooped()) {
                // Go back to the cursor's current position.
                playbackSampleIndex.store(getWaveform().getCursorSamplePosition(), std::memory_order_relaxed);
            }
            else {
                eof.store(true);
                // Stop playback.
                getApplication().stopTrack(*this);
            }

            // Exit the loop and function.
            break;
        }

        // Playback has reached the end of the current selection.
        if (getWaveform().selection() && idx >= getWaveform().getSelectionEndSample()) {
            if (getApplication().isLooped()) {
                // Go back to the start of the selection.
                playbackSampleIndex.store(getWaveform().getSelectionStartSample(), std::memory_order_relaxed);
            }
            else {
                // Stop playback.
                getApplication().stopTrack(*this);
            }

            // Exit the loop and function.
            break;
        }

        // --- Copy audio data to output device. ---

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
    ma_uint32 framesRemaining = frameCount;
    const float* pInput = input;

    while (framesRemaining > 0) {
        ma_uint32 framesToWrite = framesRemaining;
        float* pDst = nullptr;

        // Ask MiniAudio for a contiguous writable region.
        ma_pcm_rb_acquire_write(&captureRing, &framesToWrite, (void**)&pDst);

        // If we can’t write anything right now, stop — ring buffer is full.
        if (framesToWrite == 0 || pDst == nullptr) {
            // If buffer full, stop and log once.
            static std::atomic_flag overrunLogged = ATOMIC_FLAG_INIT;

            if (!overrunLogged.test_and_set()) {
                std::cerr << "[Overrun] Ring buffer full; dropped " << framesRemaining << "\n";
            }

            break;
        }

        // Copy only the granted portion.
        memcpy(pDst, pInput, framesToWrite * channels * sizeof(float));
        // Commit those frames.
        ma_pcm_rb_commit_write(&captureRing, framesToWrite);

        // Advance pointers/counters.
        pInput += framesToWrite * channels;
        framesRemaining -= framesToWrite;

        // If this loop completes in one iteration most of the time,
        // avoid calling acquire/commit again unnecessarily.
        if (framesRemaining == 0) {
            break;
        }
    }
}

void AudioTrack::prepareRecording()
{
    // Always record stereo.
    ma_uint32 numChannels = 2;
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

    // Reset the ring buffer so the next recording starts clean.
    ma_pcm_rb_reset(&captureRing);

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

        // Stop drawing waveform.
        waveform->stopLiveUpdate();
    }
}

void AudioTrack::record()
{
    prepareRecording();
    // Mark the document as "changed". 
    getApplication().documentHasChanged(id);
    // Start recording audio.
    recording.store(true);
    workerRunning.store(true);
    // Start drawing waveform.
    waveform->startLiveUpdate();

    // Start worker thread
    workerThread = std::thread(&AudioTrack::workerThreadLoop, this);
}

void AudioTrack::drainAndMergeRingBuffer()
{
    // Always recording stereo.
    const ma_uint32 numChannels = 2;

    // --- Step 1: Check how many frames are available in the PCM ring buffer ---
    ma_uint32 framesToRead = ma_pcm_rb_available_read(&captureRing);
    if (framesToRead == 0) {
        return;
    }

    float* pSrc = nullptr;
    ma_pcm_rb_acquire_read(&captureRing, &framesToRead, (void**)&pSrc);

    if (framesToRead == 0 || pSrc == nullptr) {
        // Nothing valid to read.
        return;
    }

    // --- Step 2: Prepare a preallocated interleaved buffer ---
    static thread_local std::vector<float> interleaved;
    interleaved.resize((size_t)framesToRead * numChannels);

    std::memcpy(interleaved.data(), pSrc, (size_t)framesToRead * numChannels * sizeof(float));
    ma_pcm_rb_commit_read(&captureRing, framesToRead);

    // --- Step 3: Preallocate temp buffers once per thread ---
    static thread_local std::vector<float> newLeft;
    static thread_local std::vector<float> newRight;

    // --- Step 4: Deinterleave directly (SIMD-friendly pattern) ---
    newLeft.resize(framesToRead);
    newRight.resize(framesToRead);
    const float* src = interleaved.data();

    if (isStereo()) {
        // Stereo: deinterleave.
        for (ma_uint32 i = 0; i < framesToRead; ++i) {
            newLeft[i]  = src[i * 2 + 0];
            newRight[i] = src[i * 2 + 1];
        }
    }
    // Mono 
    else {
        for (ma_uint32 i = 0; i < framesToRead; ++i) {
            // First deinterleave.
            const float L = src[i * 2 + 0];
            const float R = src[i * 2 + 1];
            // Then average stereo capture in left channel.
            newLeft[i] = 0.5f * (L + R);
        }

        // Mirror for playback.
        newRight = newLeft; 
    }

    // --- Step 5: Merge (Punch-In Aware) ---
    size_t writeIndex = captureWriteIndex.load(std::memory_order_acquire);
    size_t oldLength  = leftSamples.size();
    size_t newWriteEnd = writeIndex + framesToRead;
    const size_t blockSize = engine.getDefaultOutputSampleRate();

    // Reserve new required capacity beforehand to prevent multiple
    // vector memory allocations causing audio glitches.
    if (newWriteEnd > leftSamples.capacity()) {
        size_t newCapacity = ((newWriteEnd / blockSize) + 1) * blockSize;
        leftSamples.reserve(newCapacity);
        rightSamples.reserve(newCapacity);
    }

    // --- Step 6: Merge using direct pointer access (handles partial overlap - faster than push_back loop) ---
    if (writeIndex < oldLength) {
        // Compute how many frames fit inside the current buffer.
        size_t overwriteCount = std::min<size_t>(framesToRead, oldLength - writeIndex);

        // Overwrite the existing region.
        std::copy_n(newLeft.begin(), overwriteCount, leftSamples.begin() + writeIndex);
        std::copy_n(newRight.begin(), overwriteCount, rightSamples.begin() + writeIndex);

        // If there are still extra frames beyond oldLength, append them.
        if (overwriteCount < framesToRead) {
            size_t appendCount = framesToRead - overwriteCount;
            leftSamples.insert(leftSamples.end(),
                               newLeft.begin() + overwriteCount,
                               newLeft.begin() + overwriteCount + appendCount);
            rightSamples.insert(rightSamples.end(),
                                newRight.begin() + overwriteCount,
                                newRight.begin() + overwriteCount + appendCount);
        }
    }
    else {
        // Entirely beyond old length → just append.
        leftSamples.insert(leftSamples.end(), newLeft.begin(), newLeft.end());
        rightSamples.insert(rightSamples.end(), newRight.begin(), newRight.end());
    }

    // --- Update write cursor to the end of newly written region ---
    captureWriteIndex.store(newWriteEnd, std::memory_order_release);

    // --- Step 7: Update stats and GUI ---
    totalRecordedFrames.fetch_add(framesToRead, std::memory_order_release);
    totalFrames = leftSamples.size();

    // --- Step 8: Update dirty range atomically (for GUI) ---
    size_t prevStart = dirtyStart.load(std::memory_order_acquire);
    size_t prevEnd   = dirtyEnd.load(std::memory_order_acquire);

    // Extend range atomically
    if (prevStart == SIZE_MAX || writeIndex < prevStart) {
        dirtyStart.store(writeIndex, std::memory_order_release);
    }

    if (newWriteEnd > prevEnd) {
        dirtyEnd.store(newWriteEnd, std::memory_order_release);
    }

    newDataAvailable.store(true, std::memory_order_release);
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

// Safe method that copies only new data
bool AudioTrack::getNewSamplesCopy(std::vector<float>& leftCopy, std::vector<float>& rightCopy, size_t& newStartIndex, size_t& newCount) {
    if (!newDataAvailable.load(std::memory_order_acquire)) {
        return false;
    }

    // Atomically grab and reset dirty range
    size_t start = dirtyStart.exchange(SIZE_MAX, std::memory_order_acq_rel);
    size_t end   = dirtyEnd.exchange(0, std::memory_order_acq_rel);
    newDataAvailable.store(false, std::memory_order_release);

    if (start == SIZE_MAX || end <= start || end > leftSamples.size()) {
        return false;
    }

    newStartIndex = start;
    newCount = end - start;

    // Copy or overwrite a new chunk of recorded data.
    leftCopy.assign(leftSamples.begin() + start, leftSamples.begin() + end);
    rightCopy.assign(rightSamples.begin() + start, rightSamples.begin() + end);

    return true;
}

void AudioTrack::setNewTrack(TrackOptions options)
{
    newTrack = true;
    // Set the track recording format (ie: mono/stereo).
    stereo = options.stereo;
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
        leftSamples.clear(); 
        rightSamples.clear();

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

void AudioTrack::save(const char* filename)
{
    ma_encoder_config config = ma_encoder_config_init(
        ma_encoding_format_wav,
        ma_format_f32,      // 32-bit float samples
        2,                  // stereo
        44100               // sample rate (adjust to your app)
    );

    ma_encoder encoder;
    if (ma_encoder_init_file(filename, &config, &encoder) != MA_SUCCESS) {
        printf("Failed to initialize encoder.\n");
        return;
    }

    // Interleave the samples
    size_t frameCount = leftSamples.size();
    std::vector<float> interleaved(frameCount * 2);
    for (size_t i = 0; i < frameCount; ++i) {
        interleaved[i * 2 + 0] = leftSamples[i];
        interleaved[i * 2 + 1] = rightSamples[i];
    }

    // Write audio data
    ma_uint64 framesWritten = 0;
    ma_encoder_write_pcm_frames(&encoder, interleaved.data(), frameCount, &framesWritten);

    // Clean up
    ma_encoder_uninit(&encoder);

    printf("Wrote %llu frames to %s\n", framesWritten, filename);
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

void AudioTrack::updateTime()
{
    getApplication().getTime().update(playbackSampleIndex.load());
}

/*
 * Create widgets used to visualize audio data such as waveforms, markers... 
 */
void AudioTrack::render(int x, int y, int w, int h) 
{
    marking = std::make_unique<Marking>(x, y, w, MARKING_AREA_HEIGHT);
    waveform = std::make_unique<WaveformView>(x, y + MARKING_AREA_HEIGHT, w, h - MARKING_AREA_HEIGHT, *this, *marking);
    waveform->take_focus();    
    waveform->setStereoMode(isStereo());    
    waveform->setStereoSamples(getLeftSamples(), getRightSamples());
}

