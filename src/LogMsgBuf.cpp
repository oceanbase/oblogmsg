/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include <cstdint>
#include <cstring>
#include "MsgType.h"
#include "LogMsgBuf.h"
#include "BinLogBuf.h"
#include "MsgHeader.h"
#include "Endian.h"

namespace oceanbase {
namespace logmessage {

#define LogMsgHeadSize (sizeof(MsgHeader) + sizeof(COUNT_TYPE))
#define DefaultLogMsgBufSize 1024 * 1024 * 32
#define LOG_MSG_MAX_TAIL_SIZE 1024

inline void LogMsgBuf::checkBuf(size_t size, char*& pos, STRLEN_TYPE*& s, char*& head)
{
  if (bufSize < size + (pos - buf) + LOG_MSG_MAX_TAIL_SIZE) {
    char* tmp = new char[bufSize = (pos - buf) + 2 * (size) + LOG_MSG_MAX_TAIL_SIZE];
    memcpy(tmp, buf, pos - buf);
    pos = tmp + (pos - buf);
    s = (STRLEN_TYPE*)(tmp + ((char*)s - buf));
    head = tmp + ((char*)(head)-buf);
    if (buf != defaultBuf)
      delete[] buf;
    buf = tmp;
  }
}

LogMsgBuf::LogMsgBuf()
{
  buf = new char[bufSize = DefaultLogMsgBufSize];
  defaultBuf = buf;
  bufPos = 0;
}

LogMsgBuf::~LogMsgBuf()
{
  if (buf != NULL)
    delete[] buf;
  if (defaultBuf != buf && defaultBuf !=NULL)
    delete[] defaultBuf;
  buf = NULL;
  defaultBuf = NULL;
  bufPos = 0;
}

const char* LogMsgBuf::getValueByOffset(size_t offset)
{
  return buf + offset + LogMsgHeadSize + sizeof(DT_TYPE) + sizeof(COUNT_TYPE);
}

size_t LogMsgBuf::appendString(const char* string, size_t size)
{
  size_t offset = bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_STRING);
  if (string == NULL) {
    (*(DT_TYPE*)(buf + bufPos)) |= DC_NULL;
    return (bufPos += sizeof(DT_TYPE));
  }
  *(STRLEN_TYPE*)(buf + bufPos + sizeof(DT_TYPE)) = size + 1;
  toLeEndian(buf + bufPos + sizeof(DT_TYPE), sizeof(STRLEN_TYPE));
  memcpy(buf + bufPos + sizeof(DT_TYPE) + sizeof(STRLEN_TYPE), string, size);
  bufPos += (sizeof(DT_TYPE) + sizeof(STRLEN_TYPE) + size) + 1;
  buf[bufPos - 1] = 0;
  return offset;
}
size_t LogMsgBuf::appendString(const std::string& string)
{
  size_t offset = bufPos - LogMsgHeadSize, size = string.size();
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_STRING);
  if (size == 0) {
    (*(DT_TYPE*)(buf + bufPos)) |= DC_NULL;
    return (bufPos += sizeof(DT_TYPE));
  }
  *(STRLEN_TYPE*)(buf + bufPos + sizeof(DT_TYPE)) = size + 1;
  toLeEndian(buf + bufPos + sizeof(DT_TYPE), sizeof(STRLEN_TYPE));
  strncpy(buf + bufPos + sizeof(DT_TYPE) + sizeof(STRLEN_TYPE), string.c_str(), size + 1);
  bufPos += (sizeof(DT_TYPE) + sizeof(STRLEN_TYPE) + size + 1);
  return offset;
}
size_t LogMsgBuf::appendBuf(const char* data, size_t size)
{
  size_t offset = bufPos - LogMsgHeadSize;
  memcpy(buf + bufPos, data, size);
  bufPos += size;
  return offset;
}
size_t LogMsgBuf::appendBuf(const std::string& string)
{
  size_t offset = bufPos - LogMsgHeadSize;
  memcpy(buf + bufPos, string.c_str(), string.size());
  bufPos += string.size();
  return offset;
}

size_t LogMsgBuf::appendBuf(const BinLogBuf* sa, size_t size)
{
  size_t offset = bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_STRING | DC_ARRAY);
  bufPos += sizeof(DT_TYPE);
  if (sa == NULL || size == 0) {
    *(COUNT_TYPE*)(buf + bufPos) = (COUNT_TYPE)(0);
    toLeEndian(buf + bufPos, sizeof(COUNT_TYPE));
    bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  *(COUNT_TYPE*)(buf + bufPos) = (COUNT_TYPE)(size);
  toLeEndian(buf + bufPos, sizeof(COUNT_TYPE));
  bufPos += sizeof(COUNT_TYPE);
  COUNT_TYPE i = 0;
  char *pos = buf + bufPos, *head;
  STRLEN_TYPE* s = (STRLEN_TYPE*)pos;
  pos += sizeof(STRLEN_TYPE) * (size + 1);
  head = pos;
  for (i = 0; i < size; ++i) {
    s[i] = pos - head;
    toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
    if (sa[i].buf != NULL) {
      checkBuf(sa[i].buf_used_size, pos, s, head);
      memcpy(pos, sa[i].buf, sa[i].buf_used_size);
      pos[sa[i].buf_used_size] = 0;
      pos += (sa[i].buf_used_size + 1);
    }
  }
  s[i] = pos - head;
  toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
  bufPos = pos - buf;
  return offset;
}
size_t LogMsgBuf::appendStringArray(std::vector<std::string*>& sa)
{
  size_t offset = bufPos - LogMsgHeadSize, size = sa.size();
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_STRING | DC_ARRAY);
  bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(buf + bufPos) = (COUNT_TYPE)(sa.size());
  toLeEndian(buf + bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(sa.size()) == 0) {
    bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  bufPos += sizeof(COUNT_TYPE);
  STRLEN_TYPE len;
  COUNT_TYPE i = 0;
  char *pos = buf + bufPos, *head;
  STRLEN_TYPE* s = (STRLEN_TYPE*)pos;
  pos += sizeof(STRLEN_TYPE) * (size + 1);
  head = pos;
  for (i = 0; i < size; ++i) {
    s[i] = pos - head;
    toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
    if (sa[i] != NULL) {
      len = sa[i]->size();
      checkBuf(len, pos, s, head);
      memcpy(pos, sa[i]->c_str(), len);
      pos[len] = 0;
      pos += (len + 1);
    }
  }
  s[i] = pos - head;
  toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
  bufPos = pos - buf;
  return offset;
}
size_t LogMsgBuf::appendStringArray(const char** sa, size_t size)
{
  size_t offset = bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_STRING | DC_ARRAY);
  bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(buf + bufPos) = (COUNT_TYPE)(size);
  toLeEndian(buf + bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(size) == 0) {
    bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  bufPos += sizeof(COUNT_TYPE);
  STRLEN_TYPE len;
  COUNT_TYPE i = 0;
  char *pos = buf + bufPos, *head;
  STRLEN_TYPE* s = (STRLEN_TYPE*)pos;
  pos += sizeof(STRLEN_TYPE) * (size + 1);
  head = pos;
  for (i = 0; i < size; ++i) {
    s[i] = pos - head;
    toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
    if (sa[i] != NULL) {
      len = strlen(sa[i]) + 1;
      checkBuf(len, pos, s, head);
      memcpy(pos, sa[i], len);
      pos += len;
    }
  }
  s[i] = pos - head;
  toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
  bufPos = pos - buf;
  return offset;
}
size_t LogMsgBuf::appendDataArray(std::vector<long>& sa)
{
  size_t offset = bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_INT64 | DC_ARRAY);
  bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(buf + bufPos) = (COUNT_TYPE)(sa.size());
  toLeEndian(buf + bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(sa.size()) == 0) {
    bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  bufPos += sizeof(COUNT_TYPE);
  COUNT_TYPE i = 0, j = sa.size();
  char* pos = buf + bufPos;
  pos += sizeof(STRLEN_TYPE) * i;
  for (i = 0; i < j; ++i) {
    *(long*)pos = sa[i];
    toLeEndian(pos, sizeof(long));
    pos += sizeof(long);
  }
  bufPos = pos - buf;
  return offset;
}

size_t LogMsgBuf::appendDataArray(uint8_t* sa, size_t size)
{
  size_t offset = bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(buf + bufPos) = (DT_TYPE)(DT_UINT8 | DC_ARRAY);
  bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(buf + bufPos) = (COUNT_TYPE)(size);
  toLeEndian(buf + bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(size) == 0 || sa == NULL) {
    bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  bufPos += sizeof(COUNT_TYPE);
  memcpy(buf + bufPos, sa, size);
  bufPos += size;
  return offset;
}
void LogMsgBuf::setHead(size_t size)
{
  bufPos = size + LogMsgHeadSize + sizeof(DT_TYPE) + sizeof(COUNT_TYPE);
  if (buf != defaultBuf) {
    if (avg_size > DefaultLogMsgBufSize) {
      if (avg_size < ((bufSize + DefaultLogMsgBufSize) >> 1)) {
        delete[] buf;
        buf = new char[bufSize = (bufSize + DefaultLogMsgBufSize) >> 1];
      }
    } else {
      delete[] buf;
      buf = defaultBuf;
      bufSize = DefaultLogMsgBufSize;
    }
  }
}
void LogMsgBuf::copyHead(const char* head, size_t size)
{
  ((struct MsgHeader*)buf)->m_msgType = MT_VAR;
  toLeEndian(&(((struct MsgHeader*)buf)->m_msgType), sizeof(uint16_t));
  ((struct MsgHeader*)buf)->m_version = 1;
  toLeEndian(&(((struct MsgHeader*)buf)->m_version), sizeof(uint16_t));
  ((struct MsgHeader*)buf)->m_size = bufPos - (sizeof(struct MsgHeader));
  toLeEndian(&(((struct MsgHeader*)buf)->m_size), sizeof(uint32_t));

  *(COUNT_TYPE*)(buf + sizeof(struct MsgHeader)) = 0;
  toLeEndian(buf + sizeof(struct MsgHeader), sizeof(COUNT_TYPE));
  *(buf + LogMsgHeadSize) = (DT_TYPE)(DT_UINT8 | DC_ARRAY);
  toLeEndian(buf + LogMsgHeadSize, sizeof(DT_TYPE));
  *(COUNT_TYPE*)(buf + LogMsgHeadSize + sizeof(DT_TYPE)) = size;
  toLeEndian(buf + LogMsgHeadSize + sizeof(DT_TYPE), sizeof(COUNT_TYPE));
  memcpy(buf + LogMsgHeadSize + sizeof(DT_TYPE) + sizeof(COUNT_TYPE), head, size);
}

void LogMsgBuf::froceSetHeadSize(size_t size)
{
  *(COUNT_TYPE*)(buf + LogMsgHeadSize + sizeof(DT_TYPE)) = size;
  toLeEndian(buf + LogMsgHeadSize + sizeof(DT_TYPE), sizeof(COUNT_TYPE));
}

const char* LogMsgBuf::getString(size_t* size)
{
  *size = bufPos;
  if (buf != defaultBuf && bufPos > DefaultLogMsgBufSize)
    avg_size = ((((long)avg_size >> 1) + (long)bufPos) >> 1) + (bufPos >> 2);
  else
    avg_size = (((long)avg_size + (long)bufPos) >> 1);
  return buf;
}

}  // namespace logmessage
}  // namespace oceanbase
