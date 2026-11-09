#include "Processor.h"
#include "Editor.h"
#include <cmath>

PhraseProcessor::PhraseProcessor() : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}
void PhraseProcessor::prepareToPlay(double rate, int) { sampleRate = rate; voices = {}; beat = 0; eventIndex = 0; restart = true; }
bool PhraseProcessor::isBusesLayoutSupported(const BusesLayout& l) const { return l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo(); }
phrase::Song PhraseProcessor::getSong() const { const juce::ScopedLock lock(stateLock); return song; }
void PhraseProcessor::setSong(const phrase::Song& s, bool keepHistory) {
    { const juce::ScopedLock lock(stateLock); if (keepHistory) { history.push_back(song); if (history.size() > 20) history.erase(history.begin()); } song = s; }
    std::array<Event, 4096> next {}; int count = 0;
    for (int p = 0; p < 4; ++p) for (const auto& n : s.parts[(size_t)p]) {
        if (count + 2 > (int)next.size()) break;
        next[(size_t)count++] = {n.start, n.pitch, n.velocity, p, true};
        next[(size_t)count++] = {n.start + n.length, n.pitch, 0, p, false};
    }
    std::sort(next.begin(), next.begin() + count, [](const auto& a, const auto& b) { if (a.beat == b.beat) return a.on < b.on; return a.beat < b.beat; });
    const juce::SpinLock::ScopedLockType lock(scheduleLock); pending = next; pendingCount = count; pendingBpm = s.bpm; pendingBeats = s.bars * 4; scheduleChanged = true;
    playing = false; flush = true; restart = true;
}
bool PhraseProcessor::undo() {
    phrase::Song previous;
    { const juce::ScopedLock lock(stateLock); if (history.empty()) return false; previous = history.back(); history.pop_back(); }
    setSong(previous, false); return true;
}
bool PhraseProcessor::canUndo() const { const juce::ScopedLock lock(stateLock); return !history.empty(); }
void PhraseProcessor::togglePreview() { if (playing) stopPreview(); else { restart = true; playing = true; } }
void PhraseProcessor::stopPreview() { playing = false; flush = true; playhead = 0; }
void PhraseProcessor::noteOn(int pitch, int velocity, int part) {
    Voice* v = nullptr; for (auto& voice : voices) if (!voice.active) { v = &voice; break; }
    if (!v) v = &voices[0];
    *v = {true, pitch, part, 0, 0, -1, velocity / 127.0f};
}
void PhraseProcessor::noteOff(int pitch, int part) { for (auto& v : voices) if (v.active && v.pitch == pitch && v.part == part && v.release < 0) v.release = 0; }
float PhraseProcessor::renderSample() {
    float sum = 0;
    for (auto& v : voices) {
        if (!v.active) continue;
        const double dt = 1.0 / sampleRate;
        double freq = juce::MidiMessage::getMidiNoteInHertz(v.pitch), x = 0;
        double env = juce::jmin(1.0, v.age / 0.008) * std::exp(-v.age * (v.part == 1 ? 0.55 : 1.8));
        if (v.part == 3) {
            if (v.pitch == 35 || v.pitch == 36) { freq = 46 + 100 * std::exp(-v.age * 35); x = std::sin(v.phase); env = std::exp(-v.age * 16); }
            else { const bool hat = v.pitch >= 42; x = noise.nextFloat() * 2 - 1; env = std::exp(-v.age * (hat ? 60 : 22)); }
        } else {
            x = std::sin(v.phase) + (v.part == 2 ? 0.3 : 0.18) * std::sin(v.phase * 2) + (v.part == 0 ? 0.1 * std::sin(v.phase * 3) : 0);
        }
        if (v.release >= 0 && v.part != 3) { env *= std::exp(-v.release * 18); v.release += dt; }
        v.age += dt; v.phase = std::fmod(v.phase + juce::MathConstants<double>::twoPi * freq / sampleRate, juce::MathConstants<double>::twoPi);
        sum += (float)(x * env) * v.velocity * (v.part == 1 ? 0.055f : 0.10f);
        if (env < 0.0001 && v.age > 0.05) v.active = false;
    }
    return std::tanh(sum);
}
void PhraseProcessor::processBlock(juce::AudioBuffer<float>& audio, juce::MidiBuffer& midi) {
    juce::ScopedNoDenormals noDenormals; audio.clear();
    { const juce::SpinLock::ScopedTryLockType lock(scheduleLock);
      if (lock.isLocked() && scheduleChanged) { events = pending; eventCount = pendingCount; previewBpm = pendingBpm; previewBeats = pendingBeats; scheduleChanged = false; } }
    if (flush.exchange(false)) { voices = {}; for (int ch : {1, 2, 3, 10}) midi.addEvent(juce::MidiMessage::allNotesOff(ch), 0); }
    if (restart.exchange(false)) { beat = 0; eventIndex = 0; voices = {}; }
    // Incoming MIDI is passed through. The plugin's own preview drives its internal audition synth.
    for (int i = 0; i < audio.getNumSamples(); ++i) {
        if (playing.load(std::memory_order_relaxed)) {
            while (eventIndex < eventCount && events[(size_t)eventIndex].beat <= beat) {
                const auto& e = events[(size_t)eventIndex++]; const int channel = e.part == 3 ? 10 : e.part + 1;
                if (e.on) { noteOn(e.pitch, e.velocity, e.part); midi.addEvent(juce::MidiMessage::noteOn(channel, e.pitch, (juce::uint8)e.velocity), i); }
                else { noteOff(e.pitch, e.part); midi.addEvent(juce::MidiMessage::noteOff(channel, e.pitch), i); }
            }
            beat += previewBpm / (60.0 * sampleRate);
            if (beat >= previewBeats) { beat = 0; eventIndex = 0; voices = {}; for (int ch : {1, 2, 3, 10}) midi.addEvent(juce::MidiMessage::allNotesOff(ch), i); }
        }
        const float value = renderSample(); for (int ch = 0; ch < audio.getNumChannels(); ++ch) audio.setSample(ch, i, value);
    }
    playhead.store(playing ? beat : 0);
}
void PhraseProcessor::getStateInformation(juce::MemoryBlock& dest) {
    const auto state = phrase::object({{"song", getSong().toJson()}, {"model", model}, {"conversation", conversation.substring(0, 16000)}});
    const auto text = juce::JSON::toString(state); dest.replaceAll(text.toRawUTF8(), text.getNumBytesAsUTF8());
}
void PhraseProcessor::setStateInformation(const void* data, int size) {
    const auto state = juce::JSON::parse(juce::String::fromUTF8((const char*)data, size)); phrase::Song s; juce::String error;
    if (phrase::Song::fromJson(state["song"], s, error)) { setSong(s, false); model = state.getProperty("model", "gpt-4.1-mini").toString(); conversation = state["conversation"].toString(); }
}
juce::AudioProcessorEditor* PhraseProcessor::createEditor() { return new PhraseEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PhraseProcessor(); }
