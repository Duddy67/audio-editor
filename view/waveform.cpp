#include "../audio/track.h"
#include "../main.h"


void Waveform::initView()
{
    isStereo = track.isStereo();

    // Fit entire waveform on screen initially.
    if (track.getLength() != 0) {
        // Compute fit-to-screen zoom (pixels per sample that fits entire file).
        zoomFit = static_cast<float>(w()) / static_cast<float>(track.getLength());
        // Allow zooming out beyond fit-to-screen.
        // Note: Tweak factor (0.01 = 100× smaller than fit).
        zoomMin = zoomFit * 0.01f;

        if (zoomMax <= zoomMin) {
            // Fallback if zoomMax wasn't sensible.
            zoomMax = zoomMin * 100.0f;
        }

        // Start at fit-to-screen.
        zoomLevel = zoomFit;
    }
    else {
        zoomLevel = 1.0f;
        zoomFit = zoomMin = 1.0f;
    }

    scrollOffset = 0;
    updateScrollbar();
    redraw();
}

void Waveform::setScrollOffset(int offset) {
    scrollOffset = std::max(0, offset);
    updateScrollbar();
    redraw();
}

void Waveform::setScrollbar(Fl_Scrollbar* sb) {
    scrollbar = sb;
    updateScrollbar();
}

void Waveform::updateScrollbar() {
    if (!scrollbar || track.getLength() == 0) return;
    int visibleSamples = static_cast<int>(w() / zoomLevel);
    int maxOffset = std::max(0, (int)track.getLength() - visibleSamples);
    scrollOffset = std::clamp(scrollOffset, 0, maxOffset);
    scrollbar->maximum(maxOffset);
    scrollbar->value(scrollOffset);
    scrollbar->slider_size((float)visibleSamples / track.getLength());
}

void Waveform::prepareForRecording()
{
    //recordingStartTimeline = track.getCurrentSample();
    recordingStartTimeline = cursorSamplePosition;
    scrollOffset = 0;
    recordingStartSample = cursorSamplePosition;
    lastSyncedSample = recordingStartSample;

    // ===== Fix a 50% zoom value ====
    // Compute a comfortable starting zoom so waveform grows naturally
    zoomFit = static_cast<float>(w()) / static_cast<float>(44100 * 5); // 5 s fits width
    zoomLevel = zoomFit;
    zoomMin = zoomFit * 0.01f;
    zoomMax = zoomFit * 100.0f;
    // ==============

    updateScrollbar();
    redraw();
}

float Waveform::getDisplaySample(size_t timelineIndex, Direction channel)
{
    // --- 1. If recording → try live recording first ---
    if (track.isRecording()) {
        size_t recStart = track.getRecordingStartSample();

        if (timelineIndex >= recStart) {
            auto snap = track.getRecordingSnapshot();

            if (snap) {
                size_t localIndex = timelineIndex - recStart;
                const auto& samples = (channel == Direction::LEFT) ? snap->getLeftSamples() : snap->getRightSamples();

                if (localIndex < samples.size()) {
                    return samples[localIndex]; // LIVE recording wins
                }
            }

            // Recording zone but not yet filled
            return 0.0f;
        }
    }

    // --- 2. Fallback to clip system ---
    return track.getProcessedSample(timelineIndex, channel);
}

float Waveform::getRecordedSample(unsigned int timelineIndex, Direction channel)
{
    auto snap = track.getRecordingSnapshot();

    if (!snap) {
      return 0.0f;
    }

    size_t recordingStart = track.getRecordingStartSample(); // IMPORTANT

    if (timelineIndex < recordingStart) {
        return 0.0f;
    }

    size_t localIndex = timelineIndex - recordingStart;

    const auto& samples = (channel == Direction::LEFT)
        ? snap->getLeftSamples()
        : snap->getRightSamples();

    if (localIndex >= samples.size()) {
        return 0.0f;
    }

    return samples[timelineIndex];
}

/*
 * Check whether a selection is currently set.
 */
bool Waveform::selection()
{
    if (selectionStartSample >= 0 && selectionEndSample > 0 && selectionStartSample < selectionEndSample) {
        return true;
    }

    return false;
}

void Waveform::deleteSelection()
{
    selectionStartSample = 0; 
    selectionEndSample = 0;
}

/*
 * Computes the last drawn x position.
 */
float Waveform::getLastDrawnX() 
{
    int totalSamples = track.getLength();
    int visibleSamples = visibleSamplesCount();
    int endSample = scrollOffset + visibleSamples;

    // Compute and return last drawn x position.
    return (float)(std::min(endSample, totalSamples) - scrollOffset) * zoomLevel;
}

void Waveform::buildWaveformCache(Track& track)
{
    size_t totalSamples = track.getLength();
    const size_t samplesPerBucket = 128; // or 512, 1024
    size_t bucketCount = totalSamples / samplesPerBucket;

    cache.samplesPerBucket = samplesPerBucket;
    cache.minL.resize(bucketCount);
    cache.maxL.resize(bucketCount);
    cache.minR.resize(bucketCount);
    cache.maxR.resize(bucketCount);

    for (size_t b = 0; b < bucketCount; ++b) {
        float minValL = 1.0f;
        float maxValL = -1.0f;
        float minValR = 1.0f;
        float maxValR = -1.0f;

        size_t start = b * samplesPerBucket;
        size_t end = std::min(start + samplesPerBucket, totalSamples);

        for (size_t i = start; i < end; ++i) {
            float L = track.getProcessedSample(i, Direction::LEFT);
            float R = track.getProcessedSample(i, Direction::RIGHT);

            minValL = std::min(minValL, L);
            maxValL = std::max(maxValL, L);
            minValR = std::min(minValR, R);
            maxValR = std::max(maxValR, R);
        }

        cache.minL[b] = minValL;
        cache.maxL[b] = maxValL;
        cache.minR[b] = minValR;
        cache.maxR[b] = maxValR;
    }
}

void Waveform::draw() {
    if (!valid()) {
        glLoadIdentity();
        glViewport(0, 0, w(), h());
        // X: pixels, Y: normalized amplitude.
        // Top to bottom pixel coordinates
        glOrtho(0, w(), 0, h(), -1.0, 1.0);
    }

    int halfHeight = h() / 2;

    // White background.
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    //if (track.getLength() == 0) return;

    // Ensure full-pixel lines.
    glLineWidth(1.0f);

    // Lambda function that draws a channel.
    auto drawChannel = [&](Direction channel, int yOffset, int heightPx) {
        float samplesPerPixel = 1.0f / zoomLevel;

        // Decide rendering mode based on zoom level.
        if (samplesPerPixel > 1.0f) {
            // ZOOMED OUT: Envelope (min/max per pixel column)
            //glBegin(GL_LINES);
            glBegin(GL_TRIANGLE_STRIP);

            for (int x = 0; x < w(); ++x) {
                size_t startSample = scrollOffset + static_cast<size_t>(x * samplesPerPixel);
                size_t endSample = scrollOffset + static_cast<size_t>((x + 1) * samplesPerPixel);

                if (endSample <= startSample) {
                    endSample = startSample + 1;
                }

                float minY = 1.0f;
                float maxY = -1.0f;

                // =========================
                // CACHE (only if NOT recording)
                // =========================
                float cacheMin = 1.0f;
                float cacheMax = -1.0f;

                // Default: real samples.
                float blend = 1.0f; 

                if (!track.isRecording()) {
                    // --- Compute cache-based min/max ---
                    size_t startBucket = startSample / cache.samplesPerBucket;
                    size_t endBucket = endSample / cache.samplesPerBucket;

                    for (size_t b = startBucket; b <= endBucket; ++b) {
                        if (b >= cache.minL.size()) break;

                        float bMin = (channel == Direction::LEFT) ? cache.minL[b] : cache.minR[b];
                        float bMax = (channel == Direction::LEFT) ? cache.maxL[b] : cache.maxR[b];

                        cacheMin = std::min(cacheMin, bMin);
                        cacheMax = std::max(cacheMax, bMax);
                    }

                    // --- Smooth blend. Compute blend factor (based on zoom) ---
                    float t = std::clamp((20.0f - samplesPerPixel) / 19.0f, 0.0f, 1.0f);
                    blend = t * t * (3.0f - 2.0f * t); // smoothstep

                    if (samplesPerPixel > 20.0f) {
                        blend = 0.0f; // cache only
                    }
                }

                // =========================
                // Use cache samples by default.
                // =========================
                float realMin = cacheMin;
                float realMax = cacheMax;

                if (track.isRecording() || blend > 0.0f) {
                    // Real samples
                    realMin = 1.0f;
                    realMax = -1.0f;

                    for (size_t i = startSample; i < endSample; ++i) {
                        float s = getDisplaySample(i, channel);
                        realMin = std::min(realMin, s);
                        realMax = std::max(realMax, s);
                    }
                }

                // =========================
                // FINAL BLEND
                // =========================
                if (!track.isRecording()) {
                    minY = (1.0f - blend) * cacheMin + blend * realMin;
                    maxY = (1.0f - blend) * cacheMax + blend * realMax;
                }
                else {
                    // Recording (ignore cache).
                    minY = realMin;
                    maxY = realMax;
                }

                // Avoid disappearing lines: pad very flat sections
                // Note: Near-flat, but not completely silent → pad it
                if (std::abs(maxY - minY) < 0.01f) {
                    minY -= 0.005f;
                    maxY += 0.005f;
                }

                float yMinPx = yOffset + (1.0f - std::clamp(minY, -1.0f, 1.0f)) * (heightPx / 2.0f);
                float yMaxPx = yOffset + (1.0f - std::clamp(maxY, -1.0f, 1.0f)) * (heightPx / 2.0f);

                glVertex2f(x, yMinPx);
                glVertex2f(x, yMaxPx);
            }

            glEnd();
        }
        else {
            // ZOOMED IN: One sample per vertex, smooth line.
            glBegin(GL_LINE_STRIP);

            // Note: Add +1 sample to visible range to ensure last visible pixel is drawn.
            int visibleSamples = static_cast<int>(std::ceil(w() / zoomLevel)) + 1;
            int endSample = std::min(scrollOffset + visibleSamples, (int)track.getLength());

            for (int i = scrollOffset; i < endSample; ++i) {
                float x = (i - scrollOffset) * zoomLevel;
                float sample = getDisplaySample(i, channel);
                float y = yOffset + (1.0f - std::clamp(sample, -1.0f, 1.0f)) * (heightPx / 2.0f);
                glVertex2f(x, y);
            }

            glEnd();

            // =========================
            // NODES (very zoomed)
            // =========================
            if (samplesPerPixel <= 0.1f) {
                // Red nodes.
                glColor3f(1.0f, 0.0f, 0.0f); 
                // Size of each node.
                glPointSize(4.0f);           
                glBegin(GL_POINTS);

                for (int i = scrollOffset; i < endSample; ++i) {
                    float x = (i - scrollOffset) * zoomLevel;
                    float sample = getDisplaySample(i, channel);
                    float y = yOffset + (1.0f - std::clamp(sample, -1.0f, 1.0f)) * (heightPx / 2.0f);
                    glVertex2f(x, y);
                }

                glEnd();
            }
        }
    };

    // --- Draw current selection (if any) ---
    if (selection() || (isSelecting && !track.isPlaying() && !track.isRecording())) {
        int x1 = (std::min(selectionStartSample, selectionEndSample) - scrollOffset) * zoomLevel;
        int x2 = (std::max(selectionStartSample, selectionEndSample) - scrollOffset) * zoomLevel;

        // Clamp to visible area
        x1 = std::clamp(x1, 0, w());
        x2 = std::clamp(x2, 0, w());

        glColor3f(0.0f, 0.0f, 0.0f); // black
        glBegin(GL_QUADS);
            glVertex2f(x1, 0.0f);
            glVertex2f(x2, 0.0f);
            glVertex2f(x2, (float)h());
            glVertex2f(x1, (float)h());
        glEnd();
    }

    // Waveform color (blue).
    glColor3f(0.0f, 0.0f, 1.0f);

    if (isStereo) {
        // Draw both left and right channels.
        drawChannel(Direction::LEFT, 0, halfHeight);
        drawChannel(Direction::RIGHT, halfHeight, halfHeight);

        // --- Draw separation line between waveforms ---

        // Dim gray
        glColor3f(0.412f, 0.412f, 0.412f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        // from left
        glVertex2f(0, h() / 2);
        // to right
        glVertex2f(w(), h() / 2);
        glEnd();

        // --- Draw zero lines (middle line) for both channels. ---

        // Gainsboro
        glColor3f(0.863f, 0.863f, 0.863f);
        glBegin(GL_LINES);
            glLineWidth(1.0f);
            glVertex2f(0.0f,    halfHeight + (halfHeight / 2));
            glVertex2f((float)w(), halfHeight + (halfHeight / 2));
            //
            glLineWidth(1.0f);
            glVertex2f(0.0f,    halfHeight / 2.0f);
            glVertex2f((float)w(), halfHeight / 2.0f);
        glEnd();
    }
    // mono = full height
    else {
        drawChannel(Direction::LEFT, 0, h());

        // --- Draw zero line (middle line). ---
        glColor3f(0.863f, 0.863f, 0.863f);
        glBegin(GL_LINES);
            glLineWidth(1.0f);
            glVertex2f(0.0f,    halfHeight);
            glVertex2f((float)w(), halfHeight);
        glEnd();
    }

    // If waveforms doesn't fill the full width, paint the rest in grey
    /*float lastX = getLastDrawnX();

    if (lastX < (float)w()) {
        glBegin(GL_QUADS);
            // grey background
            glColor3f(0.3f, 0.3f, 0.3f);
            // top-right
            glVertex2f((float)w(), (float)h());
            // top-left
            glVertex2f(lastX, (float)h());
            // bottom-left
            glVertex2f(lastX, 0.0f);
            // bottom-right
            glVertex2f((float)w(), 0.0f);
        glEnd();
    }*/

    // --- Draw playback cursor ---
    int sampleToDraw = -1;

    if (track.isRecording()) {
        // Add the possible distance before the beginning of the recording.
        sampleToDraw = startSamplePosition + track.getCaptureWriteIndex();
    }
    // The cursor moves in realtime (isPlaying) or is shown at its last position (isPaused) 
    // or has been manually moved (eg: mouse click, Home key...).
    else {
        sampleToDraw = cursorSamplePosition;
    }

    if (sampleToDraw >= 0) {
        int visibleStart = scrollOffset;
        int visibleEnd = scrollOffset + static_cast<int>(std::ceil(w() / zoomLevel));

        if (sampleToDraw >= visibleStart && sampleToDraw < visibleEnd) {
            float x = (sampleToDraw - scrollOffset) * zoomLevel;
            glColor3f(1.0f, 0.0f, 0.0f);
            glLineWidth(1.0f);
            glBegin(GL_LINES);
            glVertex2f(x, 0);
            glVertex2f(x, h());

            glEnd();
        }
    }

    // --- Draw markers (if any) ---
    for (size_t i = 0; i < marking.getMarkers().size(); i++) {
        sampleToDraw = marking.getMarkers()[i]->getSamplePosition();
        float x = (sampleToDraw - scrollOffset) * zoomLevel;

        if (!marking.getMarkers()[i]->isDragging()) {
            // Realign marker's label horizontally up in the marking area.
            marking.getMarkers()[i]->alignX((int) x);
        }

        glColor3f(0.0f, 1.0f, 0.0f);
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        glVertex2f(x, 0);
        glVertex2f(x, h());
        glEnd();

        // Refresh the marking area.
        marking.redraw();
    }
}

int Waveform::handle(int event) {
    switch (event) {
        case FL_FOCUS:
            //std::cout << "Waveform got focus" << std::endl; // For debog purpose
        return 1;

        case FL_UNFOCUS:
            //std::cout << "Waveform lost focus" << std::endl; // For debog purpose
        return 1;

        // Zoom with mouse wheel
        case FL_MOUSEWHEEL: {
            // Zoom in / zoom out.
            zoomLevel *= (Fl::event_dy() < 0) ? 1.1f : 0.9f;

            zoomLevel = std::clamp(zoomLevel, zoomMin, zoomMax);

            int visibleSamples = static_cast<int>(w() / zoomLevel);
            int maxOffset = std::max(0, (int)track.getLength() - visibleSamples);
            scrollOffset = std::clamp(scrollOffset, 0, maxOffset);

            updateScrollbar();
            redraw();

            return 1;
        }

        // The user has clicked and moved the cursor along the waveform.
        case FL_PUSH: {
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                // Take focus back if needed.
                if (Fl::focus() != this) {
                    Fl_Widget::take_focus();
                }

                // Keep current selection alive while it's modified. 
                if (selectionHandle != Direction::NONE) {
                    isSelecting = true;
                    return 1;
                }

                int mouseX = Fl::event_x();
                int sample = scrollOffset + static_cast<int>(mouseX / zoomLevel);

                // Clamp within sample range
                sample = std::clamp(sample, 0, (int)track.getLength() - 1);

                // Update positions.
                startSamplePosition = sample;
                cursorSamplePosition = sample;
                // Tell the audio system to seek too.
                track.setPlaybackIndex(sample);
                track.updateTime();

                // Start a new selection.
                if (!isSelecting && selectionHandle == Direction::NONE && !track.isPlaying() && !track.isRecording()) {
                    selectionStartSample = sample;
                    selectionEndSample = sample;
                    isSelecting = true;
                }

                redraw();

                // Event handled - Stop propagation.
                return 1;
            }

            // Right click or other buttons not handled. Let parent widgets see it too.
            return 0;
        }

        case FL_RELEASE: {
            if (Fl::event_button() == FL_LEFT_MOUSE) {
                if (isSelecting) {
                    // The user is not selecting audio region. Just moving the cursor. 
                    if (selectionEndSample == selectionStartSample) {
                        isSelecting = false;
                        return 1;
                    }

                    // Check for selection reversing.
                    if (selectionEndSample < selectionStartSample) {
                        // Swap values.
                        int tmp = selectionStartSample;
                        selectionStartSample = selectionEndSample;
                        selectionEndSample = tmp;
                    }

                    // Always placing the cursor at the start of the selection.
                    cursorSamplePosition = selectionStartSample;
                    startSamplePosition = selectionStartSample;
                    // Tell the audio system to seek too.
                    track.setPlaybackIndex(selectionStartSample);
                    track.updateTime();

                    // The user is done selecting.
                    isSelecting = false;
                    selectionHandle = Direction::NONE;
                    redraw();

                    return 1;
                }
            }

            return 0;
        }

        case FL_DRAG: {
            if (Fl::event_button() == FL_LEFT_MOUSE && isSelecting) {
                // Draw the selection range.
                int mouseX = Fl::event_x();
                int sample = scrollOffset + static_cast<int>(mouseX / zoomLevel);
                // Clamp within sample range
                sample = std::clamp(sample, 0, (int)track.getLength() - 1);

                // Check for selection.
                if (selectionHandle == Direction::LEFT) {
                    selectionStartSample = sample;
                }
                // RIGHT or NONE.
                else {
                    selectionEndSample = sample;
                }

                redraw();

                return 1;
            }

            return 0;
        }

        case FL_MOVE: {
            if (selection()) {
                int mouseX = Fl::event_x();
                int sample = scrollOffset + static_cast<int>(mouseX / zoomLevel);

                // Check if mouse is near selection boundaries (with some tolerance).

                // Pixels tolerance.
                int tolerance = 3; 
                bool nearStart = abs(sample - selectionStartSample) * zoomLevel < tolerance;
                bool nearEnd = abs(sample - selectionEndSample) * zoomLevel < tolerance;

                // The mouse is over the left selection boundaries.
                if (nearStart) {
                    window()->cursor(FL_CURSOR_WE);
                    selectionHandle = Direction::LEFT;
                    return 1;
                }
                // The mouse is over the right selection boundaries.
                else if (nearEnd) {
                    window()->cursor(FL_CURSOR_WE);
                    selectionHandle = Direction::RIGHT;
                    return 1;
                }
                // The mouse is elsewhere in the window.
                else {
                    window()->cursor(FL_CURSOR_DEFAULT);
                    selectionHandle = Direction::NONE;
                    return 0;
                }
            }

            return 0;
        }

        case FL_KEYDOWN: {
            int key = Fl::event_key();

            // Spacebar: ' ' => ASCII code 32.
            if (key == ' ') {
                // Toggle start/stop.
                if (track.isPlaying()) {
                    track.getApplication().onStop(track);
                }
                else {
                    track.getApplication().onPlay(track);
                }
                   
                return 1;
            }
            else if (key == FL_Pause) {
                track.getApplication().onPause(track);

                return 1;
            }
            else if (key == FL_Home) {
                // Process only when playback is stopped.
                if (!track.isPlaying()) {
                    // Set positions to the start.
                    cursorSamplePosition = 0;
                    startSamplePosition = 0;
                    resetCursor();

                    return 1;
                }

                return 0;
            }
            else if (key == FL_End) {
                // Process only when playback is stopped.
                if (!track.isPlaying()) {
                    // Set positions to the end.
                    cursorSamplePosition = static_cast<int>(track.getLength()) - 1;
                    startSamplePosition = static_cast<int>(track.getLength()) - 1;
                    resetCursor();

                    return 1;
                }

                return 0;
            }

            return 0;
        } 

        default:
            // For events we don't handle, pass to parent.
            return Fl_Gl_Window::handle(event);
    }
}

void Waveform::resetCursor()
{
    // Get the cursor's start position.
    int resetTo = startSamplePosition;
    // Reset the cursor to its initial audio and graphic position.
    track.setPlaybackIndex(resetTo);
    cursorSamplePosition = resetTo;
    track.updateTime();

    // Compute a target offset before the cursor, (e.g: show 10% of the window before the cursor.)
    float zoom = getZoomLevel();
    // Number of samples that fit in the view
    int visibleSamples = static_cast<int>(w() / zoom);
    // Shift back by a percentage of visible samples (e.g., 10%)
    int marginSamples = static_cast<int>(visibleSamples * 0.1f);
    // Compute the new scroll offset
    int newScrollOffset = std::max(0, resetTo - marginSamples);
    // Apply it.
    setScrollOffset(newScrollOffset);
    // Force the waveform (and cursor) to repaint
    redraw();
}

void Waveform::updateCursor(Track& track) 
{
    // Reads from atomic.
    int sample = track.isRecording() ? startSamplePosition + track.getCaptureWriteIndex() : track.getCurrentSample();

    // Synchronize view with audio. 
    setCursorSamplePosition(sample);

    // --- Smart auto-scroll ---
    // Auto-scroll the view if cursor gets near right edge

    // pixels from right edge
    int margin = 30;
    float zoom = getZoomLevel();
    int viewWidth = w();
    int cursorX = static_cast<int>((sample - getScrollOffset()) * zoom);

    if (cursorX > viewWidth - margin) {
        int newOffset = sample - static_cast<int>((viewWidth - margin) / zoom);
        setScrollOffset(newOffset);
    }

    redraw();
}

// helper to compute how many samples fit inside the widget width at current zoom
int Waveform::visibleSamplesCount() const {
    if (zoomLevel <= 0.0f) return (int)track.getLength();
    // number of samples that correspond to the width: ceil(w / zoomLevel)
    int vs = static_cast<int>(std::ceil(static_cast<float>(w()) / zoomLevel));
    vs = std::max(1, vs);
    vs = std::min((int)track.getLength(), vs);

    return vs;
}

void Waveform::liveUpdate_cb(void* userdata)
{
    Waveform* self = static_cast<Waveform*>(userdata);
    self->redraw();

    if (self->isLiveUpdating) {
        Fl::repeat_timeout(0.03, liveUpdate_cb, userdata); // 30 ms refresh
    }
}

void Waveform::startLiveUpdate()
{
    if (isLiveUpdating) {
        return;
    }

    prepareForRecording();
    isLiveUpdating = true;
    Fl::add_timeout(0.30, liveUpdate_cb, this);
}

void Waveform::stopLiveUpdate()
{
    isLiveUpdating = false;
    Fl::remove_timeout(liveUpdate_cb, this);

    // Empty temporary buffers.
    buildWaveformCache(track);
}

