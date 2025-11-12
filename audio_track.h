#ifndef AUDIO_TRACK_H
#define AUDIO_TRACK_H

#include <string>
#include <iostream>
#include <filesystem>
#include <atomic>
#include <vector>
#include <thread>
#include <bits/stdc++.h> // std::map
#include <time.h>
#include "../libraries/miniaudio.h"
#include "waveform.h"
#define INITIAL_BUFFER_SIZE 10 // In seconds

// Forward declarations.
class AudioEngine;

/*
 * The AudioTrack class is a kind of interface allowing the application and the MiniAudio
 * library to communicate with each other.
 */
class AudioTrack {
    private:
        struct OriginalFileFormat {
            std::string fileName;
            ma_uint32 outputChannels;
            ma_uint32 outputSampleRate;
            ma_format outputFormat;
        };

        // Track unique id. 0 = invalid.
        unsigned int id = 0;
        ma_context context;
        ma_decoder decoder;
        ma_uint64 frameCount;
        AudioEngine& engine;
        std::vector<float> leftSamples;
        std::vector<float> rightSamples;
        int totalFrames = 0;
        bool stereo = true;
        std::atomic<int> playbackSampleIndex{0};
        std::atomic<int> captureSampleIndex{0};
        std::atomic<int> recordingStartIndex{0};
        std::atomic<bool> playing{false};
        std::atomic<bool> paused{false};
        std::vector<float> recordedLeft;
        std::vector<float> recordedRight;
        std::atomic<bool> recording{false};
        // 0 = unlimited
        int maxRecordingSamples = 0;
        // Number of frames published by audio callback.
        std::atomic<size_t> recordedFrameCount {0}; 
        // Capacity in frames (per channel).
        size_t maxRecordedFrames = 0; 
        std::mutex recordingMutex;
        // End of file flag.
        std::atomic<bool> eof{false};
        OriginalFileFormat originalFileFormat;
        std::unique_ptr<WaveformView> waveform;  
        bool newTrack = false;

        bool storeOriginalFileFormat(const char* filename);
        void uninit();
        bool decodeFile();
        void setRecordedData();

    public:
      AudioTrack(AudioEngine& e) : engine(e) {}

      void loadFromFile(const char *fileName);
      void play();
      void pause();
      void unpause();
      void stop();
      void record();
      void mixInto(float* output, int frameCount);
      void recordInto(const float* input, ma_uint32 frameCount, ma_uint32 captureChannels);
      void prepareRecording();
      // Number of recorded frames (stereo frames = frames per channel)
      size_t getRecordedFrameCount() const { return recordedFrameCount.load(std::memory_order_acquire); }
      void renderWaveform(int x, int y, int w, int h);

      // Getters.
      std::map<std::string, std::string> getOriginalFileFormat();
      bool isStereo() { return stereo; }
      bool isPlaying() const { return playing.load(); }
      bool isPaused() const { return paused.load(); }
      bool isRecording() const { return recording.load(); }
      bool isEndOfFile() const { return eof.load(); }
      bool isNewTrack() const { return newTrack; }
      int getCurrentSample() const { return playbackSampleIndex.load(); }
      int getCurrentCaptureSample() const { return captureSampleIndex.load(); }
      std::vector<float> getLeftSamples() { return leftSamples; }
      std::vector<float> getRightSamples() { return rightSamples; }
      std::vector<float>& getRecordedLeftSamples() { return recordedLeft; }
      std::vector<float>& getRecordedRightSamples() { return recordedRight; }
      unsigned int getId() { return id; }
      WaveformView& getWaveform() { return *waveform.get(); }
      // Setters.
      void setNewTrack();
      void setId(unsigned int i);
      void setPlaybackSampleIndex(int index) { playbackSampleIndex.store(index); }
      void resetEndOfFile() { eof.store(false); }
};

#endif // AUDIO_TRACK_H

