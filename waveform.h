#include <GL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Scrollbar.H>
#include <vector>
#include <functional>
#include <cmath>
#include <iostream>

// Forward declaration.
class Application;
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
        //bool playing = false;
        //bool paused = false;
        bool isStereo = true;
        // Starting position of the cursor. It can be manually moved.
        int cursorSamplePosition = 0;
        Application* app = nullptr;
        AudioTrack& track;
        int visibleSamplesCount() const;

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

        // Getters.

        int getScrollOffset() const { return scrollOffset; }
        float getZoomLevel() const { return zoomLevel; }
        int getPlaybackSample() const { return playbackSample; }
        AudioTrack& getTrack() { return track; }
        //int getCursorSamplePosition() const { return cursorSamplePosition; }

        // Setters.

        void setStereoSamples(const std::vector<float>& left, const std::vector<float>& right);
        void setScrollOffset(int offset);
        void setScrollbar(Fl_Scrollbar* sb);
        void setContext(Application* app) { this->app = app; }
        void setPlaybackSample(int sample) {
            playbackSample = sample;
            redraw();
        }
        void setStereoMode(bool stereo) { isStereo = stereo; }
};

