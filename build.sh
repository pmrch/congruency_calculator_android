#!/bin/bash

set -euo pipefail

rm -rf libs/* obj/* build/*.apk
mkdir -p jni/src jni/include

cp $ANDROID_NDK_HOME/sources/android/native_app_glue/android_native_app_glue.c jni/src/
echo "Patching jni/src/android_native_app_glue.c..."
patch -p0 < patches/android_native_app_glue_c.patch

cp $ANDROID_NDK_HOME/sources/android/native_app_glue/android_native_app_glue.h jni/include/
echo "Patching jni/include/android_native_app_glue.h..."
patch -p0 < patches/android_native_app_glue_h.patch

echo "Done, applied patches!"

# Define the android API you're targeting here
android_platform="android-35"

# Define the architecture you're targeting. Options are armeabi-v7a, arm64-v8a, x86, x86_64
target_libcpp="arm64-v8a" # for Snapdragon 845, OnePlus 6T

# set up build directories
rm -rf build/*.apk build/apk build/res-compiled
mkdir -p "build/apk/lib" "build/res-compiled"

# build shared lib with ndk-build
echo "Building sources..."
sleep 1

bear -- $ANDROID_NDK_HOME/ndk-build -j$(nproc) NDK_DEBUG=1 2>&1 | tee build.log
echo "Successfully built source code!"
sleep 1

# copy .so to build/apk/lib/$target_libcpp (this directory hierarchy is necessary so don't change it!)
cp -a libs/$target_libcpp build/apk/lib
echo "Copied the shared libs"

# ---- aapt2 phase 1: compile resources ----
# Compiles each res/ file into a flat binary representation.
# Output goes to build/res-compiled/*.flat
aapt2 compile --dir res -o build/res-compiled/

# ---- aapt2 phase 2: link resources + assets + manifest ----
# Merges the compiled .flat files, packages the assets/ dir, and produces the APK.
aapt2 link -o "build/apk/linearconcalc.unsigned.apk" \
    -I "$ANDROID_HOME/platforms/$android_platform/android.jar" \
    --manifest "AndroidManifest.xml" \
    -A "assets" \
    --debug-mode \
    build/res-compiled/*.flat

( cd build/apk && zip -0 -r linearconcalc.unsigned.apk lib )

# create the apk
#aapt package --debug-mode -f -M "AndroidManifest.xml" -S "res" -A "assets" \
#    -I "$ANDROID_HOME/platforms/$android_platform/android.jar" \
#    -F "build/apk/linearconcalc.unsigned.apk" "build/apk"

# create the keystore for signing the APK - keytool and jarsigner are utilities provided by the JDK
# NB: This only needs to happen once so in reality I comment this command out after the first run
if [ ! -f "build/debug.keystore" ]; then
    keytool -genkeypair -keystore "build/debug.keystore" -storepass android -keypass android -alias androiddebugkey -dname "cn=pdev2k" -keyalg ED25519 -keysize 2048 -validity 20000
fi

# 1. Align the unsigned APK first
zipalign 4 "build/apk/linearconcalc.unsigned.apk" "build/linearconcalc.apk"

apksigner sign \
    --v4-signing-enabled true \
    --ks "build/debug.keystore" \
    --ks-pass pass:android \
    --ks-key-alias androiddebugkey \
    "build/linearconcalc.apk"

echo "=== final APK contents ==="
unzip -l build/linearconcalc.apk

# clean apk files
rm -rf build/apk build/res-compiled