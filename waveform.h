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
        // Auto-calculated minimum zoom (fit to screen).
        float zoomMin = 0.01f;
        // Pixels per sample.
        float zoomLevel = 1.0f;
        int scrollOffset = 0;
        // -1 = not playing
        int playbackSample = -1;
        bool playing = false;
        bool paused = false;
        bool isStereo = true;
        // Position of the cursor when it is manually moved.
        int movedCursorSample = 0;
        Application* app = nullptr;
        AudioTrack& track;

    protected:
        void draw() override;
        int handle(int event) override;

    public:
        WaveformView(int X, int Y, int W, int H, AudioTrack& t)
            : Fl_Gl_Window(X, Y, W, H), track(t) {
            end();
        }

        std::function<void(int)> onSeekCallback;

        void setOnSeekCallback(std::function<void(int)> callback);
        void setStereoSamples(const std::vector<float>& left, const std::vector<float>& right);
        void setScrollOffset(int offset);
        void setScrollbar(Fl_Scrollbar* sb);
        void updateScrollbar();

        // Getters.

        int getScrollOffset() const { return scrollOffset; }
        float getZoomLevel() const { return zoomLevel; }
        bool isPlaying() const { return playing; }
        bool isPaused() const { return paused; }
        int getPlaybackSample() const { return playbackSample; }

        // Setters.

        void setContext(Application* app) { this->app = app; }
        int getMovedCursorSample() const { return movedCursorSample; }
        void setPlaying(bool state) { playing = state; }
        void setPaused(bool state) { paused = state; }
        void setPlaybackSample(int sample) {
            playbackSample = sample;
            redraw();
        }
        void setStereoMode(bool stereo) { isStereo = stereo; }
};

