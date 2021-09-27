/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#ifndef _LOG_REC_H_
#define _LOG_REC_H_

#include <string>
#include <vector>
#include <cstdint>
#include "UserData.h"
#include "BinLogBuf.h"

namespace oceanbase {
namespace logmessage {

class StrArray;
struct LogRecInfo;

#define BR_FAKE_DDL_COLNAME "ddl"
typedef enum RecordType {
  EINSERT = 0,
  EUPDATE,
  EDELETE,
  EREPLACE,
  HEARTBEAT,
  CONSISTENCY_TEST,
  EBEGIN,
  ECOMMIT,
  EDDL,
  EROLLBACK,
  EDML,

  INDEX_INSERT = 128,
  INDEX_UPDATE,
  INDEX_DELETE,
  INDEX_REPLACE,
  EUNKNOWN = 255,
} RECORD_TYPE;

enum FORMATERR {
  FORMATOK = 0,
  BUFOVERFLOW,
  TYPENOTSUPPORT,
  UNKNOWNTYPE,
  UNKNOWNOPTYPE,
  NULLTBLMETA,
  NULLCOLMETA,
  ENUMSETNULL
};

enum SOURCE_TYPE {
  SRC_MYSQL = 0x00,          // MySQL
  SRC_OCEANBASE = 0x01,      // Oceanbase
  SRC_HBASE = 0X02,          // HBase
  SRC_ORACLE = 0x03,         // Oracle
  SRC_OCEANBASE_1_0 = 0x04,  // Oceanbase V1
  SRC_DB2 = 0x05,            // DB2
  SRC_UNKNOWN = 0x06
};

enum SOURCE_CATEGORY {
  SRC_FULL_RECORDED = 0,  // can get all column value from redo log
  SRC_FULL_RETRIEVED,     // can get part column value from redo log,other need fetch
  SRC_FULL_FAKED,
  SRC_PART_RECORDED,
  SRC_NO
};

class ITableMeta;
class IDBMeta;
class IMetaDataCollections;
class ILogRecord : public UserDataInterface {
public:
  virtual ~ILogRecord()
  {}

public:
  /* setter and getter */

  // must set: record src type
  virtual void setSrcType(int type) = 0;
  virtual int getSrcType() const = 0;

  // must set
  virtual void setSrcCategory(int cartegory) = 0;
  virtual int getSrcCategory() const = 0;

  virtual void setThreadId(uint32_t threadId) = 0;
  virtual uint32_t getThreadId() = 0;

  // must set: record's create time
  virtual void setTimestamp(long timestamp) = 0;
  virtual time_t getTimestamp() = 0;

  // must set: record's type,like insert/delete/update/replace/heartbeat...
  virtual int setRecordType(int aType) = 0;
  virtual int recordType() = 0;

  // must set: ITableMeta's encoding
  virtual const char* recordEncoding() = 0;

  // must set: database name
  virtual void setDbname(const char* db) = 0;
  virtual const char* dbname() const = 0;

  // must set: table name
  virtual void setTbname(const char* table) = 0;
  virtual const char* tbname() const = 0;

  // musst set when creating mode, parse mode no need: table's meta info
  virtual void setTableMeta(ITableMeta* tblMeta) = 0;
  virtual int getTableMeta(ITableMeta*& tblMeta) = 0;
  // return table meta directly
  virtual ITableMeta* getTableMeta() = 0;
  // is record parsed from serialized data
  virtual bool isParsedRecord() = 0;

  virtual void setDBMeta(IDBMeta* dbMeta) = 0;
  virtual IDBMeta* getDBMeta() = 0;

  virtual void setExpiredTableMeta(ITableMeta* tblMeta) = 0;
  virtual std::vector<ITableMeta*>& getExpiredTableMeta() = 0;

  virtual void setExpiredDBMeta(IDBMeta* dbMeta) = 0;
  virtual std::vector<IDBMeta*>& getExpiredDBMeta() = 0;

  virtual void setExpiredMetaDataCollections(IMetaDataCollections* imc) = 0;
  virtual IMetaDataCollections* getExpiredMetaDataCollections() = 0;

  // muset set: set checkpoint,return like %d@%d
  virtual void setCheckpoint(uint64_t file, uint64_t offset) = 0;
  virtual const char* getCheckpoint() = 0;

  /**
   * ob: getCheckpoint1 High 32 bit checkpoint, getCheckpoint2 Low 32 bit checkpoint
   */
  virtual uint64_t getCheckpoint1() = 0;
  virtual uint64_t getCheckpoint2() = 0;

  virtual uint64_t getFileNameOffset() = 0;
  virtual uint64_t getFileOffset() = 0;

  virtual void setFirstInLogevent(bool b) = 0;
  virtual bool firstInLogevent() = 0;

  virtual void setId(uint64_t id) = 0;
  virtual uint64_t id() = 0;

  virtual void setInstance(const char* instance) = 0;
  virtual const char* instance() const = 0;

  // this value will save at pkValue
  virtual void setExtraInfo(const char* info) = 0;
  virtual StrArray* extraInfo() const = 0;

  virtual bool isTimemarked() const = 0;
  virtual void setTimemarked(bool marked) = 0;
  virtual void addTimemark(long time) = 0;
  virtual std::vector<long>& getTimemark() = 0;
  virtual std::vector<long>& getTimemark(size_t& length) = 0;
  virtual void curveTimemark() = 0;

public:
  /* column data */

  /**
   * 1. insert no old value,only has new value
   * 2. delete no new value,only has old value
   * 3. update/replace has new and old value
   * 4. column must sort by ITableMeta's column
   */
  virtual int putOld(std::string* val) = 0;
  virtual int putNew(std::string* val) = 0;
  virtual int putOld(const char* pos, int len) = 0;
  virtual int putNew(const char* pos, int len) = 0;
  virtual void setNewColumn(BinLogBuf* buf, int size) = 0;
  virtual void setOldColumn(BinLogBuf* buf, int size) = 0;
  virtual int getColumnCount() = 0;
  virtual int putFilterRuleVal(const char* pos, int len) = 0;
  virtual int16_t getRecordHash(
      int16_t (*hashFunc)(const char** valueList, const size_t* valueSizeList, int count)) = 0;
  virtual bool hashColumnSetted() = 0;
  virtual void setHashFuncId(int id) = 0;
  virtual int getHashFuncId() = 0;
  virtual int setHashCol(std::vector<std::string>& hashColumns) = 0;
  virtual void setHashColByPK() = 0;

  virtual void clearOld() = 0;
  virtual void clearNew() = 0;

  // get all old value
  virtual const std::vector<std::string*>& oldCols() = 0;
  virtual BinLogBuf* oldCols(unsigned int& count) = 0;
  virtual StrArray* parsedOldCols() const = 0;

  // get all new value
  virtual const std::vector<std::string*>& newCols() = 0;
  virtual BinLogBuf* newCols(unsigned int& count) = 0;
  virtual StrArray* parsedNewCols() const = 0;

  // get all column names
  virtual StrArray* parsedColNames() const = 0;

  virtual StrArray* parsedColEncodings() const = 0;
  virtual StrArray* parsedFilterRuleValues() const = 0;

  virtual const uint8_t* parsedColTypes() const = 0;
  virtual const uint8_t* parsedColFlags() const = 0;

  virtual const std::vector<int>& pkKeys() = 0;

  virtual const std::vector<int>& ukKeys() = 0;

public:
  /* tostring and parse */

  virtual int parse(const void* ptr, size_t size) = 0;
  virtual int parseFast(const void* ptr, size_t size) = 0;
  /**
   * @return true success; false fail
   */
  virtual bool parsedOK() = 0;

  virtual size_t getRealSize() = 0;

  /**
   * serialize
   * @return serialized ptr
   */
  virtual const char* toString(size_t* size, bool reserveMemory = false) = 0;

  /**
   * @return serialized ptr
   */
  virtual const char* getFormatedString(size_t* size) = 0;

  /**
   * clear all to reuse, no free memory
   */
  virtual void clear() = 0;

  /**
   * clear all,include memory
   */
  virtual void clearWithUserMemory() = 0;

  virtual void clearExpiredMeta() = 0;

  virtual void setRecordEncoding(const char* encoding) = 0;

  virtual void setSqlNo(int32_t sql_no) = 0;
  virtual int32_t sqlNo() = 0;
  virtual void setObTraceInfo(const char* ob_trace_info) = 0;
  virtual const char* obTraceInfo() = 0;

  // must set: record's create us
  virtual void setRecordUsec(uint32_t usec) = 0;
  virtual uint32_t getRecordUsec() = 0;

  // get serialized string of record directly
  virtual const char* getSerializedString(size_t* size) = 0;
};

class LogRecordImpl : public ILogRecord {
public:
  LogRecordImpl(time_t timestamp, ITableMeta* tblMeta);
  LogRecordImpl(const void* ptr, size_t size);
  LogRecordImpl(bool creating = true, bool useDMB = false);
  virtual ~LogRecordImpl();

public:
  /* setter and getter */

  virtual void setSrcType(int type);
  virtual int getSrcType() const;

  virtual void setSrcCategory(int cartegory);
  virtual int getSrcCategory() const;

  virtual void setTimestamp(long timestamp);
  virtual time_t getTimestamp();

  virtual int setRecordType(int aType);
  virtual int recordType();

  virtual const char* recordEncoding();

  virtual void setDbname(const char* db);
  virtual const char* dbname() const;

  virtual void setTbname(const char* table);
  virtual const char* tbname() const;

  virtual void setTableMeta(ITableMeta* tblMeta);
  // if record is parsed from serialized data, tblMeta can not be null
  // and will be set values parsed from serialized data.
  // if tblMeta is Null pointer, will set current table meta to tblMeta
  virtual int getTableMeta(ITableMeta*& tblMeta);
  // return table meta directly
  virtual ITableMeta* getTableMeta();
  // is record parsed from serialized data
  virtual bool isParsedRecord();

  virtual void setDBMeta(IDBMeta* dbMeta);
  virtual IDBMeta* getDBMeta();

  virtual void setExpiredTableMeta(ITableMeta* tblMeta);
  virtual std::vector<ITableMeta*>& getExpiredTableMeta();

  virtual void setExpiredDBMeta(IDBMeta* dbMeta);
  virtual std::vector<IDBMeta*>& getExpiredDBMeta();

  virtual void setExpiredMetaDataCollections(IMetaDataCollections* imc);
  virtual IMetaDataCollections* getExpiredMetaDataCollections();

  virtual void setCheckpoint(uint64_t file, uint64_t offset);
  virtual const char* getCheckpoint();

  virtual uint64_t getCheckpoint1();
  virtual uint64_t getCheckpoint2();

  virtual uint64_t getFileNameOffset();
  virtual uint64_t getFileOffset();

  virtual void setFirstInLogevent(bool b);
  virtual bool firstInLogevent();

  virtual void setRecordUsec(uint32_t usec);
  virtual uint32_t getRecordUsec();

  virtual void setId(uint64_t id);
  virtual uint64_t id();

  virtual void setInstance(const char* instance);
  virtual const char* instance() const;

  virtual void setExtraInfo(const char* info);
  virtual StrArray* extraInfo() const;

  bool isTimemarked() const;
  void setTimemarked(bool marked);
  void addTimemark(long time);
  std::vector<long>& getTimemark();
  std::vector<long>& getTimemark(size_t& length);
  virtual void curveTimemark();

  virtual void setThreadId(uint32_t threadId);
  virtual uint32_t getThreadId();

public:
  /* column data */

  virtual int putOld(std::string* val);
  virtual int putNew(std::string* val);
  virtual int putOld(const char* pos, int len);
  virtual int putNew(const char* pos, int len);
  virtual void setNewColumn(BinLogBuf* buf, int size);
  virtual void setOldColumn(BinLogBuf* buf, int size);
  virtual int getColumnCount();
  virtual int putFilterRuleVal(const char* pos, int len);

  virtual void clearOld();
  virtual void clearNew();

  virtual const std::vector<std::string*>& oldCols();
  virtual BinLogBuf* oldCols(unsigned int& count);
  virtual StrArray* parsedOldCols() const;

  virtual const std::vector<std::string*>& newCols();
  virtual BinLogBuf* newCols(unsigned int& count);
  virtual const BinLogBuf* filterValues(unsigned int& count);
  virtual StrArray* parsedNewCols() const;
  virtual StrArray* parsedFilterRuleValues() const;

  virtual StrArray* parsedColNames() const;

  virtual StrArray* parsedColEncodings() const;

  virtual const uint8_t* parsedColTypes() const;
  virtual const uint8_t* parsedColFlags() const;
  virtual const std::vector<int>& pkKeys();

  virtual const std::vector<int>& ukKeys();
  void elementAtPk(int off, char*& v, size_t& size) const;
  void elementAtNew(int off, char*& v, size_t& size) const;
  void elementAtOld(int off, char*& v, size_t& size) const;
  const char* parseColumnValue(const char* columnName, size_t* size, int* columnType);
  const char* parseColumnValue(const char* columnName, size_t* size, int* columnType, bool isPre);

public:
  /* tostring and parse */

  virtual int parse(const void* ptr, size_t size);
  virtual int parseFast(const void* ptr, size_t size);
  virtual bool parsedOK();

  virtual size_t getRealSize();

  virtual const char* toString(size_t* size, bool reserveMemory = false);

  virtual const char* getFormatedString(size_t* size);

  virtual void clear();

  virtual void clearWithUserMemory();

  virtual void clearExpiredMeta();
  virtual bool hashColumnSetted();
  virtual int16_t getRecordHash(int16_t (*hashFunc)(const char** valueList, const size_t* valueSizeList, int count));
  virtual void setHashFuncId(int id);
  virtual int getHashFuncId();
  virtual int setHashCol(std::vector<std::string>& hashColumns);
  virtual void setHashColByPK();
  virtual void getPKStringArrayData(const char*& v, size_t& size);

public:
  virtual void setUserData(void* data);
  virtual void* getUserData();

  virtual void setRecordEncoding(const char* encoding);

  virtual void setSqlNo(int32_t sql_no);
  virtual int32_t sqlNo();
  virtual void setObTraceInfo(const char* ob_trace_info);
  virtual const char* obTraceInfo();

  virtual const char* getSerializedString(size_t* size);

protected:
  LogRecInfo* m_lr;
  std::string m_buf;
  bool m_timemarked;
  void* m_userData;
};

}  // namespace logmessage
}  // namespace oceanbase

#endif /*_LOG_REC_H_*/
