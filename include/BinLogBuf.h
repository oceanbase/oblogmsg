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

#define LOG_EVENT_INIT_LEN (1024 * 32)
#include <cstdlib>
#include <cstring>
#include "ValueOrigin.h"

namespace oceanbase {
namespace logmessage {

struct BinLogBuf {
  char* buf;
  size_t buf_size;
  size_t buf_used_size;
  bool needFree;
  bool m_diff_val;
  VALUE_ORIGIN m_origin;
  BinLogBuf(bool diff_val = false, VALUE_ORIGIN origin = REDO)
  {
    buf = NULL;
    buf_size = buf_used_size = 0;
    needFree = false;
    m_diff_val = diff_val;
    m_origin = origin;
  }
  BinLogBuf(char* binlog, size_t len, bool diff_val = false, VALUE_ORIGIN origin = REDO)
  {
    buf = binlog;
    buf_size = len;
    buf_used_size = 0;
    needFree = false;
    m_diff_val = diff_val;
    m_origin = origin;
  }
  ~BinLogBuf()
  {
    if (buf != NULL && needFree == true)
      delete[] buf;
  }
};
static inline int set_binlogBuf(BinLogBuf* buf, char* binlog, size_t len, bool diff_val = false, VALUE_ORIGIN origin = REDO)
{
  bool needRealloc = true;
  if (buf->buf == NULL) {
    len > LOG_EVENT_INIT_LEN ? (buf->buf_size = len) : (buf->buf_size = LOG_EVENT_INIT_LEN);
  } else if (buf->buf_size < len) {
    buf->buf_size = len;
    delete[] buf->buf;
  } else if (LOG_EVENT_INIT_LEN > len && buf->buf_size > LOG_EVENT_INIT_LEN) {
    buf->buf_size = LOG_EVENT_INIT_LEN;
    delete[] buf->buf;
  } else
    needRealloc = false;

  if (needRealloc) {
    if ((buf->buf = new char[buf->buf_size]) == NULL)
      return -1;
  }

  buf->needFree = true;
  memcpy(buf->buf, binlog, len);
  buf->buf_used_size = len;
  buf->m_diff_val = diff_val;
  buf->m_origin = origin;
  return 0;
}

static inline void exchange_binlogBuf(BinLogBuf* buf, char* binlog, size_t len, size_t buf_used_size = 0, bool diff_val = false, VALUE_ORIGIN origin = REDO)
{
  if (buf->needFree == true)
    delete[] buf->buf;
  buf->buf = binlog;
  buf->buf_size = len;
  buf->buf_used_size = buf_used_size;
  buf->needFree = false;
  buf->m_diff_val = diff_val;
  buf->m_origin = origin;
}
static inline void exchange_binlogBuf(BinLogBuf* buf, BinLogBuf* ebuf)
{
  if (buf->needFree == true)
    delete[] buf->buf;

  if (ebuf == NULL) {
    buf->buf = NULL;
    buf->buf_size = buf->buf_used_size = 0;
    buf->m_diff_val = false;
    buf->m_origin = REDO;
    return;
  }
  buf->buf = ebuf->buf;
  ebuf->buf = NULL;
  buf->buf_size = ebuf->buf_size;
  ebuf->buf_size = 0;
  buf->buf_used_size = ebuf->buf_used_size;
  ebuf->buf_used_size = 0;
  buf->needFree = ebuf->needFree;
  ebuf->needFree = false;
  buf->m_diff_val = ebuf->m_diff_val;
  ebuf->m_diff_val = false;
  buf->m_origin = ebuf->m_origin;
  ebuf->m_origin = REDO;
}
static inline void get_binlogBuf(BinLogBuf* buf, char* binlog, size_t len, bool diff_val = false, VALUE_ORIGIN origin = REDO)
{
  if (buf->needFree == true)
    delete[] buf->buf;
  buf->buf = binlog;
  buf->buf_used_size = buf->buf_size = len;
  buf->needFree = false;
  buf->m_diff_val = diff_val;
  buf->m_origin = origin;
}
static inline int create_binlogBuf(BinLogBuf* buf, size_t len = LOG_EVENT_INIT_LEN)
{
  if (NULL == (buf->buf = new char[buf->buf_size = len]))
    return -1;
  buf->needFree = true;
  return (int)(buf->buf_used_size = 0);
}
static inline void clear_binlogBuf(BinLogBuf* buf)
{
  buf->buf_used_size = 0;
}
static inline bool binlogBuf_NULL(BinLogBuf* buf)
{
  return buf->buf == NULL;
}

}  // namespace logmessage
}  // namespace oceanbase
