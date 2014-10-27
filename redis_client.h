#ifndef  __REDIS_CLIENT_H_
#define  __REDIS_CLIENT_H_

#include <string>
#include <vector>
#include "hiredis.h"
using namespace std;
//ip 和端口、权重
struct ip_port_pair
{
    string ip;
    int port;
    int weight;
};

//将字符串形式的服务端地址端口权重转换成 struct ip_port_pair结构并放到 vector中
int string2svrlist(const char *svrs,vector<ip_port_pair> &svr_list);

//redis客户端
class redis_client{
    public:
        redis_client();
        redis_client(string svrlist);
        redis_client(string ip,int port,int weight);
        redis_client(const char *ip,int port,int weight);
        ~redis_client();

        void set_db(string db);//设置数据库名
        void set_svr_list(string svrlist);//设置服务端列表
        void set_connect_timeout(int);//设置连接超时时间
        void set_op_timeout(int);//设置操作超时时间
        void add_svr_list(string svrlist);//增加服务端列表
        void add_svr(string ip,int port,int weight);//增加单个服务端
        void add_svr(const char *ip,int port,int weight);//增加单个服务端

        int connect();//redis客户端连接函数，从服务端列表中选取一个权重最高的服务端去连接
        int use_db();//使用数据库
        int close();//关闭客户端
        int is_connected();//判断客户端是不是在连接状态
        int get_cur_svr_connted(struct ip_port_pair& pair);//获取当前连接的服务端地址端口和权重信息
        int reconnect();//重新连接客户端
        redisReply* do_command(const char* cmd);//执行redis命令
    private:
        vector<ip_port_pair>    m_svrlist;//服务端列表
    protected:
        struct timeval m_connect_timeout;//连接超时时间
        struct timeval m_op_timeout;//操作超时时间
        string         m_sdb_name;//数据库名称
        redisContext*  m_predis_contx;//连接库时返回的结构，包含所有连接状态信息
        struct ip_port_pair m_current_cnnt_server;
};

/*

redis 数据库操作命令解释：

 
连接控制


QUIT 关闭连接

AUTH (仅限启用时)简单的密码验证

适合全体类型的命令

EXISTS key 判断一个键是否存在;存在返回 1;否则返回0;

DEL key 删除某个key,或是一系列key;DEL key1 key2 key3 key4

TYPE key 返回某个key元素的数据类型 ( none:不存在,string:字符,list,set,zset,hash)

KEYS pattern 返回匹配的key列表 (KEYS foo*:查找foo开头的keys),KEYS * 就列出所有的key了，当然，复杂度O(n)

RANDOMKEY 随机获得一个已经存在的key，如果当前数据库为空，则返回空字符串

RENAME oldname newname更改key的名字，新键如果存在将被覆盖

RENAMENX oldname newname 更改key的名字，如果名字存在则更改失败

DBSIZE返回当前数据库的key的总数

EXPIRE设置某个key的过期时间（秒）,(EXPIRE bruce 1000：设置bruce这个key1000秒后系统自动删除)注意：如果在还没有过期的时候，对值进行了改变，那么那个值会被清除。

TTL查找某个key还有多长时间过期,返回时间秒

SELECT index 选择数据库

MOVE key dbindex 将指定键从当前数据库移到目标数据库 dbindex。成功返回 1;否则返回0（源数据库不存在key或目标数据库已存在同名key）;

FLUSHDB 清空当前数据库中的所有键

FLUSHALL 清空所有数据库中的所有键

处理字符串的命令

SET key value 给一个键设置字符串值。SET keyname datalength data (SET bruce 10 paitoubing:保存key为burce,字符串长度为10的一个字符串paitoubing到数据库)，data最大不可超过1G。

GET key获取某个key 的value值。如key不存在，则返回字符串“nil”；如key的值不为字符串类型，则返回一个错误。

GETSET key value可以理解成获得的key的值然后SET这个值，更加方便的操作 (SET bruce 10 paitoubing,这个时候需要修改bruce变成1234567890并获取这个以前的数据paitoubing,GETSET bruce 10 1234567890)

MGET key1 key2 … keyN 一次性返回多个键的值

SETNX key value SETNX与SET的区别是SET可以创建与更新key的value，而SETNX是如果key不存在，则创建key与value数据

MSET key1 value1 key2 value2 … keyN valueN 在一次原子操作下一次性设置多个键和值

MSETNX key1 value1 key2 value2 … keyN valueN 在一次原子操作下一次性设置多个键和值（目标键不存在情况下，如果有一个以上的key已存在，则失败）

INCR key 自增键值

INCRBY key integer 令键值自增指定数值

DECR key 自减键值

DECRBY key integer 令键值自减指定数值

处理 lists 的命令

RPUSH key value 从 List 尾部添加一个元素（如序列不存在，则先创建，如已存在同名Key而非序列，则返回错误）

LPUSH key value 从 List 头部添加一个元素

LLEN key 返回一个 List 的长度

LRANGE key start end从自定的范围内返回序列的元素 (LRANGE testlist 0 2;返回序列testlist前0 1 2元素)

LTRIM key start end修剪某个范围之外的数据 (LTRIM testlist 0 2;保留0 1 2元素，其余的删除)

LINDEX key index返回某个位置的序列值(LINDEX testlist 0;返回序列testlist位置为0的元素)

LSET key index value更新某个位置元素的值

LREM key count value 从 List 的头部（count正数）或尾部（count负数）删除一定数量（count）匹配value的元素，返回删除的元素数量。

LPOP key 弹出 List 的第一个元素

RPOP key 弹出 List 的最后一个元素

RPOPLPUSH srckey dstkey 弹出 _srckey_ 中最后一个元素并将其压入 _dstkey_头部，key不存在或序列为空则返回“nil”

处理集合(sets)的命令（有索引无序序列）

SADD key member增加元素到SETS序列,如果元素（membe）不存在则添加成功 1，否则失败 0;(SADD testlist 3 \n one)

SREM key member 删除SETS序列的某个元素，如果元素不存在则失败0，否则成功 1(SREM testlist 3 \N one)

SPOP key 从集合中随机弹出一个成员

SMOVE srckey dstkey member 把一个SETS序列的某个元素 移动到 另外一个SETS序列 (SMOVE testlist test 3\n two;从序列testlist移动元素two到 test中，testlist中将不存在two元素)

SCARD key 统计某个SETS的序列的元素数量

SISMEMBER key member 获知指定成员是否存在于集合中

SINTER key1 key2 … keyN 返回 key1, key2, …, keyN 中的交集

SINTERSTORE dstkey key1 key2 … keyN 将 key1, key2, …, keyN 中的交集存入 dstkey

SUNION key1 key2 … keyN 返回 key1, key2, …, keyN 的并集

SUNIONSTORE dstkey key1 key2 … keyN 将 key1, key2, …, keyN 的并集存入 dstkey

SDIFF key1 key2 … keyN 依据 key2, …, keyN 求 key1 的差集。官方例子：

key1 = x,a,b,c

key2 = c

key3 = a,d

SDIFF key1,key2,key3 => x,b

SDIFFSTORE dstkey key1 key2 … keyN 依据 key2, …, keyN 求 key1 的差集并存入 dstkey

SMEMBERS key 返回某个序列的所有元素

SRANDMEMBER key 随机返回某个序列的元素

处理有序集合(sorted sets)的命令 (zsets)

ZADD key score member 添加指定成员到有序集合中，如果目标存在则更新score（分值，排序用）

ZREM key member 从有序集合删除指定成员

ZINCRBY key increment member 如果成员存在则将其增加_increment_，否则将设置一个score为_increment_的成员

ZRANGE key start end 返回升序排序后的指定范围的成员

ZREVRANGE key start end 返回降序排序后的指定范围的成员

ZRANGEBYSCORE key min max 返回所有符合score >= min和score <= max的成员 ZCARD key 返回有序集合的元素数量 ZSCORE key element 返回指定成员的SCORE值 ZREMRANGEBYSCORE key min max 删除符合 score >= min 和 score <= max 条件的所有成员

排序（List, Set, Sorted Set）

SORT key BY pattern LIMIT start end GET pattern ASC|DESC ALPHA 按照指定模式排序集合或List

SORT mylist

默认升序 ASC

SORT mylist DESC

SORT mylist LIMIT 0 10

从序号0开始，取10条

SORT mylist LIMIT 0 10 ALPHA DESC

按首字符排序

SORT mylist BY weight_*

SORT mylist BY weight_* GET object_*

SORT mylist BY weight_* GET object_* GET #

SORT mylist BY weight_* STORE resultkey

将返回的结果存放于resultkey序列（List）

持久控制

SAVE 同步保存数据到磁盘

BGSAVE 异步保存数据到磁盘

LASTSAVE 返回上次成功保存到磁盘的Unix时间戳

SHUTDOWN 同步保存到服务器并关闭 Redis 服务器（SAVE+QUIT）

BGREWRITEAOF 当日志文件过长时重写日志文件

远程控制命令

INFO 提供服务器的信息和统计信息

MONITOR 实时输出所有收到的请求

SLAVEOF 修改复制选项

redis目前提供四种数据类型：string,list,set及zset(sorted set)。

* string是最简单的类型，你可以理解成与Memcached一模一个的类型，一个key对应一个value，其上支持的操作与Memcached的操 作类似。但它的功能更丰富。
*
* * list是一个链表结构，主要功能是push、pop、获取一个范围的所有值等等。操作中key理解为链表的名字。
*
* * set是集合，和我们数学中的集合概念相似，对集合的操作有添加删除元素，有对多个集合求交并差等操作。操作中key理解为集合的名字。
*
* * zset是set的一个升级版本，他在set的基础上增加了一个顺序属性，这一属性在添加修改元素的时候可以指定，每次指定后，zset会自动重新按新的 值调整顺序。可以理解了有两列的mysql表，一列存value，一列存顺序。操作中key理解为zset的名字。
*
* 协议
*
*
* redis目前只有基于TCP的文本协议，与memcache类似，有一些改进。
*
* 客户端通常发送
*
* 命令 参数… 值字节数\r\n
*
* 值\r\n
*
* 服务端的返回，根据第一个字节，可以判断：
*
* - 错误信息
*
*   + 普通文本信息
*
*   $ 变长字节数，$6表示CRLF之后有6个字节的字符
*
*   : 返回一个整数
*
*   * 返回组数，即*6表示CRLF之后将返回6组变长字符
*
*   注意事项：
*
*   Key不可包含空格或者回车符
*
*   Key不要过长或过短，应使其有意义，如”comment:1234:reply.to”
*
 

 * */


#endif  //__REDIS_CLIENT_H_


