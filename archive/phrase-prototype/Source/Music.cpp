#include "Music.h"
#include <cmath>

namespace phrase {
juce::var object(std::initializer_list<std::pair<juce::String, juce::var>> values) {
    auto* o = new juce::DynamicObject();
    for (const auto& v : values) o->setProperty(v.first, v.second);
    return juce::var(o);
}
juce::var Song::toJson() const {
    juce::Array<juce::var> tracks;
    for (int p = 0; p < 4; ++p) {
        juce::Array<juce::var> notes;
        for (const auto& n : parts[(size_t)p])
            notes.add(object({{"pitch", n.pitch}, {"start", n.start}, {"length", n.length}, {"velocity", n.velocity}}));
        tracks.add(object({{"part", partNames[(size_t)p].toLowerCase()}, {"notes", notes}}));
    }
    return object({{"title", title}, {"key", key}, {"bpm", bpm}, {"bars", bars}, {"message", message}, {"tracks", tracks}});
}
static bool number(const juce::var& v) { return v.isInt() || v.isInt64() || v.isDouble(); }
bool Song::fromJson(const juce::var& v, Song& out, juce::String& error) {
    auto fail = [&error](const juce::String& s) { error = s; return false; };
    if (!v.isObject()) return fail("Expected a musical idea object.");
    Song s;
    if (!v["title"].isString() || !v["key"].isString() || !v["message"].isString()) return fail("Missing idea description.");
    s.title = v["title"].toString().substring(0, 120); s.key = v["key"].toString().substring(0, 40);
    s.message = v["message"].toString().substring(0, 4000);
    if (!number(v["bpm"]) || !number(v["bars"])) return fail("Missing tempo or length.");
    s.bpm = (double)v["bpm"]; const double barsValue = (double)v["bars"];
    if (!std::isfinite(s.bpm) || s.bpm < 30 || s.bpm > 300 || !std::isfinite(barsValue)
        || barsValue < 1 || barsValue > 16 || std::floor(barsValue) != barsValue) return fail("Use 1-16 bars and 30-300 BPM.");
    s.bars = (int)barsValue;
    auto* tracks = v["tracks"].getArray();
    if (tracks == nullptr || tracks->size() != 4) return fail("The idea must contain four parts.");
    std::array<bool, 4> seen {};
    for (const auto& t : *tracks) {
        const int p = (int)std::distance(partNames.begin(), std::find_if(partNames.begin(), partNames.end(), [&](const auto& name) { return name.equalsIgnoreCase(t["part"].toString()); }));
        if (p == 4 || seen[(size_t)p]) return fail("Unknown or repeated musical part.");
        seen[(size_t)p] = true;
        auto* notes = t["notes"].getArray();
        if (!notes || notes->size() > 512) return fail("Each part supports up to 512 notes.");
        for (const auto& n : *notes) {
            if (!number(n["pitch"]) || !number(n["start"]) || !number(n["length"]) || !number(n["velocity"])) return fail("A note is missing musical values.");
            const double pitch = (double)n["pitch"], vel = (double)n["velocity"];
            if (!std::isfinite(pitch) || !std::isfinite(vel) || pitch < 0 || pitch > 127 || vel < 1 || vel > 127
                || std::floor(pitch) != pitch || std::floor(vel) != vel) return fail("Invalid note pitch or velocity.");
            Note note { (int)pitch, (double)n["start"], (double)n["length"], (int)vel };
            if (pitch < 0 || pitch > 127 || vel < 1 || vel > 127
                || std::floor(pitch) != pitch || std::floor(vel) != vel || !std::isfinite(note.start) || !std::isfinite(note.length)
                || note.start < 0 || note.length <= 0 || note.start + note.length > s.bars * 4.0 + 0.00001)
                return fail("A note is outside the idea's valid pitch, timing or velocity range.");
            s.parts[(size_t)p].push_back(note);
        }
        std::stable_sort(s.parts[(size_t)p].begin(), s.parts[(size_t)p].end(), [](const auto& a, const auto& b) { return a.start < b.start; });
    }
    out = std::move(s); error.clear(); return true;
}
bool Song::hasNotes() const { for (const auto& p : parts) if (!p.empty()) return true; return false; }
bool applyRevision(const Song& current, const Song& proposed, int part, Song& result, juce::String& error) {
    if (part < -1 || part > 3) { error = "Choose all parts or a single musical part."; return false; }
    auto merged = proposed;
    if (part >= 0) {
        merged = current;
        merged.parts[(size_t)part] = proposed.parts[(size_t)part];
        merged.message = proposed.message;
    }
    // Revalidate after merging: selected-part revisions cannot extend the existing idea.
    return Song::fromJson(merged.toJson(), result, error);
}
Song Song::demo() {
    Song s; s.title = "After hours"; s.bpm = 92;
    s.message = "Example idea loaded locally. Preview it, drag a part into FL Studio, or connect your API key to make something new.";
    const int roots[] {45, 41, 48, 43};
    const int melody[] {69, 72, 76, 74, 72, 69, 67, 64, 65, 69, 72, 69, 67, 71, 74, 76};
    for (int bar = 0; bar < 4; ++bar) {
        for (int interval : {0, 7, 12}) s.parts[1].push_back({roots[bar] + interval + 12, bar * 4.0, 3.75, 66});
        for (int beat = 0; beat < 4; ++beat) {
            s.parts[0].push_back({melody[bar * 4 + beat], bar * 4.0 + beat + (beat % 2 ? 0.25 : 0), 0.5, 82 + beat * 3});
            s.parts[2].push_back({roots[bar] - 12, bar * 4.0 + beat, 0.7, 88});
            s.parts[3].push_back({beat % 2 ? 38 : 36, bar * 4.0 + beat, 0.2, 90});
            for (int sub = 0; sub < 2; ++sub) s.parts[3].push_back({42, bar * 4.0 + beat + sub * 0.5, 0.1, sub ? 50 : 64});
        }
    }
    return s;
}
bool writeMidi(const Song& s, int part, const juce::File& file, juce::String& error) {
    juce::MidiFile midi; midi.setTicksPerQuarterNote(960);
    juce::MidiMessageSequence meta;
    meta.addEvent(juce::MidiMessage::tempoMetaEvent((int)std::round(60000000.0 / s.bpm)));
    meta.addEvent(juce::MidiMessage::timeSignatureMetaEvent(4, 4));
    auto end = juce::MidiMessage::endOfTrack(); end.setTimeStamp(s.bars * 4.0 * 960); meta.addEvent(end); midi.addTrack(meta);
    for (int p = 0; p < 4; ++p) {
        if (part >= 0 && p != part) continue;
        juce::MidiMessageSequence seq;
        seq.addEvent(juce::MidiMessage::textMetaEvent(3, partNames[(size_t)p]));
        const int channel = p == 3 ? 10 : p + 1;
        for (const auto& n : s.parts[(size_t)p]) {
            auto on = juce::MidiMessage::noteOn(channel, n.pitch, (juce::uint8)n.velocity); on.setTimeStamp(n.start * 960); seq.addEvent(on);
            auto off = juce::MidiMessage::noteOff(channel, n.pitch); off.setTimeStamp((n.start + n.length) * 960); seq.addEvent(off);
        }
        seq.addEvent(end); seq.updateMatchedPairs(); midi.addTrack(seq);
    }
    auto stream = file.createOutputStream();
    if (!stream || !stream->openedOk()) { error = "Could not open the MIDI file for writing."; return false; }
    stream->setPosition(0); stream->truncate();
    if (!midi.writeTo(*stream)) { error = "Could not write MIDI."; return false; }
    stream->flush(); return true;
}
bool readMidi(const juce::File& file, int part, Song& s, juce::String& error) {
    auto stream = file.createInputStream(); juce::MidiFile midi;
    if (!stream || !midi.readFrom(*stream) || midi.getTimeFormat() <= 0) { error = "Choose a MIDI file with musical beat timing (PPQ)."; return false; }
    std::vector<Note> notes; double endBeat = 0;
    for (int t = 0; t < midi.getNumTracks(); ++t) {
        auto seq = *midi.getTrack(t); seq.updateMatchedPairs();
        for (int i = 0; i < seq.getNumEvents(); ++i) {
            auto* ev = seq.getEventPointer(i);
            if (ev->message.isNoteOn() && ev->noteOffObject) {
                const double start = ev->message.getTimeStamp() / midi.getTimeFormat();
                const double length = (ev->noteOffObject->message.getTimeStamp() - ev->message.getTimeStamp()) / midi.getTimeFormat();
                if (start >= 0 && length > 0 && start + length <= 64) { notes.push_back({ev->message.getNoteNumber(), start, length, ev->message.getVelocity()}); endBeat = juce::jmax(endBeat, start + length); }
                else { error = "Import supports ideas up to 16 bars."; return false; }
            }
        }
    }
    if (notes.empty() || notes.size() > 512) { error = "Choose MIDI containing 1-512 notes, up to 16 bars."; return false; }
    s.parts[(size_t)juce::jlimit(0, 3, part)] = std::move(notes); s.bars = juce::jmax(s.bars, (int)std::ceil(endBeat / 4));
    s.title = file.getFileNameWithoutExtension(); return true;
}
}
