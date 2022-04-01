set(CMAKE_CXX_STANDARD 14)
set(PRJ_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(ROOT /home/duyao/workplace/asr)
set(LIBS ${ROOT}/libs)

file(GLOB SRC_FILES 
  bin/server.cc
  websocket/*.cc
  vad/src/*.c vad/src/*.h vad/thirdparty/webrtc/common_audio/*/*.c vad/thirdparty/webrtc/rtc_base/*.c* 
  utils/*.cc 
  segment/segment.cc
  http/*.cc
)

add_executable(
    realtime-asr
    ${SRC_FILES}
)

target_include_directories(
  realtime-asr
    PRIVATE 
      ${PRJ_DIR}
      ${PRJ_DIR}/thirdparty
      ${PRJ_DIR}/thirdparty/cppcodec
      ${PRJ_DIR}/vad/thirdparty/webrtc   
      /DATA/disk1/duyao/workplace/asr/libs/boost_1_78_0
)

link_directories(${PRJ_DIR}/libs)

target_link_libraries(
  realtime-asr
  PRIVATE
    ${PRJ_DIR}/libs/libjsoncpp.a
    pthread
    #-lidn -lrt -lcrypto -lssl
    #/home/duyao/workplace/tri/client/build/install/lib/grpcclient_static.a
    #/home/duyao/workplace/tri/client/build/install/lib/httpclient_static
)



file(GLOB CLIENT_SRC_FILES 
  bin/client.cc
  websocket/websocket_client.cc
)


add_executable(
    wsclient
    ${CLIENT_SRC_FILES}
)

target_include_directories(
  wsclient
    PUBLIC
      ${PRJ_DIR}
      ${PRJ_DIR}/thirdparty
      ${PRJ_DIR}/thirdparty/cppcodec
      ${PRJ_DIR}/vad/thirdparty/webrtc   
      /DATA/disk1/duyao/workplace/asr/libs/boost_1_78_0
)

link_directories(${PRJ_DIR}/libs)

target_link_libraries(
  wsclient
  PRIVATE
    ${PRJ_DIR}/libs/libjsoncpp.a
    pthread
    #-lidn -lrt -lcrypto -lssl
    #/home/duyao/workplace/tri/client/build/install/lib/grpcclient_static.a
    #/home/duyao/workplace/tri/client/build/install/lib/httpclient_static
)
