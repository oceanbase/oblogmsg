oblogmsg 是一种数据库增量数据的输出格式,oceanbase 的增量采集模块 liboblog 正是使用的这种消息格式来输出增量数据,oblogmsg 支持 oceanbase 中不同数据类型的增量数据的写入，具有序列化和反序列化的能力。
## 如何使用 oblogmsg

> 前置条件
>
> * cmake: >=3.20.0
> * g++: 支持 C++11 标准



### 以源码方式依赖（推荐方式）

* 可以使用 `git submodule` 的方式将 oblogmsg 作为主项目的子模块
* 在主项目的  CMakeLists.txt 文件中使用 `add_subdirectory(submodule_path_to_oblogmsg)` 来依赖 oblogmsg，该命令执行后将提供 `oceanbase::oblogmsg_shared` 和 `oceanbase::oblogmsg_static` 两个 targets
* 在主项目相关  CMakeLists.txt 文件中，对于要依赖 oblogmsg 的 target t1，使用 `target_link_libraries(t1 PRIVATE oceanbase::oblogmsg_shared)`或者 `target_link_libraries(t1 PRIVATE oceanbase::oblogmsg_static)`即可

### 依赖编译后的 oblogmsg 库

* 编译、安装 oblogmsg

```shell
# 编译 oblogmsg
git clone https://github.com/oceanbase/oblogmsg.git
mkdir oblogmsg-build
cd oblogmsg-build
cmake -S ../oblogmsg -B .
cmake --build .

# 本地安装
cmake --install . --prefix=${OBLOGMSG_INSTALL_PATH}
```

* 主项目中依赖编译、安装后的 oblogmsg
  * 在主项目  CMakeLists.txt 文件中使用 `set(CMAKE_PREFIX_PATH $ENV{OBLOGMSG_INSTALL_PATH} ${CMAKE_PREFIX_PATH})` 设置 oblogmsg 库搜索路径
  * 然后使用 `find_package(oblogmsg REQUIRED)`使加载 oblogmsg，该命令执行成功后将提供 `oceanbase::oblogmsg_shared` 和 `oceanbase::oblogmsg_static` 两个 targets
  * 在主项目相关  CMakeLists.txt 文件中，对于要依赖 oblogmsg 的 target t1，使用 `target_link_libraries(t1 PRIVATE oceanbase::oblogmsg_shared)`或者 `target_link_libraries(t1 PRIVATE oceanbase::oblogmsg_static)`即可

## oblogmsg 部分接口说明
### 创建一个 record

     1. void ILogRecord::setSrcType(int type)
        功能描述:
            设置数据源的类型
        参数：
            type  有效值有0x00,0x01,0x02,0x03,0x04,0x05,对应数据源分别是 MYSQL,OCEANBASE,HBASE,ORACLE,OCEANBASE_1_0,DB2,liboblog 设置的值是0x04,即数据源是 OCEANBASE_1_0 
        返回值：
            无返回值

     2. void ILogRecord::setCheckpoint(uint64_t file, uint64_t offset)
        功能描述:
            设置当前的分析位点信息
        参数：
            file 一个秒级的 unix 时间戳
            offset file 参数中 unix 时间戳的微秒级
        返回值：
            无返回值

     3. int ILogRecord::setRecordType(int aType)
        功能描述:
            设置一条 record 的操作类型
        参数：
            aType 一般有七种类型 插入操作0x00 (EINSERT),更新操作0x01 (EUPDATE),删除操作0x02 (EDELETE),心跳包0x04 (HEARTBEAT),事物开始0x06 (EBEGIN),事物提交0x07 (ECOMMIT), DDL 操作0x08 (EDDL), 强制更新操作 0x0c (EPUT)
        返回值：
            固定返回0

     4. void ILogRecord::setDbname(const char *dbname)
        功能描述:
            设置一条 record 来源的 db 名字
        参数：
            dbname record 的来源的数据库名字
        返回值：
            无返回值

     5. void ILogRecord::setTbname(const char *tbname)
        功能描述:
            设置一条 record 的 table 名字
        参数：
            dbname record 的表名
        返回值：
            无返回值

     6. void ILogRecord::setTableMeta(ITableMeta* tblMeta)
        功能描述:
            设置一个 record 的元数据信息
        参数：
            tblMeta record 的元数据信息，内存需由调用方申请和释放
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
            uks 包含有 uk 字段的名字的字符串，格式为 (col1,col2,col3...)，当有多个 uk 时，例如 (col1,col2),(col2,col3),需要合并成一个字符串 (col1,col2,col3) 传入
        返回值：
            无返回值

     9. void ITableMeta::setPKs(const char* pks)
        功能描述:
            设置元数据中的 PK 字段名字
        参数：
            pks 包含 pk 字段的名字的一个字符串，格式为 (col1,col2,col3...)
        返回值：
            无返回值

    10. void ITableMeta::setPkinfo(const char* info)
        功能描述:
           设置元数据中的 PK 字段 id
        参数：
            info 包含有 pk 字段 id 的一个字符串，格式为(0,1,2,3...),从0开始
        返回值：
            无返回值

    11. void ITableMeta::setUkinfo(const char* info)
        功能描述:
            设置元数据中的 UK 字段 id
        参数： 
            info 包含有 uk 字段 id 的一个字符串，格式为(0,1,2,3...),当有多个uk时，传入格式为（0,1),(1,2),(0,3)...
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
            type 列的类型，可设置的值详见 include/MetaInfo.h 中的枚举类型 logmsg_field_types
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
            设置该列是否时 pk
        参数：
            b  设置为 true 时，该列为 uk，false 时，该列不是 uk
        返回值：
            无返回值

    17. void IColMeta::setIsUK(bool b)
        功能描述:
            设置该列是否时uk
        参数：
            b  设置为 true 时，该列为 uk，false 时，该列不是 uk
        返回值：
            无返回值

    18. void IColMeta::setNotNull(bool b)
        功能描述:
            设置该列是否有非空约束
        参数：
            b  设置为 true 时，该列不能为空，false 时，该列可以为空
        返回值：
            无返回值

    19. void ILogRecord::setOldColumn(BinLogBuf* buf, int size)
        功能描述：
            初始化一片空间用来存放前镜像的字段值，空间大小为 size 个 BinLogBuf 的大小，size 必须比大于等于表的字段数目，该空间由调用方申请和释放
        参数：
            buf 空间的首地址
            size 空间中 BinLogBuf 的个数，一般为表的字段数目
        返回值：
             无返回值

    20. void ILogRecord::setNewColumn(BinLogBuf* buf, int size)
        功能描述：
            初始化一片空间用来存放后镜像的字段值，空间大小为 size 个 BinLogBuf 的大小，size 必须比大于等于表的字段数目，该空间由调用方申请和释放
        参数：
            buf 空间的首地址
            size 空间中 BinLogBuf 的个数，一般为表的字段数目
        返回值：
             无返回值

    21. int ILogRecord::putOld(const char* pos, int len)
        功能描述:
            添加一个前镜像中字段的值到 record 中
        参数：
            pos 字段值的起始地址
            len 字段值的长度
        返回值：
            固定返回0
    22. int ILogRecord::putNew(const char* pos, int len)
        功能描述:
            添加一个后镜像中字段的值到 record 中
        参数：
            pos 字段值的起始地址
            len 字段值的长度
        返回值：
            固定返回0

    23. const char* ILogRecord::toString(size_t *size, bool reserveMemory)
        功能描述:
            序列化一个record
        参数:
            size 一个 size_t 类型的地址，序列化完成后，会将序列化后数据的长度写入该地址
            reserveMemory 是否保留序列化之后的数据到 record 的数据区，若不保留，改 record 在序列化之后，里面所存储的数据会被清空，将不可用
        返回值：
            NULL 序列化失败
            非空指针 序列化之后的数据首地址

    24. ILogRecord::bool parsedOK()
        功能描述:
            获取一个 record 是否已经完成序列化
        参数:
            空
        返回值：
            true 此 record 已经完成序列化，或者此 record 是由一段数据反序列化后创建的
            false 此 record 还未进行序列化

### 反序列化一个 record,或者从一个已经序列化过的 record 中( ILogRecord::parsedOK() 返回 true )解析数据

     1. int ILogRecord::parse(const void* ptr, size_t size)
        功能描述:
            反序列化一个 record
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
            返回一个int型数值，有效值有0x00,0x01,0x02,0x03,0x04,0x05,对应数据源分别是 MYSQL,OCEANBASE,HBASE,ORACLE,OCEANBASE_1_0,DB2,liboblog 设置的值是0x04,即数据源是 OCEANBASE_1_0

     3. uint64_t ILogRecord::getCheckpoint1()/uint64_t ILogRecord::getCheckpoint2()
        功能描述:
            获取分析的位点信息
        参数:
            空
        返回值:
            getCheckpoint1:返回一个秒级的 unix 时间戳
            getCheckpoint2:返回 unix 时间戳的微秒级，可利用 getCheckpoint1*1000000+getCheckpoint2 做为分析的断点位置

     4. int ILogRecord::recordType()
        功能描述:
            获取一条 record 的操作类型
        参数：
            空
        返回值：
            返回一个int类型数值，一般有七种类型，插入操作0x00 (EINSERT),更新操作0x01 (EUPDATE),删除操作0x02 (EDELETE),心跳包0x04 (HEARTBEAT),事物开始0x06 (EBEGIN),事物提交0x07 (ECOMMIT),DDL 操作0x08 (EDDL)

     5. const char* ILogRecord::dbname() const
        功能描述:
            获取一条 record 来源的 db 名字
        参数：
            空
        返回值：
            返回一个char*类型的指针，该指针指向 record 的来源的数据库名字

     6. const char* ILogRecord::tbname() const
        功能描述:
            获取一条 record 的 table 名字
        参数：
            空
        返回值：
            返回一个 char* 类型的指针，该指针指向 record 的来源的表名字

     7. int ILogRecord::getTableMeta(ITableMeta*& tblMeta)
        功能描述:
            从一个 record 获取表的元数据信息,从一个序列化过的 record 取值时，由调用方为 tblMeta 申请和释放内存
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
            返回一个 char* 类型的指针，该指针指向 record 的元数据中的表名字

     9. const char* ITableMeta::getUKs()
        功能描述:
            获取元数据中的 UK 字段名字
        参数：
            空
        返回值：
            返回一个 char* 指针，指向包含有 uk 字段的名字的字符串，格式为 (col1,col2,col3...)

    10. const char* ITableMeta::getPKs()
        功能描述:
            获取元数据中的 PK 字段名字
        参数：
            空
        返回值：
            返回一个 char* 指针，指向包含 pk 字段的名字的一个字符串，格式为 (col1,col2,col3...)

    11. const char* ITableMeta::getPkinfo()
        功能描述:
            获取元数据中的 PK 字段 id，从0开始
        参数：
           空
        返回值：
            返回一个 char* 指针，指向包含有 pk 字段 id 的一个字符串，格式为 (0,1,2,3...)

    12. const char* ITableMeta::getUkinfo()
        功能描述:
            设置元数据中的 PK 字段 id,从0开始
        参数：
            空
        返回值：
            返回一个 char* 指针，指向包含有 uk 字段 id 的一个字符串，格式为(0,1,2,3...)

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
            返回一个 char* 指针，指向元数据的列名

    15. int IColMeta::getType()
        功能描述:
            获取列的类型
        参数：
            空
        返回值：
            返回一个 int 型数值，详见 include/MetaInfo.h 中的枚举类型 logmsg_field_types

    16. long IColMeta::getLength()
        功能描述:
            获取列的字节长度
        参数：
            空
        返回值：
            列的字节长度

    17. bool IColMeta::isPK()
        功能描述:
            获取该列是否时 pk
        参数：
            空
        返回值：
            返回一个 bool 类型，为 true 时，该列为 uk，false 时，该列不是 uk

    18. bool IColMeta::isUK()
        功能描述:
            获取该列是否时 uk
        参数：
            空
        返回值：
            返回一个 bool 类型，为 true 时，该列为 uk，false 时，该列不是 uk

    19. bool IColMeta::isNotNull()
        功能描述:
            获取该列是否有非空约束
        参数：
            空
        返回值：
            返回一个 bool 类型，为 true 时，该列有非空约束，false 时，该列可以为空

    20. StrArray* ILogRecord::parsedOldCols() const
        功能描述：
            获取 record 中前镜像的所有字段值
        参数：
            空
        返回值：
            返回一个 StrArray 类型的指针，该指针指向存着前镜像的值，可通过 StrArray->size() 接口获取数据的列数，StrArray->elementAt(int i, const char*& s, size_t& length) 接口获取每列的数据

    21. StrArray* ILogRecord::parsedNewCols() const
        功能描述：
            获取 record 中后镜像的所有字段值
        参数：
            空
        返回值：
            返回一个 StrArray 类型的指针，该指针指向存着后镜像的值，可通过 StrArray->size() 接口获取数据的列数，StrArray->elementAt(int i, const char*& s, size_t& length) 接口获取每列的数据

### 从一个未序列化过的 record 中( ILogRecord::parsedOK() 返回 false )解析前后镜像字段的数据

     1. int ILogRecord::getTableMeta(ITableMeta*& tblMeta)
        功能描述:
            从一个 record 获取表的元数据信息,从一个未序列化过的 record 取值时，tblMeta 必须为一个空指针
        参数:
            tblMeta 需为一个空指针，该接口会将此指针指向 record 的元数据地址，内存由 oblogmsg 管理，调用方不需要释放
        返回值:
            0 成功
            其他值 失败

     2. BinLogBuf* ILogRecord::newCols(unsigned int& count)
        功能描述：
            获取 record 中后镜像的所有字段值
        参数：
            count 用来返回数据的列数
        返回值：
            返回一个 BinLogBuf 类型的指针，该指针指向一个拥有 count 个 BinLogBuf 的空间，每个 BinLogBuf.buf 成员即是字段的值，BinLogBuf.buf_used_size 为字段值的长度

     3. BinLogBuf* ILogRecord::oldCols(unsigned int& count)
        功能描述：
            获取 record 中后镜像的所有字段值
        参数：
            count 用来返回数据的列数
        返回值：
            返回一个 BinLogBuf 类型的指针，该指针指向一个拥有 count 个 BinLogBuf 的空间，每个 BinLogBuf.buf 成员即是字段的值，BinLogBuf.buf_used_size 为字段值的长度
