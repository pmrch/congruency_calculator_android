#!/bin/bash
set -e -x

mkdir -p jni/src jni/include

if [ ! -f "build/hasCopied" ]; then
    cp $ANDROID_NDK_HOME/sources/android/native_app_glue/android_native_app_glue.c jni/src/
    cp $ANDROID_NDK_HOME/sources/android/native_app_glue/android_native_app_glue.h jni/include/
    touch build/hasCopied    
fi

# define some convenient ways to refer to our build tools
#aapt="$ANDROID_HOME/build-tools/33.0.2/aapt"
#zipalign="$ANDROID_HOME/build-tools/33.0.2/zipalign"

# Define the android API you're targeting here
android_platform="android-33"

# Define the architecture you're targeting. Options are armeabi-v7a, arm64-v8a, x86, x86_64
target_libcpp="arm64-v8a" # for Snapdragon 845, OnePlus 6T

# set up build directories
rm -rf build/*.apk build/apk
mkdir -p "build/apk/lib"

# build shared lib with ndk-build
# for more info on ndk-build, see https://developer.android.com/ndk/guides/ndk-build
#$ANDROID_NDK_HOME/ndk-build NDK_DEBUG=1
bear -- $ANDROID_NDK_HOME/ndk-build NDK_DEBUG=1

# copy .so to build/apk/lib/$target_libcpp (this directory hierarchy is necessary so don't change it!)
cp -a libs/$target_libcpp build/apk/lib

# create the apk
aapt package --debug-mode -f -M "AndroidManifest.xml" -S "res" \
    -I "$ANDROID_HOME/platforms/$android_platform/android.jar" \
    -F "build/apk/linearconcalc.unsigned.apk" "build/apk"

# create the keystore for signing the APK - keytool and jarsigner are utilities provided by the JDK
# NB: This only needs to happen once so in reality I comment this command out after the first run
if [ ! -f "build/debug.keystore" ]; then
    keytool -genkeypair -keystore "build/debug.keystore" -storepass android -keypass android -alias androiddebugkey -dname "cn=pdev2k" -keyalg RSA -keysize 2048 -validity 20000
fi

# sign the APK
jarsigner -sigalg SHA1withRSA -digestalg SHA1 -storepass android -keypass android -keystore "build/debug.keystore" \
    -signedjar "build/apk/linearconcalc.signed.apk" "build/apk/linearconcalc.unsigned.apk" androiddebugkey

# compress the APK - an optional step, but a standard one
zipalign 4 "build/apk/linearconcalc.signed.apk" "build/linearconcalc.apk"

apksigner sign --ks "build/debug.keystore" \
    --ks-pass pass:android \
    --ks-key-alias androiddebugkey \
    "build/linearconcalc.apk"

# clean apk files
rm -rf build/apk