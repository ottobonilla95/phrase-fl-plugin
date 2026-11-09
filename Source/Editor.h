#pragma once
#include "Processor.h"
#include "AI.h"
#include <juce_gui_extra/juce_gui_extra.h>

class PhraseEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit PhraseEditor(PhraseProcessor&);
    ~PhraseEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    class Roll : public juce::Component {
    public:
        explicit Roll(PhraseProcessor& p) : processor(p) {}
        int part = 0;
        void paint(juce::Graphics&) override;
    private: PhraseProcessor& processor;
    };
    class DragButton : public juce::TextButton {
    public:
        DragButton() : TextButton("Drag MIDI into FL Studio") {}
        std::function<void()> onDrag;
        void mouseDrag(const juce::MouseEvent& e) override { if (!dragged && e.getDistanceFromDragStart() > 6) { dragged = true; if (onDrag) onDrag(); } }
        void mouseDown(const juce::MouseEvent& e) override { dragged = false; TextButton::mouseDown(e); }
    private: bool dragged = false;
    };
    PhraseProcessor& processor;
    juce::LookAndFeel_V4 look;
    Roll roll;
    juce::TextEditor chat, prompt, key, model;
    juce::Label status, connectionLabel, modelLabel;
    juce::TextButton send {"Create idea"}, preview {"Preview"}, undo {"Undo"}, example {"Load example"};
    juce::TextButton settings {"Connection"}, exportAll {"Export all"}, import {"Import MIDI"};
    juce::TextButton closeSettings {"Done"};
    std::array<juce::TextButton, 4> tabs;
    DragButton drag;
    std::unique_ptr<phrase::AIWorker> worker;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::File exportDir;
    bool showingSettings = false, busy = false;
    void timerCallback() override;
    void generate();
    void updateState();
    void append(const juce::String& who, const juce::String& text);
    void showConnection(bool);
    void exportMidi(bool all, bool externalDrag);
    void importMidi();
    void setBusy(bool);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhraseEditor)
};
