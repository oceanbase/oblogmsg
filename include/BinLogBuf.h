#ifndef BINLOG_BUF_H_
#define BINLOG_BUF_H_
#define LOG_EVENT_INIT_LEN (1024 * 32)
#include <cstdlib>
#include <cstring>

namespace oceanbase {
namespace logmessage {

struct BinLogBuf {
  char* buf;
  size_t buf_size;
  size_t buf_used_size;
  bool needFree;
  BinLogBuf()
  {
    buf = NULL;
    buf_size = buf_used_size = 0;
    needFree = false;
  }
  BinLogBuf(char* binlog, size_t len)
  {
    buf = binlog;
    buf_size = len;
    buf_used_size = 0;
    needFree = false;
  }
  ~BinLogBuf()
  {
    if (buf != NULL && needFree == true)
      delete[] buf;
  }
};
static inline int set_binlogBuf(BinLogBuf* buf, char* binlog, size_t len)
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
  return 0;
}

static inline void exchange_binlogBuf(BinLogBuf* buf, char* binlog, size_t len, size_t buf_used_size = 0)
{
  if (buf->needFree == true)
    delete[] buf->buf;
  buf->buf = binlog;
  buf->buf_size = len;
  buf->buf_used_size = buf_used_size;
  buf->needFree = false;
}
static inline void exchange_binlogBuf(BinLogBuf* buf, BinLogBuf* ebuf)
{
  if (buf->needFree == true)
    delete[] buf->buf;

  if (ebuf == NULL) {
    buf->buf = NULL;
    buf->buf_size = buf->buf_used_size = 0;
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
}
static inline void get_binlogBuf(BinLogBuf* buf, char* binlog, size_t len)
{
  if (buf->needFree == true)
    delete[] buf->buf;
  buf->buf = binlog;
  buf->buf_used_size = buf->buf_size = len;
  buf->needFree = false;
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

#endif /* BINLOG_BUF_H_ */
