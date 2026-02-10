#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <map>
#include <string>

constexpr unsigned int HEIGHT_MENUBAR = 24;
constexpr unsigned int MODAL_WND_POS = 20;
constexpr unsigned int BUTTON_WIDTH = 80;
constexpr unsigned int BUTTON_HEIGHT = 40;
constexpr unsigned int MICRO_SPACE = 5;
constexpr unsigned int TINY_SPACE = 10;
constexpr unsigned int SMALL_SPACE = 40;
constexpr unsigned int MEDIUM_SPACE = 80;
constexpr unsigned int LARGE_SPACE = 160;
constexpr unsigned int XLARGE_SPACE = 320;
constexpr unsigned int TEXT_SIZE = 13;
constexpr unsigned int SCROLLBAR_HEIGHT = 15;
constexpr unsigned int SCROLLBAR_MARGIN = 10;
constexpr unsigned int INITIAL_BUFFER_SIZE = 10; // In seconds
constexpr unsigned int MARKING_AREA_HEIGHT = 40;
constexpr unsigned int MARKER_WIDTH = 60;
constexpr unsigned int MARKER_HEIGHT = 20;
constexpr unsigned int TAB_BORDER_THICKNESS = 10;
constexpr float VU_METER_DECAY_TIME = 1.0f;
constexpr const char* CONFIG_FILENAME = "config.json";

// --- Custom types ---

enum class Direction {LEFT, RIGHT, UP, DOWN, NONE};

enum class TimeFormat {HH_MM_SS_SSS, MM_SS_SSS, SS_SSS};

enum class EditID {
    MUTE, FADE_IN, FADE_OUT, NORMALIZE,
    VOLUME, COPY, PAST, CUT, UNDO, REDO, NONE
};

enum class Action {ACTIVATE, DEACTIVATE};

enum class MenuItemID {
    FILE_SUB, FILE_NEW, FILE_OPEN, FILE_SAVE, FILE_SAVE_AS, FILE_QUIT, EDIT_SUB,
    EDIT_UNDO, EDIT_REDO, EDIT_COPY, EDIT_PAST, EDIT_CUT, EDIT_INSERT_MARKER,
    EDIT_SETTINGS, PROCESS_SUB, PROCESS_MUTE, PROCESS_NORMALIZE, PROCESS_VOLUME,
    PROCESS_FADE_IN, PROCESS_FADE_OUT
};

struct Selection {
    int start, end;
};

inline std::map<EditID, std::string> EditLabels {
    {EditID::MUTE, "Mute"},
    {EditID::FADE_IN, "Fade in"},
    {EditID::FADE_OUT, "Fade out"},
    {EditID::NONE, ""}
};

inline std::map<MenuItemID, std::string> MenuLabels {
    {MenuItemID::FILE_SUB, "File"},
    {MenuItemID::FILE_NEW, "File/&New"},
    {MenuItemID::FILE_OPEN, "File/&Open"},
    {MenuItemID::FILE_SAVE, "File/&Save"},
    {MenuItemID::FILE_SAVE_AS, "File/_&Save as"},
    {MenuItemID::FILE_QUIT, "File/&Quit"},
    {MenuItemID::EDIT_SUB, "Edit"},
    {MenuItemID::EDIT_UNDO, "Edit/&Undo"},
    {MenuItemID::EDIT_REDO, "Edit/&Redo"},
    {MenuItemID::EDIT_COPY, "Edit/&Copy"},
    {MenuItemID::EDIT_PAST, "Edit/&Past"},
    {MenuItemID::EDIT_CUT, "Edit/&Cut"},
    {MenuItemID::EDIT_INSERT_MARKER, "Edit/&Insert marker"},
    {MenuItemID::EDIT_SETTINGS, "Edit/&Settings"},
    {MenuItemID::PROCESS_SUB, "Process"},
    {MenuItemID::PROCESS_MUTE, "Process/&Mute"},
    {MenuItemID::PROCESS_NORMALIZE, "Process/&Normalize"},
    {MenuItemID::PROCESS_VOLUME, "Process/&Volume"},
    {MenuItemID::PROCESS_FADE_IN, "Process/&Fade in"},
    {MenuItemID::PROCESS_FADE_OUT, "Process/&Fade out"}
};

#endif
