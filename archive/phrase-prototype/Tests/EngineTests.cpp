#include "Processor.h"
#include <iostream>
#include <stdexcept>
#include <cmath>

static int checks = 0;
static void check(bool ok, const char* name) { ++checks; if (!ok) throw std::runtime_error(name); }
int main() {
    juce::ScopedJuceInitialiser_GUI initialise;
    try {
        PhraseProcessor p;
        p.prepareToPlay(48000, 256);
        p.setSong(phrase::Song::demo());
        p.apiKey = "TEST_SECRET_MUST_NOT_BE_SERIALISED";
        juce::MemoryBlock state; p.getStateInformation(state);
        check(!state.toString().contains("TEST_SECRET"), "API key excluded from state");
        PhraseProcessor restored; restored.setStateInformation(state.getData(), (int)state.getSize());
        check(restored.getSong().parts[0].size() == 16 && restored.getSong().bpm == 92, "processor state restores music");
        check(restored.apiKey.isEmpty(), "state does not restore API key");
        check(p.canUndo(), "musical changes undoable");
        check(p.undo() && !p.getSong().hasNotes(), "undo restores empty idea");
        p.setSong(phrase::Song::demo()); p.togglePreview();
        juce::AudioBuffer<float> audio(2, 256); juce::MidiBuffer midi;
        bool audible = false, noteOn = false;
        for (int block = 0; block < 100; ++block) {
            midi.clear(); p.processBlock(audio, midi);
            for (const auto metadata : midi) if (metadata.getMessage().isNoteOn()) noteOn = true;
            for (int sample = 0; sample < 256; ++sample) {
                const auto value = audio.getSample(0, sample);
                check(std::isfinite(value) && std::abs(value) <= 1, "audio finite and bounded");
                if (std::abs(value) > 0.001) audible = true;
                check(value == audio.getSample(1, sample), "stereo channels consistent");
            }
        }
        check(audible && noteOn, "preview generates audio and MIDI");
        check(p.getPlayhead() > 0, "preview playhead advances");
        p.stopPreview(); midi.clear(); p.processBlock(audio, midi);
        check(!p.isPreviewing() && p.getPlayhead() == 0 && audio.getMagnitude(0, 256) == 0, "stop clears sound and playhead");
        bool allOff = false; for (auto metadata : midi) allOff |= metadata.getMessage().isAllNotesOff();
        check(allOff, "stop sends MIDI all notes off");
        std::cout << checks << " engine checks passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
