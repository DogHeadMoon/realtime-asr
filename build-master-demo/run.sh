kill -9 `ps -ef | grep realtime-asr | grep -v grep | awk '{print $2}'`
nohup ./realtime-asr 10087 >log 2>&1 &
