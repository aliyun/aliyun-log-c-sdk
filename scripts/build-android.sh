
echo "*****************************************"

cp -r ../src/*.* ../projects/android/jni
cd ../projects/android
if [ -d obj ]; then
rm -r obj
fi
cd jni

ndk-build NDK_DEBUG=0 

rm -r *.c
rm -r *.h

cd ../../../
OUTPUT_DIR=`pwd`/"build"/"lib"/"android"
if [ -d $OUTPUT_DIR ]; then
    rm -r $OUTPUT_DIR
fi
mkdir -p $OUTPUT_DIR
cp -r ./projects/android/obj/local/arm64-v8a $OUTPUT_DIR/
cp -r ./projects/android/obj/local/armeabi-v7a $OUTPUT_DIR/
cp -r ./projects/android/obj/local/x86 $OUTPUT_DIR/
cp -r ./projects/android/obj/local/x86_64 $OUTPUT_DIR/
rm -rf $OUTPUT_DIR/arm64-v8a/objs
rm -rf $OUTPUT_DIR/armeabi-v7a/objs
rm -rf $OUTPUT_DIR/x86/objs
rm -rf $OUTPUT_DIR/x86_64/objs
rm -rf ./projects/android/obj

OUTPUT_DIR=$OUTPUT_DIR/"include"
mkdir -p $OUTPUT_DIR
cp -r ./src/*.h $OUTPUT_DIR/
