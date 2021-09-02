#include "LogRecord.h"
#include "MetaInfo.h"
#include "Endian.h"
#include "MsgVarArea.h"
#include <cstdint>
#include <cstring>
#include <sys/time.h>
#include <cstddef>
#include <cstdio>
#include <cinttypes>
#include <iostream>
#include "LogMsgBuf.h"

namespace oceanbase {
namespace logmessage {

#define CLUM_SIZE 128
#define FILTER_SIZE 5
typedef std::vector<std::string*> ColMap;

const uint16_t LOGREC_VERSION = 3;                       // version num
const uint64_t LOGREC_SUB_VERSION = 0x0200000000000000;  // sub version num
#define GET_LOGREC_SUB_VERSION(v) ((v)&0xFF00000000000000)
const uint64_t LOGREC_INVALID_ID = 0;

// LogRecord head info
struct PosOfLogMsg_vb {
  uint8_t m_lrVersion;
  uint8_t m_srcType;
  uint8_t m_op;
  uint8_t m_firstInLogevent;
  uint32_t m_srcCategory;
  /*H 8 bit is sub version，other is microID*/
  uint64_t m_id;
  uint64_t m_timestamp;
};

struct PosOfLogMsg_v1 : public PosOfLogMsg_vb {
  uint32_t m_encoding;
  uint32_t m_posOfInstance;
  uint32_t m_posOfTimemark;
  uint32_t m_posOfDbName;
  uint32_t m_posOfTbName;
  uint32_t m_posOfColNames;
  uint32_t m_posOfColTypes;
  uint32_t m_posOfPkVal;
  uint64_t m_posOfFileName;
  uint64_t m_posOfFile;
  uint32_t m_posOfOldCols;
  uint32_t m_posOfNewCols;
  uint32_t m_posOfPkKeys;
  uint32_t m_posOfUkKeys;
};

struct PosOfLogMsg_v2 : public PosOfLogMsg_v1 {
  uint64_t m_posOfColEncoding;
};

struct PosOfLogMsg_v3 : public PosOfLogMsg_v2 {
  uint32_t m_posOfFilterRuleVal;
  uint32_t m_posOfEndInfo;
};
struct PosOfLogMsg_v3_1 : public PosOfLogMsg_v3 {
  uint32_t m_metaVersion;
  uint32_t m_posOfColFlag;
};
struct PosOfLogMsg_v3_2 : public PosOfLogMsg_v3_1 {
  uint32_t m_posOfColNotNull;
  uint32_t m_posOfColSigned;
  uint32_t m_posOfColDecimals;
  uint32_t m_posOfColDefault;
  uint32_t m_posOfObTraceInfo;
  int32_t m_sqlNo;
};
struct EndOfLogMsg_v1 {
  uint32_t m_threadId;
  uint32_t m_usec;
};
struct EndOfLogMsg_v2 : public EndOfLogMsg_v1 {
  uint32_t m_crc;
};
/* Currently used version of PosOfLogMsg */
typedef struct PosOfLogMsg_v3_2 PosOfLogMsg_vc;

/* PosOfLogMsg versions listed */
typedef struct PosOfLogMsg_vb PosOfLogMsg;
typedef struct EndOfLogMsg_v2 EndOfLogMsg;
struct LogRecInfo {
  static const char* ddlName[2];
  static const char* dmlName;
  static const char* heartbeatName;
  static uint8_t ddlType[2];
  static uint8_t dmlType[1];
  static uint8_t heartbeatType[1];

  bool m_creatingMode;
  bool m_parsedOK;
  bool m_tailParseOK;

  PosOfLogMsg* m_posInfo;
  EndOfLogMsg* m_endInfo;
  MsgVarArea* m_lrDataArea;
#define MAX_CP_BUF 48
  char m_cpBuf[MAX_CP_BUF];

  ITableMeta* m_tblMeta;
  IDBMeta* m_dbMeta;
  std::vector<ITableMeta*> m_expiredTableMeta;
  std::vector<IDBMeta*> m_expiredDBMeta;
  IMetaDataCollections* m_expiredMetaDataCollections;
  std::vector<std::string*> m_old_cols;
  std::vector<std::string*> m_new_cols;
  std::vector<const char*> m_extra_infos;
  std::vector<long> m_timemarks;
  std::vector<int> m_uks;
  std::vector<int> m_pks;
  BinLogBuf* m_old_clum;
  BinLogBuf* m_new_clum;
  BinLogBuf* m_filter_value;
  int m_old_count;
  int m_new_count;
  size_t m_max_count;
  size_t m_filter_count;
  size_t m_filter_max_count;
  string m_dbName;
  string m_tbName;
  string m_instance;
  string m_encoding;
  string m_ob_trace_info;
  bool useDMB;
  bool m_reservedMemory;
  LogRecInfo(time_t timestamp, ITableMeta* tblMeta)
      : m_creatingMode(true),
        m_parsedOK(false),
        m_tailParseOK(false),
        m_posInfo(NULL),
        m_endInfo(NULL),
        m_lrDataArea(NULL),
        m_tblMeta(tblMeta),
        m_dbMeta(NULL),
        m_expiredMetaDataCollections(NULL),
        useDMB(false),
        m_reservedMemory(false)
  {
    m_posInfo = new PosOfLogMsg_vc;
    m_lrDataArea = new MsgVarArea();
    m_endInfo = new EndOfLogMsg;
    m_lrDataArea->appendArray((uint8_t*)m_posInfo, sizeof(PosOfLogMsg_vc));

    memset(m_endInfo, 0, sizeof(EndOfLogMsg));
    memset(m_posInfo, -1, sizeof(PosOfLogMsg_vc));
    m_posInfo->m_lrVersion = LOGREC_VERSION;
    m_posInfo->m_id = LOGREC_INVALID_ID;
    m_posInfo->m_timestamp = timestamp;
    m_posInfo->m_firstInLogevent = false;
    m_posInfo->m_srcType = SRC_MYSQL;
    m_posInfo->m_srcCategory = SRC_FULL_RECORDED;
    m_old_clum = NULL;
    m_new_clum = NULL;
    m_filter_value = new BinLogBuf[FILTER_SIZE];
    m_old_count = m_new_count = 0;
    m_filter_count = 0;
    m_max_count = 0;
    m_filter_max_count = FILTER_SIZE;
  }

  LogRecInfo(const void* ptr, size_t size)
      : m_creatingMode(false),
        m_parsedOK(false),
        m_tailParseOK(false),
        m_posInfo(NULL),
        m_endInfo(NULL),
        m_lrDataArea(NULL),
        m_tblMeta(NULL),
        m_dbMeta(NULL),
        m_expiredMetaDataCollections(NULL),
        useDMB(false),
        m_reservedMemory(false)
  {
    m_lrDataArea = new MsgVarArea(false);
    parse(ptr, size);
    m_old_clum = NULL;
    m_new_clum = NULL;
    m_filter_value = new BinLogBuf[FILTER_SIZE];
    m_old_count = m_new_count = 0;
    m_filter_count = 0;
    m_max_count = 0;
    m_filter_max_count = FILTER_SIZE;
  }

  LogRecInfo(bool creating, bool useDMB = false)
      : m_creatingMode(creating),
        m_parsedOK(false),
        m_tailParseOK(false),
        m_posInfo(NULL),
        m_endInfo(NULL),
        m_lrDataArea(NULL),
        m_tblMeta(NULL),
        m_dbMeta(NULL),
        m_expiredMetaDataCollections(NULL)
  {
    if (creating) {
      m_posInfo = new PosOfLogMsg_vc;
      m_lrDataArea = new MsgVarArea();
      m_endInfo = new EndOfLogMsg;
      memset(m_endInfo, 0, sizeof(EndOfLogMsg));
      m_lrDataArea->appendArray((uint8_t*)m_posInfo, sizeof(PosOfLogMsg_vc));
      memset(m_posInfo, -1, sizeof(PosOfLogMsg_vc));
      m_posInfo->m_lrVersion = LOGREC_VERSION;
      m_posInfo->m_firstInLogevent = false;
      m_posInfo->m_id = LOGREC_INVALID_ID;
      m_posInfo->m_srcType = SRC_MYSQL;
      m_posInfo->m_srcCategory = SRC_FULL_RECORDED;
      this->useDMB = useDMB;
    } else {
      this->useDMB = false;
      m_lrDataArea = new MsgVarArea(false);
    }
    m_reservedMemory = false;
    m_new_clum = m_old_clum = NULL;
    m_filter_value = new BinLogBuf[FILTER_SIZE];
    m_old_count = m_new_count = 0;
    m_filter_count = 0;
    m_max_count = 0;
    m_filter_max_count = FILTER_SIZE;
  }

  ~LogRecInfo()
  {
    /**
     * clear() must be called before here if users want to manage
     * the memory themselves, otherwise their memory would be freed
     * and uncertain error or crash would come out.
     */
    clearOld();
    clearNew();
    clearExpiredMeta();
    if ((m_creatingMode || m_parsedOK) && m_posInfo != NULL)
      delete m_posInfo;
    if ((m_creatingMode || m_parsedOK) && m_endInfo != NULL)
      delete m_endInfo;
    if (m_lrDataArea != NULL)
      delete m_lrDataArea;
    delete[] m_filter_value;
  }
  void clear()
  {
    if (m_creatingMode) {
      if (!useDMB) {
        m_lrDataArea->clear();
        m_lrDataArea->appendArray((uint8_t*)m_posInfo, sizeof(PosOfLogMsg_vc));
      } else {
        if (m_reservedMemory) {
          /* free when reserverMemory is true */
          m_lrDataArea->clear();
          m_lrDataArea->appendArray((uint8_t*)m_posInfo, sizeof(PosOfLogMsg_vc));
          m_reservedMemory = false;
        }

        m_dbName.clear();
        m_tbName.clear();
        m_instance.clear();
        m_encoding.clear();
        m_ob_trace_info.clear();
      }
      memset(m_posInfo, -1, sizeof(PosOfLogMsg_vc));
      memset(m_endInfo, 0, sizeof(EndOfLogMsg));
      m_posInfo->m_lrVersion = LOGREC_VERSION;
      m_posInfo->m_id = LOGREC_INVALID_ID;
      m_posInfo->m_firstInLogevent = false;
      m_posInfo->m_srcType = SRC_MYSQL;
      m_posInfo->m_srcCategory = SRC_FULL_RECORDED;

      m_timemarks.clear();
      m_extra_infos.clear();
      m_uks.clear();
      m_pks.clear();
      if (m_old_count == 0 && m_new_count == 0) {
        m_old_cols.clear();
        m_new_cols.clear();
      } else
        m_old_count = m_new_count = 0;
      m_filter_count = 0;
    } else if (m_parsedOK) {
      // there has two logrecord，one logrecord creatingMode is true，this logrecord in memory,another record is create
      // by parse ,parseOK istrue。
      m_dbName.clear();
      m_tbName.clear();
      m_instance.clear();
      m_encoding.clear();
      if (m_posInfo != NULL) {
        delete m_posInfo;
        m_posInfo = NULL;
      }
      if (m_endInfo != NULL) {
        delete m_endInfo;
        m_endInfo = NULL;
      }
      m_timemarks.clear();
      m_uks.clear();
      m_pks.clear();
      m_extra_infos.clear();
      if (m_old_count == 0 && m_new_count == 0) {
        m_old_cols.clear();
        m_new_cols.clear();
      } else
        m_old_count = m_new_count = 0;
      m_filter_count = 0;
    }
  }

  void clearExpiredMeta()
  {
    std::vector<ITableMeta*>::iterator tbit = m_expiredTableMeta.begin();
    for (; tbit != m_expiredTableMeta.end(); tbit++)
      if (*tbit != NULL)
        delete *tbit;
    m_expiredTableMeta.clear();

    std::vector<IDBMeta*>::iterator dbit = m_expiredDBMeta.begin();
    for (; dbit != m_expiredDBMeta.end(); dbit++)
      if (*dbit != NULL)
        delete *dbit;
    m_expiredDBMeta.clear();

    if (m_expiredMetaDataCollections != NULL) {
      delete m_expiredMetaDataCollections;
      m_expiredMetaDataCollections = NULL;
    }
  }

  inline int16_t FastHash(const char* str, int len)
  {
    int16_t hash = 0;
    for (int i = 0; i < len; i++) {
      hash = ((hash << 5) + hash) + (int16_t)str[i];
    }
    return hash & 0x7FFF;
  }
  int16_t getRecordHash(int16_t (*hashFunc)(const char** valueList, const size_t* sizeList, int count))
  {
    const int* hashColumnIdx = NULL;
    int hashColumnCount = 0;
    char** hashValueList;
    size_t* hashValueSizeList;
    if (m_creatingMode && m_tblMeta) {
      if (NULL == (hashColumnIdx = m_tblMeta->getHashColumnIdx(hashColumnCount, hashValueList, hashValueSizeList)) ||
          hashColumnCount == 0)  // if no pk,hash set to 0
        return 0;
      if (m_old_count == 0 && m_new_count == 0 && (m_new_cols.size() > 0 || m_old_cols.size() > 0)) {
        if (m_posInfo->m_op == EDELETE || m_posInfo->m_op == INDEX_DELETE) {
          for (int idx = 0; idx < hashColumnCount; idx++) {
            if (m_old_cols[hashColumnIdx[idx]] == NULL)
              return 0;
            hashValueList[idx] = (char*)m_old_cols[hashColumnIdx[idx]]->c_str();
            hashValueSizeList[idx] = m_old_cols[hashColumnIdx[idx]]->size();
          }
        } else {
          for (int idx = 0; idx < hashColumnCount; idx++) {
            if (m_new_cols[hashColumnIdx[idx]] == NULL)
              return 0;
            hashValueList[idx] = (char*)m_new_cols[hashColumnIdx[idx]]->c_str();
            hashValueSizeList[idx] = m_new_cols[hashColumnIdx[idx]]->size();
          }
        }
      } else {
        if (m_posInfo->m_op == EDELETE || m_posInfo->m_op == INDEX_DELETE) {
          for (int idx = 0; idx < hashColumnCount; idx++) {
            if (m_old_clum[hashColumnIdx[idx]].buf == NULL)
              return 0;
            hashValueList[idx] = m_old_clum[hashColumnIdx[idx]].buf;
            hashValueSizeList[idx] = m_old_clum[hashColumnIdx[idx]].buf_used_size;
          }
        } else {
          for (int idx = 0; idx < hashColumnCount; idx++) {
            if (m_new_clum[hashColumnIdx[idx]].buf == NULL)
              return 0;
            hashValueList[idx] = m_new_clum[hashColumnIdx[idx]].buf;
            hashValueSizeList[idx] = m_new_clum[hashColumnIdx[idx]].buf_used_size;
          }
        }
      }
      return hashFunc((const char**)hashValueList, hashValueSizeList, hashColumnCount) & 0x7FFF;
    }
    return -1;
  }

  void exchangeEndInfoToLe(const void* v, size_t count)
  {
    if (v == NULL)
      return;

    ((EndOfLogMsg_v1*)v)->m_threadId = toLeEndianByType(((EndOfLogMsg_v1*)v)->m_threadId);
    ((EndOfLogMsg_v1*)v)->m_usec = toLeEndianByType(((EndOfLogMsg_v1*)v)->m_usec);
    if (count == sizeof(EndOfLogMsg_v1))
      return;

    ((EndOfLogMsg_v2*)v)->m_crc = toLeEndianByType(((EndOfLogMsg_v2*)v)->m_crc);
    if (count == sizeof(EndOfLogMsg_v2))
      return;
  }

  void exchangePosInfoToLe(const void* v, size_t count)
  {
    if (v == NULL)
      return;
    ((PosOfLogMsg_vc*)v)->m_srcCategory = toLeEndianByType(((PosOfLogMsg_vc*)v)->m_srcCategory);

    ((PosOfLogMsg_vc*)v)->m_id = toLeEndianByType(((PosOfLogMsg_vc*)v)->m_id);
    ((PosOfLogMsg_vc*)v)->m_timestamp = toLeEndianByType(((PosOfLogMsg_vc*)v)->m_timestamp);

    ((PosOfLogMsg_v1*)v)->m_encoding = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_encoding);
    ((PosOfLogMsg_v1*)v)->m_posOfInstance = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfInstance);
    ((PosOfLogMsg_v1*)v)->m_posOfTimemark = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfTimemark);
    ((PosOfLogMsg_v1*)v)->m_posOfDbName = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfDbName);
    ((PosOfLogMsg_v1*)v)->m_posOfTbName = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfTbName);
    ((PosOfLogMsg_v1*)v)->m_posOfColNames = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfColNames);
    ((PosOfLogMsg_v1*)v)->m_posOfColTypes = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfColTypes);
    ((PosOfLogMsg_v1*)v)->m_posOfPkVal = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfPkVal);
    ((PosOfLogMsg_v1*)v)->m_posOfFileName = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfFileName);
    ((PosOfLogMsg_v1*)v)->m_posOfFile = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfFile);
    ((PosOfLogMsg_v1*)v)->m_posOfOldCols = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfOldCols);
    ((PosOfLogMsg_v1*)v)->m_posOfNewCols = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfNewCols);
    ((PosOfLogMsg_v1*)v)->m_posOfPkKeys = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfPkKeys);
    ((PosOfLogMsg_v1*)v)->m_posOfUkKeys = toLeEndianByType(((PosOfLogMsg_v1*)v)->m_posOfUkKeys);

    ((PosOfLogMsg_v2*)v)->m_posOfColEncoding = toLeEndianByType(((PosOfLogMsg_v2*)v)->m_posOfColEncoding);

    ((PosOfLogMsg_v3*)v)->m_posOfFilterRuleVal = toLeEndianByType(((PosOfLogMsg_v3*)v)->m_posOfFilterRuleVal);
    ((PosOfLogMsg_v3*)v)->m_posOfEndInfo = toLeEndianByType(((PosOfLogMsg_v3*)v)->m_posOfEndInfo);

    ((PosOfLogMsg_v3_1*)v)->m_metaVersion = toLeEndianByType(((PosOfLogMsg_v3_1*)v)->m_metaVersion);
    ((PosOfLogMsg_v3_1*)v)->m_posOfColFlag = toLeEndianByType(((PosOfLogMsg_v3_1*)v)->m_posOfColFlag);

    ((PosOfLogMsg_v3_2*)v)->m_posOfColNotNull = toLeEndianByType(((PosOfLogMsg_v3_2*)v)->m_posOfColNotNull);
    ((PosOfLogMsg_v3_2*)v)->m_posOfColSigned = toLeEndianByType(((PosOfLogMsg_v3_2*)v)->m_posOfColSigned);
    ((PosOfLogMsg_v3_2*)v)->m_posOfColDecimals = toLeEndianByType(((PosOfLogMsg_v3_2*)v)->m_posOfColDecimals);
    ((PosOfLogMsg_v3_2*)v)->m_posOfColDefault = toLeEndianByType(((PosOfLogMsg_v3_2*)v)->m_posOfColDefault);
    ((PosOfLogMsg_v3_2*)v)->m_posOfObTraceInfo = toLeEndianByType(((PosOfLogMsg_v3_2*)v)->m_posOfObTraceInfo);
    ((PosOfLogMsg_v3_2*)v)->m_sqlNo = toLeEndianByType(((PosOfLogMsg_v3_2*)v)->m_sqlNo);
  }

  int parse(const void* ptr, size_t size)
  {
    if (m_creatingMode)
      return -1;
    if (0 != m_lrDataArea->parse(ptr, size))
      return -2;
    const void* v;
    size_t count;
    int ret = m_lrDataArea->getBuf(0, v, count);
    if (ret != 0)
      return -3;
    m_posInfo = (PosOfLogMsg*)v;
    /* version process */
    int version = toLeEndianByType(m_posInfo->m_lrVersion);
    if (version < LOGREC_VERSION) {
      // new-version program with old-format data
      if (count == sizeof(PosOfLogMsg_v1)) {
        m_posInfo = new PosOfLogMsg_v1;
      } else if (count == sizeof(PosOfLogMsg_v2)) {
        m_posInfo = new PosOfLogMsg_v2;
      } else {
        // new-format data has no enough header info
        return -4;
      }
      memcpy(m_posInfo, v, count);
    } else if (version > LOGREC_VERSION) {
      // old-version code confronts new-format data
      if (count < sizeof(PosOfLogMsg_vc)) {
        // new-format data has no enough header info
        return -4;
      }
      m_posInfo = new PosOfLogMsg_vc;
      memcpy(m_posInfo, v, sizeof(PosOfLogMsg_vc));
      int ret = m_lrDataArea->getBuf(toLeEndianByType(((PosOfLogMsg_vc*)m_posInfo)->m_posOfEndInfo), v, count);
      if (ret != 0) {
        delete m_posInfo;
        m_posInfo = NULL;
        return -3;
      }
      m_endInfo = new EndOfLogMsg;
      memcpy(m_endInfo, v, sizeof(EndOfLogMsg));
      m_tailParseOK = true;
    } else if (version == LOGREC_VERSION) {
      // count is setted to sizeof(PosOfLogMsg_v3) to make client parse correctly.
      if (count != sizeof(PosOfLogMsg_v3))
        return -4;
      m_posInfo = new PosOfLogMsg_vc;
      memcpy(m_posInfo, v, sizeof(PosOfLogMsg_vc));
      int ret = m_lrDataArea->getBuf(toLeEndianByType(((PosOfLogMsg_vc*)m_posInfo)->m_posOfEndInfo), v, count);
      if (ret != 0) {
        delete m_posInfo;
        m_posInfo = NULL;
        return -3;
      }
      m_endInfo = new EndOfLogMsg;
      memcpy(m_endInfo, v, sizeof(EndOfLogMsg));
      m_tailParseOK = true;
    }

    exchangeEndInfoToLe(m_endInfo, count);
    exchangePosInfoToLe((const char*)m_posInfo, count);
    m_parsedOK = true;
    return 0;
  }

  size_t getRealSize()
  {
    if (m_parsedOK)
      return m_lrDataArea->getRealSize();
    return 0;
  }

  void clearNew()
  {
    if (m_new_count == 0) {
      ColMap::iterator it;
      for (it = m_new_cols.begin(); it != m_new_cols.end(); it++) {
        if (*it)
          delete *it;
      }
      m_new_cols.clear();
    } else
      m_new_count = 0;
  }

  void clearOld()
  {
    if (m_old_count == 0) {
      ColMap::iterator it;
      for (it = m_old_cols.begin(); it != m_old_cols.end(); it++) {
        if (*it)
          delete *it;
      }
      m_old_cols.clear();
    } else
      m_old_count = 0;
  }

  void setSrcType(int type)
  {
    m_posInfo->m_srcType = type;
  }

  int getSrcType() const
  {
    return m_posInfo->m_srcType;
  }

  void setSrcCategory(int category)
  {
    m_posInfo->m_srcCategory = category;
  }

  int getSrcCategory() const
  {
    return m_posInfo->m_srcCategory;
  }

  int putOld(std::string* val)
  {
    m_old_cols.push_back(val);
    return 0;
  }
  int putOld(const char* pos, unsigned int len)
  {
    get_binlogBuf(&m_old_clum[m_old_count], (char*)pos, len);
    m_old_count++;
    return 0;
  }
  int putFilterRuleVal(const char* pos, unsigned int len)
  {
    if (m_filter_count >= m_filter_max_count - 1) {
      BinLogBuf* ptemp = m_filter_value;
      m_filter_value = new BinLogBuf[m_filter_max_count += FILTER_SIZE];
      memcpy(m_filter_value, (char*)ptemp, sizeof(BinLogBuf) * m_filter_count);
      delete[] ptemp;
    }
    get_binlogBuf(&m_filter_value[m_filter_count], (char*)pos, len);
    m_filter_count++;
    return 0;
  }
  int putNew(std::string* val)
  {
    m_new_cols.push_back(val);
    return 0;
  }
  int putNew(const char* pos, unsigned int len)
  {
    get_binlogBuf(&m_new_clum[m_new_count], (char*)pos, len);
    m_new_count++;
    return 0;
  }
  void initBinLogBuf(BinLogBuf* buf, int size)
  {
    if (buf == NULL)
      return;
    for (int index = 0; index < size; index++)
      buf[index].needFree = false;
  }
  void setOldColumn(BinLogBuf* buf, int size)
  {
    initBinLogBuf(buf, size);
    m_old_clum = buf;
    m_max_count = size;
  }
  void setNewColumn(BinLogBuf* buf, int size)
  {
    initBinLogBuf(buf, size);
    m_new_clum = buf;
    m_max_count = size;
  }
  int getColumncount()
  {
    return m_max_count;
  }
  StrArray* oldCols() const
  {
    if (m_parsedOK) {
      return m_lrDataArea->getStringArray(((PosOfLogMsg_vc*)m_posInfo)->m_posOfOldCols);
    }
    return NULL;
  }

  StrArray* newCols() const
  {
    if (m_parsedOK) {
      return m_lrDataArea->getStringArray(((PosOfLogMsg_vc*)m_posInfo)->m_posOfNewCols);
    }
    return NULL;
  }

  StrArray* filterValues() const
  {
    if (m_parsedOK) {
      return m_lrDataArea->getStringArray(((PosOfLogMsg_v3*)m_posInfo)->m_posOfFilterRuleVal);
    }
    return NULL;
  }

  StrArray* colNames() const
  {
    if (m_parsedOK) {
      return m_lrDataArea->getStringArray(((PosOfLogMsg_v1*)m_posInfo)->m_posOfColNames);
    }
    return NULL;
  }

  StrArray* colEncodings() const
  {
    if (m_parsedOK) {
      return m_lrDataArea->getStringArray(
          m_posInfo->m_lrVersion < 2 ? -1 : ((PosOfLogMsg_v2*)m_posInfo)->m_posOfColEncoding);
    }
    return NULL;
  }

  StrArray* colDefault() const
  {
    if (m_parsedOK) {
      return m_lrDataArea->getStringArray((GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION)
                                              ? ((PosOfLogMsg_v3_2*)m_posInfo)->m_posOfColDefault
                                              : -1);
    }
    return NULL;
  }

  size_t getColSignedOffset() const
  {
    return (GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION)
               ? ((PosOfLogMsg_v3_2*)m_posInfo)->m_posOfColSigned
               : -1;
  }

  const uint8_t* colTypes() const
  {
    if (m_parsedOK) {
      const void* v;
      size_t elSize, count;
      int ret = m_lrDataArea->getArray(((PosOfLogMsg_v1*)m_posInfo)->m_posOfColTypes, v, elSize, count);
      if (ret != 0 || elSize != sizeof(uint8_t))
        return NULL;
      return (uint8_t*)v;
    }
    return NULL;
  }
  const uint8_t* colFlags() const
  {
    if (m_parsedOK) {
      const void* v;
      size_t elSize, count;
      int ret = m_lrDataArea->getArray(
          GET_LOGREC_SUB_VERSION(m_posInfo->m_id) ? ((PosOfLogMsg_v3_1*)m_posInfo)->m_posOfColFlag : -1,
          v,
          elSize,
          count);
      if (ret != 0 || elSize != sizeof(uint8_t))
        return NULL;
      return (uint8_t*)v;
    }
    return NULL;
  }
  // get not null for all columns
  const uint8_t* colNotNull() const
  {
    if (m_parsedOK) {
      const void* v;
      size_t elSize, count;
      int ret = m_lrDataArea->getArray((GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION)
                                           ? ((PosOfLogMsg_v3_2*)m_posInfo)->m_posOfColNotNull
                                           : -1,
          v,
          elSize,
          count);
      if (ret != 0 || elSize != sizeof(uint8_t))
        return NULL;
      return (uint8_t*)v;
    }
    return NULL;
  }
  // get signed for all columns
  const uint8_t* colSigned() const
  {
    if (m_parsedOK) {
      const void* v;
      size_t elSize, count;
      int ret = m_lrDataArea->getArray((GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION)
                                           ? ((PosOfLogMsg_v3_2*)m_posInfo)->m_posOfColSigned
                                           : -1,
          v,
          elSize,
          count);
      if (ret != 0 || elSize != sizeof(uint8_t))
        return NULL;
      return (uint8_t*)v;
    }
    return NULL;
  }
  // get decimals for all columns
  const int32_t* colDecimals() const
  {
    if (m_parsedOK) {
      const void* v;
      size_t elSize, count;
      int ret = m_lrDataArea->getArray((GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION)
                                           ? ((PosOfLogMsg_v3_2*)m_posInfo)->m_posOfColDecimals
                                           : -1,
          v,
          elSize,
          count);
      if (ret != 0 || elSize != sizeof(int32_t))
        return NULL;
      return (int32_t*)v;
    }
    return NULL;
  }
  // get default value for all columns

  int setRecordType(int aType)
  {
    m_posInfo->m_op = aType;
    return 0;
  }

  int recordType() const
  {
    if (m_creatingMode || m_parsedOK)
      return m_posInfo->m_op;
    else
      return -1;
  }

  void addTimemark(long time)
  {
    m_timemarks.push_back(time);
  }

  std::vector<long>& getTimemark()
  {
    return m_timemarks;
  }

  std::vector<long>& getTimemark(size_t& length)
  {
    if (m_parsedOK) {
      if (m_timemarks.empty()) {
        const void* v;
        size_t elSize, i = 0;
        int ret = m_lrDataArea->getArray(((PosOfLogMsg_vc*)m_posInfo)->m_posOfTimemark, v, elSize, length);
        if (ret != 0 || elSize != sizeof(unsigned long)) {
          m_timemarks.clear();
        } else {
          for (i = 0; i < length; i++)
            m_timemarks.push_back(toLeEndianByType(((unsigned long*)v)[i]));
        }
      }
      return m_timemarks;
    }
    m_timemarks.clear();
    return m_timemarks;
  }

  void setTimestamp(time_t timestamp)
  {
    m_posInfo->m_timestamp = timestamp;
  }

  time_t getTimestamp()
  {
    if (m_creatingMode || m_parsedOK)
      return m_posInfo->m_timestamp;
    return 0;
  }

  const char* recordEncoding()
  {
    return m_lrDataArea->getString(((PosOfLogMsg_vc*)m_posInfo)->m_encoding);
  }

#define SET_OR_CLEAR_STRING(name, value) \
  do {                                   \
    if (NULL != value)                   \
      name = value;                      \
    else                                 \
      name.clear();                      \
  } while (0)

  void setInstance(const char* instance)
  {
    if (useDMB) {
      SET_OR_CLEAR_STRING(m_instance, instance);
    } else
      ((PosOfLogMsg_vc*)m_posInfo)->m_posOfInstance = m_lrDataArea->appendString(instance);
  }

  const char* instance()
  {
    if (useDMB)
      return m_instance.c_str();
    size_t offset = ((PosOfLogMsg_vc*)m_posInfo)->m_posOfInstance;
    return m_lrDataArea->getString(offset);
  }

  void setDbname(const char* dbname)
  {
    if (useDMB) {
      SET_OR_CLEAR_STRING(m_dbName, dbname);
    } else {
      // when m_creatingMode is false,before call function parse,m_posInfo maybe null
      if (m_posInfo != NULL) {
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfDbName = m_lrDataArea->appendString(dbname);
      }
    }
  }

  const char* dbname() const
  {
    if (useDMB)
      return m_dbName.c_str();
    size_t offset = ((PosOfLogMsg_vc*)m_posInfo)->m_posOfDbName;
    return m_lrDataArea->getString(offset);
  }

  void setTbname(const char* tbname)
  {
    if (useDMB) {
      SET_OR_CLEAR_STRING(m_tbName, tbname);
    } else {
      if (m_posInfo != NULL) {
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfTbName = m_lrDataArea->appendString(tbname);
      }
    }
  }

  const char* tbname() const
  {
    if (useDMB)
      return m_tbName.c_str();
    size_t offset = ((PosOfLogMsg_vc*)m_posInfo)->m_posOfTbName;
    return m_lrDataArea->getString(offset);
  }

  void setColNames(std::vector<std::string>& colNames)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNames = m_lrDataArea->appendStringArray(colNames);
  }
  void setColNames(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNames = m_lrDataArea->appendData(data);
  }
  std::string createStringArray(std::vector<std::string>& sa)
  {
    return MsgVarArea::createStringArrayData(sa);
  }
  std::string createStringArray(const char** sa, size_t size)
  {
    return MsgVarArea::createStringArrayData(sa, size);
  }
  std::string createStringArray(std::vector<const char*> sa)
  {
    return MsgVarArea::createStringArrayData(sa);
  }
  void setColNames(const char** colNames, size_t size)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNames = m_lrDataArea->appendStringArray(colNames, size);
  }

  void setColEncoding(std::vector<std::string>& colEncodings)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColEncoding = m_lrDataArea->appendStringArray(colEncodings);
  }
  void setColEncoding(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColEncoding = m_lrDataArea->appendData(data);
  }
  void setColTypes(const uint8_t* type, size_t size)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColTypes = m_lrDataArea->appendArray(type, size);
  }
  void setColTypes(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColTypes = m_lrDataArea->appendData(data);
  }
  void setColFlags(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColFlag = m_lrDataArea->appendData(data);
  }
  void setColNotNull(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNotNull = m_lrDataArea->appendData(data);
  }
  void setColSigned(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColSigned = m_lrDataArea->appendData(data);
  }
  void setColDecimals(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColDecimals = m_lrDataArea->appendData(data);
  }
  void setColDefault(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColDefault = m_lrDataArea->appendData(data);
  }
  void setPrimaryKeys(const int* pkIndex, size_t size)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkKeys = m_lrDataArea->appendArray(pkIndex, size);
  }
  void setPrimaryKeys(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkKeys = m_lrDataArea->appendData(data);
  }
  void setUniqueKeys(const int* ukIndex, size_t size)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfUkKeys = m_lrDataArea->appendArray(ukIndex, size);
  }
  void setUniqueKeys(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfUkKeys = m_lrDataArea->appendData(data);
  }

  void setRecordEncoding(const char* encoding)
  {
    if (useDMB)
      SET_OR_CLEAR_STRING(m_encoding, encoding);
    else
      ((PosOfLogMsg_vc*)m_posInfo)->m_encoding = m_lrDataArea->appendString(encoding);
  }

  void setSqlNo(int32_t sql_no)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_sqlNo = sql_no;
  }

  int32_t sqlNo()
  {
    if (m_creatingMode || m_parsedOK) {
      return (GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION) ? ((PosOfLogMsg_v3_2*)m_posInfo)->m_sqlNo
                                                                             : -1;
    } else {
      return -1;
    }
  }

  void setObTraceInfo(const char* ob_trace_info)
  {
    if (useDMB) {
      SET_OR_CLEAR_STRING(m_ob_trace_info, ob_trace_info);
    } else {
      ((PosOfLogMsg_vc*)m_posInfo)->m_posOfObTraceInfo = m_lrDataArea->appendString(ob_trace_info);
    }
  }

  const char* obTraceInfo()
  {
    if (m_creatingMode || m_parsedOK) {
      if (useDMB) {
        return m_ob_trace_info.c_str();
      } else {
        if (GET_LOGREC_SUB_VERSION(m_posInfo->m_id) >= LOGREC_SUB_VERSION) {
          return m_lrDataArea->getString(((PosOfLogMsg_vc*)m_posInfo)->m_posOfObTraceInfo);
        }
      }
    }
    return NULL;
  }

  void setColTypes(ITableMeta* meta, int size)
  {
    uint8_t* col_types = new uint8_t[size];
    for (int i = 0; i < size; i++)
      col_types[i] = (meta->getCol(i)->getType());
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColTypes = m_lrDataArea->appendArray(col_types, size);
    delete[] col_types;
  }
  void setColValuesBeforeImage()
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfOldCols = m_lrDataArea->appendStringArray(m_old_cols);
  }
  void setColValuesBeforeImage_1()
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfOldCols = m_lrDataArea->appendStringArray(m_old_clum, m_old_count);
  }
  void setColValuesAfterImage_1()
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfNewCols = m_lrDataArea->appendStringArray(m_new_clum, m_new_count);
  }
  void setRuleColValues()
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfFilterRuleVal =
        m_lrDataArea->appendStringArray(m_filter_value, m_filter_count);
  }
  void setColValuesAfterImage()
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfNewCols = m_lrDataArea->appendStringArray(m_new_cols);
  }
  void setTimemarks()
  {
    int timemarksSize = (int)m_timemarks.size();
    long* timemarks = new long[timemarksSize];
    for (int i = 0; i < timemarksSize; i++)
      timemarks[i] = m_timemarks[i];
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfTimemark = m_lrDataArea->appendArray(timemarks, timemarksSize);
    delete[] timemarks;
  }

  void setCheckpoint(uint64_t file, uint64_t offset)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfFileName = file;
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfFile = offset;
  }

  const char* getCheckpoint()
  {
    if (m_creatingMode || m_parsedOK) {
      snprintf(m_cpBuf, MAX_CP_BUF, "%" PRIu64 "@%" PRIu64, getFileOffset(), getFileNameOffset());
      return m_cpBuf;
    }
    return NULL;
  }

  uint64_t getFileNameOffset()
  {
    if (m_creatingMode || m_parsedOK) {
      return ((PosOfLogMsg_vc*)m_posInfo)->m_posOfFileName;
    }
    return 0;
  }

  uint64_t getFileOffset()
  {
    if (m_creatingMode || m_parsedOK)
      return ((PosOfLogMsg_vc*)m_posInfo)->m_posOfFile;
    return 0;
  }

  void setFirstInLogevent(bool b)
  {
    m_posInfo->m_firstInLogevent = b;
  }

  bool firstInLogevent()
  {
    return m_posInfo->m_firstInLogevent;
  }

  void setId(uint64_t id)
  {
    m_posInfo->m_id = LOGREC_SUB_VERSION | id;
  }

  void setThreadId(uint32_t thread_id)
  {
    m_endInfo->m_threadId = thread_id;
  }

  uint32_t getThreadId() const
  {
    if (m_creatingMode || m_tailParseOK)
      return m_endInfo->m_threadId;
    return 0;
  }

  void setRecordUsec(uint32_t usec)
  {
    m_endInfo->m_usec = usec;
  }

  uint32_t getRecordUsec() const
  {
    if (m_creatingMode || m_tailParseOK)
      return m_endInfo->m_usec;
    return 0;
  }

  uint64_t id()
  {
    return m_posInfo->m_id;
  }

  void setPKValue(std::vector<const char*> pkValue)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal = m_lrDataArea->appendStringArray(pkValue);
  }
  void setPKValue(const std::string& data)
  {
    ((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal = m_lrDataArea->appendData(data);
  }

  StrArray* pkValue() const
  {
    if (m_parsedOK)
      return m_lrDataArea->getStringArray(((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal);
    return NULL;
  }
  void getPKStringArrayData(const char*& v, size_t& size)
  {
    if (((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal != (uint32_t)-1)
      m_lrDataArea->getStringArrayData(((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal, v, size);
    else {
      v = NULL;
      size = 0;
    }
  }
  void elementAtNew(int off, char*& v, size_t& size)
  {
    if (m_parsedOK) {
      m_lrDataArea->getString(((PosOfLogMsg_vc*)m_posInfo)->m_posOfNewCols, off, v, size);
    } else {
      v = NULL;
      size = 0;
    }
  }
  void elementAtOld(int off, char*& v, size_t& size)
  {
    if (m_parsedOK) {
      m_lrDataArea->getString(((PosOfLogMsg_vc*)m_posInfo)->m_posOfOldCols, off, v, size);
    } else {
      v = NULL;
      size = 0;
    }
  }
  void elementAtPk(int off, char*& v, size_t& size)
  {
    if (m_parsedOK) {
      m_lrDataArea->getString(((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal, off, v, size);
    } else {
      v = NULL;
      size = 0;
    }
  }

  const std::vector<int>& pkKeys()
  {
    if (m_creatingMode || m_parsedOK) {
      if (m_pks.empty()) {
        const void* v = NULL;
        size_t elSize = 0;
        size_t size = 0;
        size_t i = 0;
        int ret = m_lrDataArea->getArray(((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkKeys, v, elSize, size);
        if (ret != 0 && elSize != sizeof(int)) {
          m_pks.clear();
        } else {
          for (i = 0; i < size; i++)
            m_pks.push_back(toLeEndianByType(((int*)v)[i]));
        }
      }
      return m_pks;
    }
    m_pks.clear();
    return m_pks;
  }

  const std::vector<int>& ukKeys()
  {
    if (m_creatingMode || m_parsedOK) {
      if (m_uks.empty()) {
        const void* v = NULL;
        size_t elSize = 0;
        size_t size = 0, i = 0;
        int ret = m_lrDataArea->getArray(((PosOfLogMsg_vc*)m_posInfo)->m_posOfUkKeys, v, elSize, size);
        if (ret != 0 && elSize != sizeof(int)) {
          m_uks.clear();
        } else {
          for (i = 0; i < size; i++)
            m_uks.push_back(toLeEndianByType(((int*)v)[i]));
        }
      }
      return m_uks;
    }
    m_uks.clear();
    return m_uks;
  }

  const char* getSerializedString(size_t* size)
  {
    if (!m_creatingMode && m_parsedOK) {
      /* if receive serialized record and been parsed.
       * get serialized string directly
       *
       */
      return (const char*)m_lrDataArea->getMsgBuf(*size);
    }
    /* if m_creatingMode is true or serialized
     * record not parsed success, return NULL
     */

    return NULL;
  }

  /**
   * Serialize the in-memory created record to a string
   *
   * @return the pointer to the serialized string, NULL is returned if failed
   */
  const char* toString(size_t* size, bool reserveMemory = false)
  {
    if (!m_creatingMode) {
      return getSerializedString(size);
    }
    /* Always use the latest version to do the serialization */
    int colCount = 0;
    uint8_t op = m_posInfo->m_op;
    if (useDMB)
      LogMsgSetHead(sizeof(PosOfLogMsg_vc));
    switch (op) {
      case EINSERT:
      case EDELETE:
      case EUPDATE:
      case EREPLACE:
      case INDEX_INSERT:
      case INDEX_UPDATE:
      case INDEX_REPLACE:
      case INDEX_DELETE: {
        if (!m_tblMeta) {
          /* Return NULL will terminate the call of parser */
          std::cout << "LOGMESSAGE: toString return 2" << std::endl;
          return NULL;
        }

        /**
         * Set the names of ordered columns, the order should be
         * the same as that to put old or new values of columns
         */
        m_tblMeta->trySerializeMetaDataAsMsgArea(m_extra_infos);

        if (useDMB) {
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNames = LogMsgAppendBuf(m_tblMeta->getNameData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColEncoding = LogMsgAppendBuf(m_tblMeta->getEncodingData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkKeys = LogMsgAppendBuf(m_tblMeta->getPkData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfUkKeys = LogMsgAppendBuf(m_tblMeta->getUkData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfPkVal = LogMsgAppendBuf(m_tblMeta->getkeyData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColTypes = LogMsgAppendBuf(m_tblMeta->getcolTypeData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColFlag = LogMsgAppendBuf(m_tblMeta->getColumnFlagData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNotNull = LogMsgAppendBuf(m_tblMeta->getNotNullData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColSigned = LogMsgAppendBuf(m_tblMeta->getSignedData());
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColDecimals = LogMsgAppendBuf(m_tblMeta->getDecimalsData());
          if (!m_timemarks.empty())
            ((PosOfLogMsg_vc*)m_posInfo)->m_posOfTimemark = LogMsgAppendDataArray(m_timemarks);
        } else {
          setColNames(m_tblMeta->getNameData());
          setColEncoding(m_tblMeta->getEncodingData());
          setPrimaryKeys(m_tblMeta->getPkData());
          setUniqueKeys(m_tblMeta->getUkData());
          setPKValue(m_tblMeta->getkeyData());
          if ((int)((PosOfLogMsg_vc*)m_posInfo)->m_encoding == -1)
            setRecordEncoding(m_tblMeta->getEncoding());
          setColTypes(m_tblMeta->getcolTypeData());
          setColFlags(m_tblMeta->getColumnFlagData());
          setColNotNull(m_tblMeta->getNotNullData());
          setColSigned(m_tblMeta->getSignedData());
          setColDecimals(m_tblMeta->getDecimalsData());
          /* Serialize timemarks if needed */
          if (!m_timemarks.empty())
            setTimemarks();
        }

        break;
      }
      case EDDL:
        if ((int)((PosOfLogMsg_vc*)m_posInfo)->m_encoding == -1 && !(useDMB && !m_encoding.empty()))
          setRecordEncoding("US-ASCII");
        colCount = m_new_count > 0 ? m_new_count : m_new_cols.size();
        if (!useDMB) {

          setColNames(ddlName, colCount);
          setColTypes(ddlType, colCount);
          /* Not know the actual encoding, dangerous */
        } else {
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNames = LogMsgAppendStringArray(ddlName, colCount);
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColTypes = LogMsgAppendDataArray(ddlType, colCount);
        }
        break;
      case EDML:
        if ((int)((PosOfLogMsg_vc*)m_posInfo)->m_encoding == -1 && !(useDMB && !m_encoding.empty()))
          setRecordEncoding("US-ASCII");
        if (!useDMB) {
          setColNames(&dmlName, 1);
          setColTypes(dmlType, 1);
          /* Not know the actual encoding, dangerous */
        } else {
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColNames = LogMsgAppendStringArray(&dmlName, 1);
          ((PosOfLogMsg_vc*)m_posInfo)->m_posOfColTypes = LogMsgAppendDataArray(dmlType, 1);
        }
        break;
      default:
        /**
         * Including HEARTBEAT record which has no columns, but only
         * the timestamp in PosOfLogMsg
         */
        if ((int)((PosOfLogMsg_vc*)m_posInfo)->m_encoding == -1)
          setRecordEncoding("US-ASCII");
        break;
    }

    /**
     * Serialize column values including two lines with ordered
     * columns, one is before the operation, the other is after.
     * For insert type, no old values, in a similar way, delete-type
     * record has no new values. Updating record has both values.
     */
    if (!useDMB) {
      if (m_old_count == 0 && m_new_count == 0 && (m_new_cols.size() > 0 || m_old_cols.size() > 0)) {
        setColValuesBeforeImage();
        setColValuesAfterImage();
      } else {
        setColValuesBeforeImage_1();
        setColValuesAfterImage_1();
      }
      setRuleColValues();
      if (m_endInfo != NULL) {
        unsigned char* endInfoToLe = new unsigned char[sizeof(EndOfLogMsg)];
        memcpy(endInfoToLe, (unsigned char*)m_endInfo, sizeof(EndOfLogMsg));
        exchangeEndInfoToLe(endInfoToLe, sizeof(EndOfLogMsg));
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfEndInfo =
            m_lrDataArea->appendArray((uint8_t*)endInfoToLe, sizeof(EndOfLogMsg));
        delete[] endInfoToLe;
      }
      /* Serialize the header */
      m_new_count = m_old_count = 0;
      m_filter_count = 0;
      const std::string& m = m_lrDataArea->getMessage();
      const void* v;
      size_t elSize, count;
      int ret = m_lrDataArea->getArray(0, v, elSize, count);
      if (ret != 0 || elSize != sizeof(uint8_t) || count != sizeof(PosOfLogMsg_vc)) {
        return NULL;
      }
      char* posInfoToLe = new char[sizeof(PosOfLogMsg_vc)];
      memcpy(posInfoToLe, (const char*)m_posInfo, sizeof(PosOfLogMsg_vc));
      exchangePosInfoToLe(posInfoToLe, sizeof(PosOfLogMsg_vc));

      memcpy((char*)v, posInfoToLe, sizeof(PosOfLogMsg_vc));
      /*force set header length to PosOfLogMsg_v3，because client will check it*/
      *((uint32_t*)v - 1) = toLeEndianByType((uint32_t)sizeof(PosOfLogMsg_v3));
      delete[] posInfoToLe;

      m_parsedOK = true;  // to support fetching
      *size = m.size();
      return m.c_str();
    } else {
      if ((int)((PosOfLogMsg_vc*)m_posInfo)->m_encoding == -1) {
        if (m_encoding.empty())
          ((PosOfLogMsg_vc*)m_posInfo)->m_encoding = LogMsgAppendString(m_tblMeta->getEncoding());
        else
          ((PosOfLogMsg_vc*)m_posInfo)->m_encoding = LogMsgAppendString(m_encoding);
      }
      if (m_old_count == 0 && m_new_count == 0 && (m_new_cols.size() > 0 || m_old_cols.size() > 0)) {
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfOldCols = LogMsgAppendStringArray(m_old_cols);
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfNewCols = LogMsgAppendStringArray(m_new_cols);
      } else {
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfOldCols = LogMsgAppendBuf(m_old_clum, m_old_count);
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfNewCols = LogMsgAppendBuf(m_new_clum, m_new_count);
      }
      if (m_dbName.size())
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfDbName = LogMsgAppendString(m_dbName.c_str(), m_dbName.size());
      if (m_tbName.size())
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfTbName = LogMsgAppendString(m_tbName.c_str(), m_tbName.size());
      ((PosOfLogMsg_vc*)m_posInfo)->m_posOfInstance = LogMsgAppendString(m_instance.c_str(), m_instance.size());
      ((PosOfLogMsg_vc*)m_posInfo)->m_posOfFilterRuleVal = LogMsgAppendBuf(m_filter_value, m_filter_count);
      if (m_endInfo != NULL) {
        unsigned char* endInfoToLe = new unsigned char[sizeof(EndOfLogMsg)];
        memcpy(endInfoToLe, (unsigned char*)m_endInfo, sizeof(EndOfLogMsg));
        exchangeEndInfoToLe(endInfoToLe, sizeof(EndOfLogMsg));
        ((PosOfLogMsg_vc*)m_posInfo)->m_posOfEndInfo =
            LogMsgAppendDataArray((unsigned char*)endInfoToLe, sizeof(EndOfLogMsg));
        delete[] endInfoToLe;
      }

      char* posInfoToLe = new char[sizeof(PosOfLogMsg_vc)];
      memcpy(posInfoToLe, (const char*)m_posInfo, sizeof(PosOfLogMsg_vc));
      exchangePosInfoToLe(posInfoToLe, sizeof(PosOfLogMsg_vc));
      LogMsgCopyHead(posInfoToLe, sizeof(PosOfLogMsg_vc));
      LogMsgFroceSetHeadSize(sizeof(PosOfLogMsg_v3));
      delete[] posInfoToLe;

      const char* msg = LogMsgGetString(size);

      /* Serialize the header */
      if (reserveMemory) {
        m_reservedMemory = true;
        /* Only when useDMB is true, the memory of dmb should be copied */
        if (m_lrDataArea->copy(msg, *size) != 0) {
          std::cout << "LOGMESSAGE: toString return 3" << std::endl;
          return NULL;
        }
      }
      m_new_count = m_old_count = 0;
      m_filter_count = 0;
      m_parsedOK = true;  // to support fetching
      return msg;
    }
  }
};

const char* LogRecInfo::ddlName[2] = {"ddl", "env"};
const char* LogRecInfo::dmlName = "dml";
const char* LogRecInfo::heartbeatName = "heartbeat";
uint8_t LogRecInfo::ddlType[2] = {LOGMSG_TYPE_VAR_STRING, LOGMSG_TYPE_VAR_STRING};
uint8_t LogRecInfo::dmlType[1] = {LOGMSG_TYPE_VAR_STRING};
uint8_t LogRecInfo::heartbeatType[1] = {LOGMSG_TYPE_LONG};

LogRecordImpl::LogRecordImpl(time_t timestamp, ITableMeta* tblMeta)
{
  m_lr = new LogRecInfo(timestamp, tblMeta);
  m_timemarked = false;
  m_userData = NULL;
}

LogRecordImpl::LogRecordImpl(const void* ptr, size_t size)
{
  m_lr = new LogRecInfo(ptr, size);
  m_timemarked = false;
  m_userData = NULL;
}

LogRecordImpl::LogRecordImpl(bool creating, bool useDMB)
{
  m_lr = new LogRecInfo(creating, useDMB);
  m_timemarked = false;
  m_userData = NULL;
}

/**
 * clear() must be called before here if users want to manage
 * the memory themselves, otherwise their memory would be freed
 * and uncertain error or crash would come out.
 */
LogRecordImpl::~LogRecordImpl()
{
  delete m_lr;
}

/**
 * Users may want to manage memory in heap by themselves, so
 * clear() only free the memory allocated within LogRecordImpl,
 * while clearWithUserMemory() will also clear users memory,
 * which now is actually strings stored in m_old_cols and m_new_cols.
 */
void LogRecordImpl::clear()
{
  m_lr->clear();
  m_timemarked = false;
}

void LogRecordImpl::clearWithUserMemory()
{
  if (m_lr->m_creatingMode) {
    clearOld();
    clearNew();
  }
  clear();
}

void LogRecordImpl::clearExpiredMeta()
{
  m_lr->clearExpiredMeta();
}
int16_t LogRecordImpl::getRecordHash(
    int16_t (*hashFunc)(const char** valueList, const size_t* valueSizeList, int count))
{
  return m_lr->getRecordHash(hashFunc);
}
void LogRecordImpl::clearOld()
{
  m_lr->clearOld();
}

void LogRecordImpl::clearNew()
{
  m_lr->clearNew();
}

int LogRecordImpl::parse(const void* ptr, size_t size)
{
  m_buf.assign((const char*)ptr, size);
  return m_lr->parse(m_buf.c_str(), size);
}
int LogRecordImpl::parseFast(const void* ptr, size_t size)
{
  return m_lr->parse(ptr, size);
}

bool LogRecordImpl::parsedOK()
{
  return m_lr->m_parsedOK;
}

size_t LogRecordImpl::getRealSize()
{
  return m_lr->getRealSize();
}

int LogRecordImpl::putOld(std::string* val)
{
  return m_lr->putOld(val);
}

int LogRecordImpl::putFilterRuleVal(const char* pos, int len)
{
  return m_lr->putFilterRuleVal(pos, len);
}

int LogRecordImpl::putNew(std::string* val)
{
  return m_lr->putNew(val);
}
int LogRecordImpl::putOld(const char* pos, int len)
{
  return m_lr->putOld(pos, len);
}

int LogRecordImpl::putNew(const char* pos, int len)
{
  return m_lr->putNew(pos, len);
}
void LogRecordImpl::setNewColumn(BinLogBuf* buf, int size)
{
  return m_lr->setNewColumn(buf, size);
}
void LogRecordImpl::setOldColumn(BinLogBuf* buf, int size)
{
  return m_lr->setOldColumn(buf, size);
}
int LogRecordImpl::getColumnCount()
{
  return m_lr->getColumncount();
}
const std::vector<std::string*>& LogRecordImpl::oldCols()
{
  return m_lr->m_old_cols;
}
BinLogBuf* LogRecordImpl::oldCols(unsigned int& count)
{
  count = m_lr->m_old_count;
  return m_lr->m_old_clum;
}
StrArray* LogRecordImpl::parsedOldCols() const
{
  return m_lr->oldCols();
}

const std::vector<std::string*>& LogRecordImpl::newCols()
{
  return m_lr->m_new_cols;
}
BinLogBuf* LogRecordImpl::newCols(unsigned int& count)
{
  count = m_lr->m_new_count;
  return m_lr->m_new_clum;
}
const BinLogBuf* LogRecordImpl::filterValues(unsigned int& count)
{
  count = m_lr->m_filter_count;
  return m_lr->m_filter_value;
}
StrArray* LogRecordImpl::parsedNewCols() const
{
  return m_lr->newCols();
}

StrArray* LogRecordImpl::parsedFilterRuleValues() const
{
  return m_lr->filterValues();
}

StrArray* LogRecordImpl::parsedColNames() const
{
  return m_lr->colNames();
}

StrArray* LogRecordImpl::parsedColEncodings() const
{
  return m_lr->colEncodings();
}

const uint8_t* LogRecordImpl::parsedColTypes() const
{
  return m_lr->colTypes();
}
const uint8_t* LogRecordImpl::parsedColFlags() const
{
  return m_lr->colFlags();
}

const char* LogRecordImpl::getSerializedString(size_t* size)
{
  return m_lr->getSerializedString(size);
}

const char* LogRecordImpl::toString(size_t* size, bool reserveMemory)
{
  return m_lr->toString(size, reserveMemory);
}

const char* LogRecordImpl::getFormatedString(size_t* size)
{
  if (true == m_lr->m_parsedOK) {
    if (true == m_lr->m_creatingMode) {
      // if record is not parsed from serialized,data head maybe updated，ex:recordid,should reset head info
      const void* v;
      size_t elSize, count;
      const std::string& m = m_lr->m_lrDataArea->getMessage();
      int ret = m_lr->m_lrDataArea->getArray(0, v, elSize, count);
      if (ret != 0 || elSize != sizeof(uint8_t) || (count != sizeof(PosOfLogMsg_v3))) {
        return NULL;
      }
      memcpy((void*)v, (PosOfLogMsg_vc*)(m_lr->m_posInfo), sizeof(PosOfLogMsg_vc));
      m_lr->exchangePosInfoToLe((const char*)v, count);
      *((uint32_t*)v - 1) = toLeEndianByType((uint32_t)sizeof(PosOfLogMsg_v3));
      *size = m.size();
      return m.c_str();
    } else {
      return getSerializedString(size);
    }
  } else {
    return NULL;
  }
}

int LogRecordImpl::recordType()
{
  return m_lr->recordType();
}

int LogRecordImpl::setRecordType(int aType)
{
  return m_lr->setRecordType(aType);
}

void LogRecordImpl::setTimestamp(time_t timestamp)
{
  m_lr->setTimestamp(timestamp);
}

time_t LogRecordImpl::getTimestamp()
{
  return m_lr->getTimestamp();
}

void LogRecordImpl::setInstance(const char* instance)
{
  m_lr->setInstance(instance);
}

const char* LogRecordImpl::instance() const
{
  return m_lr->instance();
}

void LogRecordImpl::setDbname(const char* dbname)
{
  m_lr->setDbname(dbname);
}

const char* LogRecordImpl::dbname() const
{
  return m_lr->dbname();
}

void LogRecordImpl::setTbname(const char* tbname)
{
  m_lr->setTbname(tbname);
}

const char* LogRecordImpl::tbname() const
{
  return m_lr->tbname();
}

void LogRecordImpl::setCheckpoint(uint64_t file, uint64_t offset)
{
  m_lr->setCheckpoint(file, offset);
}

const char* LogRecordImpl::getCheckpoint()
{
  return m_lr->getCheckpoint();
}

uint64_t LogRecordImpl::getFileNameOffset()
{
  return m_lr->getFileNameOffset();
}

uint64_t LogRecordImpl::getFileOffset()
{
  return m_lr->getFileOffset();
}

void LogRecordImpl::setTableMeta(ITableMeta* tblMeta)
{
  m_lr->m_tblMeta = tblMeta;
}

ITableMeta* LogRecordImpl::getTableMeta()
{
  return m_lr->m_tblMeta;
}

bool LogRecordImpl::isParsedRecord()
{
  if (!m_lr->m_creatingMode && m_lr->m_parsedOK) {
    return true;
  }
  return false;
}

int LogRecordImpl::getTableMeta(ITableMeta*& tblMeta)
{

  if (!m_lr->m_creatingMode && m_lr->m_parsedOK) {
    // if record is parsed from string
    if (tblMeta == NULL) {
      // tblMeta must allocated before call getTableMeta
      return -1;
    }

    tblMeta->setName(tbname());
    // set colNames
    StrArray* colNames = m_lr->colNames();
    const uint8_t* colTypes = m_lr->colTypes();
    const uint8_t* colFlags = m_lr->colFlags();
    StrArray* colEncodings = m_lr->colEncodings();
    const uint8_t* colNotNull = m_lr->colNotNull();
    const uint8_t* colSigned = m_lr->colSigned();
    const int32_t* colDecimals = m_lr->colDecimals();
    const std::vector<int> pkIndice = m_lr->pkKeys();
    size_t pkSize = pkIndice.size();
    const std::vector<int> ukIndice = m_lr->ukKeys();
    size_t ukSize = ukIndice.size();
    // pk or uk values
    StrArray* pkVal = m_lr->pkValue();
    std::string pkKeys;
    std::string ukKeys;
    size_t ukc = 0;
    if (colNames != NULL) {
      for (int i = 0; i < (int)colNames->size(); i++) {
        IColMeta* colMeta = new IColMeta();
        colMeta->setName((*colNames)[i]);
        if (colEncodings != NULL) {
          colMeta->setEncoding((*colEncodings)[i]);
        }
        if (colTypes != NULL) {
          colMeta->setType(colTypes[i]);
        }
        if (colFlags != NULL) {
          colMeta->setFlag(colFlags[i]);
        }
        if (colNotNull != NULL) {
          colMeta->setNotNull(colNotNull[i]);
        }
        if (colSigned != NULL) {
          colMeta->setSigned(colSigned[i]);
        }
        if (colDecimals != NULL) {
          colMeta->setDecimals(toLeEndianByType(colDecimals[i]));
        }
        /*
        if (colDefault != NULL) {
            colMeta->setDefault((*colDefault)[i]);
        }*/

        // find all pks
        for (size_t iPk = 0; iPk < pkSize; iPk++) {
          if (pkIndice[iPk] == i) {
            colMeta->setIsPK(true);
            tblMeta->setHasPK(true);
          }
        }
        // find all uk
        for (size_t iUk = 0; iUk < ukSize; iUk++) {
          if (ukIndice[iUk] == i) {
            colMeta->setIsUK(true);
            tblMeta->setHasUK(true);
            ukKeys.append((*colNames)[i]);
            if (ukc < ukSize - 1 && ukSize != 1) {
              ukKeys.append(",");
            }
            ukc++;
          }
        }
        tblMeta->append((*colNames)[i], colMeta);
      }
    }

    if (pkVal != NULL) {
      // set pk and uk values
      if (pkVal->size() == 1) {
        if (tblMeta->hasPK()) {
          // if only has pk
          tblMeta->setPkinfo((*pkVal)[0]);
          // get pk keys from pk info
          tblMeta->getKeysFromInfo((*pkVal)[0], pkKeys, colNames);
        } else {
          // if only has uk
          tblMeta->setUkinfo((*pkVal)[0]);
        }
      } else if (pkVal->size() == 2) {
        tblMeta->setPkinfo((*pkVal)[0]);
        tblMeta->setUkinfo((*pkVal)[1]);
        // get pk keys from pk info
        tblMeta->getKeysFromInfo((*pkVal)[0], pkKeys, colNames);
      }
    }
    if (!pkKeys.empty()) {
      tblMeta->setPKs(pkKeys.c_str());
    }
    if (!ukKeys.empty()) {
      tblMeta->setUKs(ukKeys.c_str());
    }
    if (colNames != NULL) {
      delete colNames;
    }
    if (colEncodings != NULL) {
      delete colEncodings;
    }
    if (pkVal != NULL) {
      delete pkVal;
    }
    /*
    if (colDefault != NULL) {
        delete colDefault;
    }*/
    return 0;
  }
  if (tblMeta != NULL) {
    // if record is not parsed from serialized data, tblMeta can only be null.
    return -2;
  }
  tblMeta = m_lr->m_tblMeta;
  return 0;
}

void LogRecordImpl::setDBMeta(IDBMeta* dbMeta)
{
  m_lr->m_dbMeta = dbMeta;
}

IDBMeta* LogRecordImpl::getDBMeta()
{
  return m_lr->m_dbMeta;
}

void LogRecordImpl::setExpiredTableMeta(ITableMeta* tblMeta)
{
  m_lr->m_expiredTableMeta.push_back(tblMeta);
}

std::vector<ITableMeta*>& LogRecordImpl::getExpiredTableMeta()
{
  return m_lr->m_expiredTableMeta;
}

void LogRecordImpl::setExpiredDBMeta(IDBMeta* dbMeta)
{
  m_lr->m_expiredDBMeta.push_back(dbMeta);
}

std::vector<IDBMeta*>& LogRecordImpl::getExpiredDBMeta()
{
  return m_lr->m_expiredDBMeta;
}

void LogRecordImpl::setExpiredMetaDataCollections(IMetaDataCollections* imc)
{
  m_lr->m_expiredMetaDataCollections = imc;
}

IMetaDataCollections* LogRecordImpl::getExpiredMetaDataCollections()
{
  return m_lr->m_expiredMetaDataCollections;
}

void LogRecordImpl::setExtraInfo(const char* pkValue)
{
  m_lr->m_extra_infos.push_back(pkValue);
}

StrArray* LogRecordImpl::extraInfo() const
{
  return m_lr->pkValue();
}

const std::vector<int>& LogRecordImpl::pkKeys()
{
  return m_lr->pkKeys();
}

const std::vector<int>& LogRecordImpl::ukKeys()
{
  return m_lr->ukKeys();
}

void LogRecordImpl::setFirstInLogevent(bool b)
{
  m_lr->setFirstInLogevent(b);
}

bool LogRecordImpl::firstInLogevent()
{
  return m_lr->firstInLogevent();
}

void LogRecordImpl::setId(uint64_t id)
{
  m_lr->setId(id);
}

uint64_t LogRecordImpl::id()
{
  return m_lr->id();
}

void LogRecordImpl::setThreadId(uint32_t threadId)
{
  m_lr->setThreadId(threadId);
}

uint32_t LogRecordImpl::getThreadId()
{
  return m_lr->getThreadId();
}

void LogRecordImpl::setRecordUsec(uint32_t recordUsec)
{
  m_lr->setRecordUsec(recordUsec);
}

uint32_t LogRecordImpl::getRecordUsec()
{
  return m_lr->getRecordUsec();
}

void LogRecordImpl::setSrcType(int type)
{
  m_lr->setSrcType(type);
}

int LogRecordImpl::getSrcType() const
{
  return m_lr->getSrcType();
}

void LogRecordImpl::setSrcCategory(int category)
{
  m_lr->setSrcCategory(category);
}

int LogRecordImpl::getSrcCategory() const
{
  return m_lr->getSrcCategory();
}

uint64_t LogRecordImpl::getCheckpoint1()
{
  return getFileNameOffset();
}

uint64_t LogRecordImpl::getCheckpoint2()
{
  return getFileOffset();
}

const char* LogRecordImpl::recordEncoding()
{
  return m_lr->recordEncoding();
}

/* Performance statistics */

std::vector<long>& LogRecordImpl::getTimemark()
{
  return m_lr->getTimemark();
}

std::vector<long>& LogRecordImpl::getTimemark(size_t& length)
{
  return m_lr->getTimemark(length);
}

void LogRecordImpl::addTimemark(long time)
{
  m_lr->addTimemark(time);
}

void LogRecordImpl::setTimemarked(bool marked)
{
  m_timemarked = marked;
}

bool LogRecordImpl::isTimemarked() const
{
  return m_timemarked;
}

void LogRecordImpl::curveTimemark()
{
  struct timeval t_parsedTime;
  gettimeofday(&t_parsedTime, NULL);
  addTimemark(t_parsedTime.tv_sec * 1000000 + t_parsedTime.tv_usec);
}
void LogRecordImpl::elementAtPk(int off, char*& v, size_t& size) const
{
  m_lr->elementAtPk(off, v, size);
}
void LogRecordImpl::elementAtNew(int off, char*& v, size_t& size) const
{
  m_lr->elementAtNew(off, v, size);
}
void LogRecordImpl::elementAtOld(int off, char*& v, size_t& size) const
{
  m_lr->elementAtOld(off, v, size);
}
const char* LogRecordImpl::parseColumnValue(const char* columnName, size_t* size, int* columnType)
{
  int type = m_lr->recordType();
  char* value = NULL;
  StrArray* colNames = m_lr->colNames();
  const uint8_t* colTypes = m_lr->colTypes();
  if (colNames == NULL || colTypes == NULL)
    goto END;
  for (int i = colNames->size() - 1; i >= 0; i--) {
    if (strcmp((*colNames)[i], columnName) == 0) {
      *columnType = colTypes[i];
      if (type == EUPDATE || type == EINSERT)
        m_lr->elementAtNew(i, value, *size);
      else if (type == EDELETE)
        m_lr->elementAtOld(i, value, *size);
      else
        *columnType = 255;
      break;
    }
  }
END:
  if (colNames)
    delete colNames;
  return value;
  ;
}
const char* LogRecordImpl::parseColumnValue(const char* columnName, size_t* size, int* columnType, bool isPre)
{
  // LOGMSG_TYPES: not exist column type
  *columnType = LOGMSG_TYPES;
  char* value = NULL;
  StrArray* colNames = m_lr->colNames();
  const uint8_t* colTypes = m_lr->colTypes();
  if (colNames == NULL) {
    return NULL;
  } else if (colTypes == NULL) {
    delete colNames;
    return NULL;
  }
  for (int i = colNames->size() - 1; i >= 0; i--) {
    if (strcasecmp((*colNames)[i], columnName) == 0) {
      *columnType = colTypes[i];
      if (isPre) {
        m_lr->elementAtOld(i, value, *size);
      } else {
        m_lr->elementAtNew(i, value, *size);
      }
    }
  }
  if (colNames)
    delete colNames;
  return value;
}
void LogRecordImpl::setUserData(void* data)
{
  m_userData = data;
}

void* LogRecordImpl::getUserData()
{
  return m_userData;
}

void LogRecordImpl::setRecordEncoding(const char* encoding)
{
  m_lr->setRecordEncoding(encoding);
}

void LogRecordImpl::setSqlNo(int32_t sql_no)
{
  m_lr->setSqlNo(sql_no);
}

int32_t LogRecordImpl::sqlNo()
{
  return m_lr->sqlNo();
}

void LogRecordImpl::setObTraceInfo(const char* ob_trace_info)
{
  m_lr->setObTraceInfo(ob_trace_info);
}

const char* LogRecordImpl::obTraceInfo()
{
  return m_lr->obTraceInfo();
}

void LogRecordImpl::setHashFuncId(int id)
{
  m_lr->m_tblMeta->setHashFuncId(id);
}
bool LogRecordImpl::hashColumnSetted()
{
  return m_lr->m_tblMeta->hashColumnSetted();
}
int LogRecordImpl::getHashFuncId()
{
  return m_lr->m_tblMeta->getHashFuncId();
}
int LogRecordImpl::setHashCol(std::vector<std::string>& hashColumns)
{
  return m_lr->m_tblMeta->setHashCol(hashColumns);
}
void LogRecordImpl::setHashColByPK()
{
  return m_lr->m_tblMeta->setHashColByPK();
}
void LogRecordImpl::getPKStringArrayData(const char*& v, size_t& size)
{
  m_lr->getPKStringArrayData(v, size);
}

}  // namespace logmessage
}  // namespace oceanbase
