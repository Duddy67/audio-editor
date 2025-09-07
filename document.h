#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <FL/Fl_Scrollbar.H>
#include "audio_track.h"

// Forward declarations.
class AudioEngine;

class Document : public Fl_Group {
        int x, y, w, h;
        AudioEngine& engine;
        // Unique track id.
        unsigned int trackId = 0;

    public:

        Document(int X, int Y, int W, int H, AudioEngine& e, const char *filepath)
            : Fl_Group(X, Y, W, H), engine(e) 
        {
            x = X + 10;
            y = Y + 10;
            w = W - 20;
            h = H - 100;

            auto track = std::make_unique<AudioTrack>(engine);
            track->loadFromFile(filepath);

            // Add the new track and transfer ownership.
            trackId = engine.addTrack(std::move(track));

            renderTrackWaveform();
        }

        void renderTrackWaveform() {
            auto& track = getTrack();
            track.renderWaveform(x, y, w, h);

            Fl_Scrollbar* scrollbar = new Fl_Scrollbar(10, h + 20, w, 15);
            scrollbar->type(FL_HORIZONTAL);
            scrollbar->step(1);
            scrollbar->minimum(0);

            scrollbar->callback([](Fl_Widget* w, void* data) {
                auto* sb = (Fl_Scrollbar*)w;
                auto* wf = (WaveformView*)data;
                wf->setScrollOffset(sb->value());
            }, &track.getWaveform());

            track.getWaveform().setScrollbar(scrollbar);
        }

        AudioTrack& getTrack() { return engine.getTrack(trackId); }
        unsigned int getTrackId() { return trackId; }
        void removeTrack() { engine.removeTrack(trackId); }
};

#endif // DOCUMENT_H
