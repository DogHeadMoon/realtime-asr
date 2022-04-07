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
#include <utility>
#include <vector>

#include "boost/json/src.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "http/httpaudio.h"
#include "stdio.h"
//#include "utils/log.h"
#include "json/json.h"
#include <cppcodec/base64_rfc4648.hpp>

using base64 = cppcodec::base64_rfc4648;

namespace kingsoft {

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
namespace json = boost::json;

bool DEBUG=false;
const float MIN_ACT_PERCENT = 0.6;
const int MIN_SPEAK_NFRAMES = 100;


ConnectionHandler::ConnectionHandler(
    tcp::socket&& socket, VadInst* pvad_inst)
    : ws_(std::move(socket)), pvad_inst_(pvad_inst) {
      last_sec_pt_ = 0; 
      cur_sec_pt_ = 0;
      last_frame_ = 0;
      nseg_done_ = 0;
      last_nranges_ = 0;
      noises_.emplace_back("快快快");
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

void ConnectionHandler::OnStart(){
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

  /*
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
  fs.close(); */

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
      /*
      std:unique_lock<std::mutex> lk(data_mut_);
      cond_.wait(lk, [this] {return !chunks_.empty();} );
      vector<int16_t> chunk = chunks_.front();
      chunks_.pop();
      lk.unlock();

      samples_.insert(samples_.end(), chunk.begin(), chunk.end());
      const int samples_per_frame = 160;
      int nframes = samples_.size()/samples_per_frame;
      for(int i=last_frame_;i<nframes;i++){
        int r = WebRtcVad_Process(pvad_inst_, 16000, &samples_[0] + samples_per_frame*i, samples_per_frame);
        if(state_ ==2 && i==nframes-1){
          acts_.add_act(r, true);
        }
        else{
          acts_.add_act(r);
        }
      }
      last_frame_ = nframes; */

      std::string rst;
      std::string rt;


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
          if(state_ ==2 && i==nframes-1){
            acts_.add_act(r, true);
          }
          else{
            acts_.add_act(r);
          }
          if(DEBUG){
            std::cout<<"add_act i frame : "<<i<<" act :"<<r<<std::endl;
          }
        }
        last_frame_ = nframes;
        last_nranges_ = nranges;

        int ncur_seg_num = acts_.seg_ends_.size();
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
              std::string key="seg-";
              int num_samples = end-st+1;
              std::vector<uint8_t> data(num_samples*sizeof(int16_t)/sizeof(uint8_t));
              std::memcpy(&data[0], &(samples_[st]), num_samples*sizeof(int16_t));
              key += std::to_string(i);
              std::string cur_result = httpaudio_.post(data, key);
              
              Seginfo info;
              info.st_frm = acts_.seg_sts_[i];
              info.end_frm = acts_.seg_ends_[i];
              info.result = cur_result;
              seginfos_.emplace_back(info);
              std::cout<<"seg "<< i<<" partial seg result : "<<cur_result<<std::endl;
              //std::string str_allsegs = Get_segs_result();
              //std::string rt = SerializeResult(str_allsegs);
              //OnPartialResult(rt);
              //rst = str_allsegs;
            }
          }
        }
        nseg_done_ = ncur_seg_num;

        if(state_==1 || state_== 0){
          end_spl_idx = samples_.size();
          const int min_spl = 16000 ;
          //const int min_spl = 16000 * 2;
          int nsts = acts_.seg_sts_.size();
          int nends = acts_.seg_ends_.size();
          if(nsts == nends + 1 && nsts>0){
            float act_percent = acts_.act_percent(acts_.seg_sts_[nsts-1], nframes - 1 );
            std::cout<<"act percent : "<<act_percent<<std::endl; 
            if(act_percent>0.6){
              int st = acts_.seg_sts_[nsts-1] * samples_per_frame;
              int end = end_spl_idx;
              int num_sp = end - st;
              if(num_sp>min_spl){
                std::string key="temp-";
                std::vector<uint8_t> data(num_sp*sizeof(int16_t)/sizeof(uint8_t));
                std::memcpy(&data[0], &(samples_[st]), num_sp*sizeof(int16_t));
                std::string temp_rt = httpaudio_.post(data, key);
                if(temp_rt.size()>6){
                  std::string all_segs_rt = Get_segs_result();
                  
                  if(all_segs_rt.size()>0){
                    rt = get_rsp_jsonstr(all_segs_rt+"<br />"+temp_rt+"...", state_);
                  }
                  else{
                    rt = get_rsp_jsonstr(temp_rt + "...", state_);
                  }
                  
                  std::cout<<"temp partial st: "<<acts_.seg_sts_[nsts-1]<<" end: "<<end_spl_idx/samples_per_frame<<std::endl;
                  std::cout<<"before temp all segs rt : "<<all_segs_rt<<std::endl;
                  std::cout<<"temp partial result : "<<temp_rt<<std::endl; 
                  std::cout<<"temp act percent: "<<act_percent <<std::endl;

                  OnResult(rt);
                }
              }
            }
          }
        }
        else if(state_==2){
          std::string key = "final";
          std::vector<uint8_t> data(samples_.size()*sizeof(int16_t)/sizeof(uint8_t));
          std::memcpy(&data[0], samples_.data(), samples_.size() * sizeof(int16_t));
          std::string whole_offline_rt = httpaudio_.post(data, key, "whole_pcm");
          std::string whole_segs = Get_segs_result();
          std::cout<<"whole offline rt : "<<whole_offline_rt<<std::endl;
          std::cout<<"whole segs rt :"<<whole_segs<<std::endl;
          rt = get_rsp_jsonstr(whole_segs, state_);
          OnResult(rt);
          break;
        }
      }
    }
  } catch (std::exception const& e) {
    //LOG(ERROR) << e.what();
  }
}

std::string ConnectionHandler::get_rsp_jsonstr(std::string text, int status, int role){
  Json::Value root;
  root["result"]=text;
  root["status"]=status;
  root["role"]=0;
  Json::FastWriter fastWriter;
  std::string output = fastWriter.write(root);
  return output;
}

void ConnectionHandler::OnResult(std::string content){
  ws_.text(true);
  ws_.write(asio::buffer(content));
}

void ConnectionHandler::OnError(const std::string& message) {
  /*
  json::value rv = {{"status", "failed"}, {"message", message}};
  ws_.text(true);
  ws_.write(asio::buffer(json::serialize(rv)));*/
  // Close websocket
  std::cout<<message<<std::endl;
  ws_.close(websocket::close_code::normal);
}

int ConnectionHandler::OnText(const std::string& message) {
  Json::Value root;   
  Json::Reader reader;
  bool parsingSuccessful = reader.parse( message.c_str(), root );     //parse process
  if(!parsingSuccessful) {
      std::cout  << "Failed to parse"<< reader.getFormattedErrorMessages();
      OnError("Wrong message");
      return -1;
  }

  int status =  root.get("status", -1).asInt();
  std::string base64_str = root.get("audio", "").asString();
  std::string format = root.get("format", "").asString();

  std::cout<<"get message status : "<<status<<std::endl;

  if(base64_str.size()>0){
    std::vector<uint8_t> decoded = base64::decode(base64_str);
    
    /*
    std::vector<int16_t> chunk(decoded.size()/2);
    std::memcpy(chunk.data(), decoded.data(), chunk.size()*sizeof(int16_t));
    std::lock_guard<std::mutex> lk(data_mut_);
    chunks_.push(chunk);
    cond_.notify_one();
    */

    const int16_t* pdata = reinterpret_cast<const int16_t*>(decoded.data());
    int num_samples = decoded.size()/2;
    for (int i = 0; i < num_samples; i++) {
      samples_.emplace_back(pdata[i]);
    }
    rcv_range_smps_.emplace_back(std::make_pair(samples_.size() - num_samples, samples_.size()-1));


    if(status == 0 || status == 1){
      state_ = 1;
    }
    else if(status == 2){
      state_ = 2;
      got_end_tag_ = true;
    }
    else{
      OnError("Wrong protocol");
    }
    return status;
  }
  else{
      OnError("Wrong protocol");
  }
  return -1;
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
        int state = OnText(message);
        if(decode_thread_ == nullptr){
          OnStart();
        }
        if(state == 2) {
          break;
        }
      } else {
        OnError("wrong protocol");
      }
    }

    //LOG(INFO) << "Read all pcm data, wait for decoding thread";
    if (decode_thread_ != nullptr) {
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
  std::vector<std::string> val_segs;
  
  
  for(int i=0;i<seginfos_.size();i++){
    std::string temp_rt = seginfos_[i].result;
    if(Check(temp_rt)){
      std::cout<<"seg " <<i<<" vad result : "<< temp_rt<<std::endl;
      val_segs.emplace_back(temp_rt);
    }
    else{
      std::cout<<"seg " <<i<<" invad result : "<<temp_rt<<std::endl;
    }
  }

  int n = val_segs.size();
  if(n>0){
    for(int i=0;i<n-1;i++){
      rt += val_segs[i] + "，";
    }
    rt += val_segs.back();
  }
  return rt;
}

bool ConnectionHandler::Check(std::string seg_rt){
  for(int i=0;i<noises_.size();i++){
    if(seg_rt == noises_[i]){
      return false;
    }
    if(seg_rt.size()<6){
      return false;
    }
  }
  return true;
}


void WebSocketServer::Start() {
  try {
    auto const address = asio::ip::make_address("0.0.0.0");
    std::cout<<"hello, svr start "<<std::endl;
    tcp::acceptor acceptor{ioc_, {address, static_cast<uint16_t>(port_)}};
    for (;;) {
      // This will receive the new connection
      tcp::socket socket{ioc_};
      // Block until we get a connection
      acceptor.accept(socket);
      // Launch the session, transferring ownership of the socket
      ConnectionHandler handler(std::move(socket), pvad_inst_);
      
      //add by du
      std::thread t(std::move(handler));
      //std::thread t(std::ref(handler));
      t.detach();
    }
  } catch (const std::exception& e) {
    std::cout<<"start error"<<std::endl;
    //LOG(FATAL) << e.what();
  }
}

}  // namespace kingsoft 
