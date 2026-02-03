#include "../audio/track.h"
#include "../main.h"


void Waveform::setStereoSamples(const std::vector<float>& left, const std::vector<float>& right) {
    leftSamples = left;
    rightSamples = right;

    isStereo = track.isStereo();

    // Fit entire waveform on screen initially.
    if (!leftSamples.empty()) {
        // Compute fit-to-screen zoom (pixels per sample that fits entire file).
        zoomFit = static_cast<float>(w()) / static_cast<float>(leftSamples.size());
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
    if (!scrollbar || leftSamples.empty()) return;
    int visibleSamples = static_cast<int>(w() / zoomLevel);
    int maxOffset = std::max(0, (int)leftSamples.size() - visibleSamples);
    scrollOffset = std::clamp(scrollOffset, 0, maxOffset);
    scrollbar->maximum(maxOffset);
    scrollbar->value(scrollOffset);
    scrollbar->slider_size((float)visibleSamples / leftSamples.size());
}

void Waveform::prepareForRecording()
{
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

void Waveform::pullNewRecordedSamples()
{
    std::vector<float> newLeft, newRight;
    size_t startIndex, count;

    if (track.getNewSamplesCopy(newLeft, newRight, startIndex, count)) {
        if (count == 0) return;

        size_t requiredSize = startIndex + count;

        // Ensure we have enough capacity
        if (leftSamples.size() < requiredSize) {
            leftSamples.resize(requiredSize, 0.0f);
            rightSamples.resize(requiredSize, 0.0f);
        }

        // Append new samples
        for (size_t i = 0; i < count; i++) {
            size_t globalIndex = startIndex + i;

            if (globalIndex < leftSamples.size()) {
                // Shouldn't happen with proper indexing, but safe
                leftSamples[globalIndex] = newLeft[i];
                rightSamples[globalIndex] = newRight[i];
            } else {
                // Normal case - append
                leftSamples.push_back(newLeft[i]);
                rightSamples.push_back(newRight[i]);
            }
        }

        // --- Alternative (without loop nor condition) ---
        // Copy new samples (overwrite or append)
        //std::copy_n(newLeft.begin(), count, leftSamples.begin() + startIndex);
        //std::copy_n(newRight.begin(), count, rightSamples.begin() + startIndex);

        lastSyncedSample = startIndex + count;

        // ===== Rolling window style  ====
        int head = static_cast<int>(leftSamples.size());
        int visible = visibleSamplesCount();
        int rightEdge = scrollOffset + visible;

        // Scroll only when the record head nears the right edge
        if (head > rightEdge - visible / 10) {
            scrollOffset = head - (int)(visible * 0.9f);

            if (scrollOffset < 0) { 
                scrollOffset = 0;
            }
        }
        // =====================

        // Update zoom/scroll boundaries if needed
        updateScrollbar();
    }
}

/*
 * Check whether a selection is currently set.
 */
bool Waveform::selection() {
    if (selectionStartSample >= 0 && selectionEndSample >= 0 && selectionStartSample != selectionEndSample) {
        return true;
    }

    return false;
}

/*
 * Computes the last drawn x position.
 */
float Waveform::getLastDrawnX() 
{
    int totalSamples = leftSamples.size();
    int visibleSamples = visibleSamplesCount();
    int endSample = scrollOffset + visibleSamples;

    // Compute and return last drawn x position.
    return (float)(std::min(endSample, totalSamples) - scrollOffset) * zoomLevel;
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

    if (leftSamples.empty()) return;

    // Blue waveform.
    glColor3f(0.0f, 0.0f, 1.0f);
    // Ensure full-pixel lines.
    glLineWidth(1.0f);

    // Lambda function that draws a channel.
    auto drawChannel = [&](const std::vector<float>& channel, int yOffset, int heightPx) {
        float samplesPerPixel = 1.0f / zoomLevel;

        // Decide rendering mode based on zoom level.
        if (samplesPerPixel > 5.0f) {
            // ZOOMED OUT: Envelope (min/max per pixel column)
            glBegin(GL_LINES);

            for (int x = 0; x < w(); ++x) {
                int startSample = scrollOffset + static_cast<int>(x * samplesPerPixel);
                int endSample = std::min(scrollOffset + static_cast<int>((x + 1) * samplesPerPixel), (int)channel.size());

                float minY = 1.0f, maxY = -1.0f;
                for (int i = startSample; i < endSample; ++i) {
                    float s = channel[i];
                    minY = std::min(minY, s);
                    maxY = std::max(maxY, s);
                }

                bool isSilent = true;

                for (int i = startSample; i < endSample; ++i) {
                    // Noise threshold
                    if (std::abs(channel[i]) > 0.005f) {
                        isSilent = false;
                        break;
                    }
                }

                if (isSilent) {
                    // Flat silent section → draw a thin horizontal line
                    float yFlatPx = yOffset + (1.0f - 0.0f) * (heightPx / 2.0f);  // Amplitude 0

                    glVertex2f(x, yFlatPx);
                    // 1-pixel wide horizontal line.
                    glVertex2f(x + 1, yFlatPx);
                    // Skip the rest of loop.
                    continue;
                }

                // Avoid disappearing lines: pad very flat sections
                // Note: Near-flat, but not completely silent → pad it
                if (std::abs(maxY - minY) < 0.01f) {
                    minY -= 0.005f; maxY += 0.005f;
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
            int endSample = std::min(scrollOffset + visibleSamples, (int)channel.size());

            for (int i = scrollOffset; i < endSample; ++i) {
                float x = (i - scrollOffset) * zoomLevel;
                float y = yOffset + (1.0f - std::clamp(channel[i], -1.0f, 1.0f)) * (heightPx / 2.0f);
                glVertex2f(x, y);
            }

            glEnd();

            // --- Draw nodes if zoomed in enough ---
            float samplesPerPixel = 1.0f / zoomLevel;

            if (samplesPerPixel <= 0.1f) {
                glColor3f(1.0f, 0.0f, 0.0f); // red nodes
                glPointSize(4.0f);           // size of each node
                glBegin(GL_POINTS);

                for (int i = scrollOffset; i < endSample; ++i) {
                    float x = (i - scrollOffset) * zoomLevel;
                    float y = yOffset + (1.0f - std::clamp(channel[i], -1.0f, 1.0f)) * (heightPx / 2.0f);
                    glVertex2f(x, y);
                }

                glEnd();
            }
        }
    };

    // If waveform doesn't fill the full width, paint the rest in grey
    float lastX = getLastDrawnX();

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
    }

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
        drawChannel(leftSamples, 0, halfHeight);
        drawChannel(rightSamples, halfHeight, halfHeight);

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
        drawChannel(leftSamples, 0, h());
        // --- Draw zero line (middle line). ---
        glColor3f(0.863f, 0.863f, 0.863f);
        glBegin(GL_LINES);
            glLineWidth(1.0f);
            glVertex2f(0.0f,    halfHeight);
            glVertex2f((float)w(), halfHeight);
        glEnd();
    }


    // --- Draw playback cursor ---
    int sampleToDraw = -1;

    if (track.isRecording()) {
        sampleToDraw = track.getCaptureWriteIndex();
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
            int maxOffset = std::max(0, (int)leftSamples.size() - visibleSamples);
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

                int mouseX = Fl::event_x();
                int sample = scrollOffset + static_cast<int>(mouseX / zoomLevel);

                // Clamp within sample range
                sample = std::clamp(sample, 0, (int)leftSamples.size() - 1);

                initialSamplePosition = sample;
                cursorSamplePosition = sample;
                // Tell the audio system to seek too.
                track.setPlaybackSampleIndex(sample);
                track.updateTime();

                // Start a new selection.
                if (!isSelecting && selectionHandle == NONE && !track.isPlaying() && !track.isRecording()) {
                    selectionStartSample = sample;
                    selectionEndSample = sample;
                    isSelecting = true;
                }

                // Keep current selection alive while it's modified. 
                if (selectionHandle != NONE) {
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
                    // Check for selection reversing.
                    if (selectionEndSample < selectionStartSample) {
                        // Swap values.
                        int tmp = selectionStartSample;
                        selectionStartSample = selectionEndSample;
                        selectionEndSample = tmp;
                    }

                    // Always placing the cursor at the start of the selection.
                    cursorSamplePosition = selectionStartSample;
                    // Tell the audio system to seek too.
                    track.setPlaybackSampleIndex(selectionStartSample);

                    // The user is done selecting.
                    isSelecting = false;
                    selectionHandle = NONE;

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
                sample = std::clamp(sample, 0, (int)leftSamples.size() - 1);

                // Check for selection.
                if (selectionHandle == LEFT) {
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
                    selectionHandle = LEFT;
                    return 1;
                }
                // The mouse is over the right selection boundaries.
                else if (nearEnd) {
                    window()->cursor(FL_CURSOR_WE);
                    selectionHandle = RIGHT;
                    return 1;
                }
                // The mouse is elsewhere in the window.
                else {
                    window()->cursor(FL_CURSOR_DEFAULT);
                    selectionHandle = NONE;
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
                    track.getApplication().stopTrack(track);
                }
                else {
                    track.getApplication().playTrack(track);
                }
                   
                return 1;
            }
            else if (key == FL_Pause) {
                track.getApplication().pauseTrack(track);

                return 1;
            }
            else if (key == FL_Home) {
                // Process only when playback is stopped.
                if (!track.isPlaying()) {
                    // Set positions to the start.
                    cursorSamplePosition = 0;
                    initialSamplePosition = 0;
                    resetCursor();

                    return 1;
                }

                return 0;
            }
            else if (key == FL_End) {
                // Process only when playback is stopped.
                if (!track.isPlaying()) {
                    // Set positions to the end.
                    cursorSamplePosition = static_cast<int>(leftSamples.size()) - 1;
                    initialSamplePosition = static_cast<int>(leftSamples.size()) - 1;
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
    // Get the cursor's initial position.
    int resetTo = initialSamplePosition;
    // Reset the cursor to its initial audio position.
    track.setPlaybackSampleIndex(resetTo);

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

// ---- Timer Callback ----
void Waveform::update_cursor_timer_cb(void* userdata) {
    auto& track = *(Track*)userdata;  // Dereference to get reference
    auto& waveform = track.getWaveform();
    // Reads from atomic.
    int sample = track.getCurrentSample();
    // Synchronize view with audio. 
    waveform.setCursorSamplePosition(sample);

    // --- Smart auto-scroll ---
    // Auto-scroll the view if cursor gets near right edge

    // pixels from right edge
    int margin = 30;
    float zoom = waveform.getZoomLevel();
    int viewWidth = waveform.w();
    int cursorX = static_cast<int>((sample - waveform.getScrollOffset()) * zoom);

    if (cursorX > viewWidth - margin) {
        int newOffset = sample - static_cast<int>((viewWidth - margin) / zoom);
        waveform.setScrollOffset(newOffset);
    }

    // Assuming left and right channels are the same length.
    int totalSamples = static_cast<int>(track.getLeftSamples().size());

    waveform.redraw();

    if (sample < totalSamples && track.isPlaying()) {
        // ~60 FPS
        Fl::repeat_timeout(0.016, update_cursor_timer_cb, &track);
    }
}

// helper to compute how many samples fit inside the widget width at current zoom
int Waveform::visibleSamplesCount() const {
    if (zoomLevel <= 0.0f) return (int)leftSamples.size();
    // number of samples that correspond to the width: ceil(w / zoomLevel)
    int vs = static_cast<int>(std::ceil(static_cast<float>(w()) / zoomLevel));
    vs = std::max(1, vs);
    vs = std::min((int)leftSamples.size(), vs);

    return vs;
}

void Waveform::liveUpdate_cb(void* userdata)
{
    Waveform* self = static_cast<Waveform*>(userdata);
    self->pullNewRecordedSamples();
    self->redraw();

    if (self->isLiveUpdating) {
        Fl::repeat_timeout(0.30, liveUpdate_cb, userdata); // 30 ms refresh
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
}

