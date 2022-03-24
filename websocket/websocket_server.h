#ifndef WEBSOCKET_WEBSOCKET_SERVER_H_
#define WEBSOCKET_WEBSOCKET_SERVER_H_

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>

#include "boost/asio/connect.hpp"
#include "boost/asio/ip/tcp.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/websocket.hpp"

#include "segment/segment.h"
#include "vad/thirdparty/webrtc/common_audio/vad/include/webrtc_vad.h"
//#include "utils/log.h"

#include "http/httpaudio.h"

namespace kingsoft {

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace asio = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

class ConnectionHandler {
 public:
  ConnectionHandler(tcp::socket&& socket, VadInst* pvad_inst);
  void operator()();

 private:
  void OnSpeechStart();
  void OnSpeechEnd();
  void OnText(const std::string& message);
  void OnFinish();
  void OnSpeechData(const beast::flat_buffer& buffer);
  void OnError(const std::string& message);
  void OnPartialResult(const std::string& result);
  void OnFinalResult(const std::string& result);
  void DecodeThreadFunc();
  std::string SerializeResult(bool finish);
  std::string SerializeResult(std::string raw_str);

  void reset();
  std::string Get_segs_result();

  bool continuous_decoding_ = false;
  int nbest_ = 1;
  websocket::stream<tcp::socket> ws_;
  /*
  std::shared_ptr<FeaturePipelineConfig> feature_config_;
  std::shared_ptr<DecodeOptions> decode_config_;
  std::shared_ptr<fst::SymbolTable> symbol_table_;
  std::shared_ptr<TorchAsrModel> model_;
  std::shared_ptr<fst::StdVectorFst> fst_;
  */

  bool got_start_tag_ = false;
  bool got_end_tag_ = false;
  // When endpoint is detected, stop recognition, and stop receiving data.
  bool stop_recognition_ = false;
  /*
  std::shared_ptr<FeaturePipeline> feature_pipeline_ = nullptr;
  std::shared_ptr<TorchAsrDecoder> decoder_ = nullptr;
  */
  std::shared_ptr<std::thread> decode_thread_ = nullptr;
  std::vector<int16_t> samples_;
  std::vector<pair<int, int> > rcv_range_smps_;
  int last_nranges_;
  //std::mutex sample_segs_m_;

  int state_;
  std::string last_result_, cur_result_;
  float last_sec_pt_, cur_sec_pt_;
  VadInst* pvad_inst_;
  int last_frame_;
  int nseg_done_;
  Acts acts_;

  Httpaudio httpaudio_;
  std::vector<Seginfo> seginfos_;
  //std::shared_ptr<httplib::Client> pcli;
 };

class WebSocketServer {
 public:
  WebSocketServer(int port,
                  VadInst* pvad_inst
                  )
      : port_(port),
        pvad_inst_(pvad_inst)  {};
  void Start();

 private:
  int port_;
  // The io_context is required for all I/O
  asio::io_context ioc_{1};

  VadInst* pvad_inst_;

};

}  // namespace kingsoft

#endif  // WEBSOCKET_WEBSOCKET_SERVER_H_

