#include "audio_track.h"


void WaveformView::setStereoSamples(const std::vector<float>& left, const std::vector<float>& right) {
    leftSamples = left;
    rightSamples = right;

    // Crude check, or pass in a flag.
    isStereo = (left != right);  

    // Fit entire waveform on screen initially.
    if (!leftSamples.empty()) {
        zoomMin = static_cast<float>(w()) / leftSamples.size();
        zoomLevel = zoomMin;
    }

    scrollOffset = 0;
    updateScrollbar();
    redraw();
}

void WaveformView::setScrollOffset(int offset) {
    scrollOffset = std::max(0, offset);
    updateScrollbar();
    redraw();
}

void WaveformView::setScrollbar(Fl_Scrollbar* sb) {
    scrollbar = sb;
    updateScrollbar();
}

void WaveformView::updateScrollbar() {
    if (!scrollbar || leftSamples.empty()) return;
    int visibleSamples = static_cast<int>(w() / zoomLevel);
    int maxOffset = std::max(0, (int)leftSamples.size() - visibleSamples);
    scrollOffset = std::clamp(scrollOffset, 0, maxOffset);
    scrollbar->maximum(maxOffset);
    scrollbar->value(scrollOffset);
    scrollbar->slider_size((float)visibleSamples / leftSamples.size());
}

void WaveformView::draw() {
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

    if (isStereo) {
        // Draw both left and right channels.
        glColor3f(0.0f, 0.0f, 1.0f); // Left: blue
        drawChannel(leftSamples, 0, halfHeight);

        glColor3f(0.0f, 0.5f, 0.0f); // Right: green
        drawChannel(rightSamples, halfHeight, halfHeight);

        // --- Draw separation line between waveforms ---

        // light gray
        glColor3f(0.8f, 0.8f, 0.8f); 
        glLineWidth(1.0f);
        glBegin(GL_LINES);
        // from left
        glVertex2f(0, h() / 2);     
        // to right
        glVertex2f(w(), h() / 2);   
        glEnd();
    }
    // mono = full height
    else {
        drawChannel(leftSamples, 0, h());  
    }


    // --- Draw playback cursor ---
    int sampleToDraw = -1;

    if (track.isPlaying() || track.isPaused()) {
        // The cursor moves in realtime (isPlaying) or is shown at its last position (isPaused).
        sampleToDraw = playbackSample;
    }
    else {
        // The cursor has been manually moved (eg: mouse click, Home key...).
        sampleToDraw = movedCursorSample;
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
}

int WaveformView::handle(int event) {
    switch (event) {
        case FL_FOCUS:
            std::cout << "Waveform got focus" << std::endl;
        return 1;

        case FL_UNFOCUS:
            std::cout << "Waveform lost focus" << std::endl;
        return 1;

        // Zoom with mouse wheel
        case FL_MOUSEWHEEL: {
            zoomLevel *= (Fl::event_dy() < 0) ? 1.1f : 0.9f;

            zoomLevel = std::clamp(zoomLevel, zoomMin, 100.0f);

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
                int mouseX = Fl::event_x();
                int sample = scrollOffset + static_cast<int>(mouseX / zoomLevel);

                // Clamp within sample range
                sample = std::clamp(sample, 0, (int)leftSamples.size() - 1);

                setPlaybackSample(sample);
                movedCursorSample = sample;
                // Tell the audio system to seek too.
                track.setPlaybackSampleIndex(sample);

                return 1;
            }

            return 0;
        }

        case FL_KEYDOWN: {
            int key = Fl::event_key();

            // Spacebar: ' ' => ASCII code 32.
            if (key == ' ') {
                // Toggle start/stop.
                if (track.isPlaying()) {
                    track.stop();
                    track.setPlaybackSampleIndex(0);
                    track.unpause();
                    resetCursor();
                }
                else {
                    if (track.isPaused()) {
                        resetCursor();
                    }
                    else {
                        setPlaybackSample(0);
                    }

                    track.play();
                    Fl::add_timeout(0.016, update_cursor_timer_cb, &track);
                }
                   
                return 1;
            }
            else if (key == FL_Pause) {
                if (track.isPlaying()) {
                    track.stop();
                    track.pause();
                    return 1;
                }
                // Resume from where playback paused
                else if (track.isPaused() && !track.isPlaying()) {
                    int resumeSample = getPlaybackSample();
                    track.setPlaybackSampleIndex(resumeSample);
                    track.unpause();
                    track.play();
                    Fl::add_timeout(0.016, update_cursor_timer_cb, &track);
                }

                return 0;
            }
            else if (key == FL_Home) {
                // Process only when playback is stopped.
                if (!track.isPlaying()) {
                    // Reset the audio cursor to the start position.
                    movedCursorSample = 0;
                    resetCursor();

                    return 1;
                }

                return 0;
            }
            else if (key == FL_End) {
                // Process only when playback is stopped.
                if (!track.isPlaying()) {
                    // Take the audio cursor to the end position.
                    movedCursorSample = static_cast<int>(leftSamples.size()) - 1;
                    resetCursor();

                    return 1;
                }

                return 0;
            }

            return 0;
        } 

        default:
            return Fl_Gl_Window::handle(event);
    }
}

void WaveformView::resetCursor()
{
    // Get the cursor's starting point.
    int resetTo = getMovedCursorSample();
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
void WaveformView::update_cursor_timer_cb(void* userdata) {
    auto& track = *(AudioTrack*)userdata;  // Dereference to get reference
    auto& waveform = track.getWaveform();
    // Reads from atomic.
    int sample = track.currentSample();
    waveform.setPlaybackSample(sample);

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

    if (sample < totalSamples && track.isPlaying()) {
        // ~60 FPS
        Fl::repeat_timeout(0.016, update_cursor_timer_cb, &track);
    }
}

