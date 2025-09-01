#ifndef DOCUMENT_DATA_H
#define DOCUMENT_DATA_H

#include <memory>
#include "audio_track.h"
#include "waveform.h"
#define TEMPORARY_SIZE 100

// Forward declarations.
class AudioEngine;


class DocumentData {
        std::unique_ptr<AudioTrack> track;
        std::unique_ptr<WaveformView> waveform;
        AudioEngine& engine;
        unsigned int trackId = 0;

    public:
        DocumentData(AudioEngine& e) : engine(e) {
            track = std::make_unique<AudioTrack>(engine);
        }

        ~DocumentData() = default;

        bool loadAudioFile(const char *filepath) {
            if (!track->loadFromFile(filepath)) {
                return false;
            }

            trackId = engine.addTrack(std::move(track));
            return true;
        }

        // Getters
        AudioTrack& getTrack() { return engine.getTrackById(trackId); }
        WaveformView* getWaveform() { return waveform.get(); }
};

#endif // DOCUMENT_DATA_H
