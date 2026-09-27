#!/bin/bash

source /home/patrik/.local/AndroidNdkEnv/scripts/activate_env.sh
set -euo pipefail

rm -rf libs/* obj/* build/*.apk 
bash ./build.sh

adb install build/linearconcalc.apk
adb logcat -c linearconcalc
adb logcat -s linearconcalc