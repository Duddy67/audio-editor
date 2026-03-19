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
 * Computes and returns the length sum of all clips. 
 */
size_t Track::getLength()
{
    size_t length = 0;

    for (auto clip : clips) {
        length += clip.getLength();
    }

    return length;
}

bool Track::isStereo()
{
    /*if (clips.empty()) {
        throw std::runtime_error("No clip!");
    }

    return clips.front().getSource()->isStereo();*/
    return (recordingBuffer) ? recordingBuffer->isStereo() : clips.front().getSource()->isStereo();
}

// TEMPORARY!
/*Buffer& Track::getSource()
{
    if (clips.empty()) {
        throw std::runtime_error("No clip!");
    }

    return *clips.front().getSource();
}*/

float Track::getProcessedSample(unsigned int timelineIndex, Direction channel)
{
    // Loop through existing clips.
    for (size_t i = 0; i < clips.size(); i++) {
        // Compute the gap of the clip's timeline.
        size_t clipStart = clips[i].getTimelineStart();
        size_t clipEnd = clipStart + clips[i].getLength();

        // Check if current playback position is inside the clip's timeline.
        if (timelineIndex >= clipStart && timelineIndex < clipEnd) {
            // Compute the actual source sample to read from. Map between the clip's
            // timeline and the corresponding source region.
            size_t sourceIndex = clips[i].getSourceStart() + (timelineIndex - clipStart);

            float rawSample = channel == Direction::LEFT ? clips[i].getSource()->getLeftSamples()[sourceIndex] : clips[i].getSource()->getRightSamples()[sourceIndex];

            return clips[i].processSample(rawSample, timelineIndex);
        }
    }

    return 0.0f;
}

void Track::splitClip(size_t position)
{
    // Loop through existing clips.
    for (size_t i = 0; i < clips.size(); i++) {
        Clip& clip = clips[i];

        // Compute the clip's boundaries.
        size_t start = clip.getTimelineStart();
        size_t end = start + clip.getLength();

        // Not inside this clip.
        if (position <= start || position >= end) {
            continue;
        }

        // Offset inside clip.
        size_t offset = position - start;

        // Create second clip.
        Clip newClip = clip;

        // Adjust first clip.
        clip.setLength(offset);

        // Configure new clip.
        newClip.setTimelineStart(position);
        newClip.setSourceStart(clip.getSourceStart() + offset);
        newClip.setLength(end - position);

        // Insert new clip after current one.
        clips.insert(clips.begin() + i + 1, newClip);

        return;
    }

    return;
}

void Track::insertClip(Clip clip, size_t position)
{
    clip.setTimelineStart(position);

    // Find insertion point (clips are timeline sorted)
    auto it = std::find_if(
        clips.begin(),
        clips.end(),
        [position](const Clip& c) {
            return c.getTimelineStart() > position;
        });

    clips.insert(it, clip);
}

void Track::removeClips(size_t start, size_t end)
{
    if (start >= end) {
        return;
    }

    // Ensure boundaries align with clip edges.
    splitClip(start);
    splitClip(end);

    for (auto it = clips.begin(); it != clips.end();) {
        size_t clipStart = it->getTimelineStart();
        size_t clipEnd   = clipStart + it->getLength();

        if (clipStart >= start && clipEnd <= end) {
            it = clips.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Track::copy(size_t start, size_t end)
{
    // Retrieve and clear the global clipboard.
    std::vector<Clip>& clipboard = engine.getClipboard();
    clipboard.clear();

    splitClip(start);
    splitClip(end);

    for (const Clip& clip : clips) {
        size_t clipStart = clip.getTimelineStart();
        size_t clipEnd   = clipStart + clip.getLength();

        if (clipStart >= start && clipEnd <= end) {
            Clip copy = clip;

            // normalize timeline to clipboard origin
            copy.setTimelineStart(clipStart - start);

            clipboard.push_back(copy);
        }
    }
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
        unsigned int timelineIndex = playbackSampleIndex.fetch_add(1, std::memory_order_relaxed);
        auto& waveform = getGUI().getWaveform();

        // Loop through existing clips.
        for (size_t j = 0; j < clips.size(); j++) {
            // Compute the gap of the clip's timeline.
            size_t clipStart = clips[j].getTimelineStart();
            size_t clipEnd = clipStart + clips[j].getLength();

            // First, check for the end of audio file.
            if (j == clips.size() - 1 && timelineIndex >= clipEnd && !waveform.selection()) {
                // Inform GUI that end of file has been reached.
                eof.store(true);

                // Fill remaining frames with silence
                output[i * 2] += 0.0f;
                output[i * 2 + 1] += 0.0f;

                // Exit loop.
                break;
            }

            // Check if current playback position is inside the clip's timeline.
            if (timelineIndex >= clipStart && timelineIndex < clipEnd) {
                // Playback has reached the end of the current selection.
                if (waveform.selection() && timelineIndex >= static_cast<unsigned int>(waveform.getSelectionEndSample())) {
                    if (getApplication().isLooped()) {
                        // Go back to the start of the selection.
                        playbackSampleIndex.store(waveform.getSelectionStartSample(), std::memory_order_relaxed);
                    }
                    else {
                        // Inform GUI that end of selection has been reached.
                        eos.store(true);
                    }

                    // Exit loop.
                    break;
                }

                // Compute the actual source sample to read from. Map between the clip's
                // timeline and the corresponding source region.
                size_t sourceIndex = clips[j].getSourceStart() + (timelineIndex - clipStart);

                float rawL = clips[j].getSource()->getLeftSamples()[sourceIndex];
                float rawR = clips[j].getSource()->getRightSamples()[sourceIndex];

                float processedL = clips[j].processSample(rawL, timelineIndex);
                float processedR = clips[j].processSample(rawR, timelineIndex);

                // --- Copy audio data to output device. ---

                // Left
                output[i * 2] += processedL;
                // Right
                output[i * 2 + 1] += processedR;
            }
        }
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
        stopRecording();
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

void Track::stopRecording()
{
    // Stop recording audio.
    recording.store(false);
    workerRunning.store(false);

    // Join worker thread
    if (workerThread.joinable()) {
        workerThread.join();
    }

    // Done using the ring buffer.
    ma_pcm_rb_uninit(&captureRing);

    // Check first the recording buffer exists and something has been actually recorded.
    if (!recordingBuffer || recordingBuffer->getTotalFrames() == 0) {
        return;
    }

    // Return possible unused memory (allocated through "reserve") to the system.
    recordingBuffer->getLeftSamples().shrink_to_fit();
    recordingBuffer->getRightSamples().shrink_to_fit();

    // Create a Clip from the recorded buffer (transfers ownership safely).
    std::shared_ptr<Buffer> sharedBuffer = std::move(recordingBuffer);
    Clip newClip(sharedBuffer);

    // Get the cursor initial position (0 for new track).
    auto timelineStart = (clips.empty()) ? 0 : static_cast<size_t>(gui->getWaveform().getStartSamplePosition());
    newClip.setTimelineStart(timelineStart); 

    clips.push_back(newClip);

    // Stop drawing waveform.
    gui->getWaveform().stopLiveUpdate();
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
    auto& leftSamples = recordingBuffer->getLeftSamples();
    auto& rightSamples = recordingBuffer->getRightSamples();
    //auto& leftSamples = getSource().getLeftSamples();
    //auto& rightSamples = getSource().getRightSamples();
    size_t writeIndex = captureWriteIndex.load(std::memory_order_acquire);
    size_t oldLength  = leftSamples.size();
    size_t newWriteEnd = writeIndex + framesToRead;
    const size_t blockSize = engine.getDefaultOutputSampleRate();

    // Reserve new required capacity beforehand to prevent multiple
    // vector memory allocations causing audio glitches.
    if (newWriteEnd > leftSamples.capacity()) {
        size_t newCapacity = ((newWriteEnd / blockSize) + 1) * blockSize;
        //getSource().reserve(newCapacity);
        recordingBuffer->reserve(newCapacity);
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
    recordingBuffer = std::make_unique<Buffer>();
    recordingBuffer->clear();
    // Since this is "new recording"
    clips.clear();              

    // Set the track recording format (ie: mono/stereo).
    auto file = FileIO();
    //file.setNewFileFormat(getSource().getFormat(), options.stereo, engine);
    file.setNewFileFormat(recordingBuffer->getFormat(), options.stereo, engine);
}

void Track::loadFromFile(const char *filename)
{
    auto loader = FileIO();
    loader.load(filename, clips, engine);

    // Reset index.
    playbackSampleIndex.store(0, std::memory_order_relaxed);
}

void Track::save(const char* filename)
{
    //auto file = FileIO();
    //file.save(filename, getBuffer());
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

