#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <filesystem>
#include <FL/Fl_Scrollbar.H>
#include "audio_track.h"

// Forward declarations.
class AudioEngine;

class Document : public Fl_Group {
        int xPos, yPos, width, height;
        AudioEngine& engine;
        // Unique track id.
        unsigned int trackId = 0;
        // Track state.
        bool changed = false;
        bool created = false;
        // File name and extension associated to the track.
        std::string fileName;
        std::string extension;

        void renderTrackWaveform() {
            AudioTrack& track = engine.getTrack(trackId);

            // Compute waveform area leaving space for scrollbar.
            int wf_x = xPos;
            int wf_y = yPos;
            int wf_w = width;
            int wf_h = height - (SCROLLBAR_HEIGHT + SCROLLBAR_MARGIN);

            // Parent Document.
            begin();
            // Create the track's visual widgets (waveform, marking...) as a children of Document.
            track.render(wf_x, wf_y, wf_w, wf_h);

            Fl_Scrollbar* scrollbar = new Fl_Scrollbar(wf_x, wf_y + wf_h + SCROLLBAR_MARGIN, width, SCROLLBAR_HEIGHT);
            scrollbar->type(FL_HORIZONTAL);
            scrollbar->step(1);
            scrollbar->minimum(0);

            scrollbar->callback([](Fl_Widget* w, void* data) {
                auto* sb = (Fl_Scrollbar*)w;
                auto* wf = (WaveformView*)data;
                wf->setScrollOffset(sb->value());
            }, &track.getWaveform());

            track.getWaveform().setScrollbar(scrollbar);

            // Important:  Create a dummy box that represents the waveform’s resize area
            Fl_Box* resize_box = new Fl_Box(wf_x, wf_y + MARKING_AREA_HEIGHT, wf_w, SCROLLBAR_HEIGHT + MARKING_AREA_HEIGHT);
            this->resizable(resize_box);

            // Done adding children.
            end();

            track.getWaveform().show();
            track.getWaveform().redraw();
        }

    public:

        Document(int X, int Y, int W, int H, AudioEngine& e, TrackOptions options)
            : Fl_Group(X, Y, W, H), engine(e) 
        {
            // Compute tab area.
            xPos = X + 10;
            yPos = Y + 10;
            width = W - 20;
            height = H - 100;

            // Create a new track.
            auto track = std::make_unique<AudioTrack>(engine);

            // Load the given audio file.
            if (options.filepath != nullptr) {
                track->loadFromFile(options.filepath);
                // Set the file name and extension from the file path. 
                std::filesystem::path p(options.filepath);
                fileName = p.filename();
                extension = p.extension();
            }
            // New track.
            else {
                track->setNewTrack(options);
                created = true;
                // NB: The temporary file name for this new track (eg: untitled.wav) 
                //     will be set later in the addDocument function.
            }

            // Add the new track and transfer ownership.
            trackId = engine.addTrack(std::move(track));

            renderTrackWaveform();
        }

        AudioTrack& getTrack() { return engine.getTrack(trackId); }
        unsigned int getTrackId() const { return trackId; }
        void removeTrack() { engine.removeTrack(trackId); }
        bool isChanged() const { return changed; }
        bool isNew() const { return created; }
        std::string getFileName() const { return fileName; }
        std::string getFileExtension() const { return extension; }

        void setFileName(const char* filename) {
            fileName = filename;
            // Set the file extension from the filename.
            std::filesystem::path p(filename);
            extension = p.extension();
        }

        void hasChanged() {
            changed = true;
            std::cout << "Document: " << label() << std::endl;
        }

        void saved() {
            changed = false;
        }
};

#endif // DOCUMENT_H
