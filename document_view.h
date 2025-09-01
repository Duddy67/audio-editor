#ifndef DOCUMENT_VIEW_H
#define DOCUMENT_VIEW_H

#include <FL/Fl_Scrollbar.H>

// Forward declarations.
class DocumentData;

class DocumentView : public Fl_Group {
        DocumentData* data;
        int x, y, w, h;

    public:

        DocumentView(int X, int Y, int W, int H, DocumentData* d)
            : Fl_Group(X, Y, W, H), data(d) 
        {
            x = X + 10;
            y = Y + 10;
            w = W - 20;
            h = H - 100;
        }

        DocumentData* getData() { return data; }

        void renderWaveform(AudioTrack& track) {
            auto waveform = data->getWaveform();
            waveform = new WaveformView(x, y, w, h, track);
            waveform->take_focus();    
            waveform->setStereoMode(track.isStereo());    

            Fl_Scrollbar* scrollbar = new Fl_Scrollbar(10, h + 20, w, 15);
            scrollbar->type(FL_HORIZONTAL);
            scrollbar->step(1);
            scrollbar->minimum(0);

            scrollbar->callback([](Fl_Widget* w, void* data) {
                auto* sb = (Fl_Scrollbar*)w;
                auto* wf = (WaveformView*)data;
                wf->setScrollOffset(sb->value());
            }, waveform);

            waveform->setScrollbar(scrollbar);
            waveform->setStereoSamples(track.getLeftSamples(), track.getRightSamples());
        }
};

#endif // DOCUMENT_VIEW_H
