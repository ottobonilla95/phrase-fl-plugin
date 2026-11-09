#include "Editor.h"

namespace {
const juce::Colour bg(0xff202c3b), panel(0xff29384b), ink(0xffeef2fa), muted(0xffaebbd0), line(0xff3a4b63);
const std::array<juce::Colour, 4> colours {juce::Colour(0xffffbd9f), juce::Colour(0xffbfb4ef), juce::Colour(0xff9ac9e1), juce::Colour(0xffe6cc83)};
void label(juce::Graphics& g, const juce::String& t, juce::Rectangle<int> r, float size, juce::Colour c = ink) {
    g.setColour(c); g.setFont(juce::Font(juce::FontOptions(size))); g.drawText(t, r, juce::Justification::centredLeft);
}
}
PhraseEditor::PhraseEditor(PhraseProcessor& p) : AudioProcessorEditor(p), processor(p), roll(p) {
    look.setColour(juce::TextButton::buttonColourId, panel);
    look.setColour(juce::TextButton::buttonOnColourId, line.brighter(0.4f));
    look.setColour(juce::TextButton::textColourOffId, ink);
    look.setColour(juce::TextButton::textColourOnId, ink);
    look.setColour(juce::TextEditor::backgroundColourId, panel);
    look.setColour(juce::TextEditor::textColourId, ink);
    look.setColour(juce::TextEditor::outlineColourId, line);
    look.setColour(juce::TextEditor::focusedOutlineColourId, colours[0]);
    look.setColour(juce::TextEditor::highlightColourId, line.brighter(0.3f));
    look.setColour(juce::CaretComponent::caretColourId, ink);
    setLookAndFeel(&look);
    for (auto* c : std::initializer_list<juce::Component*>{ &chat, &prompt, &key, &model, &status, &connectionLabel, &modelLabel,
         &send, &preview, &undo, &example, &settings, &exportAll, &import, &closeSettings, &drag, &roll }) addAndMakeVisible(c);
    for (int i = 0; i < 4; ++i) {
        auto& tab = tabs[(size_t)i]; addAndMakeVisible(tab); tab.setButtonText(phrase::partNames[(size_t)i]);
        tab.setClickingTogglesState(true); tab.setRadioGroupId(1);
        tab.onClick = [this, i] { roll.part = i; updateState(); };
    }
    tabs[0].setToggleState(true, juce::dontSendNotification);
    chat.setMultiLine(true); chat.setReadOnly(true); chat.setScrollbarsShown(true);
    chat.setFont(juce::Font(juce::FontOptions(15.0f))); chat.setIndents(16, 14);
    chat.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    prompt.setMultiLine(true); prompt.setReturnKeyStartsNewLine(false); prompt.setFont(juce::Font(juce::FontOptions(15.0f)));
    prompt.setTextToShowWhenEmpty("Describe a sound, a groove, or a change...", muted); prompt.setIndents(12, 12);
    prompt.onReturnKey = [this] { generate(); };
    prompt.setInputRestrictions(4000);
    key.setPasswordCharacter(0x2022); key.setText(processor.apiKey, false);
    key.setTextToShowWhenEmpty("Paste your OpenAI API key", muted); key.setInputRestrictions(512);
    key.onTextChange = [this] { processor.apiKey = key.getText().trim(); };
    model.setText(processor.model, false); model.setInputRestrictions(100);
    model.onTextChange = [this] { processor.model = model.getText().trim(); };
    connectionLabel.setText("OpenAI API key / kept in memory for this session", juce::dontSendNotification);
    modelLabel.setText("Model", juce::dontSendNotification);
    for (auto* l : {&status, &connectionLabel, &modelLabel}) l->setColour(juce::Label::textColourId, muted);
    send.setColour(juce::TextButton::buttonColourId, colours[0]); send.setColour(juce::TextButton::textColourOffId, bg);
    send.onClick = [this] { generate(); };
    preview.onClick = [this] { processor.togglePreview(); updateState(); };
    undo.onClick = [this] { if (processor.undo()) { append("Phrase", "Restored the previous idea."); updateState(); } };
    example.onClick = [this] { auto s = phrase::Song::demo(); processor.setSong(s); append("Local example", s.message); updateState(); };
    settings.onClick = [this] { showConnection(!showingSettings); };
    closeSettings.onClick = [this] { showConnection(false); };
    exportAll.onClick = [this] { exportMidi(true, false); };
    drag.onDrag = [this] { exportMidi(false, true); };
    drag.onClick = [this] { exportMidi(false, false); };
    import.onClick = [this] { importMidi(); };
    exportDir = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("Phrase-" + juce::Uuid().toString()); exportDir.createDirectory();
    if (processor.conversation.isEmpty()) append("Phrase", "What are we making?\nAsk for a melody, chords, bass or drums. Then refine the idea in conversation.\n\nTry: \"Make a 4-bar mellow house idea in A minor at 120 BPM.\"\n\nConnect your API key to begin, or load the local example to try preview and MIDI transfer.");
    else chat.setText(processor.conversation, false);
    showConnection(false); setSize(1080, 740); setResizable(true, false); setResizeLimits(980, 680, 1600, 1100);
    updateState(); startTimerHz(20);
}
PhraseEditor::~PhraseEditor() { stopTimer(); worker.reset(); processor.stopPreview(); setLookAndFeel(nullptr); }
void PhraseEditor::paint(juce::Graphics& g) {
    g.fillAll(bg);
    label(g, "Phrase", {24, 16, 230, 44}, 34);
    label(g, "Your ideas, in notes.", {25, 60, 350, 24}, 14, muted);
    g.setColour(line); g.drawHorizontalLine(102, 24.0f, (float)getWidth() - 24);
    const int split = (int)(getWidth() * 0.43);
    label(g, "Conversation", {24, 115, split - 48, 30}, 19);
    auto s = processor.getSong();
    if (!showingSettings) {
        label(g, s.hasNotes() ? s.title : "An idea starts here", {split + 24, 115, getWidth() - split - 48, 30}, 22);
        label(g, s.hasNotes() ? s.key + "   /   " + juce::String(s.bpm, 0) + " BPM   /   " + juce::String(s.bars) + " bars" : "Create an idea or import MIDI to begin", {split + 24, 150, getWidth() - split - 48, 24}, 14, muted);
    }
    label(g, "Transfer a part to the Piano Roll of your chosen instrument.", {split + 24, getHeight() - 100, getWidth() - split - 48, 22}, 13, muted);
    label(g, "Preview uses simple sounds. Your FL instruments give it character.", {split + 24, getHeight() - 77, getWidth() - split - 48, 22}, 12, muted);
    if (showingSettings) {
        g.setColour(panel); g.fillRoundedRectangle((float)getWidth() - 510, 94, 486, 248, 10);
        g.setColour(line); g.drawRoundedRectangle((float)getWidth() - 510, 94, 486, 248, 10, 1);
        label(g, "Connection settings", {getWidth() - 490, 104, 400, 28}, 20);
        label(g, "Prompts and notes are sent to OpenAI. API usage is billed separately.", {getWidth() - 490, 286, 445, 26}, 12, muted);
    }
}
void PhraseEditor::resized() {
    const int w = getWidth(), h = getHeight(), split = (int)(w * 0.43), right = w - split - 48;
    settings.setBounds(w - 145, 31, 121, 34);
    chat.setBounds(24, 154, split - 48, h - 350);
    prompt.setBounds(24, h - 182, split - 48, 90);
    send.setBounds(24, h - 78, split - 190, 38); example.setBounds(split - 154, h - 78, 130, 38);
    status.setBounds(24, h - 35, split - 48, 24);
    const int tabWidth = right / 4;
    for (int i = 0; i < 4; ++i) tabs[(size_t)i].setBounds(split + 24 + i * tabWidth, 190, tabWidth - 5, 34);
    roll.setBounds(split + 24, 238, right, h - 438);
    preview.setBounds(split + 24, h - 180, 100, 36); undo.setBounds(split + 132, h - 180, 72, 36);
    import.setBounds(split + 212, h - 180, 106, 36); exportAll.setBounds(w - 130, h - 180, 106, 36);
    drag.setBounds(split + 24, h - 138, right, 32);
    key.setBounds(w - 490, 171, 444, 34); connectionLabel.setBounds(w - 490, 139, 444, 28);
    modelLabel.setBounds(w - 490, 212, 85, 32); model.setBounds(w - 405, 212, 240, 32); closeSettings.setBounds(w - 153, 212, 107, 32);
}
void PhraseEditor::Roll::paint(juce::Graphics& g) {
    g.fillAll(panel); const auto s = processor.getSong(); const auto& notes = s.parts[(size_t)part];
    if (notes.empty()) { label(g, "No " + phrase::partNames[(size_t)part].toLowerCase() + " notes yet", getLocalBounds().reduced(25), 17, muted); return; }
    int low = 127, high = 0; for (const auto& n : notes) { low = juce::jmin(low, n.pitch); high = juce::jmax(high, n.pitch); }
    low -= 2; high += 2; if (high - low < 12) { low -= 4; high += 4; }
    const float row = (getHeight() - 28.0f) / (high - low + 1);
    const float left = 38, width = getWidth() - left - 8.0f, beatWidth = width / (s.bars * 4);
    for (int pitch = low; pitch <= high; ++pitch) {
        const float y = 24 + (high - pitch) * row;
        const int pc = (pitch % 12 + 12) % 12;
        if (pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10) { g.setColour(bg.withAlpha(0.35f)); g.fillRect(left, y, width, row); }
        if (pc == 0) label(g, juce::MidiMessage::getMidiNoteName(juce::jlimit(0, 127, pitch), true, true, 3), {3, (int)y, 34, (int)row + 1}, 10, muted);
    }
    for (int b = 0; b <= s.bars * 4; ++b) {
        g.setColour(b % 4 == 0 ? line.brighter(0.2f) : line.withAlpha(0.45f));
        g.drawVerticalLine((int)(left + b * beatWidth), 23, (float)getHeight());
        if (b % 4 == 0 && b < s.bars * 4) label(g, juce::String(b / 4 + 1), {(int)(left + b * beatWidth + 4), 2, 32, 20}, 11, muted);
    }
    for (const auto& n : notes) {
        g.setColour(colours[(size_t)part].withAlpha(0.5f + n.velocity / 254.0f));
        g.fillRoundedRectangle(left + (float)n.start * beatWidth, 24 + (high - n.pitch) * row + 1,
            juce::jmax(3.0f, (float)n.length * beatWidth - 1), juce::jmax(2.0f, row - 2), 2);
    }
    if (processor.isPreviewing()) { g.setColour(ink); g.drawVerticalLine((int)(left + processor.getPlayhead() * beatWidth), 22, (float)getHeight()); }
}
void PhraseEditor::append(const juce::String& who, const juce::String& text) {
    chat.moveCaretToEnd(); chat.insertTextAtCaret((chat.getText().isEmpty() ? "" : "\n\n") + who + "\n" + text);
    processor.conversation = chat.getText().getLastCharacters(16000);
}
void PhraseEditor::showConnection(bool show) {
    showingSettings = show;
    for (auto* c : std::initializer_list<juce::Component*>{&key, &model, &connectionLabel, &modelLabel, &closeSettings}) { c->setVisible(show); if (show) c->toFront(false); }
    roll.setVisible(!show); for (auto& tab : tabs) tab.setVisible(!show);
    repaint();
}
void PhraseEditor::setBusy(bool value) {
    busy = value; send.setEnabled(!busy); example.setEnabled(!busy); undo.setEnabled(!busy && processor.canUndo()); import.setEnabled(!busy);
    send.setButtonText(busy ? "Composing..." : "Create / revise"); prompt.setReadOnly(busy);
}
void PhraseEditor::generate() {
    if (busy) return;
    const auto text = prompt.getText().trim(); if (text.isEmpty()) { status.setText("Describe what you want to make first.", juce::dontSendNotification); return; }
    if (processor.apiKey.isEmpty() || processor.model.isEmpty()) { showConnection(true); status.setText("Add an API key and model to connect.", juce::dontSendNotification); key.grabKeyboardFocus(); return; }
    processor.stopPreview(); append("You", text); prompt.clear(); setBusy(true); status.setText("Composing your idea...", juce::dontSendNotification);
    auto safe = juce::Component::SafePointer<PhraseEditor>(this);
    worker = std::make_unique<phrase::AIWorker>(processor.apiKey, phrase::makeRequest(processor.getSong(), text, processor.model),
        [safe](bool ok, phrase::Song s, juce::String error) {
            if (!safe) return;
            safe->setBusy(false);
            if (ok) { safe->processor.setSong(s); safe->append("Phrase", s.message); safe->status.setText("Idea ready. Preview or drag a part into FL Studio.", juce::dontSendNotification); }
            else { safe->append("Connection", error); safe->status.setText("Request failed. Your previous idea is intact.", juce::dontSendNotification); }
            safe->updateState();
        });
}
void PhraseEditor::updateState() {
    const auto s = processor.getSong(); preview.setEnabled(s.hasNotes()); undo.setEnabled(!busy && processor.canUndo());
    preview.setButtonText(processor.isPreviewing() ? "Stop" : "Preview"); exportAll.setEnabled(s.hasNotes());
    drag.setEnabled(!s.parts[(size_t)roll.part].empty()); drag.setButtonText("Drag " + phrase::partNames[(size_t)roll.part].toLowerCase() + " MIDI into FL Studio");
    roll.repaint(); repaint();
}
void PhraseEditor::timerCallback() { preview.setButtonText(processor.isPreviewing() ? "Stop" : "Preview"); roll.repaint(); }
void PhraseEditor::exportMidi(bool all, bool externalDrag) {
    auto s = processor.getSong(); const int p = all ? -1 : roll.part;
    auto name = juce::File::createLegalFileName(s.title + " - " + (all ? "All parts" : phrase::partNames[(size_t)p])) + ".mid";
    if (externalDrag) {
        auto file = exportDir.getNonexistentChildFile(name, "", false); juce::String error;
        if (!phrase::writeMidi(s, p, file, error)) { status.setText(error, juce::dontSendNotification); return; }
        juce::DragAndDropContainer::performExternalDragDropOfFiles({file.getFullPathName()}, false, this);
    } else {
        chooser = std::make_unique<juce::FileChooser>("Export MIDI", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(name), "*.mid");
        auto safe = juce::Component::SafePointer<PhraseEditor>(this);
        chooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::warnAboutOverwriting,
            [safe, s, p](const juce::FileChooser& c) { if (!safe || c.getResult() == juce::File()) return; juce::String e;
                const bool ok = phrase::writeMidi(s, p, c.getResult().withFileExtension("mid"), e); safe->status.setText(ok ? "MIDI exported." : e, juce::dontSendNotification); });
    }
}
void PhraseEditor::importMidi() {
    chooser = std::make_unique<juce::FileChooser>("Import MIDI into " + phrase::partNames[(size_t)roll.part], juce::File(), "*.mid;*.midi");
    auto safe = juce::Component::SafePointer<PhraseEditor>(this); const int p = roll.part;
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [safe, p](const juce::FileChooser& c) { if (!safe || c.getResult() == juce::File()) return;
            auto s = safe->processor.getSong(); juce::String error;
            if (phrase::readMidi(c.getResult(), p, s, error)) { safe->processor.setSong(s); safe->append("Phrase", "Imported MIDI into " + phrase::partNames[(size_t)p].toLowerCase() + ". Tell me what to change."); safe->updateState(); }
            else safe->status.setText(error, juce::dontSendNotification);
        });
}
