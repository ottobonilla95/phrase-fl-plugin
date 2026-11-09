#pragma once
#include "Music.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class PhraseProcessor : public juce::AudioProcessor {
public:
    PhraseProcessor();
    const juce::String getName() const override { return "Phrase"; }
    void prepareToPlay(double rate, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return true; }
    double getTailLengthSeconds() const override { return 0.5; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;
    phrase::Song getSong() const;
    void setSong(const phrase::Song&, bool history = true);
    bool undo();
    bool canUndo() const;
    void togglePreview();
    void stopPreview();
    bool isPreviewing() const { return playing.load(); }
    double getPlayhead() const { return playhead.load(); }
    juce::String apiKey, model = "gpt-4.1-mini", conversation;
private:
    struct Event { double beat = 0; int pitch = 60, velocity = 90, part = 0; bool on = false; };
    struct Voice { bool active = false; int pitch = 60, part = 0; double phase = 0, age = 0, release = -1; float velocity = 0; };
    mutable juce::CriticalSection stateLock;
    phrase::Song song;
    std::vector<phrase::Song> history;
    juce::SpinLock scheduleLock;
    std::array<Event, 4096> pending {}, events {};
    int pendingCount = 0, eventCount = 0, eventIndex = 0;
    double pendingBpm = 120, pendingBeats = 16, previewBpm = 120, previewBeats = 16;
    bool scheduleChanged = true;
    std::array<Voice, 64> voices {};
    double sampleRate = 44100, beat = 0;
    std::atomic<bool> playing {false}, restart {false}, flush {false};
    std::atomic<double> playhead {0};
    juce::Random noise { 1234567 };
    void noteOn(int pitch, int velocity, int part);
    void noteOff(int pitch, int part);
    float renderSample();
};
