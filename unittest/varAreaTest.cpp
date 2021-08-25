/**
 * MsgVarArea API test
 */
#include "MsgVarArea.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtest/gtest.h>

using namespace oceanbase::logmessage;

union U {
  int8_t i8;
  uint8_t ui8;
  int16_t i16;
  uint16_t ui16;
  int32_t i32;
  uint32_t ui32;
  int64_t i64;
  uint64_t ui64;
  float f;
  double d;
};

static const char* g_cstr = "Hello test";
static std::string g_str = "Hello string";
static const char* g_cstrs[] = {
    "Hello test1",
    "Hello test2",
    "Hello test3",
    NULL,
    "Hello test4",
};
static std::string g_strArr[] = {
    std::string("Hello string1"),
    std::string("Hello string2"),
    std::string("Hello string3"),
    std::string("Hello string4"),
};
static std::vector<std::string*> g_strs;
static int32_t g_intArr[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

TEST(MsgVarArea, MsgVarAreaAPI)
{
  MsgVarArea creator;

  U u;
  u.i64 = 0x1234567890abcdef;
  size_t beginOffset = creator.append(u.i8);
  ASSERT_EQ(beginOffset, (size_t)0);
  creator.append(u.i8);
  creator.append(u.ui8);
  creator.append(u.i16);
  creator.append(u.ui16);
  creator.append(u.i32);
  creator.append(u.ui32);
  creator.append(u.i64);
  creator.append(u.ui64);
  creator.append(u.f);
  creator.append(u.d);
  size_t nullCStrOffset = creator.appendString((const char*)NULL);
  size_t nullStrOffset = creator.appendString((std::string*)NULL);
  size_t cstrOffset = creator.appendString(g_cstr);
  size_t strOffset = creator.appendString(&g_str);
  size_t nullCStrArrOffset = creator.appendStringArray((const char**)NULL, 0);
  size_t cstrArrOffset = creator.appendStringArray(g_cstrs, 5);
  size_t nullStrArrOffset = creator.appendStringArray(g_strs);
  g_strs.push_back(&g_strArr[0]);
  g_strs.push_back(&g_strArr[1]);
  g_strs.push_back(NULL);
  g_strs.push_back(&g_strArr[2]);
  g_strs.push_back(&g_strArr[3]);
  size_t strArrOffset = creator.appendStringArray(g_strs);
  size_t nullIntArrOffset = creator.appendArray(g_intArr, 0);
  size_t intArrOffset = creator.appendArray(g_intArr, 10);
  // get result
  const std::string& result = creator.getMessage();
  size_t resultSize = result.size();

  // -----------
  MsgVarArea parser(false);
  ASSERT_EQ(0, parser.parse(result.c_str(), result.length()));
  size_t size = parser.getRealSize();
  ASSERT_EQ(size, resultSize);

  const void* v;
  // getField
  int ret = parser.getField(beginOffset, v, size);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(size, sizeof(int8_t));

  // getString
  const char* s;
  size_t length;
  ret = parser.getString(nullCStrOffset, s, length);
  ASSERT_EQ(0, ret);
  ASSERT_EQ((void*)NULL, (void*)s);
  ret = parser.getString(nullStrOffset, s, length);
  ASSERT_EQ(0, ret);
  ASSERT_EQ((void*)NULL, (void*)s);
  ret = parser.getString(cstrOffset, s, length);
  ASSERT_EQ(0, ret);
  ASSERT_NE((void*)NULL, (void*)s);
  ASSERT_EQ(0, strcmp(s, g_cstr));
  ret = parser.getString(strOffset, s, length);
  ASSERT_EQ(0, ret);
  ASSERT_NE((void*)NULL, (void*)s);
  ASSERT_EQ(0, strcmp(s, g_str.c_str()));
  StrArray* arr;
  arr = parser.getStringArray(nullCStrArrOffset);
  ASSERT_EQ((void*)NULL, (void*)arr);
  arr = parser.getStringArray(nullStrArrOffset);
  ASSERT_EQ((void*)NULL, (void*)arr);
  arr = parser.getStringArray(cstrArrOffset);
  ASSERT_NE((void*)NULL, (void*)arr);
  size = arr->size();
  ASSERT_EQ((size_t)5, size);
  delete arr;
  arr = parser.getStringArray(strArrOffset);
  ASSERT_NE((void*)NULL, (void*)arr);
  size = arr->size();
  ASSERT_EQ((size_t)5, size);
  delete arr;
  // getArray
  size_t elSize;
  ret = parser.getArray(nullIntArrOffset, v, elSize, size);
  ASSERT_EQ(0, ret);
  ASSERT_EQ((void*)NULL, (void*)v);
  ret = parser.getArray(intArrOffset, v, elSize, size);
  ASSERT_EQ(0, ret);
  ASSERT_NE((void*)NULL, (void*)v);
  ASSERT_EQ(elSize, sizeof(int32_t));
  ASSERT_EQ(size, (size_t)10);
}

int main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
