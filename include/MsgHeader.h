#ifndef _MSG_HEADER_H_
#define _MSG_HEADER_H_

#include <cstdint>

namespace oceanbase {
namespace logmessage {

enum { MT_UNKNOWN = 0, MT_META, MT_FIXED, MT_VAR, MT_EXT };

struct MsgHeader {
  uint16_t m_msgType;
  uint16_t m_version;
  uint32_t m_size;
};

}  // namespace logmessage
}  // namespace oceanbase

#endif
