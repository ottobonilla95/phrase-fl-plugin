# FL Studio music assistant

Project restart. The previous Phrase MIDI prototype is retired and preserved in
[`archive/phrase-prototype`](archive/phrase-prototype/README.md) and Git tag
`prototype-v0.1`. The replacement is being defined; it is not implemented yet.

## Product

A conversational assistant inside FL Studio that helps the musician turn ideas
into a song by taking actions in the actual FL project.

The user can describe a musical result without knowing the instrument plugin,
preset, chord names or synthesis terminology. The assistant chooses among
supported installed instruments, creates editable musical parts, and helps the
user explore how they sound together.

Example conversation:

1. "Create a mellow piano melody in C major."
2. "Add a dreamy synth with chords that fit."
3. "Make the synth warmer and change the final chord."
4. "Keep the piano as it is and add a bassline."

## Required behavior

- Select and load a supported installed instrument plugin in an FL channel.
- Choose or adjust its sound through controls that have been verified for that plugin.
- Create notes in FL's real Piano Roll and play them through that instrument.
- Keep separate context for each instrument, including its notes and supported sound settings.
- Apply follow-up requests to the intended part while preserving unrelated parts.
- Offer audible alternatives for sounds, harmony and voicings in the song's context.
- Report actions only after FL confirms that they succeeded.
- Provide a way to undo project edits.

Online AI is the agreed starting point. Credentials must be kept out of the
repository and FL project files. Preview quality must be assessed using the
actual instrument audio.

## First technical checkpoint

Prove the control path on this Mac before building a replacement chat interface:

1. Choose and load one installed piano instrument through the intended integration.
2. Create a C-major melody as editable notes in that instrument's FL Piano Roll.
3. Play the result through FL.
4. Change the melody and reverse the edit.
5. Repeat the instrument-loading and sound-control checks with one installed synth.

After these checks, connect natural-language requests and verify the complete
request → project change → audible result → revision flow.

Support for other plugins, multiple simultaneous parts, arrangement and mixing
must expand from demonstrated control capabilities. The integration mechanism,
initial supported plugins and setup workflow remain to be validated. A VST3
window alone does not establish the required project-control capabilities.

## Archived prototype

The archived code provides a chat UI, generated MIDI, basic internal preview and
manual MIDI transfer. It does not select FL instruments or directly edit the
host project. Its original checks and documentation apply only to that prototype.
The project restart does not remove the already installed Phrase bundle.
