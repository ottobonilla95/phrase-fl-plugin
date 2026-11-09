#pragma once
#include "Music.h"
#include <juce_events/juce_events.h>

namespace phrase {
juce::var makeRequest(const Song&, const juce::String& prompt, const juce::String& model);
bool parseResponse(const juce::var&, Song&, juce::String& error);
class AIWorker : public juce::Thread {
public:
    using Callback = std::function<void(bool, Song, juce::String)>;
    AIWorker(juce::String key, juce::var request, Callback);
    ~AIWorker() override;
    void run() override;
private:
    juce::String apiKey;
    juce::var body;
    Callback callback;
    juce::CriticalSection streamLock;
    std::unique_ptr<juce::WebInputStream> stream;
};
}
