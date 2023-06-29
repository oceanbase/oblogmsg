#include "MetaInfo.h"
#include "StrArray.h"
#include <stdio.h>
#include <gtest/gtest.h>
#include <vector>
#include <string>

#define COLN "col1"
#define COLT 253
#define COLL 4000
#define COLS false
#define PK true
#define UK false
#define NN true
#define DEF "default value for col1"
#define ENC "utf-8"

#define SMP(a, b) strcmp((a), (b))

#ifdef _CASE_SENSITIVE_
#define C1 "col1"
#define C2 "Col1"
#define C3 "COL1"
#define T1 "table1"
#define T2 "Table1"
#define T3 "TABLE1"
#define D1 "db_01"
#define D2 "Db_01"
#define D3 "DB_01"
#else
#define C1 "col1"
#define C2 "col2"
#define C3 "col3"
#define T1 "table1"
#define T2 "table2"
#define T3 "table3"
#define D1 "db1"
#define D2 "db2"
#define D3 "db3"
#endif

using namespace oceanbase::logmessage;

IColMeta* createColMeta(const char* col_name = COLN)
{
  IColMeta* colMeta = new IColMeta;
  colMeta->setName(col_name);
  colMeta->setType(COLT);
  colMeta->setLength(COLL);
  colMeta->setSigned(COLS);
  colMeta->setIsPK(PK);
  colMeta->setIsUK(UK);
  colMeta->setNotNull(NN);
  colMeta->setDefault(DEF);
  colMeta->setEncoding(ENC);
  std::vector<std::string> sets;
  sets.push_back("set1");
  sets.push_back("set2");
  colMeta->setValuesOfEnumSet(sets);
  return colMeta;
}

void cmpColMeta(IColMeta& colMeta, std::vector<std::string>& sets, const char* col_name = COLN)
{
  ASSERT_EQ(0, SMP(col_name, colMeta.getName()));
  ASSERT_EQ(COLT, colMeta.getType());
  ASSERT_EQ(COLL, colMeta.getLength());
  ASSERT_EQ(COLS, colMeta.isSigned());
  ASSERT_EQ(PK, colMeta.isPK());
  ASSERT_EQ(UK, colMeta.isUK());
  ASSERT_EQ(NN, colMeta.isNotNull());
  ASSERT_EQ(0, SMP(DEF, colMeta.getDefault()));
  ASSERT_EQ(0, SMP(ENC, colMeta.getEncoding()));
  StrArray* serializedSets = colMeta.getValuesOfEnumSet();
  ASSERT_NE((void*)NULL, (void*)serializedSets);
  ASSERT_EQ(0, SMP(sets[0].c_str(), (*serializedSets)[0]));
  ASSERT_EQ(0, SMP(sets[1].c_str(), (*serializedSets)[1]));
  delete serializedSets;
}

TEST(IColMeta, IColMetaAPI)
{
  IColMeta* colMeta = createColMeta();

  /* Prepare the column meta */
  std::vector<std::string> sets;
  sets.push_back("set1");
  sets.push_back("set2");
  cmpColMeta(*colMeta, sets);

  /* Compare with parsed one */
  std::string s;
  ASSERT_EQ(0, colMeta->appendTo(s));
  IColMeta parser(s.c_str(), s.size());
  size_t msgSize = s.size();
  ASSERT_EQ(msgSize, parser.getRealSize());
  cmpColMeta(parser, sets);
  delete colMeta;
}

#define TN "table1"
ITableMeta* createTableMeta(const char* tb_name = TN)
{
  ITableMeta* tableMeta = new ITableMeta;
  tableMeta->setName(tb_name);
  tableMeta->setHasPK(PK);
  tableMeta->setHasUK(UK);
  tableMeta->setEncoding(ENC);
  tableMeta->setDBMeta(NULL);
  return tableMeta;
}

void cmpTableMeta(ITableMeta& tableMeta, const char* tb_name = TN)
{
  ASSERT_EQ(0, SMP(tb_name, tableMeta.getName()));
  ASSERT_EQ(PK, tableMeta.hasPK());
  ASSERT_EQ(UK, tableMeta.hasUK());
  ASSERT_EQ(0, SMP(ENC, tableMeta.getEncoding()));
  ASSERT_EQ((void*)NULL, (void*)tableMeta.getDBMeta());
}

TEST(ITableMeta, ITableMetaAPI)
{
  // Prepare the table meta
  ITableMeta* tableMeta = createTableMeta();
  IColMeta* colMeta = createColMeta();
  tableMeta->append(colMeta->getName(), colMeta);

  // Compare column meta
  ASSERT_EQ(colMeta, tableMeta->getCol(0));
  ASSERT_EQ(colMeta, tableMeta->getCol(COLN));
  std::vector<std::string>& colNames = tableMeta->getColNames();
  ASSERT_EQ((size_t)1, colNames.size());
  ASSERT_EQ(0, SMP(COLN, colNames[0].c_str()));

  // Compare with parsed tablemeta
  std::string s;
  ASSERT_EQ(0, tableMeta->appendTo(s));
  ITableMeta parser(s.c_str(), s.size());
  std::string c;
  ASSERT_EQ(0, colMeta->appendTo(c));
  IColMeta colParser(c.c_str(), c.size());
  ASSERT_EQ(s.size(), parser.getRealSize() + colParser.getRealSize());
  cmpTableMeta(parser);

  delete tableMeta;
}

#define DBN "db1"
IDBMeta* createDBMeta(const char* db_name = DBN)
{
  IDBMeta* dbMeta = new IDBMeta();
  dbMeta->setName(db_name);
  dbMeta->setEncoding(ENC);
  dbMeta->setMetaDataCollections(NULL);
  return dbMeta;
}

void cmpDBMeta(IDBMeta* dbMeta, const char* db_name = DBN)
{
  ASSERT_NE((void*)NULL, (void*)dbMeta);
  ASSERT_EQ(0, SMP(db_name, dbMeta->getName()));
  ASSERT_EQ(0, SMP(ENC, dbMeta->getEncoding()));
  ASSERT_EQ((void*)NULL, (void*)dbMeta->getMetaDataCollections());
}

TEST(IDBMeta, IDBMetaAPI)
{
  IDBMeta* dbMeta = createDBMeta();
  cmpDBMeta(dbMeta);
  delete dbMeta;
}

TEST(IMetaDataCollections, IMetaDataCollectionsAPI)
{
  IMetaDataCollections* meta = new IMetaDataCollections;
  ASSERT_NE((void*)NULL, (void*)meta);
  delete meta;
}

// ===== case sensitive test  =======
TEST(ITableMeta, ITableMetaCaseSensitive)
{
  IColMeta* colMeta1 = createColMeta(C1);
  IColMeta* colMeta2 = createColMeta(C2);
  IColMeta* colMeta3 = createColMeta(C3);

  ITableMeta* tableMeta = createTableMeta(TN);
  ASSERT_EQ(0, tableMeta->append(colMeta1->getName(), colMeta1));
  ASSERT_EQ(0, tableMeta->append(colMeta2->getName(), colMeta2));
  ASSERT_EQ(0, tableMeta->append(colMeta3->getName(), colMeta3));

  ASSERT_EQ(3, tableMeta->getColCount());
  ASSERT_STREQ(C1, tableMeta->getCol(0)->getName());
  ASSERT_STREQ(C2, tableMeta->getCol(1)->getName());
  ASSERT_STREQ(C3, tableMeta->getCol(2)->getName());
  ASSERT_EQ(colMeta1, tableMeta->getCol(C1));
  ASSERT_EQ(colMeta2, tableMeta->getCol(C2));
  ASSERT_EQ(colMeta3, tableMeta->getCol(C3));
  ASSERT_EQ(0, tableMeta->getColIndex(C1));
  ASSERT_EQ(1, tableMeta->getColIndex(C2));
  ASSERT_EQ(2, tableMeta->getColIndex(C3));
  ASSERT_EQ(0, tableMeta->getColNum(C1));
  ASSERT_EQ(1, tableMeta->getColNum(C2));
  ASSERT_EQ(2, tableMeta->getColNum(C3));
  ASSERT_EQ(3, tableMeta->getColNames().size());
  ASSERT_STREQ(C1, tableMeta->getColNames()[0].c_str());
  ASSERT_STREQ(C2, tableMeta->getColNames()[1].c_str());
  ASSERT_STREQ(C3, tableMeta->getColNames()[2].c_str());

  delete tableMeta;
}

TEST(IDBMeta, IDBMetaCaseSensitive)
{
  IColMeta* colMeta1 = createColMeta(C1);
  ITableMeta* tableMeta1 = createTableMeta(T1);
  ASSERT_EQ(0, tableMeta1->append(colMeta1->getName(), colMeta1));

  IColMeta* colMeta2 = createColMeta();
  ITableMeta* tableMeta2 = createTableMeta(T2);
  ASSERT_EQ(0, tableMeta2->append(colMeta1->getName(), colMeta2));

  IColMeta* colMeta3 = createColMeta();
  ITableMeta* tableMeta3 = createTableMeta(T3);
  ASSERT_EQ(0, tableMeta3->append(colMeta1->getName(), colMeta3));

  IDBMeta* dbMeta = createDBMeta();
  ASSERT_EQ(0, dbMeta->put(tableMeta1));
  ASSERT_EQ(0, dbMeta->put(tableMeta2));
  ASSERT_EQ(0, dbMeta->put(tableMeta3));

  // assert
  ASSERT_EQ(3, dbMeta->getTblCount());
  ASSERT_STREQ(T1, dbMeta->get(0)->getName());
  ASSERT_STREQ(T2, dbMeta->get(1)->getName());
  ASSERT_STREQ(T3, dbMeta->get(2)->getName());
  ASSERT_EQ(tableMeta1, dbMeta->get(T1));
  ASSERT_EQ(tableMeta2, dbMeta->get(T2));
  ASSERT_EQ(tableMeta3, dbMeta->get(T3));
  ASSERT_EQ(0, dbMeta->erase(T3, false));
  ASSERT_EQ(nullptr, dbMeta->get(T3));
  ASSERT_EQ(tableMeta2, dbMeta->get(T2));
  ASSERT_EQ(2, dbMeta->getTblCount());
  ASSERT_EQ(0, dbMeta->eraseMapIterator());
  ASSERT_EQ(1, dbMeta->getTblCount());

  const char* table_name;
  ITableMeta* table_meta;
  dbMeta->getFromMapIterator(&table_name, &table_meta);
  ASSERT_STREQ(T1, table_name);
  ASSERT_EQ(tableMeta1, table_meta);
}

TEST(IMetaDataCollections, IMetaDataCollectionsCaseSensitive)
{
  auto* meta = new IMetaDataCollections;
  ITableMeta* tableMeta1 = createTableMeta(T1);
  IDBMeta* dbMeta1 = createDBMeta(D1);
  ASSERT_EQ(0, dbMeta1->put(tableMeta1));

  ITableMeta* tableMeta2 = createTableMeta(T2);
  IDBMeta* dbMeta2 = createDBMeta(D2);
  ASSERT_EQ(0, dbMeta2->put(tableMeta2));

  ITableMeta* tableMeta3 = createTableMeta(T3);
  IDBMeta* dbMeta3 = createDBMeta(D3);
  ASSERT_EQ(0, dbMeta3->put(tableMeta3));

  ASSERT_EQ(0, meta->put(dbMeta1));
  ASSERT_EQ(0, meta->put(dbMeta2));
  ASSERT_EQ(0, meta->put(dbMeta3));

  ASSERT_EQ(3, meta->getDbCount());
  ASSERT_STREQ(D1, meta->get(0)->getName());
  ASSERT_STREQ(D2, meta->get(1)->getName());
  ASSERT_STREQ(D3, meta->get(2)->getName());
  ASSERT_EQ(dbMeta1, meta->get(D1));
  ASSERT_EQ(dbMeta2, meta->get(D2));
  ASSERT_EQ(dbMeta3, meta->get(D3));
  ASSERT_EQ(tableMeta1, meta->get(D1, T1));
  ASSERT_EQ(tableMeta2, meta->get(D2, T2));
  ASSERT_EQ(tableMeta3, meta->get(D3, T3));
  ASSERT_EQ(0, meta->erase(D1, false));
  ASSERT_EQ(2, meta->getDbCount());
  ASSERT_EQ(nullptr, meta->get(D1));
  ASSERT_EQ(dbMeta2, meta->get(D2));
  ASSERT_EQ(0, meta->erase(D2, T2, false));
  ASSERT_EQ(0, meta->get(D2)->getTblCount());
  ASSERT_EQ(2, meta->getDbCount());

  const char* db_name;
  IDBMeta* db_meta;
  meta->getFromMapIterator(&db_name, &db_meta);
  ASSERT_STREQ(D3, db_name);
  ASSERT_EQ(dbMeta3, db_meta);

  delete meta;
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
