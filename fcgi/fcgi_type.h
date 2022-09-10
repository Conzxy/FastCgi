#ifndef FCGI_TYPE_H_
#define FCGI_TYPE_H_

#include <stdint.h>

namespace fcgi {

struct RecordHeader {
  uint8_t version;
  uint8_t type;
  uint16_t request_id;
  uint16_t content_length;
  uint8_t padding_length;
  uint8_t reserved;
  /* ContentData[content_length]; */
  /* PaddingData[padding_length]; */
};


struct BeginRequestBody {
  uint16_t role;
  uint8_t flags;
  char reserved[5];
};

struct EndRequestBody {
  uint32_t app_status;
  uint8_t protocol_status;
  char reserved[3];
};

struct UnknownTypeBody {
  uint8_t type;
  char reserved[7];
};

struct UnknownTypeRecord {
  RecordHeader header;
  UnknownTypeBody body;
};

}  // namespace fcgi
#endif  // FCGI_TYPE_H_
