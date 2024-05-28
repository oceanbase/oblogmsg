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

#include <cstdio>
#include <ctime>
#include <pthread.h>
#include <vector>
#include <string>
#include "UserData.h"

namespace oceanbase {
namespace logmessage {

class StrArray;

enum logmsg_field_types {
  LOGMSG_TYPE_DECIMAL,
  LOGMSG_TYPE_TINY,
  LOGMSG_TYPE_SHORT,
  LOGMSG_TYPE_LONG,
  LOGMSG_TYPE_FLOAT,
  LOGMSG_TYPE_DOUBLE,
  LOGMSG_TYPE_NULL,
  LOGMSG_TYPE_TIMESTAMP,
  LOGMSG_TYPE_LONGLONG,
  LOGMSG_TYPE_INT24,
  LOGMSG_TYPE_DATE,
  LOGMSG_TYPE_TIME,
  LOGMSG_TYPE_DATETIME,
  LOGMSG_TYPE_YEAR,
  LOGMSG_TYPE_NEWDATE,
  LOGMSG_TYPE_VARCHAR,
  LOGMSG_TYPE_BIT,

  LOGMSG_TYPE_COMPLEX = 160, //0xa0
  LOGMSG_TYPE_ARRAY = 161, //0xa1
  LOGMSG_TYPE_STRUCT = 162, //0xa2
  LOGMSG_TYPE_CURSOR = 163, //0xa3

  LOGMSG_TYPE_OB_TIMESTAMP_WITH_TIME_ZONE = 200,
  LOGMSG_TYPE_OB_TIMESTAMP_WITH_LOCAL_TIME_ZONE = 201,
  LOGMSG_TYPE_OB_TIMESTAMP_NANO = 202,
  LOGMSG_TYPE_OB_RAW = 203,
  LOGMSG_TYPE_OB_INTERVAL_YM = 204,
  LOGMSG_TYPE_OB_INTERVAL_DS = 205,
  LOGMSG_TYPE_OB_NUMBER_FLOAT = 206,
  LOGMSG_TYPE_OB_NVARCHAR2 = 207,
  LOGMSG_TYPE_OB_NCHAR = 208,
  LOGMSG_TYPE_OB_UROWID = 209,
  LOGMSG_TYPE_ORA_BLOB = 210,
  LOGMSG_TYPE_ORA_CLOB = 211,

  LOGMSG_TYPE_ORA_BINARY_FLOAT = 212,
  LOGMSG_TYPE_ORA_BINARY_DOUBLE = 213,
  LOGMSG_TYPE_ORA_XML = 214,
  LOGMSG_TYPE_ROARINGBITMAP = 215,

  LOGMSG_TYPE_JSON = 245,
  LOGMSG_TYPE_NEWDECIMAL = 246,
  LOGMSG_TYPE_ENUM = 247,
  LOGMSG_TYPE_SET = 248,
  LOGMSG_TYPE_TINY_BLOB = 249,
  LOGMSG_TYPE_MEDIUM_BLOB = 250,
  LOGMSG_TYPE_LONG_BLOB = 251,
  LOGMSG_TYPE_BLOB = 252,
  LOGMSG_TYPE_VAR_STRING = 253,
  LOGMSG_TYPE_STRING = 254,
  LOGMSG_TYPE_GEOMETRY = 255,
  LOGMSG_TYPES
};

// column's meta
struct ColMetaInfo;
class IDBMeta;
class IMetaDataCollections;
struct _trie_tree;
class IColMeta : public UserDataInterface {
public:
  IColMeta();
  IColMeta(const void* ptr, size_t size);
  virtual ~IColMeta();

public:
  // override
  virtual const char* getName();
  virtual int getType();
  virtual long getLength();
  virtual const char* getOriginType();
  virtual long getPrecision();
  virtual long getScale();
  virtual bool isSigned();
  virtual bool isPK();
  virtual bool isRuleCol();
  virtual bool isUK();
  virtual bool isNotNull();
  virtual int getDecimals();
  virtual const char* getDefault();
  virtual const char* getEncoding();
  virtual int getRequired();
  virtual StrArray* getValuesOfEnumSet();
  virtual bool isGenerated();
  virtual void setFlag(unsigned char flag);
  virtual unsigned char getFlag();

  virtual void setName(const char* name);
  virtual void setType(int type);
  virtual void setLength(long length);
  virtual void setOriginType(const char* origin);
  virtual void setPrecision(long precision);
  virtual void setScale(long scale);
  virtual void setSigned(bool b);
  virtual void setIsPK(bool b);
  virtual void setIsRuleCol(bool b);
  virtual void setIsUK(bool b);
  virtual void setNotNull(bool b);
  virtual void setDecimals(int decimals);
  virtual void setDefault(const char* def);
  virtual void setDefault(const char* def, size_t length);
  virtual void setEncoding(const char* enc);
  virtual void setRequired(int required);
  virtual void setValuesOfEnumSet(std::vector<std::string>& v);
  virtual void setValuesOfEnumSet(std::vector<const char*>& v);
  virtual void setValuesOfEnumSet(const char** v, size_t size);

  virtual void setGenerated(bool generated = true);
  virtual void setHiddenRowKey(bool hiddenRowKey = true);
  virtual bool isHiddenRowKey();
  virtual void setPartitioned(bool partitioned = true);
  virtual bool isPartitioned();
  virtual void setDependent(bool dependent = true);
  virtual bool isDependent();

public:
  int appendTo(std::string& s);
  size_t getRealSize();
  int parse(const void* ptr, size_t size);
  bool parsedOK();

public:
  virtual void setUserData(void* data);
  virtual void* getUserData();

private:
  ColMetaInfo* m_col;
  void* m_userData;
};

// table's meta
struct TableMetaInfo;
class ITableMeta : public UserDataInterface {
public:
  ITableMeta();
  ITableMeta(const void* ptr, size_t size);
  virtual ~ITableMeta();

public:
  // override
  virtual const char* getName();
  virtual bool hasPK();
  virtual bool hasUK();
  virtual const char* getPKs();
  virtual const char* getUKs();
  virtual const int* getPKs(int& size) const;
  virtual const int* getUKs(int& size) const;
  virtual const char* getEncoding();
  virtual IDBMeta* getDBMeta();

  virtual void setName(const char* name);
  virtual void setHasPK(bool b);
  virtual void setPKIndice(const std::vector<int>& indice);
  virtual void setPKs(const char* pks);
  virtual void setHasUK(bool b);
  virtual void setUKs(const char* uks);
  virtual void setEncoding(const char* enc);
  virtual void setDBMeta(IDBMeta* dbMeta);
  virtual void setPkinfo(const char* info);
  virtual const char* getPkinfo();
  virtual void setUkinfo(const char* info);
  virtual const char* getUkinfo();
  bool hashColumnSetted();
  const int* getHashColumnIdx(int& hashCoumnCount, char**& hashValueList, size_t*& hashValueSizeList);
  void setHashColByPK();
  int setHashCol(std::vector<std::string>& hashColumns);
  void setHashFuncId(int id);
  int getHashFuncId();
  void trySerializeMetaDataAsMsgArea(std::vector<const char*>& extra_infos);
  const std::string& getNameData();
  const std::string& getEncodingData();
  const std::string& getcolTypeData();
  const std::string& getPkData();
  const std::string& getUkData();
  const std::string& getkeyData();
  const std::string& getColumnFlagData();
  const std::string& getNotNullData();
  const std::string& getSignedData();
  const std::string& getDecimalsData();
  const std::string& getDefaultData();
  const std::string& getColLengthData();
  const std::string& getOriginTypeData();
  const std::string& getColPrecisionData();
  const std::string& getColScaleData();

public:
  /**
   * get all column name
   */
  std::vector<std::string>& getColNames();

  /**
   * get PK column name
   */
  std::vector<std::string>& getPKColNames();
  std::vector<std::string>& getUKColNames();

  /**
   * get column's meta by name
   */
  virtual IColMeta* getCol(const char* colName);

  virtual int getColIndex(const char* colName);

  /**
   * get Column count
   */
  virtual int getColCount();

  /*
   * get column's meta by column id
   */
  virtual IColMeta* getCol(int index);
  virtual int getColNum(const char* colName);

  /**
   * append a column meta
   */
  virtual int append(const char* colName, IColMeta* colMeta);

  /* For partial */
  virtual bool isDropped();
  virtual void setDropped(bool value);
  virtual const char* getNewName();
  virtual void setNewName(const char* name);

public:
  int appendTo(std::string& s);
  size_t getRealSize();
  int parse(const void* ptr, size_t size);
  bool parsedOK();
  /*
   * parse key index like (2, 0, 1) or (0, 1, 2)
   * return index vector, if format is invalid,
   * return -1, if success, return 0.
   */
  int parseKeyIndex(std::string indexStr, std::vector<int>& indice);
  /*
   * get pk or uk keys from pkinfo or ukinfo like (0, 1, 2......)
   */
  int getKeysFromInfo(std::string info, std::string& keys, StrArray* colNames);

public:
  virtual void setUserData(void* data);
  virtual void* getUserData();
  virtual void** getUserDataPtr();

private:
  TableMetaInfo* m_tbl;
  void* m_userData;
  std::string m_colNameData;
  std::string m_encodingData;
  std::string m_colTypeData;
  std::string m_PkData;
  std::string m_UkData;
  std::string m_keyData;
  std::string m_columnFlagData;
  std::string m_colNotNullData;
  std::string m_colSignedData;
  std::string m_colDecimalsData;
  std::string m_colDefaultData;
  std::string m_colLengthData;
  std::string m_colOriginTypeData;
  std::string m_colPrecisionData;
  std::string m_colScaleData;

  pthread_mutex_t m_mdMutex;
  bool m_DataOk;
  int* m_hashColumnIdx;
  int m_hashColumnSize;
  bool m_hashColumnSetted;
  int m_hashFuncId;
  char** m_hashColumValueList;
  size_t* m_hashColumValueSizeList;
};

// DB's meta
struct DBMetaInfo;
class IDBMeta : public UserDataInterface {
public:
  IDBMeta();
  IDBMeta(const void* ptr, size_t size);
  virtual ~IDBMeta();

public:
  // override
  virtual const char* getName();
  virtual const char* getEncoding();
  virtual IMetaDataCollections* getMetaDataCollections();

  virtual void setName(const char* name);
  virtual void setEncoding(const char* enc);
  virtual void setMetaDataCollections(IMetaDataCollections* mdc);

  virtual int getTblCount();
  /**
   * get table's meta by name
   */
  virtual ITableMeta* get(const char* tblName);
  virtual ITableMeta* get(int index);

  /**
   * add a table's meta
   */
  virtual int put(ITableMeta* tblMeta);

  /* For partial */
  virtual bool isDropped();
  virtual void setDropped(bool value);

public:
  virtual int eraseMapIterator();
  virtual int getFromMapIterator(const char** tblName, ITableMeta** tblMeta);
  virtual int nextMapIterator(bool erase);
  virtual int resetMapIterator();
  virtual int erase(const char* tableName, bool delayDeleteMeta);
  virtual int appendTo(std::string& s);
  virtual size_t getRealSize();
  virtual int parse(const void* ptr, size_t size);
  virtual bool parsedOK();

public:
  virtual void setUserData(void* data);
  virtual void* getUserData();
  virtual void** getUserDataPtr();

private:
  DBMetaInfo* m_db;
  void* m_userData;
};

// all db's meta
struct MetaDataCollectionInfo;
class IMetaDataCollections : public UserDataInterface {
public:
  IMetaDataCollections();
  IMetaDataCollections(
      const void* ptr, size_t size, bool removePtr = false);  // removePtr means if free ptr when destroy object
  virtual ~IMetaDataCollections();

public:
  // override
  virtual unsigned getMetaVerNum();
  virtual IMetaDataCollections* getPrev();
  virtual time_t getTimestamp();

  virtual void setMetaVerNum(unsigned metaVerNum);
  virtual void setPrev(IMetaDataCollections* prev);
  virtual void setTimestamp(time_t timestamp);

  /**
   * get Db count
   */
  virtual int getDbCount();

  /**
   * get Db meta by name
   */
  virtual IDBMeta* get(const char* dbname);

  /**
   * get Db meta by id
   */
  virtual IDBMeta* get(int index);

  /**
   * get table meta by dbname and tablename
   */
  virtual ITableMeta* get(const char* dbName, const char* tblName);

  /**
   * add a db meta
   */
  virtual int put(IDBMeta* dbMeta);

  /**
   * serialize to  string
   */
  virtual int toString(std::string& s);

  /**
   * parse from readonly ptr
   */
  virtual int parse(const void* ptr, size_t size);

  /**
   * if parsed success
   */
  virtual bool parsedOK();

  virtual size_t getRealSize();

  virtual int eraseMapIterator();
  virtual int getFromMapIterator(const char** dbName, IDBMeta** dbMeta);
  virtual int nextMapIterator(bool erase);
  virtual int resetMapIterator();
  virtual int erase()
  {
    return 0;
  }
  virtual int erase(const char* dbName, bool delayDeleteMeta);
  virtual int erase(const char* dbName, const char* tbName, bool delayDeleteMeta);

public:
  virtual void setUserData(void* data);
  virtual void* getUserData();

private:
  MetaDataCollectionInfo* m_coll;
  void* m_userData;
};

}  // namespace logmessage
}  // namespace oceanbase

