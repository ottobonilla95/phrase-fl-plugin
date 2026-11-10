# FL Studio music assistant

Development in progress. There is no installable or usable replacement plugin yet.
The control bridge is experimental and has not passed the live FL Studio checkpoint.

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

## Current test availability

There is currently no new plugin to open in FL Studio. Do not use the experimental
bridge scripts on a music project: direct note editing, result verification, undo
and synth control still need live validation. The AI chat is not implemented.

Once a build passes that checkpoint, its test instructions must include the exact
installed plugin name, how to open it in FL, AI connection setup, and these checks:

1. Ask: "Create a mellow piano melody in C major." Verify a piano channel and editable notes appear in FL.
2. Play the pattern through FL.
3. Ask: "Change the final two notes." Verify only that piano part changes.
4. Undo the edit and verify the original notes return.
5. Ask: "Add a warm synth with matching chords." Verify a separate synth channel, chord notes and audible playback.

These are release acceptance checks, not features available in the current source.
