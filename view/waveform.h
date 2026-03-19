#ifndef WAVEFORM_H
#define WAVEFORM_H

#include <GL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Scrollbar.H>
#include <vector>
#include <functional>
#include <cmath>
#include <iostream>
#include "../constants.h"
#include "../marking/marking.h"

// Forward declarations.
class Track;
class Marking;

class Waveform : public Fl_Gl_Window {
        // Temporary buffers used during recording.
        std::vector<float> recordedLeftSamples;
        std::vector<float> recordedRightSamples;
        Fl_Scrollbar* scrollbar = nullptr;
        // Fit-to-screen (current starting zoom).
        float zoomFit = 1.0f;
        // Allow zooming out further.
        float zoomMin = 1.0f;
        // Allow up to 10 pixels per sample.
        float zoomMax = 10.0f;
        // Pixels per sample.
        float zoomLevel = 1.0f;
        int scrollOffset = 0;
        bool isStereo = true;
        // Current position of the cursor. It can be manually moved.
        int cursorSamplePosition = 0;
        // Start position of the cursor.
        int startSamplePosition = 0;
        int lastSyncedSample = 0;
        int recordingStartSample = 0;
        Track& track;
        Marking& marking;
        int visibleSamplesCount() const;
        bool isLiveUpdating = false;
        bool isSelecting = false;
        Direction selectionHandle = Direction::NONE;
        int selectionStartSample = -1;
        int selectionEndSample = -1;

        static void liveUpdate_cb(void* userdata);
        void prepareForRecording();
        void pullNewRecordedSamples();
        float getRecordedSample(unsigned int timelineIndex, Direction channel);

    protected:
        void draw() override;
        int handle(int event) override;

    public:
        Waveform(int X, int Y, int W, int H, Track& t, Marking& m)
            : Fl_Gl_Window(X, Y, W, H), track(t), marking(m) {
            end();

            marking.init(this);
        }

        std::function<void(int)> onSeekCallback;
        void updateCursor(Track& track);
        void initView();
        void updateScrollbar();
        void resetCursor();
        void startLiveUpdate();
        void stopLiveUpdate();
        bool selection();
        void deleteSelection();

        // Getters.

        int getScrollOffset() const { return scrollOffset; }
        float getZoomLevel() const { return zoomLevel; }
        Track& getTrack() { return track; }
        int getSelectionStartSample() const { return selectionStartSample; }
        int getSelectionEndSample() const { return selectionEndSample; }
        int getCursorSamplePosition() const { return cursorSamplePosition; }
        int getStartSamplePosition() const { return startSamplePosition; }
        float getLastDrawnX();

        // Setters.

        void setScrollOffset(int offset);
        void setScrollbar(Fl_Scrollbar* sb);
        void setCursorSamplePosition(int sample) { cursorSamplePosition = sample; }
        void setSelectionStartSample(int start) { selectionStartSample = start; }
        void setSelectionEndSample(int end) { selectionEndSample = end; }
};

#endif // WAVEFORM_H
