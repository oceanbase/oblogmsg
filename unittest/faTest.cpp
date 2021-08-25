#include "MetaInfo.h"
#include "LogRecord.h"
#include "LogMsgFactory.h"
#include <gtest/gtest.h>

using namespace oceanbase::logmessage;

TEST(LogMsgFactory, LogMsgFactoryAPI)
{
  IColMeta* colMeta = LogMsgFactory::createColMeta();
  ASSERT_NE((void*)NULL, (void*)colMeta);

  ITableMeta* tableMeta = LogMsgFactory::createTableMeta();
  ASSERT_NE((void*)NULL, (void*)tableMeta);

  IDBMeta* dbMeta = LogMsgFactory::createDBMeta();
  ASSERT_NE((void*)NULL, (void*)dbMeta);

  IMetaDataCollections* meta = LogMsgFactory::createMetaDataCollections();
  ASSERT_NE((void*)NULL, (void*)meta);

  ILogRecord* record = LogMsgFactory::createLogRecord();
  ASSERT_NE((void*)NULL, (void*)record);

  LogMsgFactory::destroy(colMeta);
  ASSERT_EQ((void*)NULL, (void*)colMeta);

  LogMsgFactory::destroy(tableMeta);
  ASSERT_EQ((void*)NULL, (void*)tableMeta);

  LogMsgFactory::destroy(dbMeta);
  ASSERT_EQ((void*)NULL, (void*)dbMeta);

  LogMsgFactory::destroy(meta);
  ASSERT_EQ((void*)NULL, (void*)meta);

  LogMsgFactory::destroy(record);
  ASSERT_EQ((void*)NULL, (void*)record);

  LogMsgFactory::destroy(record);
  ASSERT_EQ((void*)NULL, (void*)record);
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
