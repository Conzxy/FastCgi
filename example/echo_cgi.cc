#include "fcgi/fcgi_codec.h"

#include "kanon/net/user_server.h"
#include "kanon/log/logger.h"

using namespace fcgi;
using namespace kanon;

class EchoCgiServer : kanon::noncopyable {
 public:
  EchoCgiServer(EventLoop *loop, InetAddr const &addr)
    : server_(loop, addr, "EchoCgiServer")
  {
    server_.SetConnectionCallback([](TcpConnectionPtr const &conn) {
      if (conn->IsConnected()) {
        auto codec = new FcgiCodec(conn);
        codec->SetRequestHandler(
            [codec](TcpConnectionPtr const &conn, FcgiRequest request) {
              auto in = std::move(request.stdin_stream);
              codec->SendStdout(conn, request.request_id, "Context-Type: text/plain\r\n\r\n");
              codec->SendStdout(conn, request.request_id, in.GetReadBegin(),
                                in.GetReadableSize());
              in.AdvanceAll();
              codec->EndStdout(conn, request.request_id);
              codec->EndRequest(conn, request.request_id);
            });
        conn->SetContext(codec);
      } else {
        delete *AnyCast<FcgiCodec *>(conn->GetContext());
      }
    });
  }
  
  void Listen() { server_.StartRun(); }
 private:
  TcpServer server_;
};

int main(int argc, char *argv[])
{
  // kanon::SetKanonLog(false);
  uint16_t port = 9999;
  if (argc > 1) {
    port = ::atoi(argv[1]);
  }

  EventLoop loop;
  EchoCgiServer server(&loop, InetAddr(port));
  
  server.Listen();
  loop.StartLoop();
}
