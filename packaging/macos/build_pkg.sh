#!/usr/bin/env bash
# Cria "ChoirCrowd-<versão>-macOS.pkg" a partir do build Release.
# Uso:  packaging/macos/build_pkg.sh <pasta_artefacts> <versão>
#   ex: packaging/macos/build_pkg.sh build/ChoirCrowd_artefacts/Release 1.0.0
#
# Assinatura (opcional — sem isto o pkg funciona, mas o macOS mostra aviso):
#   MAC_APP_SIGN_ID       "Developer ID Application: Nome (TEAMID)"
#   MAC_INSTALLER_SIGN_ID "Developer ID Installer: Nome (TEAMID)"
#   APPLE_ID / APPLE_TEAM_ID / APPLE_APP_PASSWORD  -> notarização
set -euo pipefail

ART="${1:?pasta de artefactos}"
VERSION="${2:?versão}"
HERE="$(cd "$(dirname "$0")" && pwd)"
WORK="$(mktemp -d)"
OUT="${OUT_DIR:-$PWD}/ChoirCrowd-${VERSION}-macOS.pkg"
NAME="Choir Crowd"

sign_bundle() {
  local b="$1"
  if [[ -n "${MAC_APP_SIGN_ID:-}" ]]; then
    codesign --force --deep --timestamp --options runtime --sign "$MAC_APP_SIGN_ID" "$b"
  else
    codesign --force --deep --sign - "$b"   # ad-hoc: obrigatório em Apple Silicon
  fi
  codesign --verify --deep --strict "$b"
}

stage() {   # stage <id> <origem> <destino-de-instalação>
  local id="$1" src="$2" dest="$3"
  mkdir -p "$WORK/$id/root"
  cp -R "$src" "$WORK/$id/root/"
  sign_bundle "$WORK/$id/root/$(basename "$src")"
  pkgbuild --root "$WORK/$id/root" \
           --identifier "com.logogueto.choircrowd.$id" \
           --version "$VERSION" \
           --install-location "$dest" \
           --scripts "$HERE/scripts" \
           "$WORK/$id.pkg"
}

stage au         "$ART/AU/$NAME.component"   "/Library/Audio/Plug-Ins/Components"
stage vst3       "$ART/VST3/$NAME.vst3"      "/Library/Audio/Plug-Ins/VST3"
stage standalone "$ART/Standalone/$NAME.app" "/Applications"

sed "s/__VERSION__/$VERSION/g" "$HERE/distribution.xml" > "$WORK/distribution.xml"
mkdir -p "$WORK/resources"
cp "$HERE/welcome.html" "$WORK/resources/"

SIGN_ARGS=()
[[ -n "${MAC_INSTALLER_SIGN_ID:-}" ]] && SIGN_ARGS=(--sign "$MAC_INSTALLER_SIGN_ID")

productbuild --distribution "$WORK/distribution.xml" \
             --package-path "$WORK" \
             --resources "$WORK/resources" \
             ${SIGN_ARGS[@]+"${SIGN_ARGS[@]}"} \
             "$OUT"

if [[ -n "${MAC_INSTALLER_SIGN_ID:-}" && -n "${APPLE_ID:-}" ]]; then
  xcrun notarytool submit "$OUT" --apple-id "$APPLE_ID" --team-id "$APPLE_TEAM_ID" \
        --password "$APPLE_APP_PASSWORD" --wait
  xcrun stapler staple "$OUT"
fi

rm -rf "$WORK"
echo "Criado: $OUT"
