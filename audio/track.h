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
#include "clip.h"
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
        std::vector<Clip> clips;
        std::unique_ptr<Buffer> recordingBuffer;
        std::unique_ptr<GUI> gui;
        std::atomic<uint64_t> playbackIndex{0};
        std::atomic<size_t> captureWriteIndex {0};
        std::atomic<bool> playing{false};
        std::atomic<bool> paused{false};
        std::atomic<bool> recording{false};
        size_t recordStart = 0;
        size_t totalLength = 0;
        // The MiniAudio ring buffer (for recording).
        ma_pcm_rb captureRing;                 
        std::atomic<size_t> totalRecordedFrames {0};
        std::thread workerThread;
        std::atomic<bool> workerRunning{false};
        // End of file flag.
        std::atomic<bool> endOfFile{false};
        std::atomic<bool> endOfSelection{false};
        bool newTrack = false;
        // Used for GUI.
        std::atomic<bool> newDataAvailable{false};

        void uninit();
        void drainAndMergeRingBuffer();
        void workerThreadLoop();
        void stopRecording();
        void replaceRecording(size_t recordStart, std::shared_ptr<Buffer> buffer);
        void overdubRecording(size_t recordStart, std::shared_ptr<Buffer> buffer);

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
      void splitClip(size_t position);
      void removeClips(size_t start, size_t end);
      void insertClip(Clip clip, size_t position);
      void copy(size_t start, size_t end);

      // Getters.
      bool isPlaying() const { return playing.load(); }
      bool isPaused() const { return paused.load(); }
      bool isRecording() const { return recording.load(); }
      bool isEndOfFile() const { return endOfFile.load(); }
      bool isEndOfSelection() const { return endOfSelection.load(); }
      bool isNewTrack() const { return newTrack; }
      uint64_t getCurrentSample() const { return playbackIndex.load(); }
      unsigned int getId() const { return id; }
      std::atomic<bool>& getNewDataAvailableFlag() { return newDataAvailable; }
      size_t getTotalRecordedFrames() const { return totalRecordedFrames.load(); }
      size_t getCaptureWriteIndex() const { return captureWriteIndex.load(); }
      Application& getApplication() const { return engine.getApplication(); }
      GUI& getGUI() { return *gui; }
      size_t getLength();
      float getProcessedSample(unsigned int timelineIndex, Direction channel);
      Buffer& getRecordingBuffer() { return *recordingBuffer; }
      void printClips();

      // Setters.
      void setNewTrack(TrackOptions options);
      void setId(unsigned int i);
      void setPlaybackIndex(int index) { playbackIndex.store(index); }
      void setClips(const std::vector<Clip>& newClips) { clips = newClips; }

      ////// Facade ////////

      std::vector<Clip>& getClips() { return clips; }
      void save(const char* filename);
      bool isStereo();
      void render(int x, int y, int w, int h);
      std::vector<Clip>& getClipboard() { return engine.getClipboard(); }
};

#endif // TRACK_H
