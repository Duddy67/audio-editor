#ifndef DOCUMENT_VIEW_H
#define DOCUMENT_VIEW_H

#include <FL/Fl_Box.H>

// Forward declarations.
class DocumentData;

class DocumentView : public Fl_Group {
        DocumentData* data;

    public:

        DocumentView(int X, int Y, int W, int H, DocumentData* d)
            : Fl_Group(X, Y, W, H), data(d) 
        {
            data->getWaveform()->resize(X + 10, Y + 10, W -20, H -20);    
        }

        DocumentData* getData() { return data; }
};

#endif // DOCUMENT_VIEW_H
