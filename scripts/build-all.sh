cd ../
BUILD_DIR=`pwd`/"build"
if [ -d $BUILD_DIR ]; then
    rm -r $BUILD_DIR
fi
mkdir $BUILD_DIR

INC_DIR=`pwd`/"build"/"inc"
mkdir -p $INC_DIR
cp -r src/*.h $INC_DIR
cd scripts

sh ./build-ios.sh
sh ./build-mac.sh
sh ./build-android.sh
