#include "MetaInfo.h"
#include "LogRecord.h"
#include "StrArray.h"
#include "LogMsgFactory.h"
#include "LogMsgBuf.h"
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <gtest/gtest.h>

#define DB_NAME "hello-test"
#define PK_VALUE "hello"
#define BINLOG_FILE 0
#define BINLOG_POS 100

#define COLS false
#define PK true
#define UK false
#define NN true
#define DEF "default value for col1"
#define ENC "utf-8"

using namespace oceanbase::logmessage;

IColMeta* createColMeta(const char* name, int type, int length)
{
  IColMeta* colMeta = new IColMeta;
  colMeta->setName(name);
  colMeta->setType(type);
  colMeta->setLength(length);
  colMeta->setSigned(COLS);
  colMeta->setIsPK(PK);
  colMeta->setIsUK(UK);
  colMeta->setNotNull(NN);
  colMeta->setDefault(DEF);
  colMeta->setEncoding(ENC);
  colMeta->setGenerated(false);
  colMeta->setGenerated(true);
  colMeta->setHiddenRowKey();
  colMeta->setPartitioned();
  colMeta->setDependent();
  colMeta->setOriginType("varchar");
  colMeta->setPrecision(5);
  colMeta->setScale(4);
  return colMeta;
}

static long userPtr = 1024;
IDBMeta* createDBMeta()
{
  IDBMeta* dbMeta = new IDBMeta();
  dbMeta->setName("helloDB");
  dbMeta->setUserData((void*)userPtr);
  return dbMeta;
}

ITableMeta* createTableMeta()
{
  ITableMeta* tableMeta = new ITableMeta;
  tableMeta->setName("table1");
  tableMeta->setHasPK(PK);
  tableMeta->setHasUK(UK);
  tableMeta->setEncoding(ENC);
  tableMeta->setDBMeta(NULL);
  tableMeta->setUserData((void*)userPtr);
  return tableMeta;
}
void parseAndcheckRecord(LogRecordImpl& parser, const char* m, size_t msgSize)
{
  ASSERT_NE(0, parser.parse(m + 4, msgSize - 4));
  ASSERT_EQ(0, parser.parse(m, msgSize));
  ASSERT_EQ(msgSize, parser.getRealSize());
  ASSERT_EQ(EUPDATE, parser.recordType());
  size_t formated_size = 0;
  const char* formated_str = parser.getFormatedString(&formated_size);
  ASSERT_TRUE(formated_str != NULL);

  ASSERT_EQ(0, strcmp("utf8", parser.recordEncoding()));
  ASSERT_EQ(0, strcmp(DB_NAME, parser.dbname()));
  ASSERT_EQ(111, parser.sqlNo());
  ASSERT_EQ(0, strcmp("11111", parser.obTraceInfo()));
  // ASSERT_EQ(0, strcmp(PK_VALUE, parser.extraInfo()));
  StrArray* colNames = parser.parsedColNames();
  ASSERT_NE((void*)NULL, (void*)colNames);
  ASSERT_EQ(parser.getSrcCategory(), SRC_FULL_RECORDED);
  size_t size = colNames->size();
  ASSERT_EQ((size_t)3, size);
  delete colNames;
  StrArray* oldCols = parser.parsedOldCols();
  ASSERT_NE((void*)NULL, (void*)oldCols);
  size = oldCols->size();
  ASSERT_EQ((size_t)3, size);
  ASSERT_STREQ("Hello1", (*oldCols)[0]);
  ASSERT_EQ((void*)NULL, (void*)(*oldCols)[1]);
  const char* old_col_value;
  size_t old_col_value_len = 0;
  oldCols->elementAt(1, old_col_value, old_col_value_len);
  ASSERT_EQ(old_col_value_len, 0);

  delete oldCols;
  StrArray* newCols = parser.parsedNewCols();
  ASSERT_NE((void*)NULL, (void*)newCols);
  size = newCols->size();
  ASSERT_EQ((size_t)3, size);
  ASSERT_STREQ("Hello3", (*newCols)[0]);
  ASSERT_STREQ("Hello4", (*newCols)[1]);
  const char* new_col_value;
  size_t new_col_value_len = 0;
  newCols->elementAt(0, new_col_value, new_col_value_len);
  ASSERT_EQ(new_col_value_len, 7);
  delete newCols;
  // check getTableMeta
  ITableMeta* table_meta = LogMsgFactory::createTableMeta(LogMsgFactory::DFT_TableMeta);
  int ret = parser.getTableMeta(table_meta);
  ASSERT_EQ(ret, 0);
  const char* pks = table_meta ? (table_meta->getPKs()) : "NULL";
  EXPECT_STREQ(pks, "col1");
  const char* uks = table_meta ? (table_meta->getUKs()) : "NULL";
  ASSERT_STREQ(uks, "col2");

  int64_t column_count = table_meta ? table_meta->getColCount() : -1;
  ASSERT_EQ(column_count, 3);
  const char* has_pk = table_meta ? (table_meta->hasPK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(has_pk, "true");
  const char* has_uk = table_meta ? (table_meta->hasUK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(has_uk, "true");
  const char* pk_info = table_meta ? table_meta->getPkinfo() : "NULL";
  EXPECT_STREQ(pk_info, "(0)");
  const char* uk_info = table_meta ? table_meta->getUkinfo() : "NULL";
  ASSERT_STREQ(uk_info, "(1)");

  IColMeta* col_meta = table_meta ? table_meta->getCol(0) : NULL;
  const char* cname = col_meta ? col_meta->getName() : "NULL";
  EXPECT_STREQ("col1", cname);
  int ctype = col_meta ? col_meta->getType() : -1;
  ASSERT_EQ(ctype, 253);
  const char* is_pk = col_meta ? (col_meta->isPK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(is_pk, "true");
  const char* encoding = col_meta ? col_meta->getEncoding() : "NULL";
  EXPECT_STREQ(encoding, ENC);

  const char* is_not_null = col_meta ? (col_meta->isNotNull() ? "true" : "false") : "NULL";
  EXPECT_STREQ(is_not_null, "true");
  // const char *default_val = col_meta ? col_meta->getDefault() : "NULL";
  // EXPECT_STREQ(default_val, defaultStr);
  const char* is_signed = col_meta ? (col_meta->isSigned() ? "true" : "false") : "NULL";
  EXPECT_STREQ("false", is_signed);
  ASSERT_EQ(12345, col_meta->getDecimals());
  bool is_generated_column = col_meta->isGenerated();
  ASSERT_TRUE(is_generated_column);
  bool is_hidden_row_key = col_meta->isHiddenRowKey();
  ASSERT_TRUE(is_hidden_row_key);
  bool is_partitioned_column = col_meta->isPartitioned();
  ASSERT_TRUE(is_partitioned_column);
  bool is_dependent_column = col_meta->isDependent();
  ASSERT_TRUE(is_dependent_column);
  LogMsgFactory::destroy(table_meta);
}

static const char* specialString = "addfadf\0sadfcc";
void test()
{
  LogRecordImpl lr(time(NULL), NULL);
  std::string* s1 = new std::string("Hello1");
  std::string* s2 = NULL;
  std::string* s3 = new std::string("Hello3");
  std::string* s4 = new std::string("Hello4");
  std::string* s5 = new std::string("Hello5");
  std::string* s6 = new std::string("Hello6");
  ITableMeta* t1 = createTableMeta();
  IColMeta* c1 = createColMeta("col1", 253, 256);
  IColMeta* c2 = createColMeta("col2", 253, 256);
  IColMeta* c3 = createColMeta("col3", 253, 256);
  ASSERT_EQ((int)7, (int)strlen(specialString));
  c1->setDefault(specialString, 15);
  const char* defaultStr = c1->getDefault();
  ASSERT_EQ(0, strncmp(specialString, defaultStr, 15));
  c1->setDefault("");
  defaultStr = c1->getDefault();
  // ASSERT_EQ((int)7, (int)strlen(defaultStr));
  c1->setDecimals(12345);
  t1->append(c1->getName(), c1);
  t1->append(c2->getName(), c2);
  t1->append(c3->getName(), c3);

  t1->setPKs("col1");
  t1->setPkinfo("(0)");
  // t2->setPKIndice(std::vector<int>(0,2));
  t1->setUKs("col2");
  t1->setUkinfo("(1)");

  void** tableUserDataPtr = t1->getUserDataPtr();
  ASSERT_EQ((long)1024, (long)((int*)(*tableUserDataPtr)));
  *tableUserDataPtr = (char*)(*tableUserDataPtr) + 1;
  tableUserDataPtr = t1->getUserDataPtr();
  ASSERT_EQ((long)1025, (long)((int*)(*tableUserDataPtr)));
  lr.setDbname("db");
  lr.setTbname("tb");
  lr.setTableMeta(t1);
  lr.putOld(s1);
  lr.putOld(s2);
  lr.putOld(s5);
  lr.putNew(s3);
  lr.putNew(s4);
  lr.putNew(s6);
  lr.setUserData(NULL);
  lr.setId(4);
  ASSERT_EQ(NULL, lr.getUserData());
  lr.setDbname(DB_NAME);
  lr.setDBMeta(createDBMeta());
  IDBMeta* dbMeta = lr.getDBMeta();
  ASSERT_NE((void*)NULL, (void*)dbMeta);
  ASSERT_EQ(0, strcmp("helloDB", dbMeta->getName()));
  void** dbUserDataPtr = dbMeta->getUserDataPtr();
  ASSERT_EQ((long)1024, (long)((int*)(*dbUserDataPtr)));
  *dbUserDataPtr = (char*)(*dbUserDataPtr) + 1;
  dbUserDataPtr = dbMeta->getUserDataPtr();
  ASSERT_EQ((long)1025, (long)((int*)(*dbUserDataPtr)));
  // lr.setExtraInfo(PK_VALUE);
  lr.setRecordType(EUPDATE);
  lr.setCheckpoint(BINLOG_FILE, BINLOG_POS);
  lr.setSrcType(SRC_MYSQL);
  lr.setSrcCategory(SRC_FULL_RECORDED);
  lr.setRecordEncoding("utf8");
  lr.setSqlNo(111);
  lr.setObTraceInfo("11111");
  size_t msgSize;
  const char* m = lr.toString(&msgSize);
  ASSERT_NE((void*)NULL, (void*)m);

  LogRecordImpl parser(false);
  parseAndcheckRecord(parser, m, msgSize);

  size_t record_size;
  const char* record_str = parser.toString(&record_size);
  ASSERT_TRUE(record_str != NULL);
  // release resource
  LogMsgFactory::destroy(dbMeta);
  delete t1;

  // parse again
  LogRecordImpl parser1(false);
  parseAndcheckRecord(parser1, record_str, msgSize);
}

TEST(LogRecordImpl, MemoryLeak)
{
  for (int i = 0; i < 1; i++) {
    test();
  }
}
int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
/* vim: set ts=4 sw=4 sts=4 tw=100 : */
