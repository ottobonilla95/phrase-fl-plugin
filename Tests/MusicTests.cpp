#include "Music.h"
#include "AI.h"
#include <iostream>
#include <stdexcept>
#include <limits>

static int checks = 0;
static void check(bool ok, const char* name) { ++checks; if (!ok) throw std::runtime_error(name); }
int main() {
    try {
        auto demo = phrase::Song::demo(); phrase::Song parsed; juce::String error;
        check(phrase::Song::fromJson(demo.toJson(), parsed, error), "demo parses");
        check(parsed.parts[0].size() == 16 && parsed.parts[3].size() == 48, "all demo parts survive");
        check(juce::JSON::toString(parsed.toJson()) == juce::JSON::toString(demo.toJson()), "song round trip");
        auto invalid = demo.toJson(); invalid.getDynamicObject()->setProperty("bpm", 0);
        check(!phrase::Song::fromJson(invalid, parsed, error), "reject zero tempo");
        invalid = demo.toJson(); invalid.getDynamicObject()->setProperty("bars", 2.5);
        check(!phrase::Song::fromJson(invalid, parsed, error), "reject fractional bar count");
        invalid = demo.toJson(); invalid["tracks"][0]["notes"][0].getDynamicObject()->setProperty("length", 1000);
        check(!phrase::Song::fromJson(invalid, parsed, error), "reject notes beyond end");
        invalid = demo.toJson(); invalid["tracks"][0]["notes"][0].getDynamicObject()->setProperty("pitch", std::numeric_limits<double>::quiet_NaN());
        check(!phrase::Song::fromJson(invalid, parsed, error), "reject NaN");
        invalid = demo.toJson(); invalid["tracks"][1].getDynamicObject()->setProperty("part", "melody");
        check(!phrase::Song::fromJson(invalid, parsed, error), "reject duplicate track");
        invalid = demo.toJson(); auto* notes = invalid["tracks"][0]["notes"].getArray();
        while (notes->size() < 513) notes->add((*notes)[0]);
        check(!phrase::Song::fromJson(invalid, parsed, error), "reject excessive notes");
        auto file = juce::File::getSpecialLocation(juce::File::tempDirectory).getNonexistentChildFile("phrase-test", ".mid");
        check(phrase::writeMidi(demo, -1, file, error), "export multitrack MIDI");
        juce::MidiFile midi; auto stream = file.createInputStream();
        check(midi.readFrom(*stream), "exported MIDI readable");
        check(midi.getNumTracks() == 5 && midi.getTimeFormat() == 960, "four parts plus tempo at 960 PPQ");
        check(midi.getTrack(0)->getEventPointer(0)->message.isTempoMetaEvent(), "tempo present");
        check(std::abs(midi.getTrack(0)->getEventPointer(0)->message.getTempoSecondsPerQuarterNote() - 60.0/92) < 0.000002, "tempo preserved");
        check(midi.getTrack(0)->getEndTime() == 15360, "four-bar end preserved");
        check(midi.getTrack(4)->getEventPointer(1)->message.getChannel() == 10, "GM drums on channel 10");
        stream.reset(); check(phrase::writeMidi(demo, 0, file, error), "export single part");
        phrase::Song imported; check(phrase::readMidi(file, 0, imported, error), "import MIDI");
        check(imported.parts[0].size() == 16, "import preserves note count");
        check(imported.parts[0][1].start == 1.25 && imported.parts[0][1].length == 0.5, "import preserves timing");
        check(file.deleteFile(), "test MIDI removed");
        auto req = phrase::makeRequest(demo, "Change only the melody", "gpt-4.1-mini");
        check(req["store"].isBool() && !(bool)req["store"], "API storage disabled");
        check((bool)req["text"]["format"]["strict"] && req["text"]["format"]["schema"].isObject(), "strict output schema");
        check(req["input"].toString().contains("Change only the melody"), "user request included");
        juce::Array<juce::var> content { phrase::object({{"type", "output_text"}, {"text", juce::JSON::toString(demo.toJson())}}) };
        juce::Array<juce::var> output { phrase::object({{"content", content}}) };
        auto response = phrase::object({{"status", "completed"}, {"output", output}});
        check(phrase::parseResponse(response, parsed, error), "parse response envelope");
        response.getDynamicObject()->setProperty("status", "incomplete");
        check(!phrase::parseResponse(response, parsed, error), "reject truncated response");
        content.clear(); content.add(phrase::object({{"type", "refusal"}, {"refusal", "Cannot generate this."}}));
        output.clear(); output.add(phrase::object({{"content", content}}));
        check(!phrase::parseResponse(phrase::object({{"status", "completed"}, {"output", output}}), parsed, error), "handle refusal");
        std::cout << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}
