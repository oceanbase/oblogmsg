/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#ifndef _STR_ARRAY_H_
#define _STR_ARRAY_H_

#include <sys/types.h>

namespace oceanbase {
namespace logmessage {

class StrArray {
protected:
  StrArray() = default;

public:
  virtual ~StrArray() = default;
  virtual size_t size() = 0;
  virtual int elementAt(int i, const char*& s, size_t& length) = 0;
  virtual const char* operator[](int i) = 0;
};

}  // namespace logmessage
}  // namespace oceanbase

#endif
