#include "../main.h"
#include "file_io.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../../libraries/miniaudio.h"


void Track::setId(unsigned int i)
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
void Track::mixInto(float* output, int frameCount) 
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

    // Reset end of file and selection flags.
    eof.store(false);
    eos.store(false);

    // Fill buffer.
    for (int i = 0; i < frameCount; ++i) {
        // Increment the sample index (ie: ++).
        unsigned int idx = playbackSampleIndex.fetch_add(1, std::memory_order_relaxed);
        auto& waveform = getGUI().getWaveform();

        // End of audio file, no selection.
        if (idx >= buffer->getTotalFrames() && !waveform.selection()) {
            // Inform GUI that end of file has been reached.
            eof.store(true);
            // Exit loop and function.
            break;
        }

        // Playback has reached the end of the current selection.
        if (waveform.selection() && idx >= static_cast<unsigned int>(waveform.getSelectionEndSample())) {
            if (getApplication().isLooped()) {
                // Go back to the start of the selection.
                playbackSampleIndex.store(waveform.getSelectionStartSample(), std::memory_order_relaxed);
            }
            else {
                // Inform GUI that end of selection has been reached.
                eos.store(true);
            }

            // Exit loop and function.
            break;
        }

        // --- Copy audio data to output device. ---

        auto& left = buffer->getLeftSamples();
        auto& right = buffer->getRightSamples();

        // Left
        output[i * 2] += left[idx];   
        // Right
        output[i * 2 + 1] += right[idx];  
    }
}

void Track::recordInto(const float* input, ma_uint32 frameCount, ma_uint32 captureChannels)
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

void Track::prepareRecording()
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

void Track::play() { playing.store(true); }
void Track::pause() { paused.store(true); }
void Track::unpause() { paused.store(false); }

void Track::stop()
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
        buffer->getLeftSamples().shrink_to_fit();
        buffer->getRightSamples().shrink_to_fit();

        // Stop drawing waveform.
        gui->getWaveform().stopLiveUpdate();
    }
}

void Track::record()
{
    prepareRecording();
    // Mark the document as "changed". 
    getApplication().documentHasChanged(id);
    // Start recording audio.
    recording.store(true);
    workerRunning.store(true);
    // Start drawing waveform.
    gui->getWaveform().startLiveUpdate();

    // Start worker thread
    workerThread = std::thread(&Track::workerThreadLoop, this);
}

void Track::drainAndMergeRingBuffer()
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

    if (buffer->isStereo()) {
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
    auto& leftSamples = buffer->getLeftSamples();
    auto& rightSamples = buffer->getRightSamples();
    size_t writeIndex = captureWriteIndex.load(std::memory_order_acquire);
    size_t oldLength  = leftSamples.size();
    size_t newWriteEnd = writeIndex + framesToRead;
    const size_t blockSize = engine.getDefaultOutputSampleRate();

    // Reserve new required capacity beforehand to prevent multiple
    // vector memory allocations causing audio glitches.
    if (newWriteEnd > leftSamples.capacity()) {
        size_t newCapacity = ((newWriteEnd / blockSize) + 1) * blockSize;
        buffer->reserve(newCapacity);
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

    // --- Step 8: Update dirty range atomically (for GUI) ---
    size_t prevStart = gui->getDirtyStart().load(std::memory_order_acquire);
    size_t prevEnd   = gui->getDirtyEnd().load(std::memory_order_acquire);

    // Extend range atomically
    if (prevStart == SIZE_MAX || writeIndex < prevStart) {
        gui->getDirtyStart().store(writeIndex, std::memory_order_release);
    }

    if (newWriteEnd > prevEnd) {
        gui->getDirtyEnd().store(newWriteEnd, std::memory_order_release);
    }

    newDataAvailable.store(true, std::memory_order_release);
}

void Track::workerThreadLoop()
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

void Track::setNewTrack(TrackOptions options)
{
    newTrack = true;
    // Set the track recording format (ie: mono/stereo).
    auto file = FileIO();
    file.setNewFileFormat(buffer->getFormat(), options.stereo, getEngine());
}

void Track::loadFromFile(const char *filename)
{
    auto loader = FileIO();
    loader.load(filename, getBuffer(), getEngine());

    // Reset index.
    playbackSampleIndex.store(0, std::memory_order_relaxed);
}

void Track::save(const char* filename)
{
    auto file = FileIO();
    file.save(filename, getBuffer());
}

void Track::updateTime()
{
    getApplication().getTime().update(playbackSampleIndex.load());
}

/*
 * Create widgets used to visualize audio data such as waveforms, markers... 
 */
void Track::render(int x, int y, int w, int h) 
{
    gui = std::make_unique<GUI>(*this);
    gui->init(x, y, w, h);
}

