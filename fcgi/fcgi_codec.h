#ifndef FCGI_CODEC_H_
#define FCGI_CODEC_H_

#include <unordered_map>

#include "fcgi_constant.h"
#include "fcgi_type.h"
#include "kanon/net/buffer.h"
#include "kanon/util/noncopyable.h"

namespace fcgi {

class FcgiCodec : kanon::noncopyable {
  using ParamMap = std::unordered_map<std::string, std::string>;

 public:
  struct RequestData {
    FcgiRole role;
    FcgiFlag flags;
    uint16_t request_id;
    kanon::Buffer param_stream;
    ParamMap param_map;
    kanon::Buffer stdin_stream;
    kanon::Buffer data_stream;
    FcgiCodec *codec;

    ~RequestData() noexcept;
  };

  friend struct RequestData;
  using RequestHandler =
      std::function<void(kanon::TcpConnectionPtr const &, RequestData data)>;

  using RequestMap = std::unordered_map<uint16_t, RequestData>;

  FcgiCodec(kanon::TcpConnectionPtr const &conn);

  int ParseRequest(kanon::TcpConnectionPtr const &conn, kanon::Buffer &buffer);
  bool ParseParams(RequestData &data);

  static void SendStdout(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         char const *data, size_t len);
  static void SendStderr(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         char const *data, size_t len);
  
  static void SendStdout(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         kanon::StringView data)
  { SendStdout(conn, id, data.data(), data.size()); }

  static void SendStderr(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         kanon::StringView data)
  { SendStderr(conn, id, data.data(), data.size()); }

  static void EndRequest(kanon::TcpConnectionPtr const &conn, uint16_t id);
  static void EndStdout(kanon::TcpConnectionPtr const &conn, uint16_t id);
  static void EndStderr(kanon::TcpConnectionPtr const &conn, uint16_t id);
  
  void SetRequestHandler(RequestHandler handler)
  { request_handler_ = std::move(handler); }
 private:
  RequestMap request_map_;
  RequestHandler request_handler_;
};

using FcgiRequest = FcgiCodec::RequestData;

}  // namespace fcgi
#endif  // FCGI_CODEC_H_
