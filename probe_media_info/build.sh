cd ./ffmpeg_module/
mkdir build
cd build/
cmake ../src/
make
make install
cd ../../
rm -rf ffmpeg_module/build
go build probe_media_info.go