# AgIsoVirtualTerminal Android

This is the separate Android Studio project for the TCP-backed virtual terminal.
The desktop CMake project remains unchanged in the repository root.

## Current scope

- Android Studio/Gradle project structure
- Android 26 minimum API
- Internet permission for `CanTcpGateway`
- Separate Android application id
- JUCE-based virtual-terminal activity with the TCP CAN transport

The JUCE application sources and `TcpCANPlugin` are connected through the
separate `AgIsoVirtualTerminal.jucer` project and JUCE's Android exporter/native
bridge. SocketCAN, PCAN and other
desktop CAN drivers are intentionally not part of this project.

The `.jucer` file is intentionally kept under `android/`; it does not modify
or replace the root desktop CMake project.

## Build

Open this directory in Android Studio and run the `app` configuration. From a
machine with the Android SDK and Gradle available, use:

```bash
./AndroidStudio/gradlew -p AndroidStudio assembleDebug
```

The Docker build is the reproducible build path and creates the debug keystore
automatically. The APK is written to
`AndroidStudio/app/build/outputs/apk/debug_/debug/app-debug_-debug.apk`.
