# Phrase

An Apple Silicon VST3 instrument for composing MIDI through chat in FL Studio. Early local prototype, named Phrase for now.

## Try it

1. Build with `./scripts/build.sh` (requires Apple's Command Line Tools, `uv`, and internet on the first build).
2. Install with `./scripts/install.sh`.
3. In FL Studio, open **Options → Manage plugins**, scan for plugins and add **Phrase** as an instrument.
4. Open **Connection**, paste your OpenAI API key and select a model supporting Responses structured outputs. Default: `gpt-4.1-mini`.
5. Ask for a musical idea, then use **Preview**. Select Melody, Chords, Bass or Drums and drag its MIDI into the Piano Roll of your desired FL instrument. Clicking the drag button opens a MIDI save dialog instead.

You can also open `build/Phrase_artefacts/Release/Standalone/Phrase.app` without FL Studio. Use **Load example** to try the UI, audio preview and MIDI export without an API key. The example is fixed demo music, not AI generation.

## What works in this prototype

- Chat generates or revises a complete idea with four separate parts.
- Context includes the current idea's notes, key, tempo and length. Edits are instructed to preserve parts you did not ask to change.
- Note validation: 1–16 bars, 4/4, 30–300 BPM, up to 512 notes per part.
- Basic internal sounds for audio preview, plus MIDI output during preview. Preview runs independently of FL's transport and loops at the idea's own tempo.
- Single-part MIDI drag/export and multitrack export with tempo, bar length and General MIDI drum channel 10.
- MIDI import into the selected part for revising your existing music (all note tracks in the chosen file are merged into that part; tempo stays at the current idea's setting).
- Undo of the last 20 musical changes during the current plugin session.
- Current idea, chat display and model selection are saved with plugin state; undo history is not.

## Boundaries

This version does not inspect the open FL project, directly insert notes into other channels, arrange the Playlist, choose instruments, hear existing audio, or control mixer effects. You transfer generated MIDI to FL yourself. Imported MIDI is limited to PPQ timing and 16 bars. Preview sounds are intended for checking notes and rhythm, not finished production. Use your own FL instruments and drum mapping for final sounds.

The API key is masked and held only in plugin memory. It is never embedded in the binary, written to a settings file, or serialized into FL projects. Re-enter it after reopening the plugin instance. Prompts and current MIDI note data are sent directly over HTTPS to OpenAI; API calls set `store: false`. Your OpenAI API account's usage charges and data policies apply. The model field is editable. No API key is needed to build or run offline tests.

Requests run on a background thread. Closing the editor cancels an in-flight request. Generation failures preserve the previous musical idea.

## Build and checks

JUCE 8.0.12 is pinned and downloaded by CMake into `build/`. Build tooling is isolated in `.tools/`. The C++ tests cover musical validation, JSON round trips, MIDI timing/tempo/channel export, import and API envelope handling. Tests use synthetic responses and do not call the online API.

Source: `Source/`. Tests: `Tests/`. Compiled artifacts: `build/Phrase_artefacts/Release/`.

Verified on this Mac on 2026-11-09: Apple Silicon Release builds of VST3 and Standalone; both CTest suites passed (27 music/API checks plus audio/state checks). FL Studio 2026 v26.1.5 scanned Phrase with status `ok`, loaded its editor, and activated the local example and preview control. The plugin is installed at `/Users/otto/Library/Audio/Plug-Ins/VST3/Phrase.vst3`. Live online generation requires your API key and has not been tested. Cross-window MIDI drag into an FL instrument has not yet been checked manually; MIDI file export and reimport are covered by the offline tests.

## References and dependencies

- [OpenAI structured outputs](https://developers.openai.com/api/docs/guides/structured-outputs?api-mode=responses)
- [OpenAI GPT-4.1 mini](https://developers.openai.com/api/docs/models/gpt-4.1-mini)
- [JUCE](https://github.com/juce-framework/JUCE/tree/8.0.12) — dependency license is included in the fetched source at `build/_deps/juce-src/LICENSE.md`. Review JUCE's distribution terms before publishing this plugin.
- [FL Studio Piano Roll scripting](https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/pianoroll_scripting_api.htm) — potential next integration stage.
