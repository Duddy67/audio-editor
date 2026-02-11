#include "../main.h"

/*
 * Adds a new document (ie: audio track + waveform) to edit. 
 */
void Application::addDocument(TrackOptions options)
{
    // Height of tab label area.
    const int tabBarHeight = SMALL_SPACE; 

    // Create the group at the correct position relative to the tabs widget
    tabs->begin();

    auto doc = new Document(
        // Same coordinates as tabs.
        tabs->x(),                
        // Push down for tab bar.
        tabs->y() + tabBarHeight, 
        tabs->w(),
        tabs->h() - tabBarHeight,
        *engine,
        options
    );

    // Link the new tab to the tab closure callback function.
    doc->when(FL_WHEN_CLOSED);

    doc->callback([](Fl_Widget* w, void* userdata) {
        auto* app = static_cast<Application*>(userdata);
        app->removeDocument(static_cast<Document*>(w));
    }, this);

    std::string filename;

    if (options.filepath != nullptr) {
        //filename = std::filesystem::path(filepath).filename().string();
        filename = doc->getFileName();
    }
    else {
        newDocuments++;
        // Create new document as .wav file by default.
        filename = "New document " + std::to_string(newDocuments) + ".wav";
        doc->setFileName(filename.c_str());
    }

    // SMALL_SPACE + MEDIUM_SPACE = label max width.
    std::string label = truncateText(filename, SMALL_SPACE + MEDIUM_SPACE, FL_HELVETICA, 12); 

    // Add a tiny space on the left between the close button and the label.
    label = " " + label;
    // Pad with spaces so FLTK makes the tab ~ (SMALL_SPACE * 2) + MEDIUM_SPACE wide
    while (fl_width(label.c_str()) < (SMALL_SPACE * 2) + MEDIUM_SPACE) {
        label += " ";
    }

    // Safe string copy
    doc->copy_label(label.c_str()); 
    tabs->end();

    // It's the first document of the list.
    if (documents.size() == 0) {
        tabs->show();
        // Keep tab height constant.
        tabs->resizable(doc);
    }

    // Keep track of this document
    // documents now owns data (ie: move).
    documents.push_back(doc);

    // Switch to the newly added tab
    tabs->value(doc);

    // Force FLTK to recalc layout so the first tab displays properly
    tabs->init_sizes();
}

void Application::removeDocument(Document* document)
{
    auto it = std::find(documents.begin(), documents.end(), document);

    if (it != documents.end()) {
        // Remove the track from the track list. 
        (*it)->removeTrack();

        // Check the document widget is FLTK parented.
        // If not, memory must be freed here.
        if (!(*it)->parent()) {
            delete *it;
        }

        // Remove the document from tabs
        tabs->remove(*it);
        // then from the vector.
        documents.erase(it);

        // No document left in the tab list.
        if (documents.size() == 0) {
            tabs->hide();
        }
    }
}

void Application::removeDocuments()
{

}

/*
 * Returns the active document (ie: tab).
 */
Document& Application::getActiveDocument()
{
    auto tabs = getTabs();
    auto document = static_cast<Document*>(tabs->value());

    return *document;
}

Document& Application::getDocumentByTrackId(unsigned int trackId)
{
    for (size_t i = 0; i < documents.size(); i++) {
        if (documents[i]->getTrackId() == trackId) {
            // 
            return *documents[i];
        }
    }

    throw std::runtime_error("Couldn't find document: ");
}

void Application::documentHasChanged(unsigned int trackId)
{
    for (size_t i = 0; i < documents.size(); i++) {
        if (documents[i]->getTrackId() == trackId) {
            // Mark the given document as "changed".
            documents[i]->hasChanged();

            return;
        }
    }
}

unsigned int Application::checkChangedDocuments()
{
    unsigned int changedDocuments = 0;

    for (size_t i = 0; i < documents.size(); i++) {
        if (documents[i]->isChanged()) {
            changedDocuments++;
        }
    }

    return changedDocuments;
}

