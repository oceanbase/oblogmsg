#include "MsgType.h"
#include <typeinfo>
#include <cstring>

namespace oceanbase {
namespace logmessage {

// typename => type
static struct {
  const char* tn;
  int vt;
} g_nameTypeMap[] = {{typeid(bool).name(), DT_INT8},
    {typeid(char).name(), DT_INT8},
    {typeid(int8_t).name(), DT_INT8},
    {typeid(uint8_t).name(), DT_UINT8},
    {typeid(int16_t).name(), DT_INT16},
    {typeid(uint16_t).name(), DT_UINT16},
    {typeid(int32_t).name(), DT_INT32},
    {typeid(uint32_t).name(), DT_UINT32},
    {typeid(int64_t).name(), DT_INT64},
    {typeid(uint64_t).name(), DT_UINT64},
    {typeid(long).name(), DT_INT64},
    {typeid(unsigned long).name(), DT_UINT64},
    {typeid(float).name(), DT_FLOAT},
    {typeid(double).name(), DT_DOUBLE},
    {0}};

int MsgType::getValType(const char* typeName)
{
  for (int i = 0; g_nameTypeMap[i].tn != NULL; ++i) {
    if (strcmp(g_nameTypeMap[i].tn, typeName) == 0) {
      return g_nameTypeMap[i].vt;
    }
  }
  return DT_UNKNOWN;
}

}  // namespace logmessage
}  // namespace oceanbase
