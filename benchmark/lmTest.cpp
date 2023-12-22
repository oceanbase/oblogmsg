/*
 * dmTest.cpp
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <list>
#include <map>
#include <iostream>

#include "LogRecord.h"
#include "MetaInfo.h"
#ifdef LMB
#include "LogMsgBuf.h"
#endif
#include "itoa.h"
#include "StrArray.h"
using namespace std;
using namespace oceanbase::logmessage;

LogMsgBuf* lmb = NULL;
const char* tableMeta = "[MYTEST,litter]\n"
                        "id,MYSQL_TYPE_LONG,8,P,\n"
                        "num1,MYSQL_TYPE_SHORT,4,,\n"
                        "num2,MYSQL_TYPE_SHORT,4,,\n"
                        "num3,MYSQL_TYPE_SHORT,4,,\n"
                        "num4,MYSQL_TYPE_SHORT,4,,\n"
                        "num5,MYSQL_TYPE_SHORT,4,,\n"
                        "num6,MYSQL_TYPE_SHORT,4,,\n"
                        "num7,MYSQL_TYPE_SHORT,4,U,\n"
                        "[pk:(0)]\n"
                        "[uk:(7)]\n"
                        "[ENC:utf8]\n"
                        "[end]\n"
                        "[MYTEST,litter1]\n"
                        "id,MYSQL_TYPE_LONG,8,P,\n"
                        "num1,MYSQL_TYPE_SHORT,4,,\n"
                        "num2,MYSQL_TYPE_SHORT,4,,\n"
                        "num3,MYSQL_TYPE_SHORT,4,,\n"
                        "num4,MYSQL_TYPE_SHORT,4,,\n"
                        "num5,MYSQL_TYPE_SHORT,4,,\n"
                        "num6,MYSQL_TYPE_SHORT,4,,\n"
                        "num7,MYSQL_TYPE_SHORT,4,U,\n"
                        "char1,MYSQL_TYPE_STRING,64,,\n"
                        "char2,MYSQL_TYPE_STRING,64,,\n"
                        "char3,MYSQL_TYPE_STRING,64,,\n"
                        "char5,MYSQL_TYPE_STRING,64,,\n"
                        "char5,MYSQL_TYPE_STRING,64,,\n"
                        "char6,MYSQL_TYPE_STRING,64,,\n"
                        "char7,MYSQL_TYPE_STRING,64,,\n"
                        "char8,MYSQL_TYPE_STRING,64,,\n"
                        "char9,MYSQL_TYPE_STRING,64,,\n"
                        "char10,MYSQL_TYPE_STRING,64,,\n"
                        "char11,MYSQL_TYPE_STRING,64,,\n"
                        "[pk:(0)]\n"
                        "[uk:(7)]\n"
                        "[ENC:utf8]\n"
                        "[end]\n"
                        "[MYTEST,litter2]\n"
                        "id,MYSQL_TYPE_LONG,8,P,\n"
                        "num1,MYSQL_TYPE_SHORT,4,,\n"
                        "num2,MYSQL_TYPE_SHORT,4,,\n"
                        "num3,MYSQL_TYPE_SHORT,4,,\n"
                        "num4,MYSQL_TYPE_SHORT,4,,\n"
                        "num5,MYSQL_TYPE_SHORT,4,,\n"
                        "num6,MYSQL_TYPE_SHORT,4,,\n"
                        "num7,MYSQL_TYPE_SHORT,4,U,\n"
                        "char8,MYSQL_TYPE_STRING,64,,\n"
                        "char9,MYSQL_TYPE_STRING,64,,\n"
                        "char10,MYSQL_TYPE_STRING,64,,\n"
                        "char11,MYSQL_TYPE_STRING,64,,\n"
                        "char12,MYSQL_TYPE_STRING,64,,\n"
                        "char13,MYSQL_TYPE_STRING,64,,\n"
                        "char14,MYSQL_TYPE_STRING,64,,\n"
                        "char15,MYSQL_TYPE_STRING,64,,\n"
                        "char16,MYSQL_TYPE_STRING,64,,\n"
                        "char17,MYSQL_TYPE_STRING,64,,\n"
                        "char18,MYSQL_TYPE_STRING,64,,\n"
                        "char19,MYSQL_TYPE_STRING,128,,\n"
                        "char20,MYSQL_TYPE_STRING,128,,\n"
                        "char21,MYSQL_TYPE_STRING,128,,\n"
                        "char22,MYSQL_TYPE_STRING,128,,\n"
                        "char23,MYSQL_TYPE_STRING,128,,\n"
                        "char24,MYSQL_TYPE_STRING,128,,\n"
                        "char25,MYSQL_TYPE_STRING,128,,\n"
                        "char26,MYSQL_TYPE_STRING,128,,\n"
                        "char27,MYSQL_TYPE_STRING,128,,\n"
                        "char28,MYSQL_TYPE_STRING,128,,\n"
                        "char29,MYSQL_TYPE_STRING,128,,\n"
                        "[pk:(0)]\n"
                        "[uk:(7)]\n"
                        "[ENC:utf8]\n"
                        "[end]\n"
                        "[MYTEST,litter4]\n"
                        "id,MYSQL_TYPE_LONG,8,P,\n"
                        "num1,MYSQL_TYPE_SHORT,4,,\n"
                        "num2,MYSQL_TYPE_SHORT,4,,\n"
                        "num3,MYSQL_TYPE_SHORT,4,,\n"
                        "num4,MYSQL_TYPE_SHORT,4,,\n"
                        "num5,MYSQL_TYPE_SHORT,4,,\n"
                        "num6,MYSQL_TYPE_SHORT,4,,\n"
                        "num7,MYSQL_TYPE_SHORT,4,U,\n"
                        "char8,MYSQL_TYPE_STRING,64,,\n"
                        "char9,MYSQL_TYPE_STRING,64,,\n"
                        "char10,MYSQL_TYPE_STRING,64,,\n"
                        "char11,MYSQL_TYPE_STRING,64,,\n"
                        "char12,MYSQL_TYPE_STRING,64,,\n"
                        "char13,MYSQL_TYPE_STRING,64,,\n"
                        "char14,MYSQL_TYPE_STRING,64,,\n"
                        "char15,MYSQL_TYPE_STRING,64,,\n"
                        "char16,MYSQL_TYPE_STRING,64,,\n"
                        "char17,MYSQL_TYPE_STRING,64,,\n"
                        "char18,MYSQL_TYPE_STRING,64,,\n"
                        "num19,MYSQL_TYPE_LONG,4,,\n"
                        "num20,MYSQL_TYPE_LONG,4,,\n"
                        "num21,MYSQL_TYPE_LONG,4,,\n"
                        "num22,MYSQL_TYPE_LONG,4,,\n"
                        "num23,MYSQL_TYPE_LONG,4,,\n"
                        "num24,MYSQL_TYPE_LONG,4,,\n"
                        "num25,MYSQL_TYPE_LONG,4,,\n"
                        "char26,MYSQL_TYPE_STRING,128,,\n"
                        "char27,MYSQL_TYPE_STRING,128,,\n"
                        "char28,MYSQL_TYPE_STRING,128,,\n"
                        "char29,MYSQL_TYPE_STRING,128,,\n"
                        "char30,MYSQL_TYPE_STRING,128,,\n"
                        "char31,MYSQL_TYPE_STRING,128,,\n"
                        "char32,MYSQL_TYPE_STRING,128,,\n"
                        "char33,MYSQL_TYPE_STRING,128,,\n"
                        "char34,MYSQL_TYPE_STRING,128,,\n"
                        "char35,MYSQL_TYPE_STRING,128,,\n"
                        "char36,MYSQL_TYPE_STRING,128,,\n"
                        "num37,MYSQL_TYPE_SHORT,4,,\n"
                        "num38,MYSQL_TYPE_SHORT,4,,\n"
                        "num39,MYSQL_TYPE_SHORT,4,,\n"
                        "num40,MYSQL_TYPE_SHORT,4,,\n"
                        "num41,MYSQL_TYPE_SHORT,4,,\n"
                        "num42,MYSQL_TYPE_SHORT,4,,\n"
                        "num43,MYSQL_TYPE_SHORT,4,U,\n"
                        "char44,MYSQL_TYPE_STRING,64,,\n"
                        "char45,MYSQL_TYPE_STRING,64,,\n"
                        "char46,MYSQL_TYPE_STRING,64,,\n"
                        "char47,MYSQL_TYPE_STRING,64,,\n"
                        "char48,MYSQL_TYPE_STRING,64,,\n"
                        "char49,MYSQL_TYPE_STRING,64,,\n"
                        "char50,MYSQL_TYPE_STRING,64,,\n"
                        "char51,MYSQL_TYPE_STRING,64,,\n"
                        "char52,MYSQL_TYPE_STRING,64,,\n"
                        "char53,MYSQL_TYPE_STRING,64,,\n"
                        "char54,MYSQL_TYPE_STRING,64,,\n"
                        "num55,MYSQL_TYPE_LONG,4,,\n"
                        "num56,MYSQL_TYPE_LONG,4,,\n"
                        "num57,MYSQL_TYPE_LONG,4,,\n"
                        "num58,MYSQL_TYPE_LONG,4,,\n"
                        "num59,MYSQL_TYPE_LONG,4,,\n"
                        "num60,MYSQL_TYPE_LONG,4,,\n"
                        "num61,MYSQL_TYPE_LONG,4,,\n"
                        "num62,MYSQL_TYPE_LONG,4,,\n"
                        "num63,MYSQL_TYPE_LONG,4,,\n"
                        "[pk:(0)]\n"
                        "[uk:(7)]\n"
                        "[ENC:utf8]\n"
                        "[end]\n";

/*[dbname,tablename]
 * colname,type,size,pk_or_uk(P/U),encoding
 * ...
 * [pk:()]
 * [uk:(),(),()]
 * [ENC:]
 * [end]
 */
enum enum_field_types {
  MYSQL_TYPE_DECIMAL,
  MYSQL_TYPE_TINY,
  MYSQL_TYPE_SHORT,
  MYSQL_TYPE_LONG,
  MYSQL_TYPE_FLOAT,
  MYSQL_TYPE_DOUBLE,
  MYSQL_TYPE_NULL,
  MYSQL_TYPE_TIMESTAMP,
  MYSQL_TYPE_LONGLONG,
  MYSQL_TYPE_INT24,
  MYSQL_TYPE_DATE,
  MYSQL_TYPE_TIME,
  MYSQL_TYPE_DATETIME,
  MYSQL_TYPE_YEAR,
  MYSQL_TYPE_NEWDATE,
  MYSQL_TYPE_VARCHAR,
  MYSQL_TYPE_BIT,
  MYSQL_TYPE_TIMESTAMP2,
  MYSQL_TYPE_DATETIME2,
  MYSQL_TYPE_TIME2,
  MYSQL_TYPE_NEWDECIMAL = 246,
  MYSQL_TYPE_ENUM = 247,
  MYSQL_TYPE_SET = 248,
  MYSQL_TYPE_TINY_BLOB = 249,
  MYSQL_TYPE_MEDIUM_BLOB = 250,
  MYSQL_TYPE_LONG_BLOB = 251,
  MYSQL_TYPE_BLOB = 252,
  MYSQL_TYPE_VAR_STRING = 253,
  MYSQL_TYPE_STRING = 254,
  MYSQL_TYPE_GEOMETRY = 255

};
static bool isNumType(int type)
{
  switch (type) {

    case LOGMSG_TYPE_DECIMAL:
    case LOGMSG_TYPE_TINY:
    case LOGMSG_TYPE_SHORT:
    case LOGMSG_TYPE_LONG:
    case LOGMSG_TYPE_FLOAT:
    case LOGMSG_TYPE_DOUBLE:
    case LOGMSG_TYPE_LONGLONG:
    case LOGMSG_TYPE_INT24:
    case LOGMSG_TYPE_NEWDECIMAL:
      return true;

    default:
      return false;
  }
}
static void appendData(std::string& Sql, const char* data, int len, int colType)
{
  char dataBuf[64];

  if (len <= 0) {
    Sql.append("NULL");
    return;
  }

  if (!isNumType(colType))
    Sql.append("'");

  switch (colType) {

    case LOGMSG_TYPE_BLOB:
    case LOGMSG_TYPE_LONG_BLOB:
    case LOGMSG_TYPE_MEDIUM_BLOB:
    case LOGMSG_TYPE_TINY_BLOB:
      sprintf(dataBuf, "<lobCol,len:%d>", len);
      Sql.append(dataBuf);
      break;

    case LOGMSG_TYPE_TIMESTAMP:
      /*
       *              * when oblog.conf -> enable_convert_timestamp_to_unix_timestamp=1,timestamp data from mysql mode
       * will change to unixTime
       *                           */
      strncpy(dataBuf, data, len);
      dataBuf[len] = 0;
      Sql.append(dataBuf);
      break;

    default:
      Sql.append(data, 0, len);
      break;
  }

  if (!isNumType(colType))
    Sql.append("'");
}
void showRecord(LogRecordImpl* r)
{
  unsigned int i = 0;
  size_t oldColCount = 0;
  size_t newColCount = 0;
  const char* data;
  size_t dataLen = 0;
  int colType[1024];
  std::string Sql;
  int recordType = 0;

  StrArray* oldBuf = r->parsedOldCols();
  StrArray* newBuf = r->parsedNewCols();
  StrArray* colNames = r->parsedColNames();

  // r->newCols must input a  unsiged int type parameter to back how many newCols

  recordType = r->recordType();
  switch (recordType) {

    case EINSERT: {
      Sql.append("INSERT INTO ");
      Sql.append(r->dbname());
      Sql.append(".");
      Sql.append(r->tbname());
      Sql.append(" (");

      newColCount = newBuf->size();
      for (i = 0; i < newColCount; i++) {
        colNames->elementAt(i, data, dataLen);
        Sql.append(data);
        colType[i] = r->parsedColTypes()[i];
        if (i != newColCount - 1)
          Sql.append(",");
      }

      Sql.append(") VALUES (");

      /*
       * loop every column's data
       */
      for (i = 0; i < newColCount; i++) {
        newBuf->elementAt(i, data, dataLen);
        appendData(Sql, data, dataLen, colType[i]);
        if (i != newColCount - 1)
          Sql.append(",");
      }

      Sql.append(")");
      std::cout << Sql << ",timestamp:" << r->getTimestamp() << std::endl;
      break;
    }
    case EUPDATE: {

      Sql.append("UPDATE ");
      Sql.append(r->dbname());
      Sql.append(".");
      Sql.append(r->tbname());
      Sql.append(" SET ");

      /*
       * loop every column's new data
       */
      oldColCount = oldBuf->size();
      newColCount = newBuf->size();

      for (i = 0; i < newColCount; i++) {
        colNames->elementAt(i, data, dataLen);
        Sql.append(data);
        Sql.append("=");

        colType[i] = r->parsedColTypes()[i];
        newBuf->elementAt(i, data, dataLen);
        appendData(Sql, data, dataLen, colType[i]);

        if (i != newColCount - 1)
          Sql.append(", ");
      }

      Sql.append(" WHERE ");

      /*
       * loop every column's old data
       */
      for (i = 0; i < oldColCount; i++) {
        colNames->elementAt(i, data, dataLen);
        Sql.append(data);
        Sql.append("=");
        oldBuf->elementAt(i, data, dataLen);
        appendData(Sql, data, dataLen, colType[i]);

        if (i != oldColCount - 1)
          Sql.append(" and ");
      }

      std::cout << Sql.c_str() << ",timestamp:" << r->getTimestamp() << std::endl;
      break;
    }

    case EDELETE: {

      oldColCount = oldBuf->size();
      Sql.append("DELETE FROM ");
      Sql.append(r->dbname());
      Sql.append(".");
      Sql.append(r->tbname());
      Sql.append(" WHERE ");

      /*
       * loop every column's old data
       */
      for (i = 0; i < oldColCount; i++) {
        colNames->elementAt(i, data, dataLen);
        Sql.append(data);
        Sql.append("=");

        colType[i] = r->parsedColTypes()[i];
        oldBuf->elementAt(i, data, dataLen);
        appendData(Sql, data, dataLen, colType[i]);

        if (i != oldColCount - 1)
          Sql.append(" and ");
      }

      std::cout << Sql.c_str() << ",timestamp:" << r->getTimestamp() << std::endl;
      break;
    }
  }
}
#define TYPE_TO_STR(t) \
  {                    \
    t, #t              \
  }
struct mtp {
  int type;
  const char* tstr;
};
mtp type_list[] = {TYPE_TO_STR(MYSQL_TYPE_DECIMAL),
    TYPE_TO_STR(MYSQL_TYPE_TINY),
    TYPE_TO_STR(MYSQL_TYPE_DECIMAL),
    TYPE_TO_STR(MYSQL_TYPE_TINY),
    TYPE_TO_STR(MYSQL_TYPE_SHORT),
    TYPE_TO_STR(MYSQL_TYPE_LONG),
    TYPE_TO_STR(MYSQL_TYPE_FLOAT),
    TYPE_TO_STR(MYSQL_TYPE_DOUBLE),
    TYPE_TO_STR(MYSQL_TYPE_NULL),
    TYPE_TO_STR(MYSQL_TYPE_TIMESTAMP),
    TYPE_TO_STR(MYSQL_TYPE_LONGLONG),
    TYPE_TO_STR(MYSQL_TYPE_INT24),
    TYPE_TO_STR(MYSQL_TYPE_DATE),
    TYPE_TO_STR(MYSQL_TYPE_TIME),
    TYPE_TO_STR(MYSQL_TYPE_DATETIME),
    TYPE_TO_STR(MYSQL_TYPE_YEAR),
    TYPE_TO_STR(MYSQL_TYPE_NEWDATE),
    TYPE_TO_STR(MYSQL_TYPE_VARCHAR),
    TYPE_TO_STR(MYSQL_TYPE_BIT),
    TYPE_TO_STR(MYSQL_TYPE_TIMESTAMP2),
    TYPE_TO_STR(MYSQL_TYPE_DATETIME2),
    TYPE_TO_STR(MYSQL_TYPE_TIME2),
    TYPE_TO_STR(MYSQL_TYPE_NEWDECIMAL),
    TYPE_TO_STR(MYSQL_TYPE_ENUM),
    TYPE_TO_STR(MYSQL_TYPE_SET),
    TYPE_TO_STR(MYSQL_TYPE_TINY_BLOB),
    TYPE_TO_STR(MYSQL_TYPE_MEDIUM_BLOB),
    TYPE_TO_STR(MYSQL_TYPE_LONG_BLOB),
    TYPE_TO_STR(MYSQL_TYPE_BLOB),
    TYPE_TO_STR(MYSQL_TYPE_VAR_STRING),
    TYPE_TO_STR(MYSQL_TYPE_STRING),
    TYPE_TO_STR(MYSQL_TYPE_GEOMETRY)};
std::map<std::string, int> type_map;
int getType(const char* type)
{
  if (type_map.empty()) {
    for (unsigned int i = 0; i < sizeof(type_list) / sizeof(mtp); i++) {
      type_map.insert(std::pair<std::string, int>(string(type_list[i].tstr), type_list[i].type));
    }
  }
  std::map<std::string, int>::iterator itor = type_map.find(string(type));
  if (itor == type_map.end()) {
    std::cout << "unknow type:" << type << std::endl;
    return -1;
  }
  return itor->second;
}
list<ITableMeta*>* initRecordFromMeta()
{
  char buf[1024] = {0};
  int bufPos = 0;
  int st = 0, line = 1;
  ITableMeta* m = NULL;
  const char* pEnd = tableMeta + strlen(tableMeta);
  const char* pByte = tableMeta;
  list<ITableMeta*>* l = new list<ITableMeta*>;
  while (true) {
    bufPos = 0;
    while (pByte != pEnd && *pByte != '\n') {
      buf[bufPos++] = *pByte++;
    }
    if (pByte == pEnd)
      break;
    pByte++;
    if (buf[0] == '[') {
      if (st == 0) {
        m = new ITableMeta();
        st = 1;
        char *pos = strchr(buf + 1, ','), *pos_end;
        if (pos == NULL) {
          std::cout << "int line :" << line << "," << buf << " ERROR:can not find ',' in [DB,TANLE]" << std::endl;
          goto err;
        }
        *pos = 0;
        // todo
        if ((pos_end = strchr(pos + 1, ']')) == NULL) {
          std::cout << "int line :" << line << "," << buf << " ERROR:can not find ',' in [DB,TANLE]" << std::endl;
          goto err;
        }
        *pos_end = 0;
        m->setName(pos + 1);
        line++;
        continue;
      } else if (st == 1) {
        std::cout << "table is empty in line:" << line << " " << buf << std::endl;
        delete m;
        line++;
        st = 0;
        continue;
      } else if (st == 2) {
        if (strncasecmp(buf, "[PK:", 4) == 0) {
          m->setHasPK(true);
          if (buf[strlen(buf) - 1] == '\n') {
            buf[strlen(buf) - 1] = 0;
          }
          m->setPkinfo(buf + 4);
        } else if (strncasecmp(buf, "[UK:", 4) == 0) {
          m->setHasUK(true);
          if (buf[strlen(buf) - 1] == '\n') {
            buf[strlen(buf) - 1] = 0;
          }
          m->setUkinfo(buf + 4);
        } else if (strncasecmp(buf, "[ENC:", 5) == 0) {
          m->setHasUK(true);
          if (buf[strlen(buf) - 1] == '\n') {
            buf[strlen(buf) - 1] = 0;
          }
          m->setEncoding(buf + 5);
        } else if (strncasecmp(buf, "[END]", 5) == 0) {
          l->push_back(m);
          st = 0;
          line++;
          continue;

        } else {
          std::cout << "int line :" << line << " " << buf << " ERROR:expectd [END] or [PK:] or [UK:]" << std::endl;
          goto err;
        }
      }
    } else {
      if (st == 0) {
        std::cout << "unexpected info ,not in table define in line:" << line << " " << buf << std::endl;
        line++;
        continue;
      }
      char *pos = buf, *pos_end;
      IColMeta* colMeta = new IColMeta;
      if ((pos_end = strchr(pos, ',')) == NULL) {
        std::cout << "unexpected colname in line:" << line << " " << pos << std::endl;
        goto err;
      }
      *pos_end = 0;
      colMeta->setName(pos);
      pos = pos_end + 1;
      if ((pos_end = strchr(pos, ',')) == NULL) {
        std::cout << "unexpected colname in line:" << line << " " << pos << std::endl;
        goto err;
      }
      *pos_end = 0;
      colMeta->setType(getType(pos));
      pos = pos_end + 1;
      if ((pos_end = strchr(pos, ',')) == NULL) {
        std::cout << "expected size in line:" << line << " " << pos << std::endl;
        goto err;
      }
      if (pos != pos_end) {
        *pos_end = 0;
        colMeta->setLength(atoi(pos));
      }
      pos = pos_end + 1;
      if ((pos_end = strchr(pos, ',')) == NULL) {
        std::cout << "expected PK_OR_UK in line:" << line << " " << pos << std::endl;
        goto err;
      }
      *pos_end = 0;
      if (pos == pos_end) {
        colMeta->setIsPK(false);
        colMeta->setIsUK(false);
      } else if (*pos == 'P') {
        colMeta->setIsPK(true);
        colMeta->setIsUK(false);
      } else if (*pos == 'U') {
        colMeta->setIsPK(false);
        colMeta->setIsUK(true);
      } else {
        std::cout << "expected P or U or empty in PK_OR_UK line:" << line << " " << pos << std::endl;
        goto err;
      }
      pos = pos_end + 1;
      if (*pos == '\n' || *pos == 0) {
        colMeta->setEncoding("");
      } else {
        if (pos[strlen(pos) - 1] == '\n') {
          pos[strlen(pos) - 1] = 0;
        }
        colMeta->setEncoding(pos);
      }
      m->append(colMeta->getName(), colMeta);
      if (st == 1)
        st = 2;
    }
    line++;
  }
  return l;
err:
  delete m;
  delete l;
  return NULL;
}
static long file_id = 0;
static long file_off = 4;
static char ci_v = 0;
static short si_v = 0;
static long li_v = 0;
static float ff_v = 0.0f;
static double df_v = 0.0f;
static time_t t;
int random_str(char* v, int size)
{
  if (size <= 8) {
    strncpy(v, "ABCDEFGH", size - 1);
    return size;
  }
  long i = 0x3031323334353637;
  char* p = v;
  do {
    *(long*)p = i;
    p += sizeof(i);
    i += 0x0101010101010101;
    if (i >= 0x7D00000000000000)
      i = 0x3031323334353637;
  } while (p - v < size);
  return p - v - sizeof(i);
}
LogRecordImpl* createRecordByMeta(ITableMeta* m, int type, bool string_or_point, int* upd = NULL, int upd_size = 0)
{
#ifdef LMB
  LogRecordImpl* r = new LogRecordImpl(true, true);
#else
  LogRecordImpl* r = new LogRecordImpl(true, false);
#endif
  char *buf = NULL, *pos = NULL;
  int size = 0, csize = 0;
  if (!string_or_point)
    buf = (char*)malloc(size = m->getColCount() * 256);
  else
    buf = (char*)malloc(256);

  pos = buf;

  r->setDbname("MYTEST");
  r->setTbname(m->getName());
  r->setTableMeta(m);
  r->setRecordEncoding(m->getEncoding());
  r->setRecordType(type);
  r->setCheckpoint(file_id, file_off);
  r->setInstance("127.0.0.1:3306");
  if ((file_off += 2048) > (500 * 1024 * 1024)) {
    file_id++;
    file_off = 4;
  }
  r->setTimestamp(time(NULL));
  int upd_pos = 0;
  bool update = false, update_n_or_l = false;
  if (!string_or_point) {
    r->setNewColumn(new BinLogBuf[m->getColCount()], m->getColCount());
    r->setOldColumn(new BinLogBuf[m->getColCount()], m->getColCount());
  }
  for (int i = 0; i < m->getColCount(); i++) {
    int ctp = m->getCol(i)->getType();
    if (type == EUPDATE) {
      if (upd == NULL || i == upd[upd_pos]) {
        update = true;
        update_n_or_l = true;
        upd_pos++;
      } else
        update = false;
    }
  set_col:
    switch (ctp) {
      case MYSQL_TYPE_TINY:
        csize = sitoa(ci_v, pos);
        ci_v++;
        break;
      case MYSQL_TYPE_SHORT:
        csize = sitoa(si_v, pos);
        si_v++;
        break;
      case MYSQL_TYPE_LONG:
        csize = ltoa(li_v, pos);
        li_v++;
        break;
      case MYSQL_TYPE_TIMESTAMP:
        t = time(NULL);
        csize = ltoa(t, pos);
        break;
      case MYSQL_TYPE_FLOAT:
        sprintf(pos, "%f", ff_v);
        ff_v += 0.01f;
        csize = strlen(pos);
        break;
      case MYSQL_TYPE_DOUBLE:
        sprintf(pos, "%lf", df_v);
        df_v += 0.01f;
        csize = strlen(pos);
        break;
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_STRING:
        csize = m->getCol(i)->getLength();
        if (csize > 32 && csize < 256)
          csize = (csize >> 2) + (csize >> 1);
        else if (csize >= 256 && csize <= 2048)
          csize = (csize >> 1);
        else if (csize > 2048)
          csize = 1024;
        csize = random_str(pos, csize);
        break;
      default:
        std::cout << "not support type:" << ctp << std::endl;
        delete buf;
        delete r;
        return NULL;
    }
    if (string_or_point) {
      if (type == EINSERT || (type == EUPDATE && (!update || update_n_or_l)))
        r->putNew(new string(pos, csize));
      if (type == EDELETE || (type == EUPDATE && (!update || !update_n_or_l)))
        r->putOld(new string(pos, csize));
    } else {
      if (type == EINSERT || (type == EUPDATE && (!update || update_n_or_l)))
        r->putNew(pos, csize);
      if (type == EDELETE || (type == EUPDATE && (!update || !update_n_or_l)))
        r->putOld(pos, csize);
      pos += csize;
      *pos = 0;
      pos++;
    }
    if (type == EUPDATE && (update && update_n_or_l)) {
      update_n_or_l = false;
      goto set_col;
    }
  }
  if (!string_or_point)
    r->setUserData(buf);
  else
    free(buf);
  return r;
}
#define CHECK_TM(s, e, head, v)                                                                                \
  do {                                                                                                         \
    v = (e.tv_sec - s.tv_sec) * 1000000000 + e.tv_nsec - s.tv_nsec;                                            \
    std::cout << (head) << " " << v / 1000000000 << "s " << (v / 1000000) % 1000 << "ms " << (v / 1000) % 1000 \
              << "us " << v % 1000 << "ns" << std::endl;                                                       \
  } while (0);

const char* ts[] = {"INSERT", "UPDATE", "DELETE"};
int dm_speed_test(int type, bool string_or_point)
{
  list<ITableMeta*>* l = initRecordFromMeta();
  if (l == NULL) {
    std::cout << "init record from meta failed" << std::endl;
    return -1;
  }
  LogRecordImpl** lrl = new LogRecordImpl*[500000];

  for (list<ITableMeta*>::iterator it = l->begin(); it != l->end(); it++) {
    struct timespec s, e;
    long v = 0, sc = 20, ac = 0;
    for (int i = 0; i < 20000; i++) {
      for (int i = 0; i < sc; i++) {
        lrl[i] = createRecordByMeta(*it, type, string_or_point);
        if (lrl[i] == NULL) {
          std::cout << "create record failed" << std::endl;
          for (int j = 0; j < i; j++) {
            if (lrl[j]->getUserData() != NULL)
              free(lrl[j]->getUserData());
            if (!string_or_point) {
              unsigned int s;
              delete lrl[j]->oldCols(s);
              delete lrl[j]->newCols(s);
            }
            delete lrl[j];
          }
          for (list<ITableMeta*>::iterator itf = l->begin(); itf != l->end(); itf++)
            delete *itf;
          delete l;
          return -1;
        }
      }
      size_t size = 0;
      clock_gettime(CLOCK_REALTIME, &s);
      for (int i = 0; i < sc; i++) {
#ifdef LMB
        lrl[i]->toString(&size,lmb);
#else
        lrl[i]->toString();
#endif
      }
#if 0
            LogRecordImpl * r = new LogRecordImpl(p,size);
            showRecord(r);
            delete r;
#endif
      clock_gettime(CLOCK_REALTIME, &e);
      v += ((e.tv_sec - s.tv_sec) * 1000000000 + e.tv_nsec - s.tv_nsec);
      ac += sc;
      for (int j = 0; j < sc; j++) {
        if (lrl[j]->getUserData() != NULL)
          free(lrl[j]->getUserData());
        if (!string_or_point) {
          unsigned int s;
          BinLogBuf* b;
          b = lrl[j]->oldCols(s);
          memset(b, 0, sizeof(BinLogBuf) * ((*it)->getColCount()));
          delete[] b;
          b = lrl[j]->newCols(s);
          memset(b, 0, sizeof(BinLogBuf) * ((*it)->getColCount()));
          delete[] b;
        }
        delete lrl[j];
      }
    }
    std::cout << "table " << (*it)->getName() << " " << ts[type] << " cost time: " << v / 1000000000 << "s "
              << (v / 1000000) % 1000 << "ms " << (v / 1000) % 1000 << "us " << v % 1000 << "ns,cost " << v / ac
              << " ns per record " << std::endl;
  }
  for (list<ITableMeta*>::iterator itf = l->begin(); itf != l->end(); itf++)
    delete *itf;
  delete[] lrl;
  delete l;
  return 0;
}
int main(int argc, char* argv[])
{
#ifdef LMB
  lmb = new LogMsgBuf();
#endif
  dm_speed_test(EINSERT, false);
  dm_speed_test(EUPDATE, false);
  dm_speed_test(EDELETE, false);
#ifdef LMB
  delete lmb;
#endif
  return 0;
}
