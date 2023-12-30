cd runtime_source
mkdir build 
mkdir run
cd build
cmake ..
make
if [ "$(uname)" == "Darwin" ]
then
    if [ ! -d "../../macos_runtime" ]
    then
        mkdir ../../macos_runtime
        DESTDIR=../../macos_runtime make install
        rm ../../macos_runtime/usr/local/lib/libruntime.dylib
        rm ../../macos_runtime/usr/local/lib/libruntime.4.13.1.dylib
    fi
else
    if [ ! -d "../../linux_runtime" ]
    then
        mkdir ../../linux_runtime
        DESTDIR=../../linux_runtime make install
        rm ../../linux_runtime/usr/local/lib/libruntime.so
        rm ../../linux_runtime/usr/local/lib/libruntime.so.4.13.1
    fi
fi
cd ../..
mkdir build 
cd build
cmake ..
make