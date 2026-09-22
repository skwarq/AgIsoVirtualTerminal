#!/usr/bin/env bash
set -euo pipefail

image_name="agisovirtualterminal-android-build"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
project_dir="$(cd -- "$script_dir/.." && pwd)"

docker build -f "$script_dir/Dockerfile" -t "$image_name" "$project_dir"
docker run --rm \
  -v "$project_dir:/workspace" \
  "$image_name" \
  bash -lc 'mkdir -p /root/.android && if [[ ! -f /root/.android/debug.keystore ]]; then keytool -genkeypair -alias androiddebugkey -keyalg RSA -keysize 2048 -validity 10000 -storepass android -keypass android -keystore /root/.android/debug.keystore -dname "CN=Android Debug,O=Android,C=US"; fi && gradle -p /workspace/android/AndroidStudio assembleDebug assembleRelease --no-daemon'

echo "Debug APKs: $project_dir/android/AndroidStudio/app/build/outputs/apk/debug_/debug/*.apk"
echo "Release APKs: $project_dir/android/AndroidStudio/app/build/outputs/apk/release_/release/*.apk"
