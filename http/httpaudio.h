#ifndef HTTPAUDIO_H_
#define HTTPAUDIO_H_

#include "json/json.h"
#include "cppcodec/base64_rfc4648.hpp"
#include "http/httplib.h"

class Httpaudio{
public:
    std::string post(std::vector<uint8_t> content, std::string id);
    std::vector<uint8_t> decode(std::string base64);
};

#endif
