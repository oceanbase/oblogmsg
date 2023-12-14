#ifndef VALUE_ORIGIN_H_
#define VALUE_ORIGIN_H_
enum VALUE_ORIGIN {
  REDO = 0x00,               // 从日志中解析出的字段值
  BACK_QUERY = 0x01,         // 通过回查得到的字段值
  PADDING = 0x02             // 填充值，通常认为该值是不可信的
};
#endif /* VALUE_ORIGIN_H_ */
#ifndef VALUE_ORIGIN_H_
#define VALUE_ORIGIN_H_
enum VALUE_ORIGIN {
  REDO = 0x00,               // 从日志中解析出的字段值
  BACK_QUERY = 0x01,         // 通过回查得到的字段值
  PADDING = 0x02             // 填充值，通常认为该值是不可信的
};
#endif /* VALUE_ORIGIN_H_ */