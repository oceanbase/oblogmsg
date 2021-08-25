#ifndef _STR_ARRAY_H_
#define _STR_ARRAY_H_

#include <sys/types.h>

namespace oceanbase {
namespace logmessage {

class StrArray {
protected:
  StrArray() = default;

public:
  virtual ~StrArray() = default;
  virtual size_t size() = 0;
  virtual int elementAt(int i, const char*& s, size_t& length) = 0;
  virtual const char* operator[](int i) = 0;
};

}  // namespace logmessage
}  // namespace oceanbase

#endif
