#include "fcgi/fcgi_codec.h"

#include "kanon/net/user_server.h"
#include "kanon/log/logger.h"

using namespace fcgi;
using namespace kanon;
using namespace std;

static string PATH = "/echo/";

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
              std::string buffer;
              buffer.reserve(4096);
              buffer.append("Context-Type: text/plain\r\n\r\n");
              StringView uri(request.param_map["REQUEST_URI"]);
              StringView message = uri.substr(PATH.size());
              buffer.append(message.data(), message.size());
              codec->SendStdout(conn, request, buffer.data(), buffer.size());
              // codec->SendStdout(conn, request.request_id, "Context-Type: text/plain\r\n\r\n");
              // codec->SendStdout(conn, request.request_id, message);
              // codec->SendStdout(conn, request.request_id, in.GetReadBegin(),
              //                   in.GetReadableSize());
              // in.AdvanceAll();
              codec->EndStdout(conn, request);
              codec->EndRequest(conn, request);
            });
        conn->SetContext(codec);
      } else {
        delete *AnyCast<FcgiCodec *>(conn->GetContext());
      }
    });
  }
  
  void Listen() { server_.StartRun(); }
  void SetLoopNum(int num) { server_.SetLoopNum(num); }
 private:
  TcpServer server_;
};

int main(int argc, char *argv[])
{
  // kanon::SetKanonLog(false);
  uint16_t port = 9999;
  int thread_num = 0;  
  std::vector<char const *> args;
  while (argc > 1) {
    if (strcmp(argv[argc-1], "-p") == 0) {
      if (args.size() != 1) {
        fprintf(stderr, "No port number");
        exit(0);
      }
      port = atoi(args[0]);
      args.clear();
    } else if (strcmp(argv[argc-1], "-s") == 0) {
      if (args.size() != 0) {
        fprintf(stderr, "silent mode no arguments");
        exit(0);
      }
      LOG_INFO << "Running in silent mode";
      kanon::EnableAllLog(false);
    } else if (strcmp(argv[argc-1], "-t") == 0) {
      if (args.size() != 1) {
        fprintf(stderr, "No thread num");
        exit(0);
      }
      thread_num = ::atoi(args[0]);
      args.clear();
    } else {
      args.emplace_back(argv[argc-1]);
    }
    argc--;
  }

  EventLoop loop;
  EchoCgiServer server(&loop, InetAddr(port));
  server.SetLoopNum(thread_num);
  server.Listen();

  loop.StartLoop();
}
