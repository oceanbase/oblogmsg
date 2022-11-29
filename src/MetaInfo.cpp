/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

/**
 * MetaData.cpp
 *       MetaDataCollection FORMAT:
 *       MetaDataCollection HEAD
 *          DB1 head
 *             DB1-TBL1 HEAD
 *               DB1-TBL1-COL1
 *               DB1-TBL1-COL2
 *               ...
 *             DB1-TBL2 HEAD
 *             ...
 *          DB2 HEAD
 *          ...
 */
#include "MetaInfo.h"
#include "StrArray.h"
#include "MsgVarArea.h"
#include <unordered_map>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <algorithm>

namespace oceanbase {
namespace logmessage {

class StrHash {
public:
  size_t operator()(const std::string& s) const
  {
    unsigned int hash = 1315423911;
    const char* str = s.c_str();
    while (*str) {
      if (*str >= 'A' && *str <= 'Z')
        hash ^= ((hash << 5) + (*str++ + ('a' - 'A')) + (hash >> 2));
      else
        hash ^= ((hash << 5) + (*str++) + (hash >> 2));
    }
    return (hash & 0x7FFFFFFF);
  }
};

class IgnoreCaseComparator {
public:
  bool operator()(const std::string& s1, const std::string& s2) const
  {
    return (strcasecmp(s1.c_str(), s2.c_str()) == 0);
  }
};

/* In order to access data security, map indexes use string instead of char * */
typedef std::unordered_map<const std::string, int, StrHash, IgnoreCaseComparator> NameIndexMap;
typedef std::unordered_map<const std::string, ITableMeta*, StrHash, IgnoreCaseComparator> NameTblmetaMap;
typedef std::unordered_map<const std::string, IDBMeta*, StrHash, IgnoreCaseComparator> NameDbmetaMap;

// base class of ColMetaInfo, TableMetaInfo, BMetaInfo, ...
struct MetaInfo {
public:
  MetaInfo(size_t headerSize, bool creating)
      : m_header(NULL), m_headerSize(headerSize), m_creating(creating), m_parsedOK(false)
  {
    if (creating) {
      m_header = new uint8_t[headerSize];
      memset(m_header, -1, headerSize);
      m_data.appendArray(m_header, headerSize);
    }
  }

  ~MetaInfo()
  {
    if (m_creating)
      delete[] m_header;
  }

  int appendTo(std::string& s)
  {
    if (m_creating) {
      // replace header
      const std::string& m = m_data.getMessage();
      const void* v;
      size_t elSize, count;
      int ret = m_data.getArray(0, v, elSize, count);
      if (ret != 0 || elSize != sizeof(uint8_t) || count != m_headerSize)
        return -1;
      memcpy((void*)v, m_header, m_headerSize);

      m_parsedOK = true;  // to support fetching
      s.append(m);
      return 0;
    }
    return -2;
  }

  int parse(const void* ptr, size_t size)
  {
    if (m_creating)
      return -1;

    m_parsedOK = false;
    m_data.clear();
    if (0 != m_data.parse(ptr, size))
      return -2;

    return afterParsing();
  }

  size_t getRealSize()
  {
    return m_data.getRealSize();
  }

protected:
  int afterParsing()
  {
    const void* p;
    size_t elSize, count;
    int ret = m_data.getArray(0, p, elSize, count);
    if (ret != 0 || elSize != sizeof(uint8_t) || count != m_headerSize)
      return -3;

    m_header = (uint8_t*)p;
    m_parsedOK = true;
    return 0;
  }

public:
  MsgVarArea m_data;
  uint8_t* m_header;
  size_t m_headerSize;
  bool m_creating;
  bool m_parsedOK;
};
#define COL_FLAG_GENERATED 0x01             // col is generated
#define COL_FLAG_HIDDEN_ROWKEY (0x01 << 1)  // 0x02
#define COL_FLAG_PARTITIONED (0x01 << 2)    // 0x04
#define COL_FLAG_DEPENDENT (0x01 << 3)      // 0x08
// ---------------------ColMeta-----------------------
struct ColMetaHeader {
  uint32_t m_nameOffset;
  int32_t m_type;
  uint8_t m_signed;
  uint8_t m_isPK;
  uint8_t m_isRuleCol;
  uint8_t m_isUK;
  uint8_t m_notNull;
  uint8_t m_reserved;
  int64_t m_length;
  int32_t m_decimals;
  uint32_t m_defOffset;
  uint32_t m_encOffset;
  uint32_t m_enumSetOffset;
  uint8_t m_flag;
  int64_t m_precision;
  int64_t m_scale;
  uint32_t m_originTypeOffset;
};

struct ColMetaInfo : public MetaInfo {
  ColMetaHeader* m_colMetaHeader;
  int m_required;

  ColMetaInfo() : MetaInfo(sizeof(ColMetaHeader), true), m_colMetaHeader(NULL), m_required(-1)
  {
    m_colMetaHeader = (ColMetaHeader*)m_header;
    m_colMetaHeader->m_signed = true;
    m_colMetaHeader->m_isPK = false;
    m_colMetaHeader->m_isRuleCol = false;
    m_colMetaHeader->m_isUK = false;
    m_colMetaHeader->m_notNull = false;
    m_colMetaHeader->m_flag = 0;
    m_colMetaHeader->m_originTypeOffset = -1;
    m_colMetaHeader->m_precision = -1;
    m_colMetaHeader->m_scale = -1;
  }

  ColMetaInfo(const void* ptr, size_t size)
      : MetaInfo(sizeof(ColMetaHeader), false), m_colMetaHeader(NULL), m_required(-1)
  {
    parse(ptr, size);
  }

  ~ColMetaInfo()
  {}

  int parse(const void* ptr, size_t size)
  {
    int ret = MetaInfo::parse(ptr, size);
    if (ret == 0) {
      m_required = -1;
      m_colMetaHeader = (ColMetaHeader*)m_header;
      return 0;
    }
    return ret;
  }
};

IColMeta::IColMeta()
{
  m_col = new ColMetaInfo();
  m_userData = NULL;
}

IColMeta::IColMeta(const void* ptr, size_t size)
{
  m_col = new ColMetaInfo(ptr, size);
  m_userData = NULL;
}

IColMeta::~IColMeta()
{
  delete m_col;
}

const char* IColMeta::getName()
{
  return m_col->m_data.getString(m_col->m_colMetaHeader->m_nameOffset);
}

int IColMeta::getType()
{
  return m_col->m_colMetaHeader->m_type;
}

long IColMeta::getLength()
{
  return m_col->m_colMetaHeader->m_length;
}

const char* IColMeta::getOriginType()
{
  return m_col->m_data.getString(m_col->m_colMetaHeader->m_originTypeOffset);
}

long IColMeta::getPrecision()
{
  return m_col->m_colMetaHeader->m_precision;
}

long IColMeta::getScale()
{
  return m_col->m_colMetaHeader->m_scale;
}

bool IColMeta::isSigned()
{
  return m_col->m_colMetaHeader->m_signed;
}

bool IColMeta::isPK()
{
  return m_col->m_colMetaHeader->m_isPK;
}

bool IColMeta::isRuleCol()
{
  return m_col->m_colMetaHeader->m_isRuleCol;
}

bool IColMeta::isUK()
{
  return m_col->m_colMetaHeader->m_isUK;
}

bool IColMeta::isNotNull()
{
  return m_col->m_colMetaHeader->m_notNull;
}

int IColMeta::getDecimals()
{
  return m_col->m_colMetaHeader->m_decimals;
}

unsigned char IColMeta::getFlag()
{
  return m_col->m_colMetaHeader->m_flag;
}
const char* IColMeta::getDefault()
{
  return m_col->m_data.getString(m_col->m_colMetaHeader->m_defOffset);
}

const char* IColMeta::getEncoding()
{
  return m_col->m_data.getString(m_col->m_colMetaHeader->m_encOffset);
}

int IColMeta::getRequired()
{
  return m_col->m_required;
}

StrArray* IColMeta::getValuesOfEnumSet()
{
  return m_col->m_data.getStringArray(m_col->m_colMetaHeader->m_enumSetOffset);
}
bool IColMeta::isGenerated()
{
  return m_col->m_colMetaHeader->m_flag & COL_FLAG_GENERATED;
}
void IColMeta::setName(const char* name)
{
  m_col->m_colMetaHeader->m_nameOffset = m_col->m_data.appendString(name);
}

void IColMeta::setType(int type)
{
  m_col->m_colMetaHeader->m_type = type;
}

void IColMeta::setLength(long length)
{
  m_col->m_colMetaHeader->m_length = length;
}

void IColMeta::setOriginType(const char* origin)
{
  m_col->m_colMetaHeader->m_originTypeOffset = m_col->m_data.appendString(origin);
}

void IColMeta::setPrecision(long precision)
{
  m_col->m_colMetaHeader->m_precision = precision;
}

void IColMeta::setScale(long scale)
{
  m_col->m_colMetaHeader->m_scale = scale;
}

void IColMeta::setSigned(bool b)
{
  m_col->m_colMetaHeader->m_signed = b;
}

void IColMeta::setIsPK(bool b)
{
  m_col->m_colMetaHeader->m_isPK = b;
}

void IColMeta::setIsRuleCol(bool b)
{
  m_col->m_colMetaHeader->m_isRuleCol = b;
}

void IColMeta::setIsUK(bool b)
{
  m_col->m_colMetaHeader->m_isUK = b;
}

void IColMeta::setNotNull(bool b)
{
  m_col->m_colMetaHeader->m_notNull = b;
}

void IColMeta::setDecimals(int decimals)
{
  m_col->m_colMetaHeader->m_decimals = decimals;
}

void IColMeta::setDefault(const char* def)
{
  m_col->m_colMetaHeader->m_defOffset = m_col->m_data.appendString(def);
}

void IColMeta::setDefault(const char* def, size_t length)
{
  m_col->m_colMetaHeader->m_defOffset = m_col->m_data.appendString(def, length);
}

void IColMeta::setEncoding(const char* enc)
{
  m_col->m_colMetaHeader->m_encOffset = m_col->m_data.appendString(enc);
}

void IColMeta::setRequired(int required)
{
  m_col->m_required = required;
}

void IColMeta::setValuesOfEnumSet(std::vector<std::string>& v)
{
  m_col->m_colMetaHeader->m_enumSetOffset = m_col->m_data.appendStringArray(v);
}

void IColMeta::setValuesOfEnumSet(std::vector<const char*>& v)
{
  m_col->m_colMetaHeader->m_enumSetOffset = m_col->m_data.appendStringArray(v);
}

void IColMeta::setValuesOfEnumSet(const char** v, size_t size)
{
  m_col->m_colMetaHeader->m_enumSetOffset = m_col->m_data.appendStringArray(v, size);
}
void IColMeta::setFlag(unsigned char flag)
{
  m_col->m_colMetaHeader->m_flag = flag;
}
void IColMeta::setGenerated(bool Generated)
{
  if (Generated) {
    m_col->m_colMetaHeader->m_flag |= COL_FLAG_GENERATED;
  } else {
    // 0xffff - COL_FLAG_GENERATED to binary 0b11111110
    m_col->m_colMetaHeader->m_flag &= (0xffff - COL_FLAG_GENERATED);
  }
}
void IColMeta::setHiddenRowKey()
{
  m_col->m_colMetaHeader->m_flag |= COL_FLAG_HIDDEN_ROWKEY;
}
bool IColMeta::isHiddenRowKey()
{
  return m_col->m_colMetaHeader->m_flag & COL_FLAG_HIDDEN_ROWKEY;
}
void IColMeta::setPartitioned()
{
  m_col->m_colMetaHeader->m_flag |= COL_FLAG_PARTITIONED;
}
bool IColMeta::isPartitioned()
{
  return m_col->m_colMetaHeader->m_flag & COL_FLAG_PARTITIONED;
}
void IColMeta::setDependent()
{
  m_col->m_colMetaHeader->m_flag |= COL_FLAG_DEPENDENT;
}
bool IColMeta::isDependent()
{
  return m_col->m_colMetaHeader->m_flag & COL_FLAG_DEPENDENT;
}
int IColMeta::appendTo(std::string& s)
{
  return m_col->appendTo(s);
}

size_t IColMeta::getRealSize()
{
  return m_col->getRealSize();
}

int IColMeta::parse(const void* ptr, size_t size)
{
  return m_col->parse(ptr, size);
}

bool IColMeta::parsedOK()
{
  return m_col->m_parsedOK;
}

void IColMeta::setUserData(void* data)
{
  m_userData = data;
}
void* IColMeta::getUserData()
{
  return m_userData;
}

// --------------------TableMeta-------------------
struct TableMetaHeader {
  uint32_t m_nameOffset;
  uint8_t m_isDropped;
  uint32_t m_newNameOffset;
  uint8_t m_hasPK;
  uint8_t m_hasUK;
  uint8_t m_reserved;
  uint16_t m_colCount;
  uint32_t m_pksOffset;
  uint32_t m_uksOffset;
  uint32_t m_encOffset;
  uint32_t m_colsSize;
  uint32_t m_pkinfo;
  uint32_t m_ukinfo;
};

struct TableMetaInfo : public MetaInfo {
  TableMetaHeader* m_tblMetaHeader;
  std::vector<IColMeta*> m_cols;
  std::vector<std::string> m_colNames;
  std::vector<std::string> m_pkNames;
  std::vector<std::string> m_ukNames;
  int* m_pkIndice;
  int* m_ukIndice;
  NameIndexMap m_colsMap;
  IDBMeta* m_dbMeta;

  TableMetaInfo() : MetaInfo(sizeof(TableMetaHeader), true), m_tblMetaHeader(NULL), m_dbMeta(NULL)
  {
    m_tblMetaHeader = (TableMetaHeader*)m_header;
    m_tblMetaHeader->m_isDropped = false;
    m_tblMetaHeader->m_hasPK = false;
    m_tblMetaHeader->m_hasUK = false;
    m_tblMetaHeader->m_colCount = 0;
    m_tblMetaHeader->m_colsSize = 0;
    m_pkIndice = NULL;
    m_ukIndice = NULL;
  }

  TableMetaInfo(const void* ptr, size_t size)
      : MetaInfo(sizeof(TableMetaHeader), false), m_tblMetaHeader(NULL), m_dbMeta(NULL)
  {
    m_pkIndice = NULL;
    m_ukIndice = NULL;
    parse(ptr, size);
  }

  ~TableMetaInfo()
  {
    clear();
  }

  int appendTo(std::string& s)
  {
    if (!m_creating)
      return -11;

    // table header
    MetaInfo::appendTo(s);

    // merge all the cols
    size_t colCount = m_cols.size();
    for (size_t i = 0; i < colCount; ++i) {
      m_cols[i]->appendTo(s);
    }
    return 0;
  }

  int parse(const void* ptr, size_t size)
  {
    // parse table
    int ret = MetaInfo::parse(ptr, size);
    if (ret == 0) {
      clear();
      m_cols.clear();
      m_colsMap.clear();

      m_tblMetaHeader = (TableMetaHeader*)m_header;
      size_t colCount = m_tblMetaHeader->m_colCount;
      size_t dataSize = m_tblMetaHeader->m_colsSize;
      const size_t maxColCount = 5120;
      if (m_data.getRealSize() + dataSize > size || colCount > maxColCount)
        return -11;

      // parse col meta
      const char* p = (char*)ptr + m_data.getRealSize();
      for (size_t i = 0; i < colCount; ++i) {
        IColMeta* colMeta = new IColMeta(p, dataSize);
        if (!colMeta->parsedOK()) {
          delete colMeta;
          return -12;
        }
        size_t colSize = colMeta->getRealSize();
        if (colSize > dataSize) {
          delete colMeta;
          return -13;
        }
        m_colsMap[std::string(colMeta->getName())] = (int)m_cols.size();
        m_cols.push_back(colMeta);
        m_colNames.push_back(colMeta->getName());
        p += colSize;
        dataSize -= colSize;
      }
      return 0;
    }

    return ret;
  }

  size_t getRealSize()
  {
    if (!m_parsedOK) {
      size_t colCount = m_cols.size();
      m_tblMetaHeader->m_colCount = colCount;
      size_t colsSize = 0;
      for (size_t i = 0; i < colCount; ++i)
        colsSize += m_cols[i]->getRealSize();
      m_tblMetaHeader->m_colsSize = colsSize;
    }
    return m_data.getRealSize() + m_tblMetaHeader->m_colsSize;
  }

private:
  void clear()
  {
    for (size_t i = 0; i < m_cols.size(); ++i)
      if (m_cols[i] != NULL)
        delete m_cols[i];

    if (m_pkIndice) {
      delete[] m_pkIndice;
      m_pkIndice = NULL;
    }
    if (m_ukIndice) {
      delete[] m_ukIndice;
      m_ukIndice = NULL;
    }
  }
};

ITableMeta::ITableMeta()
{
  m_tbl = new TableMetaInfo();
  pthread_mutex_init(&m_mdMutex, NULL);
  m_userData = NULL;
  m_DataOk = false;
  m_hashColumnIdx = NULL;
  m_hashColumnSetted = false;
  m_hashFuncId = 0;
  m_hashColumnSize = 0;
  m_hashColumValueList = NULL;
  m_hashColumValueSizeList = NULL;
}

ITableMeta::ITableMeta(const void* ptr, size_t size)
{
  m_hashColumnIdx = NULL;
  m_hashColumnSize = 0;
  m_hashColumnSetted = false;
  m_hashFuncId = 0;
  m_hashColumValueList = NULL;
  m_hashColumValueSizeList = NULL;
  m_tbl = new TableMetaInfo(ptr, size);
  pthread_mutex_init(&m_mdMutex, NULL);
  m_userData = NULL;
  m_DataOk = false;
}

ITableMeta::~ITableMeta()
{
  if (m_tbl != NULL)
    delete m_tbl;
  if (m_hashColumnIdx != NULL)
    delete[] m_hashColumnIdx;
  if (m_hashColumValueList != NULL)
    delete[] m_hashColumValueList;
  if (m_hashColumValueSizeList != NULL)
    delete[] m_hashColumValueSizeList;
  m_hashColumnSize = 0;
  pthread_mutex_destroy(&m_mdMutex);
}

const char* ITableMeta::getName()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_nameOffset);
}

/* For partial, if the table has been dropped. */
bool ITableMeta::isDropped()
{
  return m_tbl->m_tblMetaHeader->m_isDropped;
}

/* For partial, the new name if renamed. */
const char* ITableMeta::getNewName()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_newNameOffset);
}

bool ITableMeta::hasPK()
{
  return m_tbl->m_tblMetaHeader->m_hasPK;
}

const char* ITableMeta::getPKs()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_pksOffset);
}

const int* ITableMeta::getPKs(int& size) const
{
  size = m_tbl->m_pkNames.size();
  return m_tbl->m_pkIndice;
}

bool ITableMeta::hasUK()
{
  return m_tbl->m_tblMetaHeader->m_hasUK;
}

const char* ITableMeta::getUKs()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_uksOffset);
}

const int* ITableMeta::getUKs(int& size) const
{
  size = m_tbl->m_ukNames.size();
  return m_tbl->m_ukIndice;
}

const char* ITableMeta::getEncoding()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_encOffset);
}

IDBMeta* ITableMeta::getDBMeta()
{
  return m_tbl->m_dbMeta;
}

void ITableMeta::setName(const char* name)
{
  m_tbl->m_tblMetaHeader->m_nameOffset = m_tbl->m_data.appendString(name);
}

/* For partial, if the table has been dropped. */
void ITableMeta::setDropped(bool value)
{
  m_tbl->m_tblMetaHeader->m_isDropped = value;
}

/* For partial, the new name if renamed. */
void ITableMeta::setNewName(const char* name)
{
  m_tbl->m_tblMetaHeader->m_newNameOffset = m_tbl->m_data.appendString(name);
}

void ITableMeta::setHasPK(bool b)
{
  m_tbl->m_tblMetaHeader->m_hasPK = b;
}

void ITableMeta::setHasUK(bool b)
{
  m_tbl->m_tblMetaHeader->m_hasUK = b;
}

void ITableMeta::setPKIndice(const std::vector<int>& indice)
{
  if (!indice.empty()) {
    if (m_tbl->m_pkIndice)
      delete[] m_tbl->m_pkIndice;

    int size = (int)indice.size();
    m_tbl->m_pkIndice = new int[size];
    for (int i = 0; i < size; i++)
      m_tbl->m_pkIndice[i] = indice[i];

    std::sort(m_tbl->m_pkIndice, m_tbl->m_pkIndice + size);
  }
  return;
}
/*
 * indexStr format is (0, 1, 2), indice is a vector of 0, 1, 2
 */
int ITableMeta::parseKeyIndex(std::string indexStr, std::vector<int>& indice)
{
  int ret = 0;
  if (indexStr.empty()) {
    // if indexStr is empty, return directly.
    return 0;
  }
  size_t size = indexStr.size();
  if (indexStr[0] != '(' || indexStr[size - 1] != ')') {
    // invalid format
    return -1;
  }

  char* dest = new char[size - 1];
  char* src = strncpy(dest, indexStr.c_str() + 1, size - 2);
  src[size - 2] = '\0';
  char *token, *save;
  do {
    token = strtok_r(src, ",", &save);
    src = NULL;
    if (token) {
      char* endptr = NULL;
      long num = strtol(token, &endptr, 10);
      if (endptr == token && num == 0) {
        // invalid format
        ret = -1;
        break;
      }
      indice.push_back(num);
    } else {
      break;
    }
  } while (true);
  delete[] dest;
  return ret;
}
/*
 * get pk or uk keys from pkinfo or ukinfo like (0, 1, 2......)
 */
int ITableMeta::getKeysFromInfo(std::string info, std::string& keys, StrArray* colNames)
{
  std::vector<int> indice;
  int ret = 0;
  ret = parseKeyIndex(info, indice);
  if (ret != 0) {
    return ret;
  }

  if (NULL == colNames) {
    return ret;
  }
  for (size_t i = 0; i < indice.size(); i++) {
    keys.append((*colNames)[indice[i]]);
    if (i != indice.size() - 1 && indice.size() != 1) {
      keys.append(",");
    }
  }

  return ret;
}

void ITableMeta::setPKs(const char* pks)
{
  m_tbl->m_tblMetaHeader->m_pksOffset = m_tbl->m_data.appendString(pks);
  char* cpks = new char[strlen(pks) + 1];
  char* src = strncpy(cpks, pks, strlen(pks) + 1);
  char *token, *save;
  do {
    token = strtok_r(src, ",", &save);
    src = NULL;
    if (token) {
      m_tbl->m_pkNames.push_back(std::string(token));
    } else {
      break;
    }
  } while (true);
  delete[] cpks;

  if (m_tbl->m_pkIndice)
    return;

  size_t size = m_tbl->m_pkNames.size();
  if (size > 0) {
    int i = 0, j;
    m_tbl->m_pkIndice = new int[size];
    std::vector<std::string>::iterator it = m_tbl->m_pkNames.begin();
    while (it != m_tbl->m_pkNames.end()) {
      j = 0;
      std::vector<std::string>::iterator it1 = m_tbl->m_colNames.begin();
      while (it1 != m_tbl->m_colNames.end()) {
        if (*it == (*it1)) {
          m_tbl->m_pkIndice[i++] = j;
          break;
        }
        it1++;
        j++;
      }
      it++;
    }

    // swap according to columns order
    for (size_t i = 0; i < size; i++) {
      for (size_t j = size - 1; j > i; j--) {
        if (m_tbl->m_pkIndice[j] < m_tbl->m_pkIndice[j - 1]) {
          int a = m_tbl->m_pkIndice[j];
          std::string s = m_tbl->m_pkNames[j];
          m_tbl->m_pkIndice[j] = m_tbl->m_pkIndice[j - 1];
          m_tbl->m_pkNames[j] = m_tbl->m_pkNames[j - 1];
          m_tbl->m_pkIndice[j - 1] = a;
          m_tbl->m_pkNames[j - 1] = s;
        }
      }
    }
  }
}

void ITableMeta::setUKs(const char* uks)
{
  m_tbl->m_tblMetaHeader->m_uksOffset = m_tbl->m_data.appendString(uks);
  char* cpks = new char[strlen(uks) + 1];
  char* src = strncpy(cpks, uks, strlen(uks) + 1);
  char *token, *save;
  do {
    token = strtok_r(src, ",", &save);
    src = NULL;
    if (token) {
      m_tbl->m_ukNames.push_back(std::string(token));
    } else {
      break;
    }
  } while (true);

  size_t size = m_tbl->m_ukNames.size();
  if (size > 0) {
    int i = 0, j;
    m_tbl->m_ukIndice = new int[size];
    std::vector<std::string>::iterator it = m_tbl->m_ukNames.begin();
    while (it != m_tbl->m_ukNames.end()) {
      j = 0;
      std::vector<std::string>::iterator it1 = m_tbl->m_colNames.begin();
      while (it1 != m_tbl->m_colNames.end()) {
        if (*it == (*it1)) {
          m_tbl->m_ukIndice[i++] = j;
          break;
        }
        it1++;
        j++;
      }
      it++;
    }
  }
  delete[] cpks;
}

std::vector<std::string>& ITableMeta::getPKColNames()
{
  return m_tbl->m_pkNames;
}

std::vector<std::string>& ITableMeta::getUKColNames()
{
  return m_tbl->m_ukNames;
}

// [TODO]
void ITableMeta::setEncoding(const char* enc)
{
  m_tbl->m_tblMetaHeader->m_encOffset = m_tbl->m_data.appendString(enc);
}

void ITableMeta::setDBMeta(IDBMeta* dbMeta)
{
  m_tbl->m_dbMeta = dbMeta;
}

std::vector<std::string>& ITableMeta::getColNames()
{
  return m_tbl->m_colNames;
}

IColMeta* ITableMeta::getCol(const char* colName)
{
  NameIndexMap& colsMap = m_tbl->m_colsMap;
  NameIndexMap::iterator i = colsMap.find(std::string(colName));
  if (i != colsMap.end())
    return m_tbl->m_cols[i->second];
  return NULL;
}

int ITableMeta::getColCount()
{
  return (int)m_tbl->m_cols.size();
}

IColMeta* ITableMeta::getCol(int index)
{
  int len = m_tbl->m_cols.size();
  if (index >= 0 && index < len)
    return m_tbl->m_cols[index];
  return NULL;
}

int ITableMeta::getColNum(const char* colName)
{
  NameIndexMap& colsMap = m_tbl->m_colsMap;
  NameIndexMap::iterator i = colsMap.find(std::string(colName));
  if (i != colsMap.end())
    return i->second;
  return -1;
}

int ITableMeta::append(const char* colName, IColMeta* colMeta)
{
  NameIndexMap& colsMap = m_tbl->m_colsMap;
  NameIndexMap::iterator i = colsMap.find(std::string(colName));
  if (i != colsMap.end())
    return -1;
  colsMap[std::string(colMeta->getName())] = (int)m_tbl->m_cols.size();
  m_tbl->m_cols.push_back(colMeta);
  m_tbl->m_colNames.push_back(colMeta->getName());
  return 0;
}

int ITableMeta::appendTo(std::string& s)
{
  return m_tbl->appendTo(s);
}

size_t ITableMeta::getRealSize()
{
  return m_tbl->getRealSize();
}

int ITableMeta::parse(const void* ptr, size_t size)
{
  return m_tbl->parse(ptr, size);
}

bool ITableMeta::parsedOK()
{
  return m_tbl->m_parsedOK;
}

void ITableMeta::setUserData(void* data)
{
  m_userData = data;
}

void* ITableMeta::getUserData()
{
  return m_userData;
}

void** ITableMeta::getUserDataPtr()
{
  return &m_userData;
}

int ITableMeta::getColIndex(const char* colName)
{
  NameIndexMap& colsMap = m_tbl->m_colsMap;
  NameIndexMap::iterator i = colsMap.find(std::string(colName));
  if (i != colsMap.end())
    return i->second;
  return -1;
}
const int* ITableMeta::getHashColumnIdx(int& hashCoumnCount, char**& hashValueList, size_t*& hashValueSizeList)
{
  if (m_hashColumnSetted) {
    hashCoumnCount = m_hashColumnSize;
    hashValueList = m_hashColumValueList;
    hashValueSizeList = m_hashColumValueSizeList;
    return m_hashColumnIdx;
  } else {
    hashCoumnCount = 0;
    hashValueList = NULL;
    hashValueSizeList = NULL;
    return NULL;
  }
}
bool ITableMeta::hashColumnSetted()
{
  return m_hashColumnSetted;
}

void ITableMeta::setPkinfo(const char* info)
{
  m_tbl->m_tblMetaHeader->m_pkinfo = m_tbl->m_data.appendString(info);
}
void ITableMeta::setHashColByPK()
{
  if (m_hashColumnIdx != NULL) {
    delete[] m_hashColumnIdx;
    m_hashColumnIdx = NULL;
    m_hashColumnSize = 0;
  }
  if (m_hashColumValueList != NULL) {
    delete[] m_hashColumValueList;
    m_hashColumValueList = NULL;
  }
  if (m_hashColumValueSizeList) {
    delete[] m_hashColumValueSizeList;
    m_hashColumValueSizeList = NULL;
  }
  const char* PKinfo = getPkinfo();
  char n;
  if (PKinfo == NULL || PKinfo[0] == '\0') {
    m_hashColumnSetted = true;  //
    return;
  }
  const char* pos = PKinfo;
  int count = 0;
  while (*pos != 0) {
    if (*pos == ',' || *pos == ')')
      count++;
    pos++;
  }
  pos = PKinfo;
  m_hashColumnIdx = new int[count];
  memset(m_hashColumnIdx, 0, sizeof(int) * count);
  m_hashColumnSize = 0;
  while ((n = *pos++) != 0) {
    if (n == '(')
      continue;
    while (n >= '0' && n <= '9') {
      m_hashColumnIdx[m_hashColumnSize] = m_hashColumnIdx[m_hashColumnSize] * 10 + n - '0';
      n = *pos++;
    }
    if (n == ',' || n == ')')
      m_hashColumnSize++;
    if (n == ')') {
      m_hashColumValueList = new char*[m_hashColumnSize + 1];
      memset(m_hashColumValueList, 0, sizeof(const char*) * (m_hashColumnSize + 1));
      m_hashColumValueSizeList = new size_t[m_hashColumnSize + 1];
      memset(m_hashColumValueSizeList, 0, sizeof(size_t) * (m_hashColumnSize + 1));
      m_hashColumnSetted = true;
      return;
    }
  }
}
int ITableMeta::setHashCol(std::vector<std::string>& hashColumns)
{
  int columnCount = hashColumns.size();
  if (m_hashColumnIdx != NULL) {
    delete[] m_hashColumnIdx;
    m_hashColumnIdx = NULL;
    m_hashColumnSize = 0;
  }
  if (m_hashColumValueList != NULL) {
    delete[] m_hashColumValueList;
    m_hashColumValueList = NULL;
  }
  if (m_hashColumValueSizeList) {
    delete[] m_hashColumValueSizeList;
    m_hashColumValueSizeList = NULL;
  }
  m_hashColumnIdx = new int[columnCount];
  memset(m_hashColumnIdx, 0, sizeof(int) * columnCount);
  for (int idx = 0; idx < columnCount; idx++) {
    int id = getColIndex(hashColumns[idx].c_str());
    if (id != -1)
      m_hashColumnIdx[idx] = id;
    else {
      delete[] m_hashColumnIdx;  // can not find column name in ITableMeta
      m_hashColumnIdx = NULL;
      return -1;
    }
  }
  m_hashColumnSize = columnCount;
  m_hashColumValueList = new char*[m_hashColumnSize + 1];
  memset(m_hashColumValueList, 0, sizeof(char*) * (m_hashColumnSize + 1));
  m_hashColumValueSizeList = new size_t[m_hashColumnSize + 1];
  memset(m_hashColumValueSizeList, 0, sizeof(size_t) * (m_hashColumnSize + 1));
  m_hashColumnSetted = true;
  return 0;
}
void ITableMeta::setHashFuncId(int id)
{
  m_hashFuncId = id;
}
int ITableMeta::getHashFuncId()
{
  return m_hashFuncId;
}

const char* ITableMeta::getPkinfo()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_pkinfo);
}

void ITableMeta::setUkinfo(const char* info)
{
  m_tbl->m_tblMetaHeader->m_ukinfo = m_tbl->m_data.appendString(info);
}

const char* ITableMeta::getUkinfo()
{
  return m_tbl->m_data.getString(m_tbl->m_tblMetaHeader->m_ukinfo);
}

void ITableMeta::trySerializeMetaDataAsMsgArea(std::vector<const char*>& extra_infos)
{
  if (m_DataOk == false) {
    pthread_mutex_lock(&m_mdMutex);
    if (m_DataOk == true) {
      pthread_mutex_unlock(&m_mdMutex);
      return;
    }

    m_colNameData = MsgVarArea::createStringArrayData(getColNames());
    int colcount = getColCount();
    std::vector<std::string> colencoding;
    std::vector<std::string> originTypes;
    for (int i = 0; i < colcount; i++) {
      IColMeta* colmeta = getCol(i);
      std::string colenc = colmeta->getEncoding();
      const char* originType = colmeta->getOriginType();
      colencoding.push_back(colenc);
      if (originType != NULL) {
        originTypes.push_back(originType);
      }
    }

    m_encodingData = MsgVarArea::createStringArrayData(colencoding);
    m_colOriginTypeData = MsgVarArea::createStringArrayData(originTypes);

    /**
     * Set the list of primary keys delimited by comma
     * TBD: use index instead of names which is useful when parsing
     */
    int count;
    const int* ki = getPKs(count);
    m_PkData = MsgVarArea::createArrayData(ki, count);

    /**
     * Set the list of unique keys delimited by comma
     * TBD: use index instead of names which is useful when parsing
     */
    ki = getUKs(count);
    m_UkData = MsgVarArea::createArrayData(ki, count);

    /**
     * Set unioned pk and uk with previous pk value remained
     */
    if (getPkinfo())
      extra_infos.push_back(getPkinfo());

    if (getUkinfo())
      extra_infos.push_back(getUkinfo());

    m_keyData = MsgVarArea::createStringArrayData(extra_infos);

    /**
     * Set the encoding for all columns in the record, so
     * the encoding of each column should be the same with
     * that of the record
     */

    /**
     * Extract and serialize the types of all columns related,
     * now every record would visit all columns to getch the
     * types
     * TBD: extract all types in advance to table meta
     */
    uint8_t* col_types = new uint8_t[getColCount()];
    uint8_t* col_flags = new uint8_t[getColCount()];
    uint8_t* col_not_null = new uint8_t[colcount];
    uint8_t* col_signed = new uint8_t[colcount];
    int32_t* col_decimals = new int32_t[colcount];
    int64_t* col_length = new int64_t[colcount];
    int64_t* col_precision = new int64_t[colcount];
    int64_t* col_scale = new int64_t[colcount];

    for (int i = 0; i < getColCount(); i++) {
      col_types[i] = (getCol(i)->getType());
      col_flags[i] = (getCol(i)->getFlag());
      col_not_null[i] = getCol(i)->isNotNull();
      col_signed[i] = getCol(i)->isSigned();
      col_decimals[i] = getCol(i)->getDecimals();
      col_length[i] = getCol(i)->getLength();
      col_precision[i] = getCol(i)->getPrecision();
      col_scale[i] = getCol(i)->getScale();
    }
    m_colTypeData = MsgVarArea::createArrayData(col_types, colcount);
    m_columnFlagData = MsgVarArea::createArrayData(col_flags, colcount);
    m_colNotNullData = MsgVarArea::createArrayData(col_not_null, colcount);
    m_colSignedData = MsgVarArea::createArrayData(col_signed, colcount);
    m_colDecimalsData = MsgVarArea::createArrayData(col_decimals, colcount);
    m_colLengthData = MsgVarArea::createArrayData(col_length, colcount);
    m_colPrecisionData = MsgVarArea::createArrayData(col_precision, colcount);
    m_colScaleData = MsgVarArea::createArrayData(col_scale, colcount);

    delete[] col_types;
    delete[] col_flags;
    delete[] col_not_null;
    delete[] col_signed;
    delete[] col_decimals;
    delete[] col_scale;
    delete[] col_length;
    delete[] col_precision;

    m_DataOk = true;
    pthread_mutex_unlock(&m_mdMutex);
  }
}

const std::string& ITableMeta::getNameData()
{
  return m_colNameData;
}
const std::string& ITableMeta::getEncodingData()
{
  return m_encodingData;
}
const std::string& ITableMeta::getcolTypeData()
{
  return m_colTypeData;
}
const std::string& ITableMeta::getPkData()
{
  return m_PkData;
}
const std::string& ITableMeta::getUkData()
{
  return m_UkData;
}
const std::string& ITableMeta::getkeyData()
{
  return m_keyData;
}
const std::string& ITableMeta::getColumnFlagData()
{
  return m_columnFlagData;
}
const std::string& ITableMeta::getNotNullData()
{
  return m_colNotNullData;
}
const std::string& ITableMeta::getSignedData()
{
  return m_colSignedData;
}
const std::string& ITableMeta::getDecimalsData()
{
  return m_colDecimalsData;
}
const std::string& ITableMeta::getDefaultData()
{
  return m_colDefaultData;
}
const std::string& ITableMeta::getColLengthData()
{
  return m_colLengthData;
}
const std::string& ITableMeta::getOriginTypeData()
{
  return m_colOriginTypeData;
}
const std::string& ITableMeta::getColPrecisionData()
{
  return m_colPrecisionData;
}
const std::string& ITableMeta::getColScaleData()
{
  return m_colScaleData;
}

// ------------------DBMeta----------------
struct DBMetaHeader {
  uint32_t m_nameOffset;
  uint8_t m_isDropped;
  uint32_t m_encOffset;
  uint32_t m_tblCount;
  uint32_t m_tblsSize;
};

struct DBMetaInfo : public MetaInfo {
  DBMetaHeader* m_dbMetaHeader;
  NameTblmetaMap m_tables;
  NameTblmetaMap::iterator m_tablesIterator;
  std::vector<ITableMeta*> m_tableOrders;
  IMetaDataCollections* m_metaDataCollections;

  DBMetaInfo() : MetaInfo(sizeof(DBMetaHeader), true), m_dbMetaHeader(NULL), m_metaDataCollections(NULL)
  {
    m_dbMetaHeader = (DBMetaHeader*)m_header;
    m_dbMetaHeader->m_isDropped = false;
    m_dbMetaHeader->m_tblCount = 0;
    m_dbMetaHeader->m_tblsSize = 0;
  }

  DBMetaInfo(const void* ptr, size_t size, IDBMeta* owner)
      : MetaInfo(sizeof(DBMetaHeader), false), m_dbMetaHeader(NULL), m_metaDataCollections(NULL)
  {
    this->parse(ptr, size, owner);
  }

  ~DBMetaInfo()
  {
    clear();
  }

  int appendTo(std::string& s)
  {
    if (!m_creating)
      return -21;

    // db header
    MetaInfo::appendTo(s);

    // merge all the tbls
    NameTblmetaMap::iterator i;
    for (i = m_tables.begin(); i != m_tables.end(); ++i) {
      ITableMeta* tblMeta = i->second;
      tblMeta->appendTo(s);
    }
    return 0;
  }

  int parse(const void* ptr, size_t size, IDBMeta* owner)
  {
    // parse db
    int ret = MetaInfo::parse(ptr, size);
    if (ret == 0) {
      clear();

      m_dbMetaHeader = (DBMetaHeader*)m_header;
      size_t tblCount = m_dbMetaHeader->m_tblCount;
      size_t dataSize = m_dbMetaHeader->m_tblsSize;
      if (m_data.getRealSize() + dataSize > size)
        return -21;

      // parse tbl meta
      const char* p = (char*)ptr + m_data.getRealSize();
      for (size_t i = 0; i < tblCount; ++i) {
        ITableMeta* tblMeta = new ITableMeta(p, dataSize);
        if (!tblMeta->parsedOK()) {
          delete tblMeta;
          return -22;
        }
        size_t tblSize = tblMeta->getRealSize();
        if (tblSize > dataSize) {
          delete tblMeta;
          return -23;
        }
        tblMeta->setDBMeta(owner);
        m_tables[std::string(tblMeta->getName())] = tblMeta;
        m_tableOrders.push_back(tblMeta);
        p += tblSize;
        dataSize -= tblSize;
      }
      return 0;
    }
    return ret;
  }

  size_t getRealSize()
  {
    if (!m_parsedOK) {
      m_dbMetaHeader->m_tblCount = m_tables.size();
      size_t tblsSize = 0;
      NameTblmetaMap::iterator i;
      for (i = m_tables.begin(); i != m_tables.end(); ++i)
        tblsSize += i->second->getRealSize();
      m_dbMetaHeader->m_tblsSize = tblsSize;
    }
    return m_data.getRealSize() + m_dbMetaHeader->m_tblsSize;
  }

private:
  void clear()
  {
    NameTblmetaMap::iterator i;
    m_tableOrders.clear();
    for (i = m_tables.begin(); i != m_tables.end(); i++) {
      ITableMeta* meta = i->second;
      if (meta != NULL) {
        i->second = NULL;
        delete meta;
      }
    }
    m_tables.clear();
  }
};

IDBMeta::IDBMeta()
{
  m_db = new DBMetaInfo();
  m_db->m_tablesIterator = m_db->m_tables.begin();
  m_userData = NULL;
}

IDBMeta::IDBMeta(const void* ptr, size_t size)
{
  m_db = new DBMetaInfo(ptr, size, this);
  m_userData = NULL;
}

IDBMeta::~IDBMeta()
{
  if (m_db != NULL)
    delete m_db;
}

const char* IDBMeta::getName()
{
  return m_db->m_data.getString(m_db->m_dbMetaHeader->m_nameOffset);
}

/* For partial, if the db is dropped. */
bool IDBMeta::isDropped()
{
  return m_db->m_dbMetaHeader->m_isDropped;
}

const char* IDBMeta::getEncoding()
{
  return m_db->m_data.getString(m_db->m_dbMetaHeader->m_encOffset);
}

IMetaDataCollections* IDBMeta::getMetaDataCollections()
{
  return m_db->m_metaDataCollections;
}

void IDBMeta::setName(const char* name)
{
  m_db->m_dbMetaHeader->m_nameOffset = m_db->m_data.appendString(name);
}

/* For partial, set if the db is dropped. */
void IDBMeta::setDropped(bool value)
{
  m_db->m_dbMetaHeader->m_isDropped = value;
}

// [TODO]
void IDBMeta::setEncoding(const char* enc)
{
  m_db->m_dbMetaHeader->m_encOffset = m_db->m_data.appendString(enc);
}

void IDBMeta::setMetaDataCollections(IMetaDataCollections* mdc)
{
  m_db->m_metaDataCollections = mdc;
}

int IDBMeta::getTblCount()
{
  return (int)m_db->m_tables.size();
}

ITableMeta* IDBMeta::get(const char* tblName)
{
  NameTblmetaMap::iterator i = m_db->m_tables.find(std::string(tblName));
  if (i != m_db->m_tables.end())
    return i->second;
  return NULL;
}

ITableMeta* IDBMeta::get(int index)
{
  if (index >= 0 && index < (int)m_db->m_tableOrders.size())
    return m_db->m_tableOrders[index];
  return NULL;
}

/* Get the name and meta. If it reaches the end, return 1. Error returns -1. */
int IDBMeta::getFromMapIterator(const char** tblName, ITableMeta** tblMeta)
{
  ITableMeta* tmp = NULL;
  const char* tnp = NULL;
  if (m_db->m_tablesIterator == m_db->m_tables.end())
    return 1;
  if ((tmp = m_db->m_tablesIterator->second) == NULL)
    return -1;
  if ((tnp = tmp->getName()) == NULL)
    return -1;
  *tblName = tnp;
  *tblMeta = tmp;
  return 0;
}

/* Move to the next element. */
int IDBMeta::nextMapIterator(bool erase)
{
  if (erase)
    return eraseMapIterator();
  else {
    m_db->m_tablesIterator++;
    return 0;
  }
}

/* Move to the first table and meta. */
int IDBMeta::resetMapIterator()
{
  m_db->m_tablesIterator = m_db->m_tables.begin();
  return 0;
}

/*
 * Call this only for partial metadata collection. We erase that in the map
 * and vector, but we don't free the TableMeta. Return -1 (<0) if any error.
 */
int IDBMeta::eraseMapIterator()
{
  const char* tableName = NULL;
  if ((m_db->m_tablesIterator == m_db->m_tables.end()) || (m_db->m_tablesIterator->second == NULL) ||
      ((tableName = m_db->m_tablesIterator->second->getName()) == NULL))
    return (-1);
  m_db->m_tablesIterator->second = NULL;
  m_db->m_tablesIterator = m_db->m_tables.erase(m_db->m_tablesIterator);
  for (std::vector<ITableMeta*>::iterator i = m_db->m_tableOrders.begin(); i != m_db->m_tableOrders.end(); i++) {
    if (strcasecmp(tableName, (*i)->getName()) == 0) {
      m_db->m_tableOrders.erase(i);
      break;
    }
  }
  return 0;
}

/* Erase a table from the metadata collection, and the TableMeta. */
int IDBMeta::erase(const char* tableName, bool delayDeleteMeta)
{
  if (tableName == NULL)
    return (-1);
  ITableMeta* meta = NULL;
  NameTblmetaMap::iterator i = m_db->m_tables.find(std::string(tableName));
  if (i != m_db->m_tables.end()) {
    meta = i->second;
    m_db->m_tables.erase(i);
    /* We shouldn't use iterator in this case, just for safety. */
    resetMapIterator();
  }
  for (std::vector<ITableMeta*>::iterator j = m_db->m_tableOrders.begin(); j != m_db->m_tableOrders.end(); j++) {
    if (strcasecmp(tableName, (*j)->getName()) == 0) {
      if (meta == NULL)
        meta = *j;
      m_db->m_tableOrders.erase(j);
      break;
    }
  }
  if (meta != NULL && delayDeleteMeta == false)
    delete meta;
  return 0;
}

/*
 * We don't accept a table name pointer as a argument in this function, as it
 * might be changed. We use the one in TableMeta instead. It will not be changed
 * during the lifecycle in the TableMeta.
 */
int IDBMeta::put(ITableMeta* tblMeta)
{
  if (tblMeta->getName() == NULL)
    return (-1);
  std::string name(tblMeta->getName());
  NameTblmetaMap::iterator i = m_db->m_tables.find(name);
  if (i != m_db->m_tables.end())
    return (-2);
  m_db->m_tables[name] = tblMeta;
  m_db->m_tableOrders.push_back(tblMeta);
  return 0;
}

int IDBMeta::appendTo(std::string& s)
{
  return m_db->appendTo(s);
}

size_t IDBMeta::getRealSize()
{
  return m_db->getRealSize();
}

int IDBMeta::parse(const void* ptr, size_t size)
{
  return m_db->parse(ptr, size, this);
}

bool IDBMeta::parsedOK()
{
  return m_db->m_parsedOK;
}

void IDBMeta::setUserData(void* data)
{
  m_userData = data;
}

void* IDBMeta::getUserData()
{
  return m_userData;
}

void** IDBMeta::getUserDataPtr()
{
  return &m_userData;
}

// -----------------MetaDataCollections-----------------
struct MetaDataCollectionHeader {
  uint32_t m_metaVerNum;
  uint32_t m_dbCount;
  uint64_t m_timestamp;
  uint32_t m_dbsSize;
};

struct MetaDataCollectionInfo : public MetaInfo {
  MetaDataCollectionHeader* m_collHeader;
  NameDbmetaMap m_dbs;
  NameDbmetaMap::iterator m_dbsIterator;
  std::vector<IDBMeta*> m_dbOrders;
  IMetaDataCollections* m_prev;
  bool m_removePtr;
  const void* m_ptr;

  MetaDataCollectionInfo()
      : MetaInfo(sizeof(MetaDataCollectionHeader), true),
        m_collHeader(NULL),
        m_prev(NULL),
        m_removePtr(false),
        m_ptr(NULL)
  {
    m_collHeader = (MetaDataCollectionHeader*)m_header;
    m_collHeader->m_metaVerNum = 0;
    m_collHeader->m_dbCount = 0;
    m_collHeader->m_dbsSize = 0;
  }

  MetaDataCollectionInfo(const void* ptr, size_t size, bool removePtr, IMetaDataCollections* owner)
      : MetaInfo(sizeof(MetaDataCollectionHeader), false),
        m_collHeader(NULL),
        m_prev(NULL),
        m_removePtr(removePtr),
        m_ptr(ptr)
  {
    this->parse(ptr, size, owner);
  }

  ~MetaDataCollectionInfo()
  {
    if (m_removePtr)
      delete[] (char*)m_ptr;
    clear();
  }

  int appendTo(std::string& s)
  {
    if (!m_creating)
      return -31;

    // coll header
    this->getRealSize();  // very important
    MetaInfo::appendTo(s);

    // merge all the dbs
    NameDbmetaMap::iterator i;
    for (i = m_dbs.begin(); i != m_dbs.end(); ++i) {
      IDBMeta* dbMeta = i->second;
      dbMeta->appendTo(s);
    }
    return 0;
  }

  int parse(const void* ptr, size_t size, IMetaDataCollections* owner)
  {
    // parse coll
    int ret = MetaInfo::parse(ptr, size);
    if (ret == 0) {
      clear();

      m_collHeader = (MetaDataCollectionHeader*)m_header;
      size_t dbCount = m_collHeader->m_dbCount;
      size_t dataSize = m_collHeader->m_dbsSize;
      if (m_data.getRealSize() + dataSize > size)
        return -31;

      // parse db meta
      const char* p = (char*)ptr + m_data.getRealSize();
      for (size_t i = 0; i < dbCount; ++i) {
        IDBMeta* dbMeta = new IDBMeta(p, dataSize);
        if (!dbMeta->parsedOK()) {
          delete dbMeta;
          return -32;
        }
        size_t dbSize = dbMeta->getRealSize();
        if (dbSize > dataSize) {
          delete dbMeta;
          return -33;
        }
        dbMeta->setMetaDataCollections(owner);
        m_dbs[std::string(dbMeta->getName())] = dbMeta;
        m_dbOrders.push_back(dbMeta);
        p += dbSize;
        dataSize -= dbSize;
      }
      return 0;
    }
    return ret;
  }

  size_t getRealSize()
  {
    if (!m_parsedOK) {
      m_collHeader->m_dbCount = m_dbs.size();
      size_t dbsSize = 0;
      NameDbmetaMap::iterator i;
      for (i = m_dbs.begin(); i != m_dbs.end(); ++i)
        dbsSize += i->second->getRealSize();
      m_collHeader->m_dbsSize = dbsSize;
    }
    return m_data.getRealSize() + m_collHeader->m_dbsSize;
  }

private:
  void clear()
  {
    NameDbmetaMap::iterator i;
    m_dbOrders.clear();
    for (i = m_dbs.begin(); i != m_dbs.end(); i++) {
      IDBMeta* meta = i->second;
      if (meta != NULL) {
        i->second = NULL;
        delete meta;
      }
    }
    m_dbs.clear();
  }
};

IMetaDataCollections::IMetaDataCollections()
{
  m_coll = new MetaDataCollectionInfo();
  m_coll->m_dbsIterator = m_coll->m_dbs.begin();
  m_userData = NULL;
}

IMetaDataCollections::IMetaDataCollections(const void* ptr, size_t size, bool removePtr)
{
  m_coll = new MetaDataCollectionInfo(ptr, size, removePtr, this);
  m_userData = NULL;
}

IMetaDataCollections::~IMetaDataCollections()
{
  delete m_coll;
}

unsigned IMetaDataCollections::getMetaVerNum()
{
  return m_coll->m_collHeader->m_metaVerNum;
}

IMetaDataCollections* IMetaDataCollections::getPrev()
{
  return m_coll->m_prev;
}

time_t IMetaDataCollections::getTimestamp()
{
  return m_coll->m_collHeader->m_timestamp;
}

void IMetaDataCollections::setMetaVerNum(unsigned metaVerNum)
{
  m_coll->m_collHeader->m_metaVerNum = metaVerNum;
}

void IMetaDataCollections::setPrev(IMetaDataCollections* prev)
{
  m_coll->m_prev = prev;
}

void IMetaDataCollections::setTimestamp(time_t timestamp)
{
  m_coll->m_collHeader->m_timestamp = timestamp;
}

int IMetaDataCollections::getDbCount()
{
  return (int)m_coll->m_dbs.size();
}

IDBMeta* IMetaDataCollections::get(const char* dbName)
{
  NameDbmetaMap::iterator i = m_coll->m_dbs.find(std::string(dbName));
  if (i == m_coll->m_dbs.end())
    return NULL;
  return i->second;
}

IDBMeta* IMetaDataCollections::get(int index)
{
  if (index >= 0 && index < (int)m_coll->m_dbs.size())
    return m_coll->m_dbOrders[index];
  return NULL;
}

ITableMeta* IMetaDataCollections::get(const char* dbName, const char* tblName)
{
  NameDbmetaMap::iterator i = m_coll->m_dbs.find(std::string(dbName));
  if (i == m_coll->m_dbs.end())
    return NULL;
  IDBMeta* dbMeta = i->second;
  if (dbMeta == NULL)
    return NULL;
  return dbMeta->get(tblName);
}

/* Get name and meta. When reaches the end, return 1. When error, return -1. */
int IMetaDataCollections::getFromMapIterator(const char** dbName, IDBMeta** dbMeta)
{
  IDBMeta* dmp = NULL;
  const char* dnp = NULL;
  if (m_coll->m_dbsIterator == m_coll->m_dbs.end())
    return 1;
  if ((dmp = m_coll->m_dbsIterator->second) == NULL)
    return -1;
  if ((dnp = dmp->getName()) == NULL)
    return -1;
  *dbName = dnp;
  *dbMeta = dmp;
  return 0;
}

int IMetaDataCollections::nextMapIterator(bool erase)
{
  if (erase)
    return eraseMapIterator();
  else {
    m_coll->m_dbsIterator++;
    return 0;
  }
}

/* Move to the first db and meta. */
int IMetaDataCollections::resetMapIterator()
{
  m_coll->m_dbsIterator = m_coll->m_dbs.begin();
  return 0;
}

/*
 * Call this only for partial metadata collection. We erase that in the map
 * and vector, but we don'free the DBMeta.
 */
int IMetaDataCollections::eraseMapIterator()
{
  const char* dbName = NULL;
  if ((m_coll->m_dbsIterator == m_coll->m_dbs.end()) || (m_coll->m_dbsIterator->second == NULL) ||
      ((dbName = m_coll->m_dbsIterator->second->getName()) == NULL))
    return (-1);
  m_coll->m_dbsIterator->second = NULL;
  m_coll->m_dbsIterator = m_coll->m_dbs.erase(m_coll->m_dbsIterator);
  for (std::vector<IDBMeta*>::iterator i = m_coll->m_dbOrders.begin(); i != m_coll->m_dbOrders.end(); i++) {
    if (strcasecmp(dbName, (*i)->getName()) == 0) {
      m_coll->m_dbOrders.erase(i);
      break;
    }
  }
  return 0;
}

/* Erase a db and DBMeta from the metadata collection. */
int IMetaDataCollections::erase(const char* dbName, bool delayDeleteMeta)
{
  if (dbName == NULL)
    return (-1);
  IDBMeta* meta = NULL;
  NameDbmetaMap::iterator i = m_coll->m_dbs.find(std::string(dbName));
  if (i != m_coll->m_dbs.end()) {
    meta = i->second;
    m_coll->m_dbs.erase(i);
  }
  for (std::vector<IDBMeta*>::iterator i = m_coll->m_dbOrders.begin(); i != m_coll->m_dbOrders.end(); i++) {
    if (strcasecmp(dbName, (*i)->getName()) == 0) {
      if (meta == NULL)
        meta = *i;
      m_coll->m_dbOrders.erase(i);
      break;
    }
  }
  if (meta != NULL && delayDeleteMeta == false)
    delete meta;
  return 0;
}

int IMetaDataCollections::erase(const char* dbName, const char* tblName, bool delayDeleteMeta)
{
  if (dbName == NULL || tblName == NULL)
    return (-1);
  NameDbmetaMap::iterator i = m_coll->m_dbs.find(std::string(dbName));
  if (i == m_coll->m_dbs.end())
    return 1;
  IDBMeta* meta = i->second;
  if (meta == NULL)
    return 2;
  return meta->erase(tblName, delayDeleteMeta);
}

/*
 * We don't accept (char *) db name pointer, as it could be changed.
 * We use the one in DBMeta instead. It won't be changed during the lifecycle
 * of DBMeta.
 */
int IMetaDataCollections::put(IDBMeta* dbMeta)
{
  if (dbMeta->getName() == NULL)
    return (-1);
  NameDbmetaMap::iterator i = m_coll->m_dbs.find(std::string(dbMeta->getName()));
  if (i != m_coll->m_dbs.end())
    return (-2);
  m_coll->m_dbs[std::string(dbMeta->getName())] = (IDBMeta*)dbMeta;
  m_coll->m_dbOrders.push_back((IDBMeta*)dbMeta);
  return 0;
}

int IMetaDataCollections::toString(std::string& s)
{
  s.clear();
  return m_coll->appendTo(s);
}

int IMetaDataCollections::parse(const void* ptr, size_t size)
{
  return m_coll->parse(ptr, size, this);
}

bool IMetaDataCollections::parsedOK()
{
  return m_coll->m_parsedOK;
}

size_t IMetaDataCollections::getRealSize()
{
  return m_coll->getRealSize();
}

void IMetaDataCollections::setUserData(void* data)
{
  m_userData = data;
}

void* IMetaDataCollections::getUserData()
{
  return m_userData;
}

}  // namespace logmessage
}  // namespace oceanbase
