rm -rf CMakeCache.txt  CMakeFiles  cmake_install.cmake  Makefile 
cmake .. && make -j48 2>&1 | tee make.log 

:<< COMMENT
if [ -f "realtime-asr" ]; then
kill -9 `ps -ef | grep realtime-asr | grep -v grep | awk '{print $2}'`
./realtime-asr 10087 | tee log
fi
COMMENT
