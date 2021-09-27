/* Copyright (c) 2021 OceanBase and/or its affiliates. All rights reserved.
oblogmsg is licensed under Mulan PSL v2.
You can use this software according to the terms and conditions of the Mulan PSL v2.
You may obtain a copy of Mulan PSL v2 at:
         http://license.coscl.org.cn/MulanPSL2
THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
See the Mulan PSL v2 for more details. */

#ifndef _LOGMSGBUF_H_
#define _LOGMSGBUF_H_
#include <vector>
#include <string>

using namespace std;

namespace oceanbase {
namespace logmessage {

struct BinLogBuf;
int LogMsgInit();
int LogMsgLocalInit();
void LogMsgDestroy();
void LogMsgLocalDestroy();
const char* LogMsgGetValueByOffset(size_t offset);
size_t LogMsgAppendString(const char* string, size_t size);
size_t LogMsgAppendString(const std::string& string);
size_t LogMsgAppendBuf(const char* data, size_t size);
size_t LogMsgAppendBuf(const std::string& string);
size_t LogMsgAppendBuf(const BinLogBuf* sa, size_t size);
size_t LogMsgAppendStringArray(std::vector<std::string*>& sa);
size_t LogMsgAppendStringArray(const char** sa, size_t size);
void LogMsgSetHead(size_t size);
void LogMsgCopyHead(const char* head, size_t size);
void LogMsgFroceSetHeadSize(size_t size);
const char* LogMsgGetString(size_t* size);
size_t LogMsgAppendDataArray(std::vector<long>& sa);
size_t LogMsgAppendDataArray(uint8_t* sa, size_t size);

}  // namespace logmessage
}  // namespace oceanbase

#endif /* _LOGMSGBUF_H_ */
