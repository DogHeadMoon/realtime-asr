sox -t raw -c 1 -e signed-integer -b 16 -r 16000 finalcpp.pcm finalcpp.wav
rc copy -P finalcpp.wav one:temp-audio
