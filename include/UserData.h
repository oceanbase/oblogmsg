#ifndef __USER_DATA_H__
#define __USER_DATA_H__

namespace oceanbase {
namespace logmessage {

class UserDataInterface {
public:
  virtual ~UserDataInterface()
  {}
  virtual void setUserData(void* data) = 0;
  virtual void* getUserData() = 0;
};

}  // namespace logmessage
}  // namespace oceanbase

#endif
