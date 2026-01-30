#ifndef VU_METER_H
#define VU_METER_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <cmath>


class VuMeter : public Fl_Widget {
    private:
        // Current level 0–1.
        float level;         
        // Current held peak 0–1.
        float peakHold;     
        // For normal level smoothing.
        float decayRate;     
        // For the peak hold fall speed.
        float peakDecayRate; 
        // Time used to decrease level and peak 
        // just after playback stopped.
        float vuDecayTimer = 0.0f;

    public:

        VuMeter(int X, int Y, int W, int H)
            : Fl_Widget(X, Y, W, H), level(0.0f), peakHold(0.0f), decayRate(0.02f), peakDecayRate(0.005f) {}

        void setLevel(float newLevel, float newPeak)
        {
            // Smooth RMS (already smoothed externally).
            level = newLevel;

            // Update peak hold with external peak value
            if (newPeak > peakHold) {
                peakHold = newPeak;  // Rise immediately.
            } 
            else {
                peakHold -= peakDecayRate;  // Fall slowly.

                if (peakHold < 0) {
                    peakHold = 0;
                }
            }

            redraw();
        }

        void resetDecayTimer()
        {
            vuDecayTimer = 0.0f;
            // Reset decay rate as well to its initial value.
            peakDecayRate = 0.005f;
        }

        void decayTimer()
        {
            vuDecayTimer += 0.05f;

            // When third of VU_METER_DECAY_TIME is reached (ie: 1.0f ~1 sec)  
            if (vuDecayTimer >= 0.3f) {
                // make the peak decreasing faster to prevent being frozen
                // somewhere along the bar when the timer stop.
                peakDecayRate = 0.1f;
            }
        }

        float getVuDecayTimer() const { return vuDecayTimer; }

        void draw() override
        {
            fl_draw_box(FL_FLAT_BOX, x(), y(), w(), h(), FL_DARK3);

            // Color gradient
            if (level < 0.6f) {
                fl_color(FL_GREEN);
            }
            else if (level < 0.85f) {
                fl_color(FL_YELLOW);
            }
            else {
                fl_color(FL_RED);
            }

            if (type() == FL_VERTICAL) {
                int barHeight = static_cast<int>(level * h());
                int yTop = y() + h() - barHeight;
                // Draw the filled part (bottom to top)
                fl_rectf(x(), yTop, w(), barHeight);

                // --- Draw peak-hold line ---
                int peakY = y() + h() - static_cast<int>(peakHold * h());
                fl_color(FL_WHITE);
                // Thin horizontal white line
                fl_rectf(x(), peakY, w(), 2);
            }
            else {  // FL_HORIZONTAL
                // Make sure the bar visually reaches the top when the signal is peaking.
                level = (peakHold == 1) ? peakHold : level;
                int barWidth = static_cast<int>(level * w());
                // Draw the filled part (left to right)
                fl_rectf(x(), y(), barWidth, h());

                // --- Draw peak-hold line ---
                int peakX = x() + static_cast<int>(peakHold * w());
                fl_color(FL_WHITE);
                // Thin vertical white line
                fl_rectf(peakX - 1, y(), 2, h()); 
            }


            fl_color(FL_BLACK);
            fl_rect(x(), y(), w(), h());
        }
};

#endif // VU_METER_H
