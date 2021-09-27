/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#ifndef __USER_DATA_H__
#define __USER_DATA_H__

namespace oceanbase {
namespace logmessage {

class UserDataInterface {
public:
  virtual ~UserDataInterface()
  {}
  virtual void setUserData(void* data) = 0;
  virtual void* getUserData() = 0;
};

}  // namespace logmessage
}  // namespace oceanbase

#endif
