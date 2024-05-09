
echo "*****************************************"

cd ..
cp -r src projects/windows

cd projects/windows

if [ -d build ]; then
rm -r build
fi

cmake . -DCMAKE_BUILD_TYPE=Release
make clean
make

rm -rf src
rm -rf CMakeFiles
rm -rf CMakeCache.txt
rm -rf Makefile
rm -rf cmake_install.cmake

cd ../../
OUTPUT_DIR=`pwd`/"build"/"lib"/"windows"
if [ -d $OUTPUT_DIR ]; then
    rm -r $OUTPUT_DIR
fi
mkdir -p $OUTPUT_DIR
cp ./projects/windows/build/Release/lib/*.a $OUTPUT_DIR/

