#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <FL/Fl_Scrollbar.H>
#include "audio_track.h"
#define SB_H 15
#define SB_MARGIN 10

// Forward declarations.
class AudioEngine;

class Document : public Fl_Group {
        int xPos, yPos, width, height;
        AudioEngine& engine;
        // Unique track id.
        unsigned int trackId = 0;
        bool changed = false;

        void renderTrackWaveform() {
            //auto& track = getTrack();
            AudioTrack& track = engine.getTrack(trackId);

            // Compute waveform area leaving space for scrollbar.
            int wf_x = xPos;
            int wf_y = yPos;
            int wf_w = width;
            int wf_h = height - (SB_H + SB_MARGIN);

            // Parent Document.
            begin();
            // Create the WaveformView as a child of Document.
            track.renderWaveform(wf_x, wf_y, wf_w, wf_h);

            Fl_Scrollbar* scrollbar = new Fl_Scrollbar(wf_x, wf_y + wf_h + SB_MARGIN, width, SB_H);
            scrollbar->type(FL_HORIZONTAL);
            scrollbar->step(1);
            scrollbar->minimum(0);

            scrollbar->callback([](Fl_Widget* w, void* data) {
                auto* sb = (Fl_Scrollbar*)w;
                auto* wf = (WaveformView*)data;
                wf->setScrollOffset(sb->value());
            }, &track.getWaveform());

            track.getWaveform().setScrollbar(scrollbar);

            // Create a dummy box that represents the waveform’s resize area
            Fl_Box* resize_box = new Fl_Box(wf_x, wf_y, wf_w, SB_H);
            this->resizable(resize_box);

            // Done adding children.
            end();

            track.getWaveform().show();
            track.getWaveform().redraw();
        }

    public:

        Document(int X, int Y, int W, int H, AudioEngine& e, const char *filepath = nullptr)
            : Fl_Group(X, Y, W, H), engine(e) 
        {
            // Compute tab area.
            xPos = X + 10;
            yPos = Y + 10;
            width = W - 20;
            height = H - 100;

            auto track = std::make_unique<AudioTrack>(engine);

            // Load the given audio file.
            if (filepath != nullptr) {
                track->loadFromFile(filepath);
            }
            else {
                track->setNewTrack();
            }

            // Add the new track and transfer ownership.
            trackId = engine.addTrack(std::move(track));

            renderTrackWaveform();
        }

        AudioTrack& getTrack() { return engine.getTrack(trackId); }
        unsigned int getTrackId() const { return trackId; }
        void removeTrack() { engine.removeTrack(trackId); }
        bool isChanged() const { return changed; }

        void hasChanged() {
            changed = true;
            std::cout << "Document: " << label() << std::endl;
        }

        void saved() {
            changed = false;
        }
};

#endif // DOCUMENT_H
