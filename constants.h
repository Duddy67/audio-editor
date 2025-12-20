#ifndef CONSTANTS_H
#define CONSTANTS_H

constexpr unsigned int HEIGHT_MENUBAR = 24;
constexpr unsigned int MODAL_WND_POS = 20;
constexpr unsigned int TINY_SPACE = 10;
constexpr unsigned int SMALL_SPACE = 40;
constexpr unsigned int MEDIUM_SPACE = 80;
constexpr unsigned int LARGE_SPACE = 160;
constexpr unsigned int XLARGE_SPACE = 320;
constexpr unsigned int TEXT_SIZE = 13;
constexpr unsigned int SCROLLBAR_HEIGHT = 15;
constexpr unsigned int SCROLLBAR_MARGIN = 10;
constexpr unsigned int INITIAL_BUFFER_SIZE = 10; // In seconds
constexpr float VU_METER_DECAY_TIME = 1.0f;
constexpr const char* CONFIG_FILENAME = "config.json";

// --- Custom types ---
typedef enum direction {
    NONE, LEFT, RIGHT, UP, DOWN
}Direction;

#endif
