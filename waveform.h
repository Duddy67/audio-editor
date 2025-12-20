#include <GL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Scrollbar.H>
#include <vector>
#include <functional>
#include <cmath>
#include <iostream>
#include "constants.h"

// Forward declaration.
class AudioTrack;

class WaveformView : public Fl_Gl_Window {
        std::vector<float> leftSamples;
        std::vector<float> rightSamples;
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
        // -1 = not playing
        int playbackSample = -1;
        bool isStereo = true;
        // Starting position of the cursor. It can be manually moved.
        int cursorSamplePosition = 0;
        int lastSyncedSample = 0;
        int recordingStartSample = 0;
        AudioTrack& track;
        int visibleSamplesCount() const;
        bool isLiveUpdating = false;
        bool isSelecting = false;
        Direction selectionHandle = NONE;
        int selectionStartSample = -1;
        int selectionEndSample = -1;

        static void liveUpdate_cb(void* userdata);
        void prepareForRecording();
        void pullNewRecordedSamples();

    protected:
        void draw() override;
        int handle(int event) override;

    public:
        WaveformView(int X, int Y, int W, int H, AudioTrack& t)
            : Fl_Gl_Window(X, Y, W, H), track(t) {
            end();
        }

        std::function<void(int)> onSeekCallback;
        static void update_cursor_timer_cb(void* userdata);

        void updateScrollbar();
        void resetCursor();
        void startLiveUpdate();
        void stopLiveUpdate();
        bool selection();

        // Getters.

        int getScrollOffset() const { return scrollOffset; }
        float getZoomLevel() const { return zoomLevel; }
        int getPlaybackSample() const { return playbackSample; }
        AudioTrack& getTrack() { return track; }
        int getSelectionStartSample() const { return selectionStartSample; }
        int getSelectionEndSample() const { return selectionEndSample; }
        int getCursorSamplePosition() const { return cursorSamplePosition; }

        // Setters.

        void setStereoSamples(const std::vector<float>& left, const std::vector<float>& right);
        void setScrollOffset(int offset);
        void setScrollbar(Fl_Scrollbar* sb);
        void setPlaybackSample(int sample) {
            playbackSample = sample;
            redraw();
        }
        void setStereoMode(bool stereo) { isStereo = stereo; }
};

