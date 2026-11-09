#!/bin/zsh
set -eu
cd "${0:A:h:h}"
source_plugin="$PWD/build/Phrase_artefacts/Release/VST3/Phrase.vst3"
target_root="$HOME/Library/Audio/Plug-Ins/VST3"
if [[ ! -d "$source_plugin" ]]; then
    print -u2 'Build Phrase first: ./scripts/build.sh'
    exit 1
fi
mkdir -p "$target_root"
if [[ -e "$target_root/Phrase.vst3" ]]; then
    backup_path="$target_root/Phrase.backup.$(date +%Y%m%d-%H%M%S).vst3"
    mv "$target_root/Phrase.vst3" "$backup_path"
fi
ditto "$source_plugin" "$target_root/Phrase.vst3"
codesign --force --deep --sign - "$target_root/Phrase.vst3"
print "Installed: $target_root/Phrase.vst3"
