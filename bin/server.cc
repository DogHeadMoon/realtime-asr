#include <iostream>
#include "websocket/websocket_server.h"
#include "json/json.h"

#include <chrono>
#include <ctime>
#include <iomanip>


#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

using namespace std;

int get_audio(string js, string& audio_base64, string& id){
    Json::Reader reader;
    Json::Value json_object;
    if (!reader.parse(js, json_object)){
        return 1;
    }
    audio_base64=json_object["audioBase64"].asString();
    id=json_object["id"].asString();
    return 0;
}

int main(int argc, char* argv[])
{
  // create a file rotating logger with 5mb size max and 3 rotated files
  auto logger = spdlog::rotating_logger_mt("file_logger", "wsserver.log", 1024 * 1024 * 100, 3);
  logger->info("listen on {}", argv[1]);
  logger->flush();

  VadInst* pvad_inst = WebRtcVad_Create();
  WebRtcVad_Init(pvad_inst);
  //WebRtcVad_set_mode(pvad_inst, 1);
  WebRtcVad_set_mode(pvad_inst, 2);

  kingsoft::WebSocketServer wss(atoi(argv[1]), pvad_inst);
  wss.Start();
  delete pvad_inst;
  return 0;
}
