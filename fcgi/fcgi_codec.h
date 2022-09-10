#ifndef FCGI_CODEC_H_
#define FCGI_CODEC_H_

#include <unordered_map>

#include "fcgi_constant.h"
#include "fcgi_type.h"
#include "kanon/buffer/chunk_list.h"
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
    FcgiCodec *codec = nullptr;

    /** The request resource is managed by RequestHandler */
    ~RequestData() noexcept;
  };

  friend struct RequestData;
  using RequestHandler =
      std::function<void(kanon::TcpConnectionPtr const &, RequestData data)>;

  using RequestMap = std::unordered_map<uint16_t, RequestData>;
  using FcgiRequest = FcgiCodec::RequestData;

  explicit FcgiCodec(kanon::TcpConnectionPtr const &conn);

  /*----------------------*/
  /* Output stdout stream */
  /*----------------------*/

  static void SendStdout(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         char const *data, size_t len);

  /** Convenient API for id version */
  static void SendStdout(kanon::TcpConnectionPtr const &conn,
                         FcgiRequest const &request, char const *data,
                         size_t len)
  {
    SendStdout(conn, request.request_id, data, len);
  }

  static void SendStdout(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         kanon::StringView data)
  {
    SendStdout(conn, id, data.data(), data.size());
  }

  static void SendStdout(kanon::TcpConnectionPtr const &conn,
                         FcgiRequest const &request, kanon::StringView data)
  {
    SendStdout(conn, request.request_id, data);
  }

  static void SendStdout(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         kanon::ChunkList &output);

  // static void SendStdout(kanon::TcpConnectionPtr const &conn, FcgiRequest
  // const&request,
  //                        kanon::ChunkList &output);

  /*----------------------*/
  /* Output stderr stream */
  /*----------------------*/

  static void SendStderr(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         char const *data, size_t len);

  static void SendStderr(kanon::TcpConnectionPtr const &conn,
                         FcgiRequest const &request, char const *data,
                         size_t len)
  {
    SendStderr(conn, request.request_id, data, len);
  }

  static void SendStderr(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         kanon::StringView data)
  {
    SendStderr(conn, id, data.data(), data.size());
  }

  static void SendStderr(kanon::TcpConnectionPtr const &conn,
                         FcgiRequest const &request, kanon::StringView data)
  {
    SendStderr(conn, request.request_id, data);
  }

  static void SendStderr(kanon::TcpConnectionPtr const &conn, uint16_t id,
                         kanon::ChunkList &output);

  // static void SendStderr(kanon::TcpConnectionPtr const &conn, FcgiRequest
  // const&request,
  //                        kanon::ChunkList &output);
  /*-------------------*/
  /* Send terminator   */
  /*-------------------*/

  static void
  EndRequest(kanon::TcpConnectionPtr const &conn, uint16_t id,
             uint32_t app_status = 0,
             FcgiProtocolStatus protocol_status = FCGI_REQUEST_COMPLETE);

  static void
  EndRequest(kanon::TcpConnectionPtr const &conn, FcgiRequest const &request,
             uint32_t app_status = 0,
             FcgiProtocolStatus protocol_status = FCGI_REQUEST_COMPLETE)
  {
    EndRequest(conn, request.request_id, app_status, protocol_status);
  }

  static void EndStdout(kanon::TcpConnectionPtr const &conn, uint16_t id);
  static void EndStdout(kanon::TcpConnectionPtr const &conn,
                        FcgiRequest const &request)
  {
    EndStdout(conn, request.request_id);
  }

  static void EndStderr(kanon::TcpConnectionPtr const &conn, uint16_t id);
  static void EndStderr(kanon::TcpConnectionPtr const &conn,
                        FcgiRequest const &request)
  {
    EndStderr(conn, request.request_id);
  }

  /*-----------------------*/
  /* Connection management */
  /*-----------------------*/

  static void Close(kanon::TcpConnectionPtr const &conn,
                    FcgiRequest const *request);

  /*-----------------------*/
  /* Handler register      */
  /*-----------------------*/

  void SetRequestHandler(RequestHandler handler)
  {
    request_handler_ = std::move(handler);
  }

 private:
  int ParseRequest(kanon::TcpConnectionPtr const &conn, kanon::Buffer &buffer);

  bool ParseParams(RequestData &data);

  void RemoveRequest(uint16_t request_id);

  /**
   * Because FasgCgi allow interleaved request,
   * and the request may process complete asynchronously.
   *
   * We must store the request in a associated container.
   */
  RequestMap request_map_;

  /**
   * Handle request
   * The request owner is transfered to the handler(By move).
   */
  RequestHandler request_handler_;
};

using FcgiRequest = FcgiCodec::RequestData;

} // namespace fcgi
#endif // FCGI_CODEC_H_
