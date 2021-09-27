/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#ifndef _MSG_VAR_AREA_H_
#define _MSG_VAR_AREA_H_

#include "StrArray.h"
#include "MsgType.h"
#include "MsgHeader.h"

#include <cstdio>
#include <string>
#include <vector>
#include <typeinfo>
#include "BinLogBuf.h"

namespace oceanbase {
namespace logmessage {
// ---------- Header ------------
struct VarAreaHeader {
  MsgHeader m_msgHeader;
  COUNT_TYPE m_count;
};

class MsgVarArea {
public:
  MsgVarArea(bool creating = true);
  MsgVarArea(const void* ptr, size_t size);
  ~MsgVarArea()
  {}

  // methods for creating
  template <typename T>
  size_t append(T val)
  {
    return append(typeid(T).name(), (const void*)&val, sizeof(T));
  }

  template <typename T>
  size_t appendStringArray(T& sa);

  size_t appendString(const char* s);
  size_t appendData(const std::string& s);
  static std::string createStringArrayData(
      const char** sa, COUNT_TYPE count, STRLEN_TYPE* slen, bool appendTail, bool appendAlway);

  size_t appendString(const char* s, size_t length);
  size_t appendString(std::string* s);
  size_t appendStringArray(const BinLogBuf* sa, size_t size);
  size_t appendStringArray(const char** sa, size_t size);
  static std::string createStringArrayData(const char** sa, size_t size);
  static std::string createStringArrayData(std::vector<std::string*>& sa);
  static std::string createStringArrayData(std::vector<std::string>& sa);
  static std::string createStringArrayData(std::vector<const char*>& sa);

  template <typename T>
  size_t appendArray(T* a, size_t size)
  {
    return appendArray(typeid(T).name(), (const void*)a, sizeof(T), size);
  }
  template <typename T>
  static std::string createArrayData(T* a, size_t size)
  {
    return createArray(typeid(T).name(), (const void*)a, sizeof(T), size);
  }
  const std::string& getMessage();

private:
  size_t append(const char* typeName, const void* ptr, size_t size);  // return offset
  size_t appendArray(const char* typeName, const void* a, size_t elSize, size_t count);
  static std::string createArray(const char* typeName, const void* a, size_t elSize, size_t count);
  void afterAppending();

public:
  // methods for fetching
  void clear();
  size_t getRealSize();
  const void* getMsgBuf(size_t& size);
  int parse(const void* ptr, size_t size);
  int copy(const void* ptr, size_t size);
  int getField(size_t offset, const void*& ptr, size_t& size);
  int getString(size_t offset, const char*& s, size_t& length);
  const char* getString(size_t offset);
  void getString(size_t offset, const int off, char*& v, size_t& size);
  StrArray* getStringArray(size_t offset);
  int getStringArray(size_t offset, const char*& saPtr, size_t& count, const OFFSET_TYPE*& offsets);
  int getArray(size_t offset, const void*& a, size_t& elSize, size_t& size);
  int getBuf(size_t offset, const void*& a, size_t& size);
  void getStringArrayData(unsigned int offset, const char*& v, size_t& size);

private:
  COUNT_TYPE m_count;
  std::string m_data;  // save header data
  const char* m_ptr;
  size_t m_size;
  bool m_creating;
  bool m_parsedOK;
  VarAreaHeader* m_areaHeader;
  const char* m_areaPtr;  // data begin
  size_t m_areaSize;      // data size
  const char* m_areaEnd;  // data end
};

}  // namespace logmessage
}  // namespace oceanbase

#endif
