# LogMessage
  LogMessage是一种数据库增量数据的输出格式,oceanbase的增量采集模块liboblog正是使用的这种消息格式来输出增量数据,LogMessage支持oceanbase中不同数据类型的增量数据的写入，具有序列化和反序列化的能力。
# 如何编译
  * LogMessage的编译依赖
      1. g++(推荐版本g++-5.2.0)
      2. cmake(推荐版本3.20)
  * 执行cmake . && make,编译完成后，src目录下的liboblogmsg.so和liboblogmsg.a即为编译产物,使用时需要包含头文件目录include

# LogMessage部分接口说明
  * 创建一个record

     1. void ILogRecord::setSrcType(int type)
        功能描述:
            设置数据源的类型
        参数：
            type  有效值有0x00,0x01,0x02,0x03,0x04,0x05,对应数据源分别是MYSQL,OCEANBASE,HBASE,ORACLE,OCEANBASE_1_0,DB2,liboblog设置的值是0x04,即数据源是OCEANBASE_1_0
        返回值：
            无返回值

     2. void ILogRecord::setCheckpoint(uint64_t file, uint64_t offset)
        功能描述:
            设置当前的分析位点信息
        参数：
            file 一个秒级的unix时间戳
            offset file参数中unix时间戳的微秒级
        返回值：
            无返回值

     3. int ILogRecord::setRecordType(int aType)
        功能描述:
            设置一条record的操作类型
        参数：
            aType 一般有七种类型 插入操作0x00(EINSERT),更新操作0x01(EUPDATE),删除操作0x02(EDELETE),心跳包0x04(HEARTBEAT),事物开始0x06(EBEGIN),事物提交0x07(ECOMMIT),DDL操作0x08(EDDL)
        返回值：
            固定返回0

     4. void ILogRecord::setDbname(const char *dbname)
        功能描述:
            设置一条record来源的db名字
        参数：
            dbname record的来源的数据库名字
        返回值：
            无返回值

     5. void ILogRecord::setTbname(const char *tbname)
        功能描述:
            设置一条record的table名字
        参数：
            dbname record的表名
        返回值：
            无返回值

     6. void ILogRecord::setTableMeta(ITableMeta* tblMeta)
        功能描述:
            设置一个record的元数据信息
        参数：
            tblMeta record的元数据信息，内存需由调用方申请和释放
        返回值：
            无返回值

     7. void ITableMeta::setName(const char* name)
        功能描述:
            设置元数据中的表名字
        参数：
            name 表的名字
        返回值：
            无返回值

     8. void ITableMeta::setUKs(const char* uks)
        功能描述:
            设置元数据中的UK字段名字
        参数：
            uks 包含有uk字段的名字的字符串，格式为(col1,col2,col3...)，当有多个uk时，例如(col1,col2),(col2,col3),需要合并成一个字符串(col1,col2,col3)传入
        返回值：
            无返回值

     9. void ITableMeta::setPKs(const char* pks)
        功能描述:
            设置元数据中的PK字段名字
        参数：
            pks 包含pk字段的名字的一个字符串，格式为(col1,col2,col3...)
        返回值：
            无返回值

    10. void ITableMeta::setPkinfo(const char* info)
        功能描述:
           设置元数据中的PK字段id
        参数：
            info 包含有pk字段id的一个字符串，格式为(0,1,2,3...),从0开始
        返回值：
            无返回值

    11. void ITableMeta::setUkinfo(const char* info)
        功能描述:
            设置元数据中的UK字段id
        参数：
            info 包含有uk字段id的一个字符串，格式为(0,1,2,3...),当有多个uk时，传入格式为（0,1),(1,2),(0,3)...
        返回值：
            无返回值

    12. int ITableMeta::append(const char* colName, IColMeta* colMeta);
        功能描述:
            向一个表的元数据信息添加一个列的元数据信息
        参数：
            colName 列的名字
            colMeta 列的元数据信息
        返回值：
            -1 此列名在表中已存在时，添加会失败，返回-1
            0 添加成功

    13. void IColMeta::setName(const char* name)
        功能描述:
            为一个列的元数据信息设置列名
        参数：
            name 此列的名字
        返回值：
            无返回值

    14. void IColMeta::setType(int type)
        功能描述:
            设置列的类型
        参数：
            type 列的类型，可设置的值详见include/MetaInfo.h中的枚举类型logmsg_field_types
        返回值：
            无返回值

    15. void IColMeta::setLength(long length)
        功能描述:
            设置列的字节长度
        参数：
            length 该列的字节长度
        返回值：
            无返回值

    16. void IColMeta::setIsPK(bool b)
        功能描述:
            设置该列是否时pk
        参数：
            b  设置为true时，该列为uk，false时，该列不是uk
        返回值：
            无返回值

    17. void IColMeta::setIsUK(bool b)
        功能描述:
            设置该列是否时uk
        参数：
            b  设置为true时，该列为uk，false时，该列不是uk
        返回值：
            无返回值

    18. void IColMeta::setNotNull(bool b)
        功能描述:
            设置该列是否有非空约束
        参数：
            b  设置为true时，该列不能为空，false时，该列可以为空
        返回值：
            无返回值

    19. void ILogRecord::setOldColumn(BinLogBuf* buf, int size)
        功能描述：
            初始化一片空间用来存放前镜像的字段值，空间大小为size个BinLogBuf的大小，size必须比大于等于表的字段数目，该空间由调用方申请和释放
        参数：
            buf 空间的首地址
            size 空间中BinLogBuf的个数，一般为表的字段数目
        返回值：
             无返回值

    20. void ILogRecord::setNewColumn(BinLogBuf* buf, int size)
        功能描述：
            初始化一片空间用来存放后镜像的字段值，空间大小为size个BinLogBuf的大小，size必须比大于等于表的字段数目，该空间由调用方申请和释放
        参数：
            buf 空间的首地址
            size 空间中BinLogBuf的个数，一般为表的字段数目
        返回值：
             无返回值

    21. int ILogRecord::putOld(const char* pos, int len)
        功能描述:
            添加一个前镜像中字段的值到record中
        参数：
            pos 字段值的起始地址
            len 字段值的长度
        返回值：
            固定返回0
    22. int ILogRecord::putNew(const char* pos, int len)
        功能描述:
            添加一个后镜像中字段的值到record中
        参数：
            pos 字段值的起始地址
            len 字段值的长度
        返回值：
            固定返回0

    23. const char* ILogRecord::toString(size_t *size, bool reserveMemory)
        功能描述:
            序列化一个record
        参数:
            size 一个size_t类型的地址，序列化完成后，会将序列化后数据的长度写入该地址
            reserveMemory 是否保留序列化之后的数据到record的数据区，若不保留，改record在序列化之后，里面所存储的数据会被清空，将不可用
        返回值：
            NULL 序列化失败
            非空指针 序列化之后的数据首地址

    24. ILogRecord::bool parsedOK()
        功能描述:
            获取一个record是否已经完成序列化
        参数:
            空
        返回值：
            true 此record已经完成序列化，或者此record是由一段数据反序列化后创建的
            false 此record还未进行序列化

  * 反序列化一个record,或者从一个已经序列化过的record中(ILogRecord::parsedOK()返回true)解析数据

     1. int ILogRecord::parse(const void* ptr, size_t size)
        功能描述:
            反序列化一个record
        参数:
            ptr  保存着序列化数据的首地址
            size 序列化数据的长度
        返回值：
            0 成功
            其他值 失败

     2. int ILogRecord::getSrcType()
        功能描述:
            获取数据源的类型
        参数：
            空
        返回值：
            返回一个int型数值，有效值有0x00,0x01,0x02,0x03,0x04,0x05,对应数据源分别是MYSQL,OCEANBASE,HBASE,ORACLE,OCEANBASE_1_0,DB2,liboblog设置的值是0x04,即数据源是OCEANBASE_1_0

     3. uint64_t ILogRecord::getCheckpoint1()/uint64_t ILogRecord::getCheckpoint2()
        功能描述:
            获取分析的位点信息
        参数:
            空
        返回值:
            getCheckpoint1:返回一个秒级的unix时间戳
            getCheckpoint2:返回unix时间戳的微秒级，可利用 getCheckpoint1*1000000+getCheckpoint2做为分析的断点位置

     4. int ILogRecord::recordType()
        功能描述:
            获取一条record的操作类型
        参数：
            空
        返回值：
            返回一个int类型数值，一般有七种类型，插入操作0x00(EINSERT),更新操作0x01(EUPDATE),删除操作0x02(EDELETE),心跳包0x04(HEARTBEAT),事物开始0x06(EBEGIN),事物提交0x07(ECOMMIT),DDL操作0x08(EDDL)

     5. const char* ILogRecord::dbname() const
        功能描述:
            获取一条record来源的db名字
        参数：
            空
        返回值：
            返回一个char*类型的指针，该指针指向record的来源的数据库名字

     6. const char* ILogRecord::tbname() const
        功能描述:
            获取一条record的table名字
        参数：
            空
        返回值：
            返回一个char*类型的指针，该指针指向record的来源的表名字

     7. int ILogRecord::getTableMeta(ITableMeta*& tblMeta)
        功能描述:
            从一个record获取表的元数据信息,从一个序列化过的record取值时，由调用方为tblMeta申请和释放内存
        参数:
            tblMeta 表元数据信息的首地址
        返回值:
            0 成功
            其他值 失败

     8. const char* ITableMeta::getName()
        功能描述:
            获取元数据中的表名字
        参数：
            name 表的名字
        返回值：
            返回一个char*类型的指针，该指针指向record的元数据中的表名字

     9. const char* ITableMeta::getUKs()
        功能描述:
            获取元数据中的UK字段名字
        参数：
            空
        返回值：
            返回一个char*指针，指向包含有uk字段的名字的字符串，格式为(col1,col2,col3...)

    10. const char* ITableMeta::getPKs()
        功能描述:
            获取元数据中的PK字段名字
        参数：
            空
        返回值：
            返回一个char*指针，指向包含pk字段的名字的一个字符串，格式为(col1,col2,col3...)

    11. const char* ITableMeta::getPkinfo()
        功能描述:
            获取元数据中的PK字段id，从0开始
        参数：
           空
        返回值：
            返回一个char*指针，指向包含有pk字段id的一个字符串，格式为(0,1,2,3...)

    12. const char* ITableMeta::getUkinfo()
        功能描述:
            设置元数据中的PK字段id,从0开始
        参数：
            空
        返回值：
            返回一个char*指针，指向包含有uk字段id的一个字符串，格式为(0,1,2,3...)

    13. int ITableMeta::getColCount()
        功能描述:
            获取元数据中的字段数量
        参数：
            空
        返回值：
            返回字段的数量

    14. const char* IColMeta::getName()
        功能描述:
            获取一个列的元数据中的列名
        参数：
            空
        返回值：
            返回一个char*指针，指向元数据的列名

    15. int IColMeta::getType()
        功能描述:
            获取列的类型
        参数：
            空
        返回值：
            返回一个int型数值，详见include/MetaInfo.h中的枚举类型logmsg_field_types

    16. long IColMeta::getLength()
        功能描述:
            获取列的字节长度
        参数：
            空
        返回值：
            列的字节长度

    17. bool IColMeta::isPK()
        功能描述:
            获取该列是否时pk
        参数：
            空
        返回值：
            返回一个bool类型，为true时，该列为uk，false时，该列不是uk

    18. bool IColMeta::isUK()
        功能描述:
            获取该列是否时uk
        参数：
            空
        返回值：
            返回一个bool类型，为true时，该列为uk，false时，该列不是uk

    19. bool IColMeta::isNotNull()
        功能描述:
            获取该列是否有非空约束
        参数：
            空
        返回值：
            返回一个bool类型，为true时，该列有非空约束，false时，该列可以为空

    20. StrArray* ILogRecord::parsedOldCols() const
        功能描述：
            获取record中前镜像的所有字段值
        参数：
            空
        返回值：
            返回一个StrArray类型的指针，该指针指向存着前镜像的值，可通过StrArray->size()接口获取数据的列数，StrArray->elementAt(int i, const char*& s, size_t& length)接口获取每列的数据

    21. StrArray* ILogRecord::parsedNewCols() const
        功能描述：
            获取record中后镜像的所有字段值
        参数：
            空
        返回值：
            返回一个StrArray类型的指针，该指针指向存着后镜像的值，可通过StrArray->size()接口获取数据的列数，StrArray->elementAt(int i, const char*& s, size_t& length)接口获取每列的数据

  * 从一个未序列化过的record中(ILogRecord::parsedOK()返回false)解析前后镜像字段的数据

     1. int ILogRecord::getTableMeta(ITableMeta*& tblMeta)
        功能描述:
            从一个record获取表的元数据信息,从一个未序列化过的record取值时，tblMeta必须为一个空指针
        参数:
            tblMeta 需为一个空指针，该接口会将此指针指向record的元数据地址，内存由logmessage管理，调用方不需要释放
        返回值:
            0 成功
            其他值 失败

     2. BinLogBuf* ILogRecord::newCols(unsigned int& count)
        功能描述：
            获取record中后镜像的所有字段值
        参数：
            count 用来返回数据的列数
        返回值：
            返回一个BinLogBuf类型的指针，该指针指向一个拥有count个BinLogBuf的空间，每个BinLogBuf.buf成员即是字段的值，BinLogBuf.buf_used_size为字段值的长度

     3. BinLogBuf* ILogRecord::oldCols(unsigned int& count)
        功能描述：
            获取record中后镜像的所有字段值
        参数：
            count 用来返回数据的列数
        返回值：
            返回一个BinLogBuf类型的指针，该指针指向一个拥有count个BinLogBuf的空间，每个BinLogBuf.buf成员即是字段的值，BinLogBuf.buf_used_size为字段值的长度
