/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#include "LogMsgFactory.h"
#include "MetaInfo.h"
#include "LogRecord.h"

using namespace std;

namespace oceanbase {
namespace logmessage {

string LogMsgFactory::DFT_ColMeta = "ColMetaImpl";
string LogMsgFactory::DFT_TableMeta = "TableMetaImpl";
string LogMsgFactory::DFT_DBMeta = "DBMetaImpl";
string LogMsgFactory::DFT_METAS = "MetaDataCollectionsImpl";
string LogMsgFactory::DFT_LR = "LogRecordImpl";

IColMeta* LogMsgFactory::createColMeta(const string& type)
{
  if (type == DFT_ColMeta)
    return new IColMeta();
  else
    return NULL;
}

IColMeta* LogMsgFactory::createColMeta(const string& type, const char* ptr, size_t size)
{
  if (type == DFT_ColMeta)
    return new IColMeta(ptr, size);
  else
    return NULL;
}

ITableMeta* LogMsgFactory::createTableMeta(const string& type)
{
  if (type == DFT_TableMeta)
    return new ITableMeta();
  else
    return NULL;
}

ITableMeta* LogMsgFactory::createTableMeta(const string& type, const char* ptr, size_t size)
{
  if (type == DFT_TableMeta)
    return new ITableMeta(ptr, size);
  else
    return NULL;
}

IDBMeta* LogMsgFactory::createDBMeta(const string& type)
{
  if (type == DFT_DBMeta)
    return new IDBMeta();
  else
    return NULL;
}

IDBMeta* LogMsgFactory::createDBMeta(const string& type, const char* ptr, size_t size)
{
  if (type == DFT_DBMeta)
    return new IDBMeta(ptr, size);
  else
    return NULL;
}

IMetaDataCollections* LogMsgFactory::createMetaDataCollections(const string& type)
{
  if (type == DFT_METAS)
    return new IMetaDataCollections();
  return NULL;
}

IMetaDataCollections* LogMsgFactory::createMetaDataCollections(
    const string& type, const char* ptr, size_t size, bool clearGarbage)
{
  if (type == DFT_METAS)
    return new IMetaDataCollections(ptr, size, clearGarbage);
  return NULL;
}

ILogRecord* LogMsgFactory::createLogRecord(const std::string& type, bool creating)
{
  if (type == DFT_LR)
    return new LogRecordImpl(creating, true);
  return NULL;
}

ILogRecord* LogMsgFactory::createLogRecord(const std::string& type, const char* ptr, size_t size)
{
  if (type == DFT_LR)
    return new LogRecordImpl(ptr, size);
  return NULL;
}

void LogMsgFactory::destroy(ILogRecord*& record)
{
  if (record) {
    record->clear();
    delete record;
    record = NULL;
  }
}

void LogMsgFactory::destroyWithUserMemory(ILogRecord*& record)
{
  if (record) {
    delete record;
    record = NULL;
  }
}

void LogMsgFactory::destroy(IColMeta*& colMeta)
{
  if (colMeta) {
    delete colMeta;
    colMeta = NULL;
  }
}

void LogMsgFactory::destroy(ITableMeta*& tableMeta)
{
  if (tableMeta) {
    delete tableMeta;
    tableMeta = NULL;
  }
}

void LogMsgFactory::destroy(IDBMeta*& dbMeta)
{
  if (dbMeta) {
    delete dbMeta;
    dbMeta = NULL;
  }
}

void LogMsgFactory::destroy(IMetaDataCollections*& metaColls)
{
  if (metaColls) {
    delete metaColls;
    metaColls = NULL;
  }
}

}  // namespace logmessage
}  // namespace oceanbase
