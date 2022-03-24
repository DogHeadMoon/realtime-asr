#include <iostream>
#include "websocket/websocket_server.h"
#include "json/json.h"

#include <chrono>
#include <ctime>
#include <iomanip>

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
  cout<<"listen on "<< argv[1]<<endl;

  VadInst* pvad_inst = WebRtcVad_Create();
  WebRtcVad_Init(pvad_inst);
  WebRtcVad_set_mode(pvad_inst, 1);

  kingsoft::WebSocketServer wss(atoi(argv[1]), pvad_inst);
    cout<<"listen on "<< argv[1]<<endl;
  wss.Start();
    cout<<"listen on "<< argv[1]<<endl;

  delete pvad_inst;
}
