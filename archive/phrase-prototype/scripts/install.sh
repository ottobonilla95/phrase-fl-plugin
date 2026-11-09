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
    backup_root="$HOME/Library/Application Support/Phrase/Backups"
    mkdir -p "$backup_root"
    backup_folder="$(mktemp -d "$backup_root/install-XXXXXXXX")"
    mv "$target_root/Phrase.vst3" "$backup_folder/Phrase.vst3"
fi
ditto "$source_plugin" "$target_root/Phrase.vst3"
codesign --force --deep --sign - "$target_root/Phrase.vst3"
print "Installed: $target_root/Phrase.vst3"
