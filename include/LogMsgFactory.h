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

#include <string>

namespace oceanbase {
namespace logmessage {

class IColMeta;
class ITableMeta;
class IDBMeta;
class IMetaDataCollections;
class ILogRecord;

class LogMsgFactory {
public:
  static std::string DFT_ColMeta;
  static std::string DFT_TableMeta;
  static std::string DFT_DBMeta;
  static std::string DFT_METAS;
  static std::string DFT_LR;

public:
  /**
   * Create an empty ColMeta by a specific type
   *
   * @param type is the name of the specific colmeta, default is "ColMetaImpl"
   * @return instantialized colmeta
   */
  static IColMeta* createColMeta(const std::string& type = DFT_ColMeta);

  /**
   * Create a meaningful ColMeta by a specific type with given serialized
   * data and size
   *
   * @param type is the name of the specific colmeta, default is "ColMetaImpl"
   * @param ptr  is the address of a consecutive memory in which the
   *               serialized colmeta is stored
   * @param size is the length of the consecutive memory
   * @return instantialized colmeta
   */
  static IColMeta* createColMeta(const std::string& type, const char* ptr, size_t size);

  /**
   * Create an empty TableMeta by a specific type
   *
   * @param type is the name of the specific tablemeta, default is "TableMetaImpl"
   * @return instantialized tablemeta
   */
  static ITableMeta* createTableMeta(const std::string& type = DFT_TableMeta);

  /**
   * Create a meaningful TableMeta by a specific type with given serialized data
   * and size
   *
   * @param type is the name of the specific tablemeta, default is
   *               "TableMetaImpl"
   * @param ptr  is the address of a consecutive memory in which the
   *             date of a serialized tablemeta is stored
   * @param size is the length of the consecutive memory
   * @return instantialized tablemeta
   */
  static ITableMeta* createTableMeta(const std::string& type, const char* ptr, size_t size);

  /**
   * Create an empty DBMeta by a specific type
   *
   * @param type is the name of the specific dbmeta, default is "DBMetaImpl"
   * @return instantialized dbmeta
   */
  static IDBMeta* createDBMeta(const std::string& type = DFT_DBMeta);

  /**
   * Create a meaningful DBMeta by a specific type with given serialized
   * data and size
   *
   * @param type is the name of the specific dbmeta, default is "DBMetaImpl"
   * @param ptr  is the address of a consecutive memory in which the
   *               serialized dbmeta is stored
   * @param size is the length of the consecutive memory
   * @return instantialized dbmeta
   */
  static IDBMeta* createDBMeta(const std::string& type, const char* ptr, size_t size);

  /**
   * Create an empty MetaDataCollections by a specific type
   *
   * @param type is the name of the specific metadatacollections,
   *             default is "MetaDataCollectionsImpl"
   * @return instantialized metadatacollections
   */
  static IMetaDataCollections* createMetaDataCollections(const std::string& type = DFT_METAS);

  /**
   * Create a meaningful MetaDataCollections by a specific type with given
   * serialized data and size
   *
   * @param type is the name of the specific metadatacollections,
   *             default is "DBMetaImpl"
   * @param ptr  is the address of a consecutive memory in which the
   *             serialized metadatacollections is stored
   * @param size is the length of the consecutive memory
   * @param freeMem is to direct whether the assigned ptr should be
   *             freed after the metadatacollections is destroyed
   * @return instantialized metadatacollections
   */
  static IMetaDataCollections* createMetaDataCollections(
      const std::string& type, const char* ptr, size_t size, bool freeMem);

  /**
   * Create a binlog record by the specific type with the judgement whether
   * need initilizing non-serialized members in memory
   *
   * @param type is the name of the specific LogRecord,
   *             default is "LogRecordImpl"
   * @param creating is to differentiate two kinds of usage, if creating is
   *             true, it means the created binlog record has not been
   *             serilized, all in-memory functions can be called. Otherwise
   *             if creating is false, only after-serialized function could
   *             be called
   * @return the instantialized binlog record
   */
  static ILogRecord* createLogRecord(const std::string& type = DFT_LR, bool creating = true);

  /**
   * Create a binlog record by the specific type from data stored in the
   * consecutive memory directed by ptr with length size
   *
   * @param type is the name of the specific LogRecord,
   *             default is "LogRecordImpl"
   * @param ptr  is the address of the consecutive memory
   * @param size is the length of the memory
   * @return the instantialized binlog record
   */
  static ILogRecord* createLogRecord(const std::string& type, const char* ptr, size_t size);

  /**
   * Destroy the binlog record created by the factory with the memory
   * allocated by users with putOld and putNew
   *
   * @param record is the created record which is freed hereby
   */
  static void destroy(ILogRecord*& record);

  /**
   * Destroy the binlog record created by the factory, the users memory
   * is not freed
   *
   * @param record is the created record which is freed hereby
   */
  static void destroyWithUserMemory(ILogRecord*& record);

  /**
   * Destroy the column meta created by the factory
   *
   * @param colMeta is the meta of the column to be freed
   */
  static void destroy(IColMeta*& colMeta);

  /**
   * Destroy the table meta created by the factory
   *
   * @param tableMeta is the meta of the table to be freed
   */
  static void destroy(ITableMeta*& tableMeta);

  /**
   * Destroy the database meta created by the factory
   *
   * @param dbMeta is the meta of the database to be freed
   */
  static void destroy(IDBMeta*& dbMeta);

  /**
   * Destroy the collections of all db meta created by the factory
   *
   * @param metaColls is the collections of all meta to be freed
   */
  static void destroy(IMetaDataCollections*& metaColls);
};

}  // namespace logmessage
}  // namespace oceanbase

