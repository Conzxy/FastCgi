#include "fcgi_constant.h"

using namespace fcgi;

char const *fcgi::FcgiType2String(FcgiType t) noexcept
{
  switch (t) {
    case FCGI_BEGIN_REQUEST:
      return "Begin request";
    case FCGI_ABORT_REQUEST:
      return "Abort request";
    case FCGI_END_REQUEST:
      return "End request";
    case FCGI_PARAMS:
      return "Params";
    case FCGI_STDIN:
      return "Stdin";
    case FCGI_STDOUT:
      return "Stdout";
    case FCGI_STDERR:
      return "Stderr";
    case FCGI_DATA:
      return "Data";
    case FCGI_GET_VALUES:
      return "Get values";
    case FCGI_GET_VALUES_RESULT:
      return "Get values result";
    case FCGI_UNKNOWN_TYPE:
      return "Unknown type";
  }

  return "Unknown type";
}

char const *fcgi::FcgiRole2String(FcgiRole r) noexcept
{
  switch (r) {
    case FCGI_RESPONDER:
      return "Responder";
    case FCGI_AUTHORIZER:
      return "Authorizer";
    case FCGI_FILTER:
      return "Filter";
  }
  return "Unknown role";
}

char const *fcgi::FcgiProtocolStatus2String(FcgiProtocolStatus ps) noexcept
{
  switch (ps) {
    case FCGI_REQUEST_COMPLETE:
      return "Request complete";
    case FCGI_CANT_MAX_CONN:
      return "Can't max conn";
    case FCGI_OVERLOADED:
      return "Overloaded";
    case FCGI_UNKNOWN_ROLE:
      return "Unknown role";
  }
  return "Unknown protocol status";
}

bool fcgi::IsKeepConnection(FcgiFlag flag) noexcept
{
  return (flag & FCGI_KEEP_CONN) == 1;
}
