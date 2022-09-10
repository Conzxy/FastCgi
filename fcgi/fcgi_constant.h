#ifndef FCGI_CONSTANT_H_
#define FCGI_CONSTANT_H_

#include <stdint.h>

namespace fcgi {

/* FCGI 1.1 */
#define FCGI_VERSION_1 1

/* Type field in the record header */
enum FcgiType : unsigned char {
  FCGI_BEGIN_REQUEST = 1,
  FCGI_ABORT_REQUEST,
  FCGI_END_REQUEST,
  FCGI_PARAMS,
  FCGI_STDIN,
  FCGI_STDOUT,
  FCGI_STDERR,
  FCGI_DATA,
  FCGI_GET_VALUES,
  FCGI_GET_VALUES_RESULT,
  FCGI_UNKNOWN_TYPE,
  FCGI_MAXTYPE = FCGI_UNKNOWN_TYPE,
};

/* Used for management record header */
#define FCGI_NULL_REQUEST_ID 0

/* Used for FCGI_END_REQUEST Body */
enum FcgiRole : unsigned char {
  FCGI_RESPONDER = 1,
  FCGI_AUTHORIZER,
  FCGI_FILTER,
};

using FcgiFlag = uint8_t;

/* flags in the FCGI_BEGIN_REQUEST Body.
 * flags & FCGI_KEEP_CONN == 0, close immediately 
 */
#define FCGI_KEEP_CONN 1

/* Protocol Status for FCGI_END_REQUEST Body */
enum FcgiProtocolStatus : unsigned char {
  FCGI_REQUEST_COMPLETE = 0,
  FCGI_CANT_MAX_CONN,
  FCGI_OVERLOADED,
  FCGI_UNKNOWN_ROLE,
};

char const *FcgiType2String(FcgiType t) noexcept;

char const *FcgiRole2String(FcgiRole r) noexcept;

char const *FcgiProtocolStatus2String(FcgiProtocolStatus ps) noexcept;

bool IsKeepConnection(FcgiFlag flag) noexcept;
}  // namespace fcgi

#endif  // FCGI_CONSTANT_H_
