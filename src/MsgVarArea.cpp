#include "MsgVarArea.h"
#include "MsgType.h"
#include "Endian.h"
#include <string>
#include <cstring>
#include <cstdint>

namespace oceanbase {
namespace logmessage {

#define VAR_MSG_VERSION 1

/*
 * V1 format:
 * <field-count>
 * <type of field><value of field>
 * <type of field><value of field>
 * ...
 * <type of field><value of field>
 */

// ------------ VarMsgArea -------------
class MsgStrArray : public StrArray {
public:
  MsgStrArray(const char*& saPtr, size_t& count, const OFFSET_TYPE*& offsets);
  virtual ~MsgStrArray();
  virtual size_t size();
  virtual int elementAt(int i, const char*& s, size_t& length);
  virtual const char* operator[](int i);

private:
  const char* m_saPtr;
  size_t m_count;
  const OFFSET_TYPE* m_offsets;
};

template size_t MsgVarArea::appendStringArray<std::vector<std::string*>>(std::vector<std::string*>&);
template size_t MsgVarArea::appendStringArray<std::vector<std::string>>(std::vector<std::string>&);
template size_t MsgVarArea::appendStringArray<std::vector<const char*>>(std::vector<const char*>&);

// -------- MsgVarArea -----------

MsgVarArea::MsgVarArea(bool creating)
    : m_count(0),
      m_ptr(NULL),
      m_size(0),
      m_creating(creating),
      m_parsedOK(false),
      m_areaHeader(NULL),
      m_areaPtr(NULL),
      m_areaSize(0),
      m_areaEnd(NULL)
{
  if (creating)
    m_data.append(sizeof(VarAreaHeader), STUFF_CHAR);  // reserved
}

MsgVarArea::MsgVarArea(const void* ptr, size_t size)
    : m_ptr(NULL),
      m_size(0),
      m_creating(false),
      m_parsedOK(false),
      m_areaHeader(NULL),
      m_areaPtr(NULL),
      m_areaSize(0),
      m_areaEnd(NULL)
{
  parse(ptr, size);
}

size_t MsgVarArea::append(const char* typeName, const void* ptr, size_t size)
{
  // type、val
  DT_TYPE valType = (DT_TYPE)MsgType::getValType(typeName);
  if (valType == DT_UNKNOWN)
    return (size_t)-1;
  size_t offset = m_data.length() - sizeof(VarAreaHeader);
  m_data.append((const char*)&valType, sizeof(DT_TYPE));
  m_data.append((const char*)ptr, size);
  m_count++;
  afterAppending();
  return offset;
}
size_t MsgVarArea::appendData(const std::string& s)
{
  size_t offset = m_data.length() - sizeof(VarAreaHeader);
  m_data.append(s);
  m_count++;
  afterAppending();
  return offset;
}

size_t MsgVarArea::appendString(const char* s, size_t length)
{
  // type、len、val
  DT_TYPE valType = DT_STRING;
  size_t offset = m_data.length() - sizeof(VarAreaHeader);
  if (s == NULL) {
    valType |= DC_NULL;
    m_data.append((const char*)&valType, sizeof(DT_TYPE));
  } else {
    STRLEN_TYPE len = (STRLEN_TYPE)(length);
    STRLEN_TYPE lenEndia = len;
    toLeEndian(&lenEndia, sizeof(STRLEN_TYPE));
    m_data.append((const char*)&valType, sizeof(DT_TYPE));
    m_data.append((const char*)&lenEndia, sizeof(STRLEN_TYPE));
    m_data.append((const char*)s, len);
  }
  m_count++;
  afterAppending();
  return offset;
}

size_t MsgVarArea::appendString(const char* s)
{
  return appendString(s, s == NULL ? 0 : strlen(s) + 1);
}

size_t MsgVarArea::appendString(std::string* s)
{
  return appendString(s == NULL ? NULL : s->c_str(), s == NULL ? 0 : s->length() + 1);
}
std::string MsgVarArea::createStringArrayData(
    const char** sa, COUNT_TYPE count, STRLEN_TYPE* slen, bool appendTail, bool appendAlway)
{
  // type、count、strOffsets、strings
  std::string data;
  DT_TYPE valType = (DT_TYPE)(DT_STRING | DC_ARRAY);
  data.append((const char*)&valType, sizeof(DT_TYPE));
  if (sa == NULL || count == 0) {
    data.append(sizeof(COUNT_TYPE), '\0');  // count == 0
    return data;
  }
  COUNT_TYPE countEndia = count;
  toLeEndian(&countEndia, sizeof(COUNT_TYPE));
  data.append((const char*)&countEndia, sizeof(COUNT_TYPE));
  OFFSET_TYPE* strOffsets = new OFFSET_TYPE[count + 1];
  OFFSET_TYPE* strOffsetsEndia = new OFFSET_TYPE[count + 1];
  OFFSET_TYPE strOffset = 0;

  // for strOffsets
  STRLEN_TYPE len;
  COUNT_TYPE i, j;
  for (i = 0; i < count; ++i) {
    strOffsets[i] = strOffset;
    strOffsetsEndia[i] = strOffset;
    toLeEndian(&(strOffsetsEndia[i]), sizeof(OFFSET_TYPE));
    strOffset += slen[i];
  }
  strOffsets[i] = strOffset;
  strOffsetsEndia[i] = strOffset;
  toLeEndian(&(strOffsetsEndia[i]), sizeof(OFFSET_TYPE));

  data.append((const char*)strOffsetsEndia, sizeof(OFFSET_TYPE) * (count + 1));
  for (i = 0, j = 1; i < count; ++i, ++j) {
    len = strOffsets[j] - strOffsets[i];
    if (len > 0) {
      if (appendTail) {
        data.append(sa[i], len - 1);
        data.append("\0", 1);
      } else
        data.append(sa[i], len);
    } else if (appendAlway)
      data.append(sa[i], len);
  }
  delete[] strOffsets;
  delete[] strOffsetsEndia;
  return data;
}
size_t MsgVarArea::appendStringArray(const BinLogBuf* sa, size_t size)
{
  // type、count、strOffsets、strings
  COUNT_TYPE i, count = (COUNT_TYPE)size;
  STRLEN_TYPE* slen = new STRLEN_TYPE[count];
  const char* strData[count];
  size_t offset = m_data.length() - sizeof(VarAreaHeader);

  for (i = 0; i < count; ++i) {
    if (sa[i].buf == NULL)
      slen[i] = 0;
    else
      slen[i] = (STRLEN_TYPE)(sa[i].buf_used_size + 1);
    if (slen[i] > 0)
      strData[i] = sa[i].buf;
  }
  m_data.append(createStringArrayData(strData, count, slen, true, false));
  if (sa != NULL && size != 0)
    m_count++;
  afterAppending();
  delete[] slen;
  return offset;
}
size_t MsgVarArea::appendStringArray(const char** sa, size_t size)
{
  size_t offset = m_data.length() - sizeof(VarAreaHeader);

  m_data.append(createStringArrayData(sa, size));
  if (sa != NULL && size != 0)
    m_count++;
  afterAppending();
  return offset;
}
std::string MsgVarArea::createStringArrayData(const char** sa, size_t size)
{
  // type、count、strOffsets、strings
  std::string data;
  COUNT_TYPE i, count = (COUNT_TYPE)size;
  STRLEN_TYPE* slen = new STRLEN_TYPE[count];

  for (i = 0; i < count; ++i) {
    if (sa[i] == NULL)
      slen[i] = 0;
    else
      slen[i] = (STRLEN_TYPE)(strlen(sa[i]) + 1);
  }
  data = createStringArrayData(sa, count, slen, false, false);
  delete[] slen;
  return data;
}

std::string MsgVarArea::createStringArrayData(std::vector<std::string*>& sa)
{
  // type、count、strOffsets、strings
  std::string data;
  COUNT_TYPE i, count = (COUNT_TYPE)sa.size();
  STRLEN_TYPE* slen = new STRLEN_TYPE[count];
  const char* strData[count];
  for (i = 0; i < count; ++i) {
    if (sa[i] == NULL)
      slen[i] = 0;
    else
      slen[i] = (STRLEN_TYPE)(sa[i]->length() + 1);
    if (slen[i] > 0)
      strData[i] = sa[i]->c_str();
  }
  data = createStringArrayData(strData, count, slen, false, false);
  delete[] slen;
  return data;
}

// deal type vector<string*> vector<string> vector<const char*>
template <typename T>
size_t MsgVarArea::appendStringArray(T& sa)
{
  // type、count、strOffsets、strings
  COUNT_TYPE count = (COUNT_TYPE)sa.size();
  size_t offset = m_data.length() - sizeof(VarAreaHeader);

  m_data.append(createStringArrayData(sa));
  if (count != 0)
    m_count++;
  afterAppending();
  return offset;
}

std::string MsgVarArea::createStringArrayData(std::vector<std::string>& sa)
{
  // type、count、strOffsets、strings
  std::string data;
  COUNT_TYPE i, count = (COUNT_TYPE)sa.size();
  STRLEN_TYPE* slen = new STRLEN_TYPE[count];
  const char* strData[count];

  for (i = 0; i < count; ++i) {
    slen[i] = (STRLEN_TYPE)(sa[i].length() + 1);
    strData[i] = sa[i].c_str();
  }
  data = createStringArrayData(strData, count, slen, false, true);
  delete[] slen;
  return data;
}
std::string MsgVarArea::createStringArrayData(std::vector<const char*>& sa)
{
  // type、count、strOffsets、strings
  std::string data;
  COUNT_TYPE i, count = (COUNT_TYPE)sa.size();
  STRLEN_TYPE* slen = new STRLEN_TYPE[count];
  const char* strData[count];
  for (i = 0; i < count; ++i) {
    if (sa[i] == NULL)
      slen[i] = 0;
    else
      slen[i] = (STRLEN_TYPE)(strlen(sa[i]) + 1);
    strData[i] = sa[i];
  }
  data = createStringArrayData(strData, count, slen, false, false);
  delete[] slen;
  return data;
}

size_t MsgVarArea::appendArray(const char* typeName, const void* a, size_t elSize, size_t size)
{
  // type、count、array
  DT_TYPE valType = MsgType::getValType(typeName);
  if (valType == DT_UNKNOWN)
    return (size_t)-1;
  size_t offset = m_data.length() - sizeof(VarAreaHeader);
  m_data.append(createArray(typeName, a, elSize, size));
  if (a != NULL && elSize != 0 && size != 0) {
    m_count++;
  }
  afterAppending();
  return offset;
}

std::string MsgVarArea::createArray(const char* typeName, const void* a, size_t elSize, size_t size)
{
  // type、count、array
  std::string data;
  size_t i = 0;

  DT_TYPE valType = MsgType::getValType(typeName);
  if (valType == DT_UNKNOWN)
    return std::string(" ");
  valType |= DC_ARRAY;
  COUNT_TYPE count = (COUNT_TYPE)size;
  toLeEndian(&count, sizeof(COUNT_TYPE));
  data.append((const char*)&valType, sizeof(DT_TYPE));
  if (a == NULL || elSize == 0 || size == 0) {
    data.append(sizeof(COUNT_TYPE), '\0');  // count
    return data;
  }
  data.append((const char*)&count, sizeof(COUNT_TYPE));

  char* aEndia = new char[elSize * size];
  memcpy(aEndia, (const char*)a, elSize * size);
  for (; i < size; i++) {
    toLeEndian(aEndia + i * elSize, elSize);
  }

  data.append((const char*)aEndia, elSize * size);
  delete[] aEndia;
  return data;
}
const std::string& MsgVarArea::getMessage()
{
  uint32_t size = m_data.length() - sizeof(MsgHeader);
  uint16_t mt_var = MT_VAR;
  uint16_t msg_version = VAR_MSG_VERSION;

  VarAreaHeader header = {
      {toLeEndianByType(mt_var), toLeEndianByType(msg_version), toLeEndianByType(size)}, toLeEndianByType(m_count)};
  m_data.replace(0, sizeof(header), (const char*)&header, sizeof(header));

  // to support fetching
  m_ptr = m_data.c_str();
  m_size = size;
  m_areaHeader = (VarAreaHeader*)m_ptr;
  m_areaPtr = (const char*)(m_areaHeader + 1);
  m_areaSize = m_data.length() - sizeof(VarAreaHeader);
  m_areaEnd = m_areaPtr + m_areaSize;
  m_parsedOK = true;

  return m_data;
}

void MsgVarArea::clear()
{
  m_ptr = NULL;
  m_size = 0;
  m_areaHeader = NULL;
  m_areaPtr = NULL;
  m_areaSize = 0;
  m_areaEnd = NULL;
  m_count = 0;
  m_data.clear();
  if (m_data.capacity() > 1024 * 20)
    std::string().swap(m_data);
  m_data.append(sizeof(VarAreaHeader), STUFF_CHAR);
}

size_t MsgVarArea::getRealSize()
{
  if (m_parsedOK)
    return m_areaSize + sizeof(VarAreaHeader);
  return m_data.length();
}

const void* MsgVarArea::getMsgBuf(size_t& size)
{
  size = m_size;
  return m_ptr;
}

int MsgVarArea::parse(const void* ptr, size_t size)
{

  clear();
  if (sizeof(MsgHeader) > size)
    return -1;
  m_areaHeader = (VarAreaHeader*)ptr;
  MsgHeader* msgHeader = (MsgHeader*)&(m_areaHeader->m_msgHeader);
  if (toLeEndianByType(msgHeader->m_size) + sizeof(MsgHeader) > size)
    return -2;
  if (toLeEndianByType(msgHeader->m_msgType) != MT_VAR)
    return -3;
  if (toLeEndianByType(msgHeader->m_version) > VAR_MSG_VERSION)
    return -4;

  m_areaPtr = (const char*)(m_areaHeader + 1);
  m_areaSize = toLeEndianByType(msgHeader->m_size) - (sizeof(VarAreaHeader) - sizeof(MsgHeader));
  m_areaEnd = m_areaPtr + m_areaSize;

  m_ptr = (const char*)ptr;
  m_size = size;
  m_parsedOK = true;
  return 0;
}

int MsgVarArea::copy(const void* ptr, size_t size)
{
  m_data.assign((const char*)ptr, size);
  return 0;
}

int MsgVarArea::getField(size_t offset, const void*& ptr, size_t& size)
{
  if (!m_parsedOK && !m_creating)
    return -1;
  if (offset < 0 || offset >= m_areaSize)
    return -2;
  const char* p = m_areaPtr + offset;
  DT_TYPE* t = (DT_TYPE*)p;
  if ((*t & DC_ARRAY) != 0) {
    if ((*t & DT_MASK) == DT_STRING) {
      const char* saPtr;
      size_t count;
      const OFFSET_TYPE* offsets;
      int ret = getStringArray(offset, saPtr, count, offsets);
      if (ret != 0)
        return ret;
      ptr = saPtr;
      size = toLeEndianByType(offsets[count]);
      return 0;
    }

    size_t elSize, count;
    const void* a;
    int ret = getArray(offset, a, elSize, count);
    if (ret != 0)
      return ret;
    ptr = a;
    size = elSize * count;
    return 0;
  }

  int vt = *t & DT_MASK;
  p += sizeof(DT_TYPE);
  ptr = p;
  switch (vt) {
    case DT_INT8:
    case DT_UINT8:
      size = sizeof(int8_t);
      break;
    case DT_INT16:
    case DT_UINT16:
      size = sizeof(int16_t);
      break;
    case DT_INT32:
    case DT_UINT32:
      size = sizeof(int32_t);
      break;
    case DT_INT64:
    case DT_UINT64:
      size = sizeof(int64_t);
      break;
    case DT_FLOAT:
      size = sizeof(float);
      break;
    case DT_DOUBLE:
      size = sizeof(double);
      break;
    case DT_STRING:
      if ((*t & DC_NULL) != 0) {
        size = 0;
        ptr = NULL;
      } else {
        STRLEN_TYPE len = *(STRLEN_TYPE*)p;
        toLeEndian(&len, sizeof(STRLEN_TYPE));
        size = len;
        ptr = p + sizeof(STRLEN_TYPE);
      }
      break;
    default:
      return -3;
  }

  if (p + size > m_areaEnd)
    return -4;
  return 0;
}

int MsgVarArea::getString(size_t offset, const char*& s, size_t& length)
{
  if (!m_parsedOK && !m_creating)
    return -1;
  if (offset < 0 || offset >= m_areaSize)
    return -2;
  const char* p = m_areaPtr + offset;
  DT_TYPE* t = (DT_TYPE*)p;
  if ((*t & DC_ARRAY) != 0)
    return -3;
  if ((*t & DC_NULL) != 0) {
    s = NULL;
    length = 0;
    return 0;
  }
  p += sizeof(DT_TYPE);
  if (p + sizeof(STRLEN_TYPE) >= m_areaEnd)
    return -4;
  STRLEN_TYPE len = *(STRLEN_TYPE*)p;
  toLeEndian(&len, sizeof(STRLEN_TYPE));
  p += sizeof(STRLEN_TYPE);
  if (p + len > m_areaEnd)
    return -5;
  s = p;
  length = len - 1;
  return 0;
}

StrArray* MsgVarArea::getStringArray(size_t offset)
{
  const char* saPtr;
  size_t count;
  const OFFSET_TYPE* offsets;
  int ret = getStringArray(offset, saPtr, count, offsets);
  if (ret != 0)
    return NULL;
  if (count == 0)
    return NULL;
  return new MsgStrArray(saPtr, count, offsets);
}

int MsgVarArea::getStringArray(size_t offset, const char*& saPtr, size_t& count, const OFFSET_TYPE*& offsets)
{
  if (!m_parsedOK && !m_creating)
    return -1;
  if (offset < 0 || offset >= m_areaSize)
    return -2;
  const char* p = m_areaPtr + offset;
  DT_TYPE* t = (DT_TYPE*)p;
  if ((*t & DC_ARRAY) == 0 || (*t & DT_MASK) != DT_STRING)
    return -3;
  p += sizeof(DT_TYPE);
  if (p + sizeof(COUNT_TYPE) > m_areaEnd)
    return -4;
  COUNT_TYPE Count = *(COUNT_TYPE*)p;
  toLeEndian(&Count, sizeof(COUNT_TYPE));
  count = Count;
  if (count == 0) {
    saPtr = NULL;
    offsets = NULL;
    return 0;
  }
  p += sizeof(COUNT_TYPE);
  if (p + (count + 1) * sizeof(OFFSET_TYPE) > m_areaEnd)
    return -5;
  OFFSET_TYPE* strOffsets = (OFFSET_TYPE*)p;
  OFFSET_TYPE strOffset;
  p += (count + 1) * sizeof(OFFSET_TYPE);
  strOffset = strOffsets[count];
  toLeEndian(&strOffset, sizeof(OFFSET_TYPE));
  if (p + strOffset > m_areaEnd)
    return -6;
  saPtr = p;
  offsets = strOffsets;
  return 0;
}
void MsgVarArea::getString(size_t offset, const int off, char*& v, size_t& size)
{
  const char* pos = m_areaPtr + offset;
  COUNT_TYPE count = *(COUNT_TYPE*)(pos + sizeof(DT_TYPE));
  toLeEndian(&count, sizeof(COUNT_TYPE));
  if (offset < 0 || offset >= m_areaSize || off < 0 || off > count) {
    v = NULL;
    size = 0;
    return;
  }
  OFFSET_TYPE* ot = (OFFSET_TYPE*)(pos + sizeof(DT_TYPE) + sizeof(COUNT_TYPE));
  if (toLeEndianByType(ot[off + 1]) <= toLeEndianByType(ot[off])) {
    v = NULL;
    size = 0;
    return;
  }
  v = (char*)ot + sizeof(OFFSET_TYPE) * (1 + count + toLeEndianByType(ot[off]));
  size = toLeEndianByType(ot[off + 1]) - toLeEndianByType(ot[off]) - 1;
}
int MsgVarArea::getBuf(size_t offset, const void*& a, size_t& size)
{
  const char* p = m_areaPtr + offset;
  COUNT_TYPE count = *(COUNT_TYPE*)(p + sizeof(DT_TYPE));
  toLeEndian(&count, sizeof(COUNT_TYPE));
  size = count;
  if (p + size > m_areaEnd)
    return -2;
  a = p + sizeof(DT_TYPE) + sizeof(COUNT_TYPE);
  return 0;
}
void MsgVarArea::getStringArrayData(uint32_t offset, const char*& v, size_t& size)
{
  v = m_areaPtr + offset;
  COUNT_TYPE cnt = *(COUNT_TYPE*)(v + sizeof(DT_TYPE));
  toLeEndian(&cnt, sizeof(COUNT_TYPE));
  if (cnt == 0) {
    v = NULL;
    size = 0;
    return;
  }
  OFFSET_TYPE* ot = (OFFSET_TYPE*)(v + sizeof(DT_TYPE) + sizeof(COUNT_TYPE));
  size = toLeEndianByType(ot[cnt]) + (cnt + 1) * sizeof(OFFSET_TYPE) + sizeof(DT_TYPE) + sizeof(COUNT_TYPE);
}
int MsgVarArea::getArray(size_t offset, const void*& a, size_t& elSize, size_t& size)
{

  if (!m_parsedOK && !m_creating)
    return -1;
  if (offset < 0 || offset >= m_areaSize)
    return -2;
  const char* p = m_areaPtr + offset;
  DT_TYPE* t = (DT_TYPE*)p;
  if ((*t & DC_ARRAY) == 0)
    return -3;
  p += sizeof(DT_TYPE);
  if (p + sizeof(COUNT_TYPE) > m_areaEnd)
    return -4;
  COUNT_TYPE count = *(COUNT_TYPE*)(p);
  toLeEndian(&count, sizeof(COUNT_TYPE));
  size = count;
  if (size == 0) {
    a = NULL;
    elSize = 0;
    return 0;
  }
  switch (*t & DT_MASK) {
    case DT_INT8:
    case DT_UINT8:
      elSize = sizeof(int8_t);
      break;
    case DT_INT16:
    case DT_UINT16:
      elSize = sizeof(int16_t);
      break;
    case DT_INT32:
    case DT_UINT32:
      elSize = sizeof(int32_t);
      break;
    case DT_INT64:
    case DT_UINT64:
      elSize = sizeof(int64_t);
      break;
    case DT_FLOAT:
      elSize = sizeof(float);
      break;
    case DT_DOUBLE:
      elSize = sizeof(double);
      break;
    default:
      return -5;
  }
  p += sizeof(COUNT_TYPE);
  if (p + elSize * size > m_areaEnd)
    return -6;
  a = p;
  return 0;
}

void MsgVarArea::afterAppending()
{
  m_areaPtr = m_data.c_str() + sizeof(VarAreaHeader);
  m_areaSize = m_data.length() - sizeof(VarAreaHeader);
  m_areaEnd = m_areaPtr + m_areaSize;
}

const char* MsgVarArea::getString(size_t offset)
{
  const char* s;
  size_t length;
  int ret = getString(offset, s, length);
  if (ret == 0)
    return s;
  return NULL;
}

// cz-string array wrapper
MsgStrArray::MsgStrArray(const char*& saPtr, size_t& count, const uint32_t*& offsets)
    : m_saPtr(saPtr), m_count(count), m_offsets(offsets)
{}

MsgStrArray::~MsgStrArray()
{}

size_t MsgStrArray::size()
{
  return m_count;
}

int MsgStrArray::elementAt(int i, const char*& s, size_t& length)
{
  if (i < 0 || (size_t)i >= m_count)
    return -1;
  length = toLeEndianByType(m_offsets[i + 1]) - toLeEndianByType(m_offsets[i]);
  s = (length > 0) ? m_saPtr + toLeEndianByType(m_offsets[i]) : NULL;
  return 0;
}

const char* MsgStrArray::operator[](int i)
{
  if (i < 0 || (size_t)i >= m_count)
    return NULL;
  size_t length = toLeEndianByType(m_offsets[i + 1]) - toLeEndianByType(m_offsets[i]);
  return (length > 0) ? m_saPtr + toLeEndianByType(m_offsets[i]) : NULL;
}

}  // namespace logmessage
}  // namespace oceanbase
