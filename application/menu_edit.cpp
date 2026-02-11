#include "../main.h"
#include "../audio/edit/mute.h"
#include "../audio/edit/fade_in.h"
#include "../audio/edit/fade_out.h"
#include "../audio/edit/delete.h"

const Selection Application::getSelection(Track& track)
{
    // Get the current selection.
    auto& waveform = track.getWaveform();
    int start = waveform.getSelectionStartSample();
    int end = waveform.getSelectionEndSample();
    int totalSamples = static_cast<int>(track.getLeftSamples().size());

    // Make sure selection is valid.
    if (start >= end || start > totalSamples || end > totalSamples) {
        Selection selection = {0, 0};
        return selection; 
    }

    Selection selection = {start, end};
    return selection;
}

void Application::onUndo(Track& track)
{
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.undo(track);
    auto& waveform = track.getWaveform();
    waveform.redraw();
    std::string label = "";

    // No more edit command left.
    if (audioHistory.getLastUndo() == EditID::NONE) {
        label = MenuLabels[MenuItemID::EDIT_UNDO];
        updateMenuItem(MenuItemID::EDIT_UNDO, Action::DEACTIVATE, label);
    }
    else {
        label = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[audioHistory.getLastUndo()]; 
        updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, label);
    }

    label = MenuLabels[MenuItemID::EDIT_REDO] + " " + EditLabels[audioHistory.getLastRedo()]; 
    updateMenuItem(MenuItemID::EDIT_REDO, Action::ACTIVATE, label);
}

void Application::onRedo(Track& track)
{
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.redo(track);
    auto& waveform = track.getWaveform();
    waveform.redraw();
    std::string label = "";

    if (audioHistory.getLastRedo() == EditID::NONE) {
        label = MenuLabels[MenuItemID::EDIT_REDO];
        updateMenuItem(MenuItemID::EDIT_REDO, Action::DEACTIVATE, label);
    }
    else {
        label = MenuLabels[MenuItemID::EDIT_REDO] + " " + EditLabels[audioHistory.getLastRedo()]; 
        updateMenuItem(MenuItemID::EDIT_REDO, Action::ACTIVATE, label);
    }

    label = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[audioHistory.getLastUndo()]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, label);
}

void Application::onMute(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start >= selection.end) {
        return; 
    }

    auto muteCmd = std::make_unique<Mute>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.apply(std::move(muteCmd), track);
    track.getWaveform().redraw();

    //
    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::MUTE]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
}

void Application::onFadeIn(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start == 0 && selection.end == 0) {
        return; 
    }

    auto fadeInCmd = std::make_unique<FadeIn>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.apply(std::move(fadeInCmd), track);
    track.getWaveform().redraw();

    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::FADE_IN]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
}

void Application::onFadeOut(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start >= selection.end) {
        return; 
    }

    // Create a new fade out command process.
    auto fadeOutCmd = std::make_unique<FadeOut>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    // Apply the command.
    audioHistory.apply(std::move(fadeOutCmd), track);
    track.getWaveform().redraw();

    // Update the Undo menu item accordingly.
    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::FADE_OUT]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
}

void Application::onDelete(Track& track)
{
    // Get the current selection.
    auto selection = getSelection(track);

    if (selection.start == 0 && selection.end == 0) {
        return; 
    }

    auto deleteCmd = std::make_unique<Delete>(selection.start, selection.end);
    // Get the history from the track's parent document.
    auto& audioHistory = getActiveDocument().getAudioHistory();
    audioHistory.apply(std::move(deleteCmd), track);
    track.getWaveform().redraw();

    //
    std::string newLabel = MenuLabels[MenuItemID::EDIT_UNDO] + " " + EditLabels[EditID::DELETE]; 
    updateMenuItem(MenuItemID::EDIT_UNDO, Action::ACTIVATE, newLabel);
}

