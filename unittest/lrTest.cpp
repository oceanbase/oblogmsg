/**
 * ILogRecord API test
 */
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
  colMeta->setHiddenRowKey(false);
  colMeta->setHiddenRowKey(true);
  colMeta->setPartitioned(false);
  colMeta->setPartitioned(true);
  colMeta->setDependent(false);
  colMeta->setDependent(true);
  colMeta->setOriginType("varchar");
  colMeta->setPrecision(5);
  colMeta->setScale(3);
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

static const char* specialString = "addfadf\0sadfcc";

ILogRecord* createLogRecord(bool useDMB = true, bool useBinlogBuf = false)
{
  ILogRecord* lr = new LogRecordImpl(true, useDMB);
  std::string* s1 = new std::string("Hello1");
  std::string* s3 = new std::string("Hello3");
  std::string* s4 = new std::string("Hello4");

  std::string* full_json = new std::string("{\"name\":\"LiMing\",\"age\",24,\"address\":\"China\"}");
  std::string* json_diff = new std::string("{\"op\":\"replace\",\"value\":\"ab\",\"path\":\"$.name\"}");

  ITableMeta* t1 = createTableMeta();
  t1->setName("table1");
  IColMeta* c1 = createColMeta("col1", 253, 256);
  IColMeta* c2 = createColMeta("col2", 253, 256);
  IColMeta* c3 = createColMeta("col3", 253, 256);

  c1->setDefault(specialString, 15);
  c1->getDefault();
  c1->setDefault(specialString);
  c1->getDefault();
  t1->append(c1->getName(), c1);
  t1->append(c2->getName(), c2);
  t1->append(c3->getName(), c3);

  t1->setPKs("col1,col2");
  t1->setPKIndice(std::vector<int>(0, 1));
  lr->setTimestamp(1515141568);
  lr->setTableMeta(t1);

  if (useBinlogBuf) {
    BinLogBuf* oldVals = new BinLogBuf[3];
    BinLogBuf* newVals = new BinLogBuf[3];
    lr->setOldColumn(oldVals, 3);
    lr->setNewColumn(newVals, 3);
    lr->putOld(s1->c_str(), s1->length());
    lr->putOld(NULL, 0);
    lr->putOld(full_json->c_str(), full_json->length());
    lr->putNew(s3->c_str(), s3->length());
    lr->putNew(s4->c_str(), s4->length());
    lr->putNewJsonDiff(json_diff->c_str(), json_diff->length());
  } else {
    lr->putOld(s1);
    lr->putOld(NULL);
    lr->putOld(full_json);
    lr->putNew(s3);
    lr->putNew(s4);
    lr->putNewJsonDiff(json_diff);
  }

  lr->setUserData(NULL);
  lr->setDbname(DB_NAME);
  lr->setTbname("table1");
  lr->setDBMeta(createDBMeta());
  lr->setRecordType(EUPDATE);
  lr->setCheckpoint(BINLOG_FILE, BINLOG_POS);
  lr->setSrcType(SRC_MYSQL);
  lr->setSrcCategory(SRC_PART_RECORDED);
  lr->setRecordEncoding("utf8");
  return lr;
}

typedef struct _TestThreadInfo {
  bool* quit;
  size_t sample_size;
  const char* sample;
} TestThreadInfo;

void* create(void* argv)
{
  LogMsgBuf* lmb = new LogMsgBuf();
  TestThreadInfo* info = (TestThreadInfo*)argv;
  long matched = 0;
  long mismatched = 0;
  while (*(info->quit) == false) {
    ILogRecord* sample = createLogRecord(true);
    size_t pre_toString_size;
    sample->toString(&pre_toString_size, lmb, true);

    size_t sample_msg_size;
    const char* sample_msg_content = sample->getFormatedString(&sample_msg_size);
    if (sample_msg_size != info->sample_size) {
      std::cout << "sample size is " << info->sample_size << ", but tested one is " << sample_msg_size << std::endl;
      return NULL;
    }

    if (strncmp(info->sample, sample_msg_content, info->sample_size) != 0) {
      mismatched++;
      std::cout << "sample and tested message not same: sample [" << info->sample << "], tested [" << sample_msg_content
                << "]" << std::endl;
    } else {
      matched++;
    }

    LogMsgFactory::destroy(sample);
  }
  std::cout << "matched " << matched << ", mismatched " << mismatched << std::endl;
  delete lmb;
  return NULL;
}

#define Concurrency 32
#define WaitTime 10
TEST(LogRecordImpl, ConcurrencyToString)
{
  bool quit = false;
  /* Create one sample for copmare */
  LogMsgBuf* lmb = new LogMsgBuf();
  ILogRecord* sample = createLogRecord(true);
  size_t sample_msg_size;
  const char* sample_msg_content = sample->toString(&sample_msg_size, lmb);

  /* ToString in multi-pthreads */

  TestThreadInfo testThreadInfo;
  testThreadInfo.quit = &quit;
  testThreadInfo.sample_size = sample_msg_size;
  testThreadInfo.sample = sample_msg_content;
  pthread_t* testThreads = new pthread_t[Concurrency];
  for (int i = 0; i < Concurrency; i++) {
    if (pthread_create(testThreads + i, NULL, create, &testThreadInfo) != 0)
      FAIL();
  }

  for (int i = 0; i < WaitTime; i++)
    sleep(1);

  /* Destroy the dmb of sample */
  quit = true;
  for (int i = 0; i < Concurrency; i++)
    pthread_join(*(testThreads + i), NULL);
  delete[] testThreads;
  delete lmb;
}
/*
TEST(LogRecordImpl, ParseAndGet) {
    LogMsgInit();
    ILogRecord* lr = createLogRecord();
    size_t pre_toString_size;
    const char * pre_msg_content = lr->toString(&pre_toString_size);
    ASSERT_EQ(pre_msg_content == NULL, 0);

    ILogRecord *parser = LogMsgFactory::createLogRecord("LogRecordImpl", false);
    ASSERT_EQ(0, parser->parse(pre_msg_content, pre_toString_size));
    ASSERT_EQ(pre_toString_size, parser->getRealSize());
    ITableMeta *tableMeta = parser->getTableMeta();
    ASSERT_STREQ(tableMeta->getName(), "table1");
    ASSERT_EQ(tableMeta->hasPK(), PK);
    ASSERT_EQ(tableMeta->hasUK(), UK);
    ASSERT_STREQ(tableMeta->getPkinfo(), "col1,col2");
    ASSERT_EQ(tableMeta->getUkinfo() == NULL, true);
    int pkSize = 0;
    const int* pks = tableMeta->getPKs(pkSize);
    ASSERT_EQ(pkSize, 2);
    ASSERT_EQ(pks[0], 0);
    ASSERT_EQ(pks[1], 1);
}*/

TEST(LogRecordImpl, LogRecordImplAPI)
{

  LogRecordImpl lr(time(NULL), NULL);
  std::string* s1 = new std::string("Hello1");
  //	std::string *s2 = NULL;
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
  lr.putOld(NULL);
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
  ASSERT_NE(0, parser.parse(m + 4, msgSize - 4));
  ASSERT_EQ(0, parser.parse(m, msgSize));
  ASSERT_EQ(msgSize, parser.getRealSize());
  ASSERT_EQ(EUPDATE, parser.recordType());
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
  ASSERT_STREQ(s1->c_str(), (*oldCols)[0]);
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
  ASSERT_STREQ(s3->c_str(), (*newCols)[0]);
  ASSERT_STREQ(s4->c_str(), (*newCols)[1]);
  const char* new_col_value;
  size_t new_col_value_len = 0;
  newCols->elementAt(0, new_col_value, new_col_value_len);
  ASSERT_EQ(new_col_value_len, 7);
  delete newCols;
  delete t1;
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

  // 224
  /*
  std::ifstream ifs("javaGenedMessage", std::ios::binary);
  int len = 224;
  char *c = new char[len];
  ifs.read(c, sizeof(char)*len);
  LogRecordImpl parser1(false);
  ASSERT_EQ(0, parser1.parse(c, len));
  ASSERT_EQ(0, strcmp("db", parser1.dbname()));
  ASSERT_EQ(0, strcmp("table", parser1.tbname()));
  colNames = parser1.parsedColNames();
  ASSERT_NE((void*)NULL, (void*)colNames);
  ASSERT_EQ(2, (int)colNames->size());
  ASSERT_EQ(0, strcmp("col1", (*colNames)[0]));
  ASSERT_EQ(0, strcmp("col2", (*colNames)[1]));
  oldCols = parser1.parsedOldCols();
  ASSERT_NE((void*)NULL, (void*)oldCols);
  ASSERT_EQ(2, (int)oldCols->size());
  ASSERT_EQ(0, strcmp("old1", (*oldCols)[0]));
  ASSERT_EQ(0, strcmp("old2", (*oldCols)[1]));
  newCols= parser1.parsedNewCols();
  ASSERT_NE((void*)NULL, (void*)newCols);
  ASSERT_EQ(2, (int)newCols->size());
  ASSERT_EQ(0, strcmp("new1", (*newCols)[0]));
  ASSERT_EQ(0, strcmp("new2", (*newCols)[1]));
  */
}

TEST(LogRecordImpl, LogRecordImplTestPKS1)
{

  LogRecordImpl lr(time(NULL), NULL);
  std::string* s1 = new std::string("Hello1");
  //	std::string *s2 = NULL;
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

  t1->setPKs("col3,col1");
  t1->setPkinfo("2,0)");
  // t2->setPKIndice(std::vector<int>(0,2));
  t1->setUKs("col2,col1,col3");
  t1->setUkinfo("(0,1),(1,2),(0)");

  void** tableUserDataPtr = t1->getUserDataPtr();
  ASSERT_EQ((long)1024, (long)((int*)(*tableUserDataPtr)));
  *tableUserDataPtr = (char*)*tableUserDataPtr + 1;
  tableUserDataPtr = t1->getUserDataPtr();
  ASSERT_EQ((long)1025, (long)((int*)(*tableUserDataPtr)));
  lr.setDbname("db");
  lr.setTbname("tb");
  lr.setTableMeta(t1);
  lr.putOld(s1);
  lr.putOld(NULL);
  lr.putOld(s5);
  lr.putNew(s3);
  lr.putNew(s4);
  lr.putNew(s6);
  lr.setUserData(NULL);
  lr.setId(4);
  ASSERT_EQ(NULL, lr.getUserData());

  // check table meta
  ITableMeta* p = NULL;
  int ret = lr.getTableMeta(p);
  ASSERT_EQ(ret, 0);
  ASSERT_TRUE(p == t1);

  lr.setDbname(DB_NAME);
  lr.setDBMeta(createDBMeta());
  IDBMeta* dbMeta = lr.getDBMeta();
  ASSERT_NE((void*)NULL, (void*)dbMeta);
  ASSERT_EQ(0, strcmp("helloDB", dbMeta->getName()));
  void** dbUserDataPtr = dbMeta->getUserDataPtr();
  ASSERT_EQ((long)1024, (long)((int*)(*dbUserDataPtr)));
  *dbUserDataPtr = (char*)*dbUserDataPtr + 1;
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
  ASSERT_NE(0, parser.parse(m + 4, msgSize - 4));
  ASSERT_EQ(0, parser.parse(m, msgSize));
  ASSERT_EQ(msgSize, parser.getRealSize());
  ASSERT_EQ(EUPDATE, parser.recordType());
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
  ASSERT_STREQ(s1->c_str(), (*oldCols)[0]);
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
  ASSERT_STREQ(s3->c_str(), (*newCols)[0]);
  ASSERT_STREQ(s4->c_str(), (*newCols)[1]);
  const char* new_col_value;
  size_t new_col_value_len = 0;
  newCols->elementAt(0, new_col_value, new_col_value_len);
  ASSERT_EQ(new_col_value_len, 7);
  delete newCols;
  delete t1;
  // check getTableMeta
  ITableMeta* table_meta = LogMsgFactory::createTableMeta(LogMsgFactory::DFT_TableMeta);
  ret = parser.getTableMeta(table_meta);
  const char* pks = table_meta ? (table_meta->getPKs()) : "NULL";
  ASSERT_TRUE(pks == NULL);
  const char* uks = table_meta ? (table_meta->getUKs()) : "NULL";
  ASSERT_STREQ(uks, "col1,col2,col3");

  int64_t column_count = table_meta ? table_meta->getColCount() : -1;
  ASSERT_EQ(column_count, 3);
  const char* has_pk = table_meta ? (table_meta->hasPK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(has_pk, "true");
  const char* has_uk = table_meta ? (table_meta->hasUK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(has_uk, "true");
  const char* pk_info = table_meta ? table_meta->getPkinfo() : "NULL";
  EXPECT_STREQ(pk_info, "2,0)");
  const char* uk_info = table_meta ? table_meta->getUkinfo() : "NULL";
  ASSERT_STREQ(uk_info, "(0,1),(1,2),(0)");

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
  // LogMsgFactory::destroy(table_meta);
}

TEST(LogRecordImpl, LogRecordImplTestPKS2)
{

  LogRecordImpl lr(time(NULL), NULL);
  std::string* s1 = new std::string("Hello1");
  //	std::string *s2 = NULL;
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

  void** tableUserDataPtr = t1->getUserDataPtr();
  ASSERT_EQ((long)1024, (long)((int*)(*tableUserDataPtr)));
  *tableUserDataPtr = (char*)*tableUserDataPtr + 1;
  tableUserDataPtr = t1->getUserDataPtr();
  ASSERT_EQ((long)1025, (long)((int*)(*tableUserDataPtr)));
  lr.setDbname("db");
  lr.setTbname("tb");
  lr.setTableMeta(t1);
  lr.putOld(s1);
  lr.putOld(NULL);
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
  *dbUserDataPtr = (char*)*dbUserDataPtr + 1;
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
  ASSERT_NE(0, parser.parse(m + 4, msgSize - 4));
  ASSERT_EQ(0, parser.parse(m, msgSize));
  ASSERT_EQ(msgSize, parser.getRealSize());
  ASSERT_EQ(EUPDATE, parser.recordType());
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
  ASSERT_STREQ(s1->c_str(), (*oldCols)[0]);
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
  ASSERT_STREQ(s3->c_str(), (*newCols)[0]);
  ASSERT_STREQ(s4->c_str(), (*newCols)[1]);
  const char* new_col_value;
  size_t new_col_value_len = 0;
  newCols->elementAt(0, new_col_value, new_col_value_len);
  ASSERT_EQ(new_col_value_len, 7);
  delete newCols;
  delete t1;
  // check getTableMeta
  ITableMeta* table_meta = LogMsgFactory::createTableMeta(LogMsgFactory::DFT_TableMeta);
  int ret = parser.getTableMeta(table_meta);
  ASSERT_EQ(ret, 0);
  const char* pks = table_meta ? (table_meta->getPKs()) : "NULL";
  ASSERT_TRUE(pks == NULL);
  const char* uks = table_meta ? (table_meta->getUKs()) : "NULL";
  ASSERT_TRUE(uks == NULL);

  int64_t column_count = table_meta ? table_meta->getColCount() : -1;
  ASSERT_EQ(column_count, 3);
  const char* has_pk = table_meta ? (table_meta->hasPK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(has_pk, "false");
  const char* has_uk = table_meta ? (table_meta->hasUK() ? "true" : "false") : "NULL";
  EXPECT_STREQ(has_uk, "false");
  const char* pk_info = table_meta ? table_meta->getPkinfo() : "NULL";
  ASSERT_TRUE(pk_info == NULL);
  const char* uk_info = table_meta ? table_meta->getUkinfo() : "NULL";
  ASSERT_TRUE(uk_info == NULL);
  LogMsgFactory::destroy(table_meta);
}

TEST(LogRecordImpl, ParseTest1)
{
  LogMsgBuf* lmb = new LogMsgBuf();
  ILogRecord* sample = createLogRecord(true);
  size_t sample_msg_size;
  const char* sample_msg_content;
  // record header field
  int srcType = 1;
  int category = 1;
  int recordType = 1;
  long timeStamp = 1663253940;
  bool b = true;
  const uint64_t LOGREC_SUB_VERSION = 0x0500000000000000;  // sub version num
  uint64_t setId = 123456789;
  uint64_t getId = setId | LOGREC_SUB_VERSION;
  int file = 10;
  int offset = 99;
  int sqlNo = 100;
  // record tail field
  int threadId = 0xFFFF0000;
  int usec = 123456789;

  ASSERT_FALSE(sample->isJsonDiffColVal("col1"));
  ASSERT_FALSE(sample->isJsonDiffColVal("col2"));
  ASSERT_TRUE(sample->isJsonDiffColVal("col3"));

  sample_msg_content = sample->toString(&sample_msg_size, lmb);

  ILogRecord* sample1 = LogMsgFactory::createLogRecord("LogRecordImpl", false);
  sample1->parse(sample_msg_content, sample_msg_size);
  // set record header field
  sample1->setSrcType(srcType);
  sample1->setSrcCategory(category);
  sample1->setRecordType(recordType);
  sample1->setTimestamp(timeStamp);
  sample1->setFirstInLogevent(b);
  sample1->setId(setId);
  sample1->setCheckpoint(file, offset);
  sample1->setSqlNo(sqlNo);
  // set record tail field
  sample1->setThreadId(threadId);
  sample1->setRecordUsec(usec);

  sample_msg_content = sample1->toString(&sample_msg_size, lmb);

  ILogRecord* sample2 = LogMsgFactory::createLogRecord("LogRecordImpl", false);
  sample2->parse(sample_msg_content, sample_msg_size);

  ASSERT_EQ(srcType, sample2->getSrcType());
  ASSERT_EQ(category, sample2->getSrcCategory());
  ASSERT_EQ(recordType, sample2->recordType());
  ASSERT_EQ(timeStamp, sample2->getTimestamp());
  ASSERT_EQ(b, sample2->firstInLogevent());
  ASSERT_EQ(getId, sample2->id());
  ASSERT_EQ(file, sample2->getCheckpoint1());
  ASSERT_EQ(offset, sample2->getCheckpoint2());
  ASSERT_EQ(sqlNo, sample2->sqlNo());

  ASSERT_EQ(threadId, sample2->getThreadId());
  ASSERT_EQ(usec, sample2->getRecordUsec());

  size_t size;
  const uint8_t* new_cols_json_diff = sample2->isNewColsJsonDiff(size);
  ASSERT_EQ((uint8_t) 0, new_cols_json_diff[0]);
  ASSERT_EQ((uint8_t) 0, new_cols_json_diff[1]);
  ASSERT_EQ((uint8_t) 1, new_cols_json_diff[2]);
  ASSERT_EQ((size_t) 3, size);

  LogMsgFactory::destroy(sample);
  LogMsgFactory::destroy(sample1);
  LogMsgFactory::destroy(sample2);
}

TEST(LogRecordImpl, ParseTest2)
{
  LogMsgBuf* lmb = new LogMsgBuf();
  ILogRecord* sample = createLogRecord(true);
  size_t sample_msg_size;
  const char* sample_msg_content;
  // record header field
  int srcType = 1;
  int category = 1;
  int recordType = 1;
  long timeStamp = 1663253940;
  bool b = true;
  const uint64_t BR_SUB_VERSION = 0x0500000000000000;  // sub version num
  uint64_t setId = 123456789;
  uint64_t getId = setId | BR_SUB_VERSION;
  int file = 10;
  int offset = 99;
  int sqlNo = 100;
  // record tail field
  int threadId = 0xFFFF0000;
  int usec = 123456789;
  // record data field
  const char* tableName = "table";
  const char* dbName = "database";
  ASSERT_FALSE(sample->isJsonDiffColVal("col1"));
  ASSERT_FALSE(sample->isJsonDiffColVal("col2"));
  ASSERT_TRUE(sample->isJsonDiffColVal("col3"));
  sample_msg_content = sample->toString(&sample_msg_size, lmb);
  ILogRecord* sample1 = new LogRecordImpl(false, false);
  sample1->parse(sample_msg_content, sample_msg_size);
  // set record header field
  sample1->setSrcType(srcType);
  sample1->setSrcCategory(category);
  sample1->setRecordType(recordType);
  sample1->setTimestamp(timeStamp);
  sample1->setFirstInLogevent(b);
  sample1->setId(setId);
  sample1->setCheckpoint(file, offset);
  sample1->setSqlNo(sqlNo);
  // set record tail field
  sample1->setThreadId(threadId);
  sample1->setRecordUsec(usec);
  // set record data field
  sample1->setTbname(tableName);
  sample1->setDbname(dbName);
  ASSERT_STREQ(tableName, sample1->tbname());
  ASSERT_STREQ(dbName, sample1->dbname());
  sample_msg_content = sample1->toString(&sample_msg_size, lmb);
  ILogRecord* sample2 = new LogRecordImpl(false, false);
  sample2->parse(sample_msg_content, sample_msg_size);
  ASSERT_EQ(srcType, sample2->getSrcType());
  ASSERT_EQ(category, sample2->getSrcCategory());
  ASSERT_EQ(recordType, sample2->recordType());
  ASSERT_EQ(timeStamp, sample2->getTimestamp());
  ASSERT_EQ(b, sample2->firstInLogevent());
  ASSERT_EQ(getId, sample2->id());
  ASSERT_EQ(file, sample2->getCheckpoint1());
  ASSERT_EQ(offset, sample2->getCheckpoint2());
  ASSERT_EQ(sqlNo, sample2->sqlNo());
  ASSERT_EQ(threadId, sample2->getThreadId());
  ASSERT_EQ(usec, sample2->getRecordUsec());
  size_t size;
  const uint8_t* new_cols_json_diff = sample2->isNewColsJsonDiff(size);
  ASSERT_EQ((uint8_t) 0, new_cols_json_diff[0]);
  ASSERT_EQ((uint8_t) 0, new_cols_json_diff[1]);
  ASSERT_EQ((uint8_t) 1, new_cols_json_diff[2]);
  ASSERT_EQ((size_t) 3, size);
  LogMsgFactory::destroy(sample);
  LogMsgFactory::destroy(sample1);
  LogMsgFactory::destroy(sample2);
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
