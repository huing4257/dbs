# REPORT

> 计16 胡赢

## 系统架构

​	可分为如下几个模块：系统管理，文件管理，文法解析，错误处理，输出处理，选择处理。

## 具体设计

### 数据类型

​	定义了如下的一些数据类型以方便使用：

```cpp
//record，具体转换在表中进行操作
using Value = std::variant<int, double, std::string>;
typedef std::vector<Value> Record;
// for index
typedef  std::vector<int> Key;
typedef std::pair<Key,int> IndexRecord;
```

### 系统管理 dbsystem

#### database

​	使用文件夹表示数据库，使用`Database`类表示数据库的元数据，其中存放若干个`table`类。使用一个全局变量记录当前存在的数据库，使用一个指针`current_db`来表示当前使用的数据库（存在问题，应当采用更好的方式判断当前数据库是否存在，如使用`optional`包裹一个智能指针）。实现 USE 时初始化数据表等方法。

#### table

​	使用`Table`类表示数据表，其实体为数据库文件夹中的文件，在数据库启动时读取文件初始化，在创建/更新时写回文件。其中，各个字符域如名字，key的名字等为非定长方式存储，在写文件时存入长度。

​	从数据表中获取数据时需要得知数据的 row id ，可以通过索引或者遍历得到。

​	每一数据表文件第一页记录元信息，按类中声明的顺序存放各个字段，不定长的字符串字段要先存入长度，不定个数的primary key，index等也要先存入个数。

​	每一数据页页头留出64B的空间用于保存相关信息，之后的数据按照数据长度依次放置，其中，数据长度在创建表时进行计算，将字符串4字节对齐。

#### index

​	使用`Index`类表示索引（B+树）的实例，具体与`Table`类似，实体为数据库文件夹中的文件，存储元信息并且实现与文件同步的方法。实现记录的插入、删除、查找等函数。节点抽象为`PageNode`，存储供B+树使用的表头，实现存取数据的函数。

### 文件管理 filesystem

​	使用文档提供的页式文件系统，主要使用创建打开文件，获取、写回page等功能。

### 文法解析 parser

​	使用 antlr 生成的文法解析器和其 runtime ，编写 `Visitor`类继承原visitor类并且实现相关操作。

### 工具 utils

#### 错误处理

​	实现自用的错误类，用于创建错误并且在main函数中捕获进行输出。

#### 输出模块

​	用于对所有输出内容进行统一格式化，除错误外其他输出均使用`Output`提供的接口，针对用户模式和批处理模式输出不同的格式。

#### 选择语句

​	实现`Where`类，用于对 where 子句进行解析，提供判断记录是否符合条件的接口。相关的处理类如下，计划实现多种，但时间有限未能完成。

```cpp
using Value = std::variant<int, double, std::string>;
typedef std::vector<Value> Record;
enum class SelectorType {
    OPERATOR_EXPRESSION,
    OPERATOR_SELECT,
    NULL_,
    IN_LIST,
    IN_SELECT,
    LIKE_STRING
};
class Where {
public:
    std::vector<std::string> column;
    virtual bool choose(Record record) = 0;
    virtual ~Where() = default;
};
class OperatorExpression : public Where {
public:
    std::string op;
    std::string value;
    bool choose(Record record) override;
    ~OperatorExpression() override = default;
};
```

### 查询

​	对第一个出现的与索引匹配的where子句进行优化，在B+树中寻找到相关范围的索引，返回给其他子句处理

	## 主要接口说明

### Table

```cpp
// Table
//将数据存入从指定缓冲区指针开始的区域
void record_to_buf(const std::vector<Value> &record, unsigned int *buf) const;
//将数据表元信息写入缓存
void write_file();
//读出数据表元信息
void read_file();
//构建数据表相关内容
bool construct();
//更新数据条数
void update() const;
//添加索引
void add_index(const std::string& index_name, 
               const std::vector<std::string>& keys, bool unique = false);
//删除索引
void drop_index(std::string index_name);
//将索引内容写入元信息文件
void update_index() const;
//已知row id，向其中插入数据
void insert_into_index(const std::vector<Value> &record, int record_id);
//为索引填充key位置
void fill_index_ki(Index &i);
//添加记录
bool add_record(const std::vector<Value> &record);
//字符串数组转为记录
[[nodiscard]] std::vector<Value> str_to_record(const std::vector<std::string> &line) const;
//记录转为字符串数组
[[nodiscard]] std::vector<std::string> record_to_str(const std::vector<Value> &record) const;
//多条记录转位多个字符串数组
[[nodiscard]] std::vector<std::vector<std::string>> records_to_str(const std::vector<std::optional<std::vector<Value>>> &record) const ;
//row id范围查找
[[nodiscard]] std::vector<std::optional<std::vector<Value>>> get_record_range(std::pair<int,int>) const;
//返回全部记录
[[nodiscard]] std::vector<std::optional<std::vector<Value>>> all_records() const{
    return get_record_range({0, record_num-1});
}
//更新记录
void update_record(int i, const std::vector<Value>& record);
//写入一整页，从而高效利用缓存
void write_whole_page(std::vector<std::vector<Value>> &data);
//删除记录（将有效位置为0）
void delete_record(int i);
};
```

### Database

```cpp
//初始化相关数据表
void use_database();
//关闭数据表
void close_database();
//删除数据库的预处理
void drop_table(const std::string &table_name);
//查找数据表
[[nodiscard]] int get_table_index(const std::string &table_name) const;
//创建并打开数据表
void create_open_table(Table table);
//查找列索引（一项时为列名，两项时为表名+列名）
[[nodiscard]] int get_column_index(const std::vector<std::string> &column) const
```

### Index

```cpp
//Index
//与文件同步
void write_file() const;
void read_file();
//使用pageid创建pagenode实例
PageNode page_to_node(unsigned int page_id);
//存入记录
void records_to_buf(unsigned int* buf, const std::vector<IndexRecord>& record) const;
//查找某个key所在的表
int search_page_node(const Key& key);
//寻找记录
std::optional<int> search_record(const Key& key);
//增删记录
void insert(const Key &key, int record_id);
void remove(const Key& key);
```

```cpp
//PageNode
//均为B+树相关操作
void update() const;
void insert(const Key& key, int id);
void split();
//方便修改
void pop_front();
void pop_back();
void push_back(IndexRecord record);
void push_front(IndexRecord record);
[[nodiscard]] std::vector<IndexRecord> to_vec() const;
// only use when is_leaf == true
[[nodiscard]] std::optional<int> search(const Key &key, bool unique) const;
bool borrow();
// only use when is_leaf == true
void remove(const Key& key);
void merge();
void update_record(int i, const Key& key) const;
int get_index_in_parent();
```

## 实验结果

![Screenshot 2024-01-20 at 18.08.36](/Users/huing/Library/Application Support/typora-user-images/Screenshot 2024-01-20 at 18.08.36.png)

实现了基础的数据库，通过了42分的测例，由于内存访问出错的原因，在大量载入数据后table元信息遭到破坏，故无法通过index-data。

## 小组分工

+ 单人组队，无分工。

## 参考文献

+ 文档所提供的页式文件系统
+ antlr代码生成与runtime
  + https://www.antlr.org/download.html
+ 一些与B/B+树相关的博客
  + https://ivanzz1001.github.io/records/post/data-structure/2018/06/16/ds-bplustree
  + https://segmentfault.com/a/1190000038749020
  + https://oi-wiki.org/ds/bplus-tree/