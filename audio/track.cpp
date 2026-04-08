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
 * Clips played in overdub mode are taken into account. 
 */
size_t Track::getLength()
{
    size_t length = 0;

    for (auto clip : clips) {
        // Check for overdubed clips.
        if (clip.getTimelineStart() + clip.getLength() > length) {
            length = clip.getTimelineStart() + clip.getLength();
        }
    }

    return length;
}

bool Track::isStereo()
{
    // Check for brand new audio document (ie: recordingBuffer).
    return (recordingBuffer) ? recordingBuffer->isStereo() : clips.front().getSource()->isStereo();
}

float Track::getProcessedSample(unsigned int timelineIndex, Direction channel)
{
    float processedSample = 0.0f;

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

            processedSample += clips[i].processSample(rawSample, timelineIndex);
        }
    }

    return processedSample;
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
    endOfFile.store(false);
    endOfSelection.store(false);

    // Fill buffer.
    for (int i = 0; i < frameCount; ++i) {
        // Increment the timeline index (based on sample).
        unsigned int timelineIndex = playbackIndex.fetch_add(1, std::memory_order_relaxed);
        auto& waveform = getGUI().getWaveform();

        // Loop through existing clips.
        for (size_t j = 0; j < clips.size(); j++) {
            // Compute the gap of the clip's timeline.
            size_t clipStart = clips[j].getTimelineStart();
            size_t clipEnd = clipStart + clips[j].getLength();

            // First, check for the end of audio file.
            if (!waveform.selection() && timelineIndex >= totalLength) {
                // Inform GUI that end of file has been reached.
                endOfFile.store(true);

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
                        playbackIndex.store(waveform.getSelectionStartSample(), std::memory_order_relaxed);
                    }
                    else {
                        // Inform GUI that end of selection has been reached.
                        endOfSelection.store(true);
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
        ma_result result = ma_pcm_rb_acquire_write(&captureRing, &framesToWrite, (void**)&pDst);

        // If we can’t write anything right now, stop — ring buffer is full.
        if (result != MA_SUCCESS || framesToWrite == 0 || pDst == nullptr) {
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

    // Always set to zero since a new buffer is created for each recording.
    captureWriteIndex.store(0);
    // Clear count.
    totalRecordedFrames.store(0, std::memory_order_release);
}

void Track::play()
{
    // Optimize a bit.
    totalLength = getLength();
    // Start playback.
    playing.store(true);
}

void Track::pause() { paused.store(true); }
void Track::unpause() { paused.store(false); }

void Track::stop()
{
    playing.store(false);

    if (recording.load()) {
        stopRecording();
        // Debugging
        printClips();
    }
}

void Track::record()
{
    if (!clips.empty()) {
        // Create a new recording buffer.
        recordingBuffer = std::make_unique<Buffer>();
        recordingBuffer->clear();
        recordingBuffer->setFormat(clips.front().getSource()->getFormat());
    }

    // Make sure no flag is left as true, (or it would stop recording automatically).
    endOfFile.store(false);
    endOfSelection.store(false);

    prepareRecording();
    // Set the record start to the timeline.
    recordStart = playbackIndex.load();
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
    // No more new data available.
    newDataAvailable.store(false, std::memory_order_release);

    // Check first the recording buffer exists and something has been
    // actually recorded in the temporary sample vectors.
    if (!recordingBuffer || writeLeft.size() == 0) {
        return;
    }

    // Return possible unused memory (allocated through "reserve") to the system.
    writeLeft.shrink_to_fit();
    writeRight.shrink_to_fit();

    // Copy the new captured data to the left and right sample vectors of the buffer.
    recordingBuffer->setSamples(writeLeft, writeRight);
    // Reset the temporary sample vectors.
    writeLeft.clear();
    writeRight.clear();

    // Create a Clip from the recorded buffer (transfers ownership safely).
    std::shared_ptr<Buffer> sharedBuffer = std::move(recordingBuffer);
    Clip newClip(sharedBuffer);

    // Get the cursor initial position (0 for new track).
    auto timelineStart = (clips.empty()) ? 0 : recordStart;

    // It's the very first recording for this track.
    if (clips.empty()) {
        Clip newClip(sharedBuffer);
        newClip.setTimelineStart(timelineStart);
        clips.push_back(newClip);
    }
    else {
        replaceRecording(timelineStart, sharedBuffer);
        //overdubRecording(timelineStart, sharedBuffer);
    }

    // Stop drawing waveform.
    gui->getWaveform().stopLiveUpdate();
}

void Track::replaceRecording(size_t recordStart, std::shared_ptr<Buffer> buffer)
{
    size_t recordEnd = recordStart + buffer->getTotalFrames();

    // 1. Split boundaries
    splitClip(recordStart);
    splitClip(recordEnd);

    // 2. Remove overlapping region
    removeClips(recordStart, recordEnd);

    // 3. Insert recorded clip
    Clip newClip(buffer);
    newClip.setTimelineStart(recordStart);

    insertClip(newClip, recordStart);
}

/*
 * Layering.
 */
void Track::overdubRecording(size_t recordStart, std::shared_ptr<Buffer> buffer)
{
    Clip newClip(buffer);
    newClip.setTimelineStart(recordStart);

    clips.push_back(newClip);
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
    ma_result result = ma_pcm_rb_acquire_read(&captureRing, &framesToRead, (void**)&pSrc);

    if (result != MA_SUCCESS || framesToRead == 0 || pSrc == nullptr) {
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

    // --- Step 5: Store data ---
    size_t writeIndex = captureWriteIndex.load(std::memory_order_acquire);
    size_t newWriteEnd = writeIndex + framesToRead;
    const size_t blockSize = engine.getDefaultOutputSampleRate();

    // Reserve new required capacity beforehand to prevent multiple
    // vector memory allocations causing audio glitches.
    if (newWriteEnd > writeLeft.capacity()) {
        size_t newCapacity = ((newWriteEnd / blockSize) + 1) * blockSize;
        writeLeft.reserve(newCapacity);
        writeRight.reserve(newCapacity);
    }

    // Store recording data into temporary sample vectors. 
    writeLeft.insert(writeLeft.end(), newLeft.begin(), newLeft.end());
    writeRight.insert(writeRight.end(), newRight.begin(), newRight.end());

    // --- Update write cursor to the end of newly written region ---
    captureWriteIndex.store(newWriteEnd, std::memory_order_release);

    // Inform snapshot that new data is available.
    newDataAvailable.store(true, std::memory_order_release);

    // --- Step 6: Update stats and GUI ---
    totalRecordedFrames.fetch_add(framesToRead, std::memory_order_release);
}

void Track::workerThreadLoop()
{
    // Lower priority to avoid starving GUI
    // (optional, platform-specific)
    // setLowPriority();

    auto lastSnapshot = std::chrono::steady_clock::now();

    while (workerRunning.load(std::memory_order_acquire)) {
        drainAndMergeRingBuffer();

        auto now = std::chrono::steady_clock::now();
        // Publish every ~50 ms.
        // Make sure some new data is available.
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSnapshot).count() > 50 && newDataAvailable.exchange(false)) {
            publishSnapshot();
            lastSnapshot = now;
        }

        // Sleep 1–2 ms for smooth draining
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    // One last drain and snapshot after stop.
    drainAndMergeRingBuffer();
    publishSnapshot();
}

/*
 * Snapshot system (immutable).
 * Prevent GUI to read directly from audio thread while recording.
 * (ie: to read memory that is being reallocated by the audio
 *  thread which eventualy leads to segmentation fault).
 */
void Track::publishSnapshot()
{
    if (writeLeft.size() == 0) {
        return; 
    }

    auto snap = std::make_shared<Buffer>();
    snap->setSamples(writeLeft, writeRight);

    std::atomic_store(&snapshot, snap);
}

void Track::setNewTrack(TrackOptions options)
{
    // Set the current state of a brand new track/file.
    newTrack = true;
    // Since this is "new recording"
    clips.clear();              

    // Set the new track recording format (ie: mono/stereo, default sample rate...).
    Format format;
    format.outputChannels = options.stereo ? 2 : 1;
    format.outputSampleRate = engine.getDefaultOutputSampleRate();
    format.outputFormat = engine.getDefaultOutputFormat();

    // Create the first recording buffer for this track.
    recordingBuffer = std::make_unique<Buffer>();
    recordingBuffer->clear();
    recordingBuffer->setFormat(format);
}

void Track::loadFromFile(const char *filename)
{
    auto loader = FileIO();
    loader.load(filename, clips, engine);

    // Reset index.
    playbackIndex.store(0, std::memory_order_relaxed);
}

void Track::save(const char* filename)
{
    //auto file = FileIO();
    //file.save(filename, getBuffer());
}

void Track::updateTime()
{
    getApplication().getTime().update(playbackIndex.load());
}

/*
 * Create widgets used to visualize audio data such as waveforms, markers... 
 */
void Track::render(int x, int y, int w, int h) 
{
    gui = std::make_unique<GUI>(*this);
    gui->init(x, y, w, h);
}

/*
 * Displays the clip list in the console.
 * Function used for debugging purpose.
 */
void Track::printClips()
{
    std::cout << "---- Clip list ----" << std::endl;

    for (auto clip : clips) {
        std::cout << "timeline start: " << clip.getTimelineStart() << std::endl;
        std::cout << "length: " << clip.getLength() << std::endl;
        std::cout << "time line end: " << clip.getTimelineStart() + clip.getLength() << std::endl;
        std::cout << "source start: " << clip.getSourceStart() << std::endl;
        std::cout << "source end: " << clip.getSourceEnd() << std::endl;
        std::cout << "----------------------" << std::endl;
    }

    std::cout << "total length: " << getLength() << std::endl;
    std::cout << "----------------------" << std::endl;
}
