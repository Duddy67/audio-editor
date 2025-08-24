#ifndef DOCUMENT_DATA_H
#define DOCUMENT_DATA_H

#include <memory>
#include "audio_track.h"
#include "waveform.h"
#define TEMPORARY_SIZE 100

// Forward declarations.
//class AudioTrack;
//class WaveformView;
class AudioEngine;


class DocumentData {
        std::unique_ptr<AudioTrack> track;
        std::unique_ptr<WaveformView> waveform;

    public:
        DocumentData(AudioEngine* pEngine) {
            track = std::make_unique<AudioTrack>(pEngine);
            // 
            waveform = std::make_unique<WaveformView>(TEMPORARY_SIZE, TEMPORARY_SIZE, TEMPORARY_SIZE, TEMPORARY_SIZE);
        }

        ~DocumentData() = default;

        AudioTrack* getTrack() { return track.get(); }
        WaveformView* getWaveform() { return waveform.get(); }
};

#endif // DOCUMENT_DATA_H
