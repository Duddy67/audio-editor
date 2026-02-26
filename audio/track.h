#ifndef TRACK_H
#define TRACK_H

#include <FL/Fl_Group.H>
#include <string>
#include <iostream>
#include <filesystem>
#include <atomic>
#include <vector>
#include <thread>
#include <time.h>
#include "../../libraries/miniaudio.h"
#include "../view/waveform.h"
#include "engine.h"
#include "file_io.h"
#include "buffer.h"
#include "gui.h"
#include "../marking/marking.h"

// Forward declarations.
class Engine;
class Application;

struct TrackOptions {
    // Open file. 
    const char *filepath = nullptr;
    // New file.
    bool stereo = true;
};

/*
 * The Track class is a kind of interface allowing the application and the MiniAudio
 * library to communicate with each other.
 */
class Track {
    private:

        // Track unique id. 0 = invalid.
        unsigned int id = 0;
        ma_uint64 frameCount;
        Engine& engine;
        std::unique_ptr<Buffer> buffer = std::make_unique<Buffer>();
        std::unique_ptr<GUI> gui;
        std::atomic<uint64_t> playbackSampleIndex{0};
        std::atomic<size_t> captureWriteIndex {0};
        std::atomic<bool> playing{false};
        std::atomic<bool> paused{false};
        std::atomic<bool> recording{false};
        // The MiniAudio ring buffer (for recording).
        ma_pcm_rb captureRing;                 
        std::atomic<size_t> totalRecordedFrames {0};
        std::thread workerThread;
        std::atomic<bool> workerRunning{false};
        // End of file flag.
        std::atomic<bool> eof{false};
        // End of selection flag.
        std::atomic<bool> eos{false};
        bool newTrack = false;
        // Used for GUI.
        std::atomic<bool> newDataAvailable{false};

        void uninit();
        void drainAndMergeRingBuffer();
        void workerThreadLoop();

    public:
      Track(Engine& e) : engine(e) {}

      void loadFromFile(const char *fileName);
      void play();
      void pause();
      void unpause();
      void stop();
      void record();
      void mixInto(float* output, int frameCount);
      void recordInto(const float* input, ma_uint32 frameCount, ma_uint32 captureChannels);
      void prepareRecording();
      void updateTime();

      // Getters.
      bool isPlaying() const { return playing.load(); }
      bool isPaused() const { return paused.load(); }
      bool isRecording() const { return recording.load(); }
      bool isEndOfFile() const { return eof.load(); }
      bool isEndOfSelection() const { return eos.load(); }
      bool isNewTrack() const { return newTrack; }
      uint64_t getCurrentSample() const { return playbackSampleIndex.load(); }
      unsigned int getId() const { return id; }
      std::atomic<bool>& getNewDataAvailableFlag() { return newDataAvailable; }
      size_t getTotalRecordedFrames() const { return totalRecordedFrames.load(); }
      size_t getCaptureWriteIndex() const { return captureWriteIndex.load(); }
      bool getNewSamplesCopy(std::vector<float>& leftCopy, std::vector<float>& rightCopy, size_t& newStartIndex, size_t& newCount);
      Application& getApplication() const { return engine.getApplication(); }
      const Engine& getEngine() { return engine; }
      Buffer& getBuffer() { return *buffer; }
      GUI& getGUI() { return *gui; }

      // Setters.
      void setNewTrack(TrackOptions options);
      void setId(unsigned int i);
      void setPlaybackSampleIndex(int index) { playbackSampleIndex.store(index); }
      //void resetEndOfFile() { eof.store(false); }

      ////// Facade ////////

      std::vector<float>& getLeftSamples() { return buffer->getLeftSamples(); }
      std::vector<float>& getRightSamples() { return buffer->getRightSamples(); }
      void save(const char* filename);
      bool isStereo() { return buffer->isStereo(); }
      void render(int x, int y, int w, int h);
};

#endif // TRACK_H

