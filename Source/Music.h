#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

namespace phrase {
struct Note { int pitch = 60; double start = 0, length = 1; int velocity = 90; };
struct Song {
    juce::String title = "Untitled idea", key = "A minor", message;
    double bpm = 120;
    int bars = 4;
    std::array<std::vector<Note>, 4> parts;
    juce::var toJson() const;
    static bool fromJson(const juce::var&, Song&, juce::String& error);
    static Song demo();
    bool hasNotes() const;
};
inline const std::array<juce::String, 4> partNames { "Melody", "Chords", "Bass", "Drums" };
bool writeMidi(const Song&, int part, const juce::File&, juce::String& error);
bool readMidi(const juce::File&, int part, Song&, juce::String& error);
bool applyRevision(const Song& current, const Song& proposed, int part, Song& result, juce::String& error);
juce::var object(std::initializer_list<std::pair<juce::String, juce::var>> values);
}
