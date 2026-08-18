#!/bin/sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")" && pwd)
component="${root}/RapReady One.component"
vst3="${root}/RapReady One.vst3"
au_dest="${HOME}/Library/Audio/Plug-Ins/Components/RapReady One.component"
vst3_dest="${HOME}/Library/Audio/Plug-Ins/VST3/RapReady One.vst3"

if [ ! -d "${component}" ]; then
  echo "Could not find RapReady One.component next to this script." >&2
  echo "Run this from the extracted RapReadyOne-macOS-universal folder." >&2
  exit 1
fi

mkdir -p "${HOME}/Library/Audio/Plug-Ins/Components"
rm -rf "${au_dest}"
ditto "${component}" "${au_dest}"
xattr -dr com.apple.quarantine "${au_dest}" 2>/dev/null || true
codesign --force --deep --sign - "${au_dest}" 2>/dev/null || true
echo "Installed Audio Unit: ${au_dest}"

if [ -d "${vst3}" ]; then
  mkdir -p "${HOME}/Library/Audio/Plug-Ins/VST3"
  rm -rf "${vst3_dest}"
  ditto "${vst3}" "${vst3_dest}"
  xattr -dr com.apple.quarantine "${vst3_dest}" 2>/dev/null || true
  codesign --force --deep --sign - "${vst3_dest}" 2>/dev/null || true
  echo "Installed VST3: ${vst3_dest}"
fi

killall -9 AudioComponentRegistrar 2>/dev/null || true
echo
echo "Quit Logic Pro completely, reopen it, then Reset & Rescan"
echo "Bedroom Labs / RapReady One in Plug-in Manager."
