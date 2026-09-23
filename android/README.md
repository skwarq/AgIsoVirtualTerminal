# AgIsoVirtualTerminal Android

This is the Android version of AgIsoVirtualTerminal. It connects to a
[CanTcpGateway](https://github.com/skwarq/CanTcpGateway) running on a computer
or device that has access to the CAN adapter. The phone runs the VT user
interface; it does not connect directly to a CAN adapter.

## Requirements

- Android 7.0 (API 24) or newer.
- The phone and CanTcpGateway host must be reachable on the same local network.
- No Python installation is needed when using the prebuilt gateway artifact.
- A CAN adapter and supported CAN backend configured on the gateway host.

## Download and run (no local build needed)

Both programs are published as ready-to-run GitHub Actions artifacts. You do not need Android Studio, Gradle, or Python to use them.

### 1. Download the Android APK

Open the [Android Build workflow](https://github.com/skwarq/AgIsoVirtualTerminal/actions/workflows/build.yml), select the latest successful run on the main branch, and download the `AgIsoVirtualTerminal-Android-Release` artifact. Extract the downloaded archive; it contains one APK with the supported Android ABIs.

### 2. Download CanTcpGateway for the computer connected to CAN

Open the [CanTcpGateway Build workflow](https://github.com/skwarq/CanTcpGateway/actions/workflows/build.yml), select the latest successful run, and download the artifact matching the computer OS and CAN backend:

- Linux with SocketCAN: `CanTcpGateway-ubuntu-22.04` or `CanTcpGateway-ubuntu-24.04`.
- Windows with PCAN: `CanTcpGateway-windows-pcan`.
- Windows with CANable/SLCAN: `CanTcpGateway-windows-slcan`.

Extract the downloaded artifact. Set up the CAN interface and install any adapter driver required by the selected backend.

### 3. Start the gateway

On Linux, from the directory containing the downloaded binary (replace `can0` and the bitrate for your setup):

```bash
chmod +x CanTcpGateway
./CanTcpGateway --interface socketcan --can can0 --bitrate 250000 --port 29500
```

On Windows with PCAN, for example:

```powershell
.\CanTcpGateway.exe --interface pcan --can PCAN_USBBUS1 --bitrate 250000 --port 29500
```

See the [gateway README](https://github.com/skwarq/CanTcpGateway#readme) for other CAN backends and options. The gateway listens on TCP port `29500` and UDP discovery port `29501` by default. Allow both through the computer firewall.

### 4. Install and connect the phone

Copy the extracted APK to the phone and open it to install. Connect the phone to the same Wi-Fi/LAN as the gateway; guest Wi-Fi or access-point isolation can block device-to-device traffic.

In AgIsoVirtualTerminal, open **Configure CAN Hardware** and choose **Discovery** (default), or **Manual IP and TCP port** and enter the gateway computer IP plus TCP port `29500`. Tap **OK**, then **Start**. Discovery starts on Start (or AutoStart), not merely by opening the app. If network broadcast is blocked, use Manual mode. The mode and manual address are saved.

> **Safety:** CanTcpGateway has no authentication. Any connected client can send CAN frames. Use it only on a trusted test network and a controlled CAN setup.

> **Updating:** CI APKs are signed with a temporary debug key. Android may refuse to install an APK from a later workflow run over an earlier one. If that happens, uninstall the old app first, then install the new APK; uninstalling also removes the saved app settings.


## Optional: Build locally with Docker

For development, or if you need to build the APK yourself, use the repository Docker script instead of setting up Android Studio and the Android SDK manually. From the repository root, with Docker installed, run:

```bash
./android/build-android.sh
```

The script builds both Debug and Release APKs and writes them to:

- `android/AndroidStudio/app/build/outputs/apk/debug_/debug/`
- `android/AndroidStudio/app/build/outputs/apk/release_/release/`

The first run downloads the Android build tools into the Docker image. The generated APKs are signed with a temporary debug key; installing an APK from a later Docker run over an earlier one may require uninstalling the old app first, which also removes its saved settings.
