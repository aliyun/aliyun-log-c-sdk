
echo "*****************************************"
cp -r ../src ../projects/iOS
cd ../projects/iOS

TARGET="aliyun_log_c_sdk"
Manager="libaliyun_log_c_sdk"
BUILD_DIR=`pwd`/"build"
ARM_DIR=$BUILD_DIR/"arm"
if [ -d $BUILD_DIR ]; then
    rm -r $BUILD_DIR
fi
mkdir $BUILD_DIR
xcodebuild -project ./$TARGET.xcodeproj -scheme $TARGET VALID_ARCHS="armv7 arm64" -configuration Release -sdk iphoneos CONFIGURATION_BUILD_DIR=$ARM_DIR

cp $ARM_DIR/$Manager.a $BUILD_DIR/libaliyun_log_c_sdk.a

rm -r $ARM_DIR
rm -r src

cd ../../
OUTPUT_DIR=`pwd`/"build"/"lib"/"iOS"
if [ -d $OUTPUT_DIR ]; then
    rm -r $OUTPUT_DIR
fi
mkdir -p $OUTPUT_DIR
cp ./projects/iOS/build/*.a $OUTPUT_DIR/

