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

using namespace oceanbase::logmessage;

IColMeta* createColMeta()
{
  IColMeta* colMeta = new IColMeta;
  colMeta->setName(COLN);
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

void cmpColMeta(IColMeta& colMeta, std::vector<std::string>& sets)
{
  ASSERT_EQ(0, SMP(COLN, colMeta.getName()));
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

ITableMeta* createTableMeta()
{
  ITableMeta* tableMeta = new ITableMeta;
  tableMeta->setName(TN);
  tableMeta->setHasPK(PK);
  tableMeta->setHasUK(UK);
  tableMeta->setEncoding(ENC);
  tableMeta->setDBMeta(NULL);
  return tableMeta;
}

void cmpTableMeta(ITableMeta& tableMeta)
{
  ASSERT_EQ(0, SMP(TN, tableMeta.getName()));
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
IDBMeta* createDBMeta()
{
  IDBMeta* dbMeta = new IDBMeta();
  dbMeta->setName(DBN);
  dbMeta->setEncoding(ENC);
  dbMeta->setMetaDataCollections(NULL);
  return dbMeta;
}

void cmpDBMeta(IDBMeta* dbMeta)
{
  ASSERT_NE((void*)NULL, (void*)dbMeta);
  ASSERT_EQ(0, SMP(DBN, dbMeta->getName()));
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

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
