/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "MsgType.h"
#include <typeinfo>
#include <cstring>

namespace oceanbase {
namespace logmessage {

// typename => type
static struct {
  const char* tn;
  int vt;
} g_nameTypeMap[] = {{typeid(bool).name(), DT_INT8},
    {typeid(char).name(), DT_INT8},
    {typeid(int8_t).name(), DT_INT8},
    {typeid(uint8_t).name(), DT_UINT8},
    {typeid(int16_t).name(), DT_INT16},
    {typeid(uint16_t).name(), DT_UINT16},
    {typeid(int32_t).name(), DT_INT32},
    {typeid(uint32_t).name(), DT_UINT32},
    {typeid(int64_t).name(), DT_INT64},
    {typeid(uint64_t).name(), DT_UINT64},
    {typeid(long).name(), DT_INT64},
    {typeid(unsigned long).name(), DT_UINT64},
    {typeid(float).name(), DT_FLOAT},
    {typeid(double).name(), DT_DOUBLE},
    {0}};

int MsgType::getValType(const char* typeName)
{
  for (int i = 0; g_nameTypeMap[i].tn != NULL; ++i) {
    if (strcmp(g_nameTypeMap[i].tn, typeName) == 0) {
      return g_nameTypeMap[i].vt;
    }
  }
  return DT_UNKNOWN;
}

}  // namespace logmessage
}  // namespace oceanbase
