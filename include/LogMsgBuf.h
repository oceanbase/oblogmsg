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

#include <vector>
#include <string>

using namespace std;

namespace oceanbase {
namespace logmessage {

struct BinLogBuf;
class LogMsgBuf {
public:
  LogMsgBuf();
  ~LogMsgBuf();
  const char* getValueByOffset(size_t offset);
  size_t appendString(const char* string, size_t size);
  size_t appendString(const std::string& string);
  size_t appendBuf(const char* data, size_t size);
  size_t appendBuf(const std::string& string);
  size_t appendBuf(const BinLogBuf* sa, size_t size);
  size_t appendStringArray(std::vector<std::string*>& sa);
  size_t appendStringArray(const char** sa, size_t size);
  void setHead(size_t size);
  void copyHead(const char* head, size_t size);
  void froceSetHeadSize(size_t size);
  const char* getString(size_t* size);
  size_t appendDataArray(std::vector<long>& sa);
  size_t appendDataArray(uint8_t* sa, size_t size);

private:
  inline void checkBuf(size_t size, char*& pos, uint32_t*& s, char*& head);

private:
  char* buf;
  size_t bufSize;
  size_t bufPos;
  char* defaultBuf;
  size_t avg_size;
};

}  // namespace logmessage
}  // namespace oceanbase
