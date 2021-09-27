/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

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
