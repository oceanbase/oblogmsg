/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#pragma once

#include <cstdint>

namespace oceanbase {
namespace logmessage {

enum {
  DT_UNKNOWN = 0x0000,
  DT_INT8 = 0x0001,
  DT_UINT8 = 0x0002,
  DT_INT16 = 0x0003,
  DT_UINT16 = 0x0004,
  DT_INT32 = 0x0005,
  DT_UINT32 = 0x0006,
  DT_INT64 = 0x0007,
  DT_UINT64 = 0x0008,
  DT_FLOAT = 0x0009,
  DT_DOUBLE = 0x000a,
  DT_STRING = 0x000b,
  TOTAL_DT,
  DT_MASK = 0x000F,

  DC_ARRAY = 0x0010,
  DC_NULL = 0x0020,
  DC_MASK = 0x0030,

};

const char STUFF_CHAR = '\0';
typedef uint8_t DT_TYPE;
typedef uint32_t STRLEN_TYPE;
typedef uint32_t COUNT_TYPE;
typedef uint32_t OFFSET_TYPE;

class MsgType {
public:
  static int getValType(const char* typeName);
};

}  // namespace logmessage
}  // namespace oceanbase

