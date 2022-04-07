// Copyright (c) 2020 Mobvoi Inc (Binbin Zhang)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "includes/wav.h"
#include "includes/timer.h"
#include "websocket/websocket_client.h"

std::string FLAGS_host = "127.0.0.1";
int FLAGS_port = 10089; 
std::string FLAGS_wav_path = "/DATA/disk1/duyao/workplace/e2e-release/record1.wav";

int main(int argc, char *argv[]) {
  kingsoft::WebSocketClient client(FLAGS_host, FLAGS_port);

  /*
  client.set_nbest(FLAGS_nbest);
  client.set_continuous_decoding(FLAGS_continuous_decoding);
  client.SendStartSignal(); */

  std::cout<<"line 31"<<std::endl;
  kingsoft::WavReader wav_reader(FLAGS_wav_path);
  const int sample_rate = 16000;
  // Only support 16K
  //CHECK_EQ(wav_reader.sample_rate(), sample_rate);
  const int num_sample = wav_reader.num_sample();
  std::vector<float> pcm_data(wav_reader.data(),
                              wav_reader.data() + num_sample);

  /*
  // Send data every 0.5 second
  const float interval = 0.5;
  const int sample_interval = interval * sample_rate;
  for (int start = 0; start < num_sample; start += sample_interval) {
    if (client.done()) {
      break;
    }
    int end = std::min(start + sample_interval, num_sample);
    // Convert to short
    std::vector<int16_t> data;
    data.reserve(end - start);
    for (int j = start; j < end; j++) {
      data.push_back(static_cast<int16_t>(pcm_data[j]));
    }
    // TODO(Binbin Zhang): Network order?
    // Send PCM data
    client.SendBinaryData(data.data(), data.size() * sizeof(int16_t));
    VLOG(2) << "Send " << data.size() << " samples";
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(interval * 1000)));
  }
  */

  std::vector<int16_t> data(num_sample);
  for(int i=0;i<num_sample;i++){
    data[i] = pcm_data[i];
  }
  /*
  std::cout<<"before send"<<std::endl;
  client.SendData(data.data(), data.size() * sizeof(int16_t), 2);
  std::cout<<"after send"<<std::endl; */

  const float interval = 0.5;
  const int sample_interval = interval * sample_rate;
  for (int start = 0; start < num_sample; start += sample_interval) {
    if (client.done()) {
      break;
    }
    int end = std::min(start + sample_interval, num_sample);

    /*
    for (int j = start; j < end; j++) {
      data.push_back(static_cast<int16_t>(pcm_data[j]));
    }*/
    // TODO(Binbin Zhang): Network order?
    // Send PCM data

    //client.SendBinaryData(data.data(), data.size() * sizeof(int16_t));
    int status = 0;

    if(end == num_sample){
      status = 2;
    }
    else{
      if(start == 0){
        status = 0;
      }
      else{
        status = 1;
      }
    }
    
    client.SendData(data.data() + start, (end - start) * sizeof(int16_t), status);
    std::cout << "Send " << end-start << " samples, status : " <<status <<std::endl;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int>(interval * 1000)));
  }



  kingsoft::Timer timer;
  //client.SendEndSignal();
  client.Join();
  //VLOG(2) << "Total latency: " << timer.Elapsed() << "ms.";
  return 0;
}
