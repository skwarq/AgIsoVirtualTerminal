FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive \
    ANDROID_SDK_ROOT=/opt/android-sdk \
    ANDROID_HOME=/opt/android-sdk \
    GRADLE_VERSION=8.11.1 \
    PATH=/opt/gradle/bin:/opt/android-sdk/cmdline-tools/latest/bin:/opt/android-sdk/platform-tools:/opt/android-sdk/emulator:$PATH

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates curl unzip openjdk-17-jdk && \
    rm -rf /var/lib/apt/lists/* && \
    mkdir -p "$ANDROID_SDK_ROOT/cmdline-tools" /opt/gradle && \
    curl -fsSL -o /tmp/android-tools.zip \
      https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip && \
    unzip -q /tmp/android-tools.zip -d "$ANDROID_SDK_ROOT/cmdline-tools" && \
    mv "$ANDROID_SDK_ROOT/cmdline-tools/cmdline-tools" "$ANDROID_SDK_ROOT/cmdline-tools/latest" && \
    rm /tmp/android-tools.zip && \
    yes | sdkmanager --licenses >/dev/null && \
    sdkmanager "platform-tools" "platforms;android-35" "build-tools;35.0.0" "ndk;28.1.13356709" "cmake;3.22.1" && \
    curl -fsSL -o /tmp/gradle.zip \
      "https://services.gradle.org/distributions/gradle-${GRADLE_VERSION}-bin.zip" && \
    unzip -q /tmp/gradle.zip -d /opt/gradle && \
    ln -s "/opt/gradle/gradle-${GRADLE_VERSION}" /opt/gradle/current && \
    ln -s /opt/gradle/current/bin /opt/gradle/bin && \
    rm /tmp/gradle.zip

WORKDIR /workspace/android
CMD ["gradle", "assembleDebug", "--no-daemon"]
