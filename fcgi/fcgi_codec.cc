#include "fcgi_codec.h"

#include "kanon/log/logger.h"
#include "kanon/net/tcp_connection.h"

using namespace kanon;
using namespace fcgi;

static constexpr int FCGI_RECORD_HEADER_LENGTH = sizeof(RecordHeader);
static constexpr int FCGI_BEGIN_REQUEST_BODY_LENGTH = sizeof(BeginRequestBody);
static constexpr int FCGI_END_REQUEST_BODY_LENGTH = sizeof(EndRequestBody);

static_assert(FCGI_RECORD_HEADER_LENGTH == 8, "");
static_assert(FCGI_BEGIN_REQUEST_BODY_LENGTH == 8, "");
static_assert(FCGI_END_REQUEST_BODY_LENGTH == 8, "");

#define PARSE_ERR -1
#define PARSE_SHORT 1
#define PARSE_OK 0
#define PARSE_OK_PARTLY 2

#define MAX_CONTENT_LENGTH 65535

FcgiCodec::FcgiCodec(TcpConnectionPtr const &conn)
{
  conn->SetMessageCallback(
      [this](TcpConnectionPtr const &conn, Buffer &buffer, TimeStamp) {
        for (;;) {
          switch (ParseRequest(conn, buffer)) {
            case PARSE_ERR:
              LOG_ERROR << "Parse error";
              return;
            case PARSE_SHORT:
              return;
          }
        }
      });
}

int FcgiCodec::ParseRequest(TcpConnectionPtr const &conn, Buffer &buffer)
{
  uint32_t required_length = 0;
  while (buffer.GetReadableSize() >= FCGI_RECORD_HEADER_LENGTH) {
    RecordHeader header;
    memcpy(&header, buffer.GetReadBegin(), FCGI_RECORD_HEADER_LENGTH);
    header.request_id = sock::ToHostByteOrder16(header.request_id);
    header.content_length = sock::ToHostByteOrder16(header.content_length);

    required_length = header.content_length + header.padding_length +
                      FCGI_RECORD_HEADER_LENGTH;
    if (buffer.GetReadableSize() < required_length) return PARSE_SHORT;

    LOG_TRACE << "Version: " << header.version;
    LOG_TRACE << "Type: " << FcgiType2String((FcgiType)header.type);
    LOG_TRACE << "Request id: " << header.request_id;
    LOG_TRACE << "Content length: " << header.content_length;
    LOG_TRACE << "Padding length: " << header.padding_length;

    buffer.AdvanceRead(FCGI_RECORD_HEADER_LENGTH);
    required_length -= FCGI_RECORD_HEADER_LENGTH;
    switch (header.type) {
      case FCGI_BEGIN_REQUEST:
      {
        BeginRequestBody body;
        memcpy(&body, buffer.GetReadBegin(), FCGI_BEGIN_REQUEST_BODY_LENGTH);
        body.role = sock::ToHostByteOrder32(body.role);
        LOG_TRACE << "Role: " << body.role;
        LOG_TRACE << "IsKeepConn: " << IsKeepConnection(body.flags);
        auto &request = request_map_[header.request_id];
        request.role = (FcgiRole)body.role;
        request.request_id = header.request_id;
        request.flags = body.flags;
      } break;

      case FCGI_ABORT_REQUEST:
      {

      } break;

      case FCGI_PARAMS:
      {
        auto &request = request_map_[header.request_id];
        if (header.content_length > 0) {
          request.param_stream.Append(buffer.GetReadBegin(),
                                      header.content_length);
        } else {
          /* Terminator of the FCGI_PARAMS */
          LOG_TRACE << "Terminator of PARAMS";
          ParseParams(request);
          buffer.AdvanceRead(header.padding_length);
          continue;
        }
      } break;

      case FCGI_STDIN:
      {
        auto &request = request_map_[header.request_id];
        if (header.content_length > 0) {
          request.stdin_stream.Append(buffer.GetReadBegin(),
                                      header.content_length);
        } else {
          /* Request complete, can process it */
          request.codec = this;
          request_handler_(conn, std::move(request));
          buffer.AdvanceRead(required_length);
          return PARSE_OK;
        }

      } break;

      case FCGI_DATA:
      {

      } break;

      /* Management record */
      case FCGI_GET_VALUES:
      {

      } break;
    }

    if (buffer.GetReadableSize() >= required_length) {
      LOG_TRACE << "advanced length = " << required_length;
      buffer.AdvanceRead(required_length);
    }
  }

  return PARSE_SHORT;
}

static uint32_t ReadNamePairLength(Buffer &buffer)
{
  assert(buffer.GetReadableSize() >= 1);
  auto len8 = buffer.GetReadBegin8();
  if ((len8 >> 7) == 1) {
    assert(buffer.GetReadableSize() >= 4);
    /* Clear the highest bit */
    return buffer.Read32() & 0x7fffffff;
  }
  return buffer.Read8();
}

bool FcgiCodec::ParseParams(RequestData &data)
{
  auto &stream = data.param_stream;
  uint32_t name_len = 0;
  uint32_t value_len = 0;
  std::string name;
  std::string value;
  while (stream.GetReadableSize() > 0) {
    name_len = ReadNamePairLength(stream);
    value_len = ReadNamePairLength(stream);
    LOG_TRACE << "name_len = " << name_len;
    LOG_TRACE << "value_len = " << value_len;
    name = stream.RetrieveAsString(name_len);
    value = stream.RetrieveAsString(value_len);
    LOG_TRACE << name << ": " << value;
    data.param_map[std::move(name)] = std::move(value);
  }

  return true;
}

static inline void SendStream(TcpConnectionPtr const &conn, FcgiType type,
                              uint16_t id, char const *data, size_t len)
{
  int count = len / MAX_CONTENT_LENGTH + 1;
  size_t has_written = 0;
  for (int i = 0; i < count; ++i) {
    ChunkList output;
    uint16_t clen =
        ((i == count - 1) ? (uint16_t)len : (uint16_t)MAX_CONTENT_LENGTH);
    RecordHeader header{
        .version = FCGI_VERSION_1,
        .type = type,
        .request_id = id,
        .content_length = clen,
        .padding_length = (uint8_t)(-clen & 7), /* completion of 8 */
        .reserved = 0,
    };

    LOG_TRACE << "ContentLength = " << header.content_length;
    LOG_TRACE << "PaddingLength = " << header.padding_length;

    header.content_length = sock::ToNetworkByteOrder16(header.content_length);
    header.request_id = sock::ToNetworkByteOrder16(header.request_id);

    output.Append(&header, FCGI_RECORD_HEADER_LENGTH);
    /* The header.content_length has converted to network byteorder */
    output.Append(data+has_written, clen);
    has_written += clen;
    LOG_TRACE << "has_written = " << has_written;
    output.Append("\0\0\0\0\0\0\0\0", header.padding_length);
    conn->Send(output);
    len -= clen;
  }
}
void FcgiCodec::SendStdout(TcpConnectionPtr const &conn, uint16_t id,
                           char const *data, size_t len)
{
  LOG_TRACE << "stdout";
  SendStream(conn, FCGI_STDOUT, id, data, len);
}

void FcgiCodec::SendStderr(TcpConnectionPtr const &conn, uint16_t id,
                           char const *data, size_t len)
{
  LOG_TRACE << "stderr";
  SendStream(conn, FCGI_STDERR, id, data, len);
}

static inline void EndTerminator(TcpConnectionPtr const &conn, FcgiType type,
                                 uint16_t id)
{
  LOG_TRACE << FcgiType2String(type);
  RecordHeader header{
      .version = FCGI_VERSION_1,
      .type = type,
      .request_id = id,
      .content_length = 0,
      .padding_length = 0,
      .reserved = 0,
  };

  LOG_TRACE << "request_id = " << header.request_id;
  header.request_id = sock::ToNetworkByteOrder16(header.request_id);

  conn->Send(&header, FCGI_RECORD_HEADER_LENGTH);
}

void FcgiCodec::EndRequest(TcpConnectionPtr const &conn, uint16_t id)
{
  LOG_TRACE << "Send END_REQUEST Message";
  LOG_TRACE << "request_id = " << id;

  RecordHeader header{
      .version = FCGI_VERSION_1,
      .type = FCGI_END_REQUEST,
      .request_id = sock::ToNetworkByteOrder16(id),
      .content_length =
          sock::ToNetworkByteOrder16(FCGI_END_REQUEST_BODY_LENGTH),
      .padding_length = 0,
      .reserved = 0,
  };


  ChunkList output;
  output.Append(&header, FCGI_RECORD_HEADER_LENGTH);

  // EndRequestBody body{0, FCGI_REQUEST_COMPLETE, {0, 0, 0}};
  // output.Append(&body, FCGI_END_REQUEST_BODY_LENGTH);
  output.Append32(0);
  output.Append32(0);
  conn->Send(output);
}

void FcgiCodec::EndStdout(TcpConnectionPtr const &conn, uint16_t id)
{
  EndTerminator(conn, FCGI_STDOUT, id);
}

void FcgiCodec::EndStderr(TcpConnectionPtr const &conn, uint16_t id)
{
  EndTerminator(conn, FCGI_STDERR, id);
}

FcgiCodec::RequestData::~RequestData() noexcept
{
  if (codec) codec->request_map_.erase(request_id);
}
