#ifndef TIME_H
#define TIME_H

#include <iostream>
#include <cmath>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Button.H>
#include "constants.h"


class Time : public Fl_Box {
        struct TimePosition
        {
            // Absolute position in samples.
            uint64_t samples;
            // Samples per second.
            uint32_t sampleRate;

            // Whole hours.
            uint64_t hours() const
            {
                return samples / (uint64_t(sampleRate) * 3600ULL);
            }

            // Total time in seconds (fractional).
            double seconds() const
            {
                return samples / static_cast<double>(sampleRate);
            }

            // Minutes within the current hour (0–59).
            uint64_t minutes() const
            {
                return uint64_t(seconds()) / 60;
            }

            // Seconds within the current minute (0–59).
            uint64_t secondsPart() const
            {
                return uint64_t(seconds()) % 60;
            }

            // Milliseconds within the current second (0–999).
            uint64_t milliseconds() const
            {
                return (samples * 1000ULL / uint64_t(sampleRate)) % 1000;
            }
        };

        Fl_Menu_Button* menu = nullptr;
        uint64_t currentSample = 0;
        uint32_t sampleRate = 44100; // default
        TimeFormat timeFormat;

        void createMenu();

    public:

        Time(int X, int Y, int W, int H, const char* L, TimeFormat tf = MM_SS_SSS)
            : Fl_Box(X, Y, W, H, L), timeFormat(tf)
        {
            color(FL_WHITE);
            box(FL_DOWN_BOX);
        }

        int handle(int event) override;
        TimePosition getTimePosition(uint64_t sample, uint32_t sampleRate);
        void update(uint64_t sample);
        void setFormat(TimeFormat format);
        void setSampleRate(uint32_t sampleRate) { this->sampleRate = sampleRate; }
};

#endif // TIME_H
