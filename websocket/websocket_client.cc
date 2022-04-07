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

#include "websocket/websocket_client.h"
//#include "boost/json/src.hpp"
#include "json/json.h"



namespace kingsoft {

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>
//namespace json = boost::json;

WebSocketClient::WebSocketClient(const std::string& host, int port)
    : host_(host), port_(port) {
  Connect();
  t_.reset(new std::thread(&WebSocketClient::ReadLoopFunc, this));
}

void WebSocketClient::Connect() {
  tcp::resolver resolver{ioc_};
  // Look up the domain name
  auto const results = resolver.resolve(host_, std::to_string(port_));
  // Make the connection on the IP address we get from a lookup
  auto ep = asio::connect(ws_.next_layer(), results);
  // Update the host_ string. This will provide the value of the
  // Host HTTP header during the WebSocket handshake.
  // See https://tools.ietf.org/html/rfc7230#section-5.4
  std::string host = host_ + ":" + std::to_string(ep.port());
  // Perform the websocket handshake
  ws_.handshake(host, "/");
}

void WebSocketClient::SendTextData(const std::string& data) {
  ws_.text(true);
  ws_.write(asio::buffer(data));
}

void WebSocketClient::SendBinaryData(const void* data, size_t size) {
  ws_.binary(true);
  ws_.write(asio::buffer(data, size));
}

void WebSocketClient::Close() { ws_.close(websocket::close_code::normal); }

void WebSocketClient::ReadLoopFunc() {
  std::cout<<"in ReadLoopFunc"<<std::endl;
  try {
    while (true) {
      beast::flat_buffer buffer;
      std::cout<<"in ReadLoopFunc"<<std::endl;
      ws_.read(buffer);
      std::string message = beast::buffers_to_string(buffer.data());
      //LOG(INFO) << message;
      //CHECK(ws_.got_text());
      
      if(ws_.got_text()){
        std::cout<<"got text right"<<std::endl;
      }

      Json::Value root;   
      Json::Reader reader;
      bool parsingSuccessful = reader.parse( message.c_str(), root );     //parse process
      if ( !parsingSuccessful )
      {
        std::cout  << "Failed to parse"<< reader.getFormattedErrorMessages();
      }
      else{
        std::string rst = root.get("result", "" ).asString();
        if(rst.size()>0){
          std::cout<<"receive result "<<rst<<std::endl;
        }
        else{
          std::cout<<"receive nothing" <<std::endl;
        }
        int state = root.get("status", -1 ).asInt();
        std::cout<<"receive status : "<<state <<std::endl;
        if(state == 2){
          break;
        }

      }
      

      /*
      json::object obj = json::parse(message).as_object();
      if (obj["status"] != "ok") {
        break;
      }
      if (obj["type"] == "speech_end") {
        done_ = true;
        break;
      }*/


    }
  } catch (beast::system_error const& se) {
    // This indicates that the session was closed
    if (se.code() != websocket::error::closed) {
      //LOG(ERROR) << se.code().message();
      std::cout<< se.code().message()<<std::endl;
    }
  } catch (std::exception const& e) {
    //LOG(ERROR) << e.what();
    std::cout<< e.what()<<std::endl;
  }
}

void WebSocketClient::Join() { t_->join(); }

std::string WebSocketClient::get_json_str(std::string base64, int status){
  Json::Value root;
  root["status"]=status;
  root["audio"]=base64;
  Json::FastWriter fastWriter;
  std::string output = fastWriter.write(root);
  return output;
}

void WebSocketClient::SendData(const void* data, size_t size, int status){
  std::vector<uint8_t> raw_data(size);
  std::memcpy(raw_data.data(), data, size);
  std::string base64_str = base64::encode(raw_data);
  std::string msg = get_json_str(base64_str, status);
  ws_.text(true);
  ws_.write(asio::buffer(msg));
}



/*
void WebSocketClient::SendStartSignal() {
  // TODO(Binbin Zhang): Add sample rate and other setting surpport
  json::value start_tag = {{"signal", "start"},
                           {"nbest", nbest_},
                           {"continuous_decoding", continuous_decoding_}};
  std::string start_message = json::serialize(start_tag);
  this->SendTextData(start_message);
}

void WebSocketClient::SendEndSignal() {
  json::value end_tag = {{"signal", "end"}};
  std::string end_message = json::serialize(end_tag);
  this->SendTextData(end_message);
}
*/

}  // namespace
