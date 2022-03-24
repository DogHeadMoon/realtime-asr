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

#include "websocket/websocket_server.h"

#include <thread>
#include <utility>
#include <vector>

#include "boost/json/src.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "http/httpaudio.h"
#include "stdio.h"
//#include "utils/log.h"


namespace kingsoft {

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
namespace json = boost::json;

bool DEBUG=false;
const float MIN_ACT_PERCENT = 0.65;
const int MIN_SPEAK_NFRAMES = 100;


ConnectionHandler::ConnectionHandler(
    tcp::socket&& socket, VadInst* pvad_inst)
    : ws_(std::move(socket)), pvad_inst_(pvad_inst) {
      last_sec_pt_ = 0; 
      cur_sec_pt_ = 0;
      last_frame_ = 0;
      nseg_done_ = 0;
      last_nranges_ = 0;
    }

void ConnectionHandler::OnSpeechStart() {
  //LOG(INFO) << "Recieved speech start signal, start reading speech";
  std::cout<< "Recieved speech start signal, start reading speech"<<std::endl;
  got_start_tag_ = true;
  json::value rv = {{"status", "ok"}, {"type", "server_ready"}};
  ws_.text(true);
  ws_.write(asio::buffer(json::serialize(rv)));
  std::cout<<"OnSpeechStart()"<<std::endl;
  decode_thread_ = std::make_shared<std::thread>(&ConnectionHandler::DecodeThreadFunc, this);
}

void ConnectionHandler::OnSpeechEnd() {
  /* ori
  LOG(INFO) << "Received speech end signal";
  if (feature_pipeline_ != nullptr) {
    feature_pipeline_->set_input_finished();
  }
  got_end_tag_ = true;
  */
  got_end_tag_ = true;
  state_ = 3;
}

void ConnectionHandler::OnPartialResult(const std::string& result) {
  //LOG(INFO) << "Partial result: " << result;
  json::value rv = {
      {"status", "ok"}, {"type", "partial_result"}, {"nbest", result}};
  ws_.text(true);
  ws_.write(asio::buffer(json::serialize(rv)));
}

void ConnectionHandler::OnFinalResult(const std::string& result) {
  //LOG(INFO) << "Final result: " << result;
  json::value rv = {
      {"status", "ok"}, {"type", "final_result"}, {"nbest", result}};
  ws_.text(true);
  ws_.write(asio::buffer(json::serialize(rv)));
}

void ConnectionHandler::OnFinish() {
  // Send finish tag
  json::value rv = {{"status", "ok"}, {"type", "speech_end"}};
  ws_.text(true);
  ws_.write(asio::buffer(json::serialize(rv)));
}

void ConnectionHandler::OnSpeechData(const beast::flat_buffer& buffer) {
  // Read binary PCM data
  int num_samples = buffer.size() / sizeof(int16_t);

  const int16_t* pdata = static_cast<const int16_t*>(buffer.data().data());
  const int header = 22;
  if(num_samples>header){
  }
  for (int i = header; i < num_samples; i++) {
    samples_.emplace_back(pdata[i]);
  }

  ofstream fs("samples.txt", ios::app);
  for(int i=0;i<100;i++){
    fs<<pdata[i]<<" ";
  }
  fs<<endl;
  for(int i=num_samples-10;i<num_samples;i++){
    fs<<pdata[i]<<" ";
  }
  fs<<endl;
  fs<<endl;
  fs.close();

  rcv_range_smps_.emplace_back(std::make_pair(samples_.size() - num_samples + header, samples_.size()-1));
  state_ = 1;
}

std::string ConnectionHandler::SerializeResult(bool finish) {
  // add by du
  json::object jpath({{"sentence",  cur_result_}});  
  json::array nbest;
  nbest.emplace_back(jpath);
  return json::serialize(nbest);
}


std::string ConnectionHandler::SerializeResult(std::string raw_str){
  json::object jpath({{"sentence",  raw_str}});  
  json::array nbest;
  nbest.emplace_back(jpath);
  return json::serialize(nbest);
}

void ConnectionHandler::DecodeThreadFunc() {
   try {
    while (true) {
      int cur_size = samples_.size();
      if (state_ == 3) {
        acts_.end();
        std::cout<<"OnSpeechEnd"<<std::endl;
        std::string key = "final";
        std::cout<<"tatal samples n : "<<samples_.size() <<std::endl;

        /*
        int nsts = acts_.seg_sts_.size();
        int nends = acts_.seg_ends_.size();
        if(nsts == nends + 1 && nsts>0){
        }*/


        /*
        FILE * pFile;
        pFile = fopen ("finalcpp.pcm", "wb");
        fwrite (&samples_[0] , sizeof(samples_[0]), samples_.size(), pFile);
        fclose (pFile); */
        /*
        vector<T>::const_iterator first = myVec.begin() + 100000;
        vector<T>::const_iterator last = myVec.begin() + 101000;
        vector<T> newVec(first, last); */

        std::vector<uint8_t> data(samples_.size()*sizeof(int16_t)/sizeof(uint8_t));
        
        /*
        std::memcpy(&data[0], samples_.data(), samples_.size() * sizeof(int16_t));
        std::string whole_rt = httpaudio_.post(data, key);
        std::string rt = SerializeResult(whole_rt);
        */
        std::string rt = Get_segs_result();
        OnFinalResult(rt);
        OnFinish();
        reset();
        stop_recognition_ = true;
        break;
      } else if (state_ == 2) {
        //decoder_->Rescoring();
        std::string result = SerializeResult(true);
        OnFinalResult(result);
        stop_recognition_ = true;
        break;

        // If it's not continuous decoidng, continue to do next recognition
        // otherwise stop the recognition
        
        /*
        if (continuous_decoding_) {
          decoder_->ResetContinuousDecoding();
        } else {
          OnFinish();
          stop_recognition_ = true;
          break;
        } */
      } else {
        if (state_ == 1) {
          bool bvad = true;
          if(bvad){
            const int samples_per_frame = 160;
            int nranges = rcv_range_smps_.size();     
            if(nranges > last_nranges_){
              //std::cout<<"nranges : "<<nranges << "  lastnranges : "<<last_nranges_<<std::endl;
              int end_spl_idx = rcv_range_smps_[nranges-1].second;
              int nframes = (end_spl_idx + 1)/samples_per_frame;
              if(DEBUG){
                std::cout<<"current n frames : "<<nframes<<std::endl; 
              }

              for(int i=last_frame_;i<nframes;i++){
                int r = WebRtcVad_Process(pvad_inst_, 16000, &samples_[0] + samples_per_frame*i, samples_per_frame);
                acts_.add_act(r, false);
                if(DEBUG){
                  std::cout<<"add_act i frame : "<<i<<" act :"<<r<<std::endl;
                }
              }
              last_frame_ = nframes;
              last_nranges_ = nranges;

              int ncur_seg_num = acts_.seg_ends_.size();

              if(DEBUG){
                for(int i=0;i<ncur_seg_num;i++){
                    std::cout<<"seg "<<i<<" st : "<<acts_.seg_sts_[i]<<std::endl;
                    std::cout<<"seg "<<i<<" end : "<<acts_.seg_ends_[i]<<std::endl;
                }
              }

              int num_samples = samples_.size();
              for(int i=nseg_done_; i<ncur_seg_num;i++){
                int st = acts_.seg_sts_[i] * samples_per_frame;
                int end = acts_.seg_ends_[i] * samples_per_frame;
                int n_spk_frames = acts_.seg_ends_[i]-acts_.seg_sts_[i];
                std::cout<<"seg "<<i<<" st: "<<acts_.seg_sts_[i]  <<" end: "<<acts_.seg_ends_[i]
                    <<"   nframes : "<<n_spk_frames<<std::endl;
                
                if(st<num_samples && end<num_samples){
                  float perct = acts_.act_percent(acts_.seg_sts_[i], acts_.seg_ends_[i]);
                  std::cout <<"seg "<<i<<"  acts percentage : "<<perct <<std::endl;
                  
                  if( perct>MIN_ACT_PERCENT && n_spk_frames>MIN_SPEAK_NFRAMES ){
                    std::string key="a";
                    int num_samples = end-st+1;
                    std::vector<uint8_t> data(num_samples*sizeof(int16_t)/sizeof(uint8_t));
                    std::memcpy(&data[0], &(samples_[st]), num_samples*sizeof(int16_t));
                    std::string cur_result = httpaudio_.post(data, key);
                    

                    Seginfo info;
                    info.st_frm = acts_.seg_sts_[i];
                    info.end_frm = acts_.seg_ends_[i];
                    info.result = cur_result;
                    seginfos_.emplace_back(info);
                    std::cout<<"seg "<< i<<" partial seg result : "<<cur_result<<std::endl;
                    std::string str_allsegs = Get_segs_result();
                    std::string rt = SerializeResult(str_allsegs);
                    OnPartialResult(rt);
                  }
                }
                
              }
              nseg_done_ = ncur_seg_num;

              /*
              int st = 0;
              if(acts_.seg_ends_.size()>0){
                st = acts_.seg_ends_[acts_.seg_ends_.size()-1] * samples_per_frame;
              }
              int end = end_spl_idx;
              std::string key="a";
              int num_sp = end-st+1;
              std::vector<uint8_t> data(num_sp*sizeof(int16_t)/sizeof(uint8_t));
              std::memcpy(&data[0], &(samples_[st]), num_sp*sizeof(int16_t));
              cur_result_ = httpaudio_.post(data, key);
              std::string rt = SerializeResult(false);
              std::cout<<"partial result : "<<cur_result_<<std::endl; 
              OnPartialResult(rt); */
            
              const int min_spl = 16000 * 1.5;
              int nsts = acts_.seg_sts_.size();
              int nends = acts_.seg_ends_.size();
              if(nsts == nends + 1 && nsts>0){
                if(acts_.act_percent(acts_.seg_sts_[nsts-1], end_spl_idx/samples_per_frame)>0.5){
                  int st = acts_.seg_sts_[nsts-1] * samples_per_frame;
                  int end = end_spl_idx;
                  int num_sp = end - st;
                  if(num_sp>min_spl){
                    std::string key="a";
                    std::vector<uint8_t> data(num_sp*sizeof(int16_t)/sizeof(uint8_t));
                    std::memcpy(&data[0], &(samples_[st]), num_sp*sizeof(int16_t));
                    std::string temp_rt = httpaudio_.post(data, key);
                    std::string all_segs_rt = Get_segs_result();
                    std::string rt = SerializeResult(all_segs_rt+"<br />"+temp_rt+"...");
                    std::cout<<"temp partial st: "<<acts_.seg_sts_[nsts-1]<<" end: "<<end_spl_idx/samples_per_frame<<std::endl;
                    std::cout<<"all segs rt : "<<all_segs_rt<<std::endl;
                    std::cout<<"temp partial result : "<<temp_rt<<std::endl; 
                    OnPartialResult(rt); 
                  }

                }
              }
            
            }

          }
        }

      }
    }
  } catch (std::exception const& e) {
    //LOG(ERROR) << e.what();
  }
}

void ConnectionHandler::OnError(const std::string& message) {
  json::value rv = {{"status", "failed"}, {"message", message}};
  ws_.text(true);
  ws_.write(asio::buffer(json::serialize(rv)));
  // Close websocket
  ws_.close(websocket::close_code::normal);
}

void ConnectionHandler::OnText(const std::string& message) {
  json::value v = json::parse(message);
  if (v.is_object()) {
    json::object obj = v.get_object();
    if (obj.find("signal") != obj.end()) {
      json::string signal = obj["signal"].as_string();
      if (signal == "start") {
        if (obj.find("nbest") != obj.end()) {
          if (obj["nbest"].is_int64()) {
            nbest_ = obj["nbest"].as_int64();
          } else {
            OnError("integer is expected for nbest option");
          }
        }
        if (obj.find("continuous_decoding") != obj.end()) {
          if (obj["continuous_decoding"].is_bool()) {
            continuous_decoding_ = obj["continuous_decoding"].as_bool();
          } else {
            OnError(
                "boolean true or false is expected for "
                "continuous_decoding option");
          }
        }
        OnSpeechStart();
      } else if (signal == "end") {
        OnSpeechEnd();
      } else {
        OnError("Unexpected signal type");
      }
    } else {
      OnError("Wrong message header");
    }
  } else {
    OnError("Wrong protocol");
  }
}

void ConnectionHandler::operator()() {
  try {
    // Accept the websocket handshake
    std::cout<<"hello , handle opertor ()"<<std::endl;
    ws_.accept();
    for (;;) {
      // This buffer will hold the incoming message
      beast::flat_buffer buffer;
      // Read a message
      ws_.read(buffer);
      if (ws_.got_text()) {
        std::string message = beast::buffers_to_string(buffer.data());
        std::cout<<"recived message : "<<message<<std::endl;
        //LOG(INFO) << message;
        OnText(message);
        if (got_end_tag_) {
          break;
        }
      } else {
        if (!got_start_tag_) {
          OnError("Start signal is expected before binary data");
        } else {
          if (stop_recognition_) {
            break;
          }
          std::cout<<"operator() buffer n samples : "<<buffer.size() / sizeof(int16_t) <<std::endl;
          OnSpeechData(buffer);
        }
      }
    }

    //LOG(INFO) << "Read all pcm data, wait for decoding thread";
    if (decode_thread_ != nullptr) {
      // add by du
      decode_thread_->join();
    }


  } catch (beast::system_error const& se) {
    // This indicates that the session was closed
    if (se.code() != websocket::error::closed) {
      //LOG(ERROR) << se.code().message();
    }
  } catch (std::exception const& e) {
    //LOG(ERROR) << e.what();
  }
}

void ConnectionHandler::reset(){
  samples_.clear();
  acts_.reset();
  last_result_ = "";
  cur_result_ = "";
  last_sec_pt_ = 0;
  cur_sec_pt_ = 0;
} 

std::string ConnectionHandler::Get_segs_result(){
  std::string rt;
  for(int i=0;i<seginfos_.size();i++){
    if(seginfos_[i].result != ""){
      rt += seginfos_[i].result + "，";
    }
  }
  return rt;
}

void WebSocketServer::Start() {
  try {
    auto const address = asio::ip::make_address("0.0.0.0");
    //auto const address = asio::ip::make_address("127.0.0.1");
    std::cout<<"hello, svr start "<<std::endl;
    tcp::acceptor acceptor{ioc_, {address, static_cast<uint16_t>(port_)}};
    for (;;) {
      // This will receive the new connection
      tcp::socket socket{ioc_};
      // Block until we get a connection
      acceptor.accept(socket);
      // Launch the session, transferring ownership of the socket
      ConnectionHandler handler(std::move(socket), pvad_inst_);
      std::thread t(std::move(handler));
      t.detach();
    }
  } catch (const std::exception& e) {
    std::cout<<"start error"<<std::endl;
    //LOG(FATAL) << e.what();
  }
}

}  // namespace kingsoft 
