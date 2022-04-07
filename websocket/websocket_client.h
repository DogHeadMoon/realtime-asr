#ifndef WEBSOCKET_WEBSOCKET_CLIENT_H_
#define WEBSOCKET_WEBSOCKET_CLIENT_H_

#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "boost/asio/connect.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/websocket.hpp"

#include "includes/utils.h"
#include <cppcodec/base64_rfc4648.hpp>
using base64 = cppcodec::base64_rfc4648;

namespace kingsoft {

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

class WebSocketClient {
 public:
  WebSocketClient(const std::string& host, int port);

  void SendTextData(const std::string& data);
  void SendBinaryData(const void* data, size_t size);
  void ReadLoopFunc();
  void Close();
  void Join();
  //void SendStartSignal();
  //void SendEndSignal();
  //void set_nbest(int nbest) { nbest_ = nbest; }
  //void set_continuous_decoding(bool continuous_decoding) {
  //  continuous_decoding_ = continuous_decoding;
  //}
  std::string get_json_str(std::string base64, int status);
  void SendData(const void* data, size_t size, int status);
  bool done() const { return done_; }

 private:
  void Connect();
  std::string host_;
  int port_;
  int nbest_ = 1;
  bool continuous_decoding_ = false;
  bool done_ = false;
  asio::io_context ioc_;
  websocket::stream<tcp::socket> ws_{ioc_};
  std::unique_ptr<std::thread> t_{nullptr};

  DISALLOW_COPY_AND_ASSIGN(WebSocketClient);
};

}  // namespace 

#endif  // WEBSOCKET_WEBSOCKET_CLIENT_H_
