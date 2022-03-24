#include "http/httpaudio.h"
using base64 = cppcodec::base64_rfc4648;


std::string get_json(std::string result, std::string name){
    Json::Value fromScratch;
    fromScratch["result"] = result;
    fromScratch["id"] = name;
    Json::StyledWriter styledWriter;
    std::string json_str = styledWriter.write(fromScratch);
    return json_str;
}

std::string parse_json(std::string jsonstr){
    Json::Value root;   
    Json::Reader reader;
    bool parsingSuccessful = reader.parse( jsonstr.c_str(), root );     //parse process
    if ( !parsingSuccessful )
    {
        std::cout  << "Failed to parse"<< reader.getFormattedErrorMessages();
        return "";
    }
    else{
        return root.get("result", "" ).asString();
    }
}

std::string format_json(std::string basee64_str, std::string id){
    Json::Value fromScratch;
    fromScratch["audioBase64"] = basee64_str;
    fromScratch["id"] = id;
    fromScratch["scene"] = 0;
    fromScratch["aue"] = "pcm";
    Json::StyledWriter styledWriter;
    std::string json_str = styledWriter.write(fromScratch);
    return json_str;
}

std::string Httpaudio::post(std::vector<uint8_t> content, std::string id){
  //std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  std::string base64_str = base64::encode(content);
  //std::cout<<base64_str<<std::endl;
  std::string json = format_json(base64_str, id);
  httplib::Client cli("127.0.0.1", 10086);
  std::string content_type = "application/json";

  std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp_st = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
  std::uint64_t start_tick = (std::uint64_t)tp_st.time_since_epoch().count();
  auto res = cli.Post("/", json.c_str(), json.size(), content_type.c_str());
  std::string rt;
  if(res){
      int status = res->status;
      std::string jsonbody = res->body;
      if(status == 200){
        std::cout<<"status : " <<status <<std::endl; 
        std::cout<<"jsonbody : " <<jsonbody <<std::endl;
        rt = parse_json(jsonbody);
        std::cout<<rt<<std::endl; 
      }
  }
  std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds> tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
  std::uint64_t key_end = (std::uint64_t)tp.time_since_epoch().count();
  std::uint64_t consume  = key_end - start_tick; 
  std::cout<<"time consume : "<<consume<<std::endl;
  return rt;
}

std::vector<uint8_t> Httpaudio::decode(std::string base64){
    std::vector<uint8_t> decoded = base64::decode(base64);
    return decoded;
}