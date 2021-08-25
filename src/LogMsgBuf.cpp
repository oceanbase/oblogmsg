#include <cstdint>
#include <cstring>
#include "MsgType.h"
#include "LogMsgBuf.h"
#include "BinLogBuf.h"
#include "MsgHeader.h"
#include "Endian.h"

namespace oceanbase {
namespace logmessage {

typedef struct _LogMsgBuf {
  char* buf;
  size_t bufSize;
  size_t bufPos;
  char* defaultBuf;
  size_t avg_size;
} LogMsgBuf;
#define LogMsgHeadSize (sizeof(MsgHeader) + sizeof(COUNT_TYPE))
#define DefaultLogMsgBufSize 1024 * 1024 * 32
#define LOG_MSG_MAX_TAIL_SIZE 1024

static LogMsgBuf lmb_global = {NULL, 0, 0, NULL, 0};
static __thread LogMsgBuf* lmb = &lmb_global;

static inline void lm_check_buf(LogMsgBuf* lmbuf, size_t size, char*& pos, STRLEN_TYPE*& s, char*& head)
{
  if (lmbuf->bufSize < size + (pos - lmbuf->buf) + LOG_MSG_MAX_TAIL_SIZE) {
    char* tmp = new char[lmbuf->bufSize = (pos - lmbuf->buf) + 2 * (size) + LOG_MSG_MAX_TAIL_SIZE];
    memcpy(tmp, lmbuf->buf, pos - lmbuf->buf);
    pos = tmp + (pos - lmbuf->buf);
    s = (STRLEN_TYPE*)(tmp + ((char*)s - lmbuf->buf));
    head = tmp + ((char*)(head)-lmbuf->buf);
    if (lmbuf->buf != lmbuf->defaultBuf)
      delete[] lmbuf->buf;
    lmbuf->buf = tmp;
  }
}

int LogMsgLocalInit()
{
  LogMsgLocalDestroy();
  if (NULL == (lmb = new LogMsgBuf))
    return -1;
  if (NULL == (lmb->buf = new char[lmb->bufSize = DefaultLogMsgBufSize])) {
    delete lmb;
    lmb = &lmb_global;
    return -1;
  }
  lmb->defaultBuf = lmb->buf;
  lmb->bufPos = 0;
  return 0;
}

void LogMsgLocalDestroy()
{
  if (lmb == &lmb_global)
    return;

  if (lmb != NULL) {
    if (lmb->buf != NULL)
      delete[] lmb->buf;
    if (lmb->defaultBuf != lmb->buf && lmb->defaultBuf != NULL)
      delete[] lmb->defaultBuf;
    delete lmb;
    lmb = &lmb_global;
  }
}

int LogMsgInit()
{
  if (lmb != &lmb_global) {
    LogMsgLocalDestroy();
    lmb = &lmb_global;
  }
  LogMsgDestroy();
  if (NULL == (lmb_global.buf = new char[lmb_global.bufSize = DefaultLogMsgBufSize]))
    return -1;
  lmb_global.defaultBuf = lmb_global.buf;
  lmb_global.bufPos = 0;
  return 0;
}

void LogMsgDestroy()
{
  if (lmb != &lmb_global)
    return;
  if (lmb->buf != NULL)
    delete[] lmb->buf;
  if (lmb->defaultBuf != lmb->buf && lmb->defaultBuf != NULL)
    delete[] lmb->defaultBuf;
  lmb->buf = lmb->defaultBuf = NULL;
}

const char* LogMsgGetValueByOffset(size_t offset)
{
  return lmb->buf + offset + LogMsgHeadSize + sizeof(DT_TYPE) + sizeof(COUNT_TYPE);
}

size_t LogMsgAppendString(const char* string, size_t size)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_STRING);
  if (string == NULL) {
    (*(DT_TYPE*)(lmb->buf + lmb->bufPos)) |= DC_NULL;
    return (lmb->bufPos += sizeof(DT_TYPE));
  }
  *(STRLEN_TYPE*)(lmb->buf + lmb->bufPos + sizeof(DT_TYPE)) = size + 1;
  toLeEndian(lmb->buf + lmb->bufPos + sizeof(DT_TYPE), sizeof(STRLEN_TYPE));
  memcpy(lmb->buf + lmb->bufPos + sizeof(DT_TYPE) + sizeof(STRLEN_TYPE), string, size);
  lmb->bufPos += (sizeof(DT_TYPE) + sizeof(STRLEN_TYPE) + size) + 1;
  lmb->buf[lmb->bufPos - 1] = 0;
  return offset;
}
size_t LogMsgAppendString(const std::string& string)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize, size = string.size();
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_STRING);
  if (size == 0) {
    (*(DT_TYPE*)(lmb->buf + lmb->bufPos)) |= DC_NULL;
    return (lmb->bufPos += sizeof(DT_TYPE));
  }
  *(STRLEN_TYPE*)(lmb->buf + lmb->bufPos + sizeof(DT_TYPE)) = size + 1;
  toLeEndian(lmb->buf + lmb->bufPos + sizeof(DT_TYPE), sizeof(STRLEN_TYPE));
  strncpy(lmb->buf + lmb->bufPos + sizeof(DT_TYPE) + sizeof(STRLEN_TYPE), string.c_str(), size + 1);
  lmb->bufPos += (sizeof(DT_TYPE) + sizeof(STRLEN_TYPE) + size + 1);
  return offset;
}
size_t LogMsgAppendBuf(const char* data, size_t size)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  memcpy(lmb->buf + lmb->bufPos, data, size);
  lmb->bufPos += size;
  return offset;
}
size_t LogMsgAppendBuf(const std::string& string)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  memcpy(lmb->buf + lmb->bufPos, string.c_str(), string.size());
  lmb->bufPos += string.size();
  return offset;
}

size_t LogMsgAppendBuf(const BinLogBuf* sa, size_t size)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_STRING | DC_ARRAY);
  lmb->bufPos += sizeof(DT_TYPE);
  if (sa == NULL || size == 0) {
    *(COUNT_TYPE*)(lmb->buf + lmb->bufPos) = (COUNT_TYPE)(0);
    toLeEndian(lmb->buf + lmb->bufPos, sizeof(COUNT_TYPE));
    lmb->bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  *(COUNT_TYPE*)(lmb->buf + lmb->bufPos) = (COUNT_TYPE)(size);
  toLeEndian(lmb->buf + lmb->bufPos, sizeof(COUNT_TYPE));
  lmb->bufPos += sizeof(COUNT_TYPE);
  COUNT_TYPE i = 0;
  char *pos = lmb->buf + lmb->bufPos, *head;
  STRLEN_TYPE* s = (STRLEN_TYPE*)pos;
  pos += sizeof(STRLEN_TYPE) * (size + 1);
  head = pos;
  for (i = 0; i < size; ++i) {
    s[i] = pos - head;
    toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
    if (sa[i].buf != NULL) {
      lm_check_buf(lmb, sa[i].buf_used_size, pos, s, head);
      memcpy(pos, sa[i].buf, sa[i].buf_used_size);
      pos[sa[i].buf_used_size] = 0;
      pos += (sa[i].buf_used_size + 1);
    }
  }
  s[i] = pos - head;
  toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
  lmb->bufPos = pos - lmb->buf;
  return offset;
}
size_t LogMsgAppendStringArray(std::vector<std::string*>& sa)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize, size = sa.size();
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_STRING | DC_ARRAY);
  lmb->bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(lmb->buf + lmb->bufPos) = (COUNT_TYPE)(sa.size());
  toLeEndian(lmb->buf + lmb->bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(sa.size()) == 0) {
    lmb->bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  lmb->bufPos += sizeof(COUNT_TYPE);
  STRLEN_TYPE len;
  COUNT_TYPE i = 0;
  char *pos = lmb->buf + lmb->bufPos, *head;
  STRLEN_TYPE* s = (STRLEN_TYPE*)pos;
  pos += sizeof(STRLEN_TYPE) * (size + 1);
  head = pos;
  for (i = 0; i < size; ++i) {
    s[i] = pos - head;
    toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
    if (sa[i] != NULL) {
      len = sa[i]->size();
      lm_check_buf(lmb, len, pos, s, head);
      memcpy(pos, sa[i]->c_str(), len);
      pos[len] = 0;
      pos += (len + 1);
    }
  }
  s[i] = pos - head;
  toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
  lmb->bufPos = pos - lmb->buf;
  return offset;
}
size_t LogMsgAppendStringArray(const char** sa, size_t size)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_STRING | DC_ARRAY);
  lmb->bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(lmb->buf + lmb->bufPos) = (COUNT_TYPE)(size);
  toLeEndian(lmb->buf + lmb->bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(size) == 0) {
    lmb->bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  lmb->bufPos += sizeof(COUNT_TYPE);
  STRLEN_TYPE len;
  COUNT_TYPE i = 0;
  char *pos = lmb->buf + lmb->bufPos, *head;
  STRLEN_TYPE* s = (STRLEN_TYPE*)pos;
  pos += sizeof(STRLEN_TYPE) * (size + 1);
  head = pos;
  for (i = 0; i < size; ++i) {
    s[i] = pos - head;
    toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
    if (sa[i] != NULL) {
      len = strlen(sa[i]) + 1;
      lm_check_buf(lmb, len, pos, s, head);
      memcpy(pos, sa[i], len);
      pos += len;
    }
  }
  s[i] = pos - head;
  toLeEndian(&(s[i]), sizeof(STRLEN_TYPE));
  lmb->bufPos = pos - lmb->buf;
  return offset;
}
size_t LogMsgAppendDataArray(std::vector<long>& sa)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_INT64 | DC_ARRAY);
  lmb->bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(lmb->buf + lmb->bufPos) = (COUNT_TYPE)(sa.size());
  toLeEndian(lmb->buf + lmb->bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(sa.size()) == 0) {
    lmb->bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  lmb->bufPos += sizeof(COUNT_TYPE);
  COUNT_TYPE i = 0, j = sa.size();
  char* pos = lmb->buf + lmb->bufPos;
  pos += sizeof(STRLEN_TYPE) * i;
  for (i = 0; i < j; ++i) {
    *(long*)pos = sa[i];
    toLeEndian(pos, sizeof(long));
    pos += sizeof(long);
  }
  lmb->bufPos = pos - lmb->buf;
  return offset;
}

size_t LogMsgAppendDataArray(uint8_t* sa, size_t size)
{
  size_t offset = lmb->bufPos - LogMsgHeadSize;
  *(DT_TYPE*)(lmb->buf + lmb->bufPos) = (DT_TYPE)(DT_UINT8 | DC_ARRAY);
  lmb->bufPos += sizeof(DT_TYPE);
  *(COUNT_TYPE*)(lmb->buf + lmb->bufPos) = (COUNT_TYPE)(size);
  toLeEndian(lmb->buf + lmb->bufPos, sizeof(COUNT_TYPE));
  if ((COUNT_TYPE)(size) == 0 || sa == NULL) {
    lmb->bufPos += sizeof(COUNT_TYPE);
    return offset;
  }
  lmb->bufPos += sizeof(COUNT_TYPE);
  memcpy(lmb->buf + lmb->bufPos, sa, size);
  lmb->bufPos += size;
  return offset;
}
void LogMsgSetHead(size_t size)
{
  lmb->bufPos = size + LogMsgHeadSize + sizeof(DT_TYPE) + sizeof(COUNT_TYPE);
  if (lmb->buf != lmb->defaultBuf) {
    if (lmb->avg_size > DefaultLogMsgBufSize) {
      if (lmb->avg_size < ((lmb->bufSize + DefaultLogMsgBufSize) >> 1)) {
        delete[] lmb->buf;
        lmb->buf = new char[lmb->bufSize = (lmb->bufSize + DefaultLogMsgBufSize) >> 1];
      }
    } else {
      delete[] lmb->buf;
      lmb->buf = lmb->defaultBuf;
      lmb->bufSize = DefaultLogMsgBufSize;
    }
  }
}
void LogMsgCopyHead(const char* head, size_t size)
{
  ((struct MsgHeader*)lmb->buf)->m_msgType = MT_VAR;
  toLeEndian(&(((struct MsgHeader*)lmb->buf)->m_msgType), sizeof(uint16_t));
  ((struct MsgHeader*)lmb->buf)->m_version = 1;
  toLeEndian(&(((struct MsgHeader*)lmb->buf)->m_version), sizeof(uint16_t));
  ((struct MsgHeader*)lmb->buf)->m_size = lmb->bufPos - (sizeof(struct MsgHeader));
  toLeEndian(&(((struct MsgHeader*)lmb->buf)->m_size), sizeof(uint32_t));

  *(COUNT_TYPE*)(lmb->buf + sizeof(struct MsgHeader)) = 0;
  toLeEndian(lmb->buf + sizeof(struct MsgHeader), sizeof(COUNT_TYPE));
  *(lmb->buf + LogMsgHeadSize) = (DT_TYPE)(DT_UINT8 | DC_ARRAY);
  toLeEndian(lmb->buf + LogMsgHeadSize, sizeof(DT_TYPE));
  *(COUNT_TYPE*)(lmb->buf + LogMsgHeadSize + sizeof(DT_TYPE)) = size;
  toLeEndian(lmb->buf + LogMsgHeadSize + sizeof(DT_TYPE), sizeof(COUNT_TYPE));
  memcpy(lmb->buf + LogMsgHeadSize + sizeof(DT_TYPE) + sizeof(COUNT_TYPE), head, size);
}

void LogMsgFroceSetHeadSize(size_t size)
{
  *(COUNT_TYPE*)(lmb->buf + LogMsgHeadSize + sizeof(DT_TYPE)) = size;
  toLeEndian(lmb->buf + LogMsgHeadSize + sizeof(DT_TYPE), sizeof(COUNT_TYPE));
}

const char* LogMsgGetString(size_t* size)
{
  *size = lmb->bufPos;
  if (lmb->buf != lmb->defaultBuf && lmb->bufPos > DefaultLogMsgBufSize)
    lmb->avg_size = ((((long)lmb->avg_size >> 1) + (long)lmb->bufPos) >> 1) + (lmb->bufPos >> 2);
  else
    lmb->avg_size = (((long)lmb->avg_size + (long)lmb->bufPos) >> 1);
  return lmb->buf;
}

}  // namespace logmessage
}  // namespace oceanbase
