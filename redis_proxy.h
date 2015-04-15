 
/**
 * @file redis_proxy.h
 * @author way
 * @date 2014/08/28 17:41:46
 * @brief 
 *  
 **/

#ifndef  __REDIS_PROXY_H_
#define  __REDIS_PROXY_H_

#include <stdint.h>
#include <string>
#include <vector>

struct redisContext;
struct redisReply;

namespace tis {

class RedisProxy {
public:
    static const uint32_t DEFAULT_RETRY_NUM = 1;
    static const long DEFAULT_TIMEOUT = 2000;

    static const int REDIS_RETURN_OK = 0;
    static const int REDIS_REQUEST_ERR = 1;
    static const int REDIS_RETURN_ERR = 2;

    static const int REDIS_SET_OK = 0;
    static const int REDIS_SET_ERR = 1;

    static const int REDIS_GET_OK = 0;
    static const int REDIS_GET_NOT_EXIST = 1;
    static const int REDIS_GET_ERR = 2;

    static const int REDIS_DEL_OK = 0;
    static const int REDIS_DEL_NOT_EXIST = 1;
    static const int REDIS_DEL_ERR = 2;

    static const int REDIS_EXISTS_YES = 0;
    static const int REDIS_EXISTS_NO = 1;
    static const int REDIS_EXISTS_ERR = 2;

    static const int REDIS_SETEX_OK = 0;
    static const int REDIS_SETEX_ERR = 1;

    static const int REDIS_INCR_OK = 0;
    static const int REDIS_INCR_ERR = 1;

    static const int REDIS_LPUSH_OK = 0;
    static const int REDIS_LPUSH_ERR = 1;

    static const int REDIS_LRANGE_OK = 0;
    static const int REDIS_LRANGE_ERR = 1;

    static const int REDIS_RPUSH_OK = 0;
    static const int REDIS_RPUSH_ERR = 1;

    static const int REDIS_SMEMBERS_OK = 0;
    static const int REDIS_SMEMBERS_ERR = 1;

    static const int REDIS_SADD_OK = 0;
    static const int REDIS_SADD_ERR = 1;

    static const int REDIS_SREM_OK = 0;
    static const int REDIS_SREM_ERR = 1;

    static const int REDIS_LTRIM_OK = 0;
    static const int RDIS_LTRIM_ERR = 1;

    static const int REDIS_HGET_OK = 0;
    static const int REDIS_HGET_NOT_EXIST = 1;
    static const int REDIS_HGET_ERR = 2;

    static const int REDIS_ZADD_OK = 0;
    static const int REDIS_ZADD_ERR = 1;

    static const int REDIS_ZCARD_OK = 0;
    static const int REDIS_ZCARD_ERR = 1;

    static const int REDIS_ZSCORE_OK = 0;
    static const int REDIS_ZSCORE_NOT_EXIST = 1;
    static const int REDIS_ZSCORE_ERR = 2;

    static const int REDIS_ZINCR_OK = 0;
    static const int REDIS_ZINCR_ERR = 1;

    static const int REDIS_ZREM_OK = 0;
    static const int REDIS_ZREM_ERR = 1;

    static const int REDIS_ZRANGE_OK = 0;
    static const int REDIS_ZRANGE_ERR = 1;

    static const int REDIS_ZREMRANGEBYRANK_OK = 0;
    static const int REDIS_ZREMRANGEBYRANK_ERR = 1;
public:
    RedisProxy();
    virtual ~RedisProxy();
    void set_retry_num(uint32_t retry_num);
    void set_timeout(long milliseconde);
    const char* get_host() const { return _host; }
    uint32_t get_port() const { return _port; }
    uint32_t get_retry_num() const { return _retry_num; }
    long get_timeout() const { return _timeout; }
    RedisProxy* duplicate() const;
    int connect(const char* host, uint32_t port);
    void close_connection();
    int shutdown();
    bool is_alive();
    int set(const char* key, const char* value, uint32_t size);
    int get(const char* key, std::string& value);
    int del(const char* key);
    int exists(const char* key);
    int setex(const char* key, const char* value, uint32_t size, uint64_t expire_time);
    int incr(const char* key, int64_t* value);
    int lpush(const char* key, 
                const char* value, 
                uint32_t size, 
                uint64_t* list_len = NULL);
    int rpush(const char* key,
                const char* value,
                uint32_t size,
                uint64_t* list_len = NULL);
    int smembers(const char* key, 
                std::vector<std::string>* value_vec);
    int sadd(const char* key,
                const char* value,
                uint32_t size,
                uint64_t* set_len = NULL);
    int srem(const char* key,
                const char* value,
                uint32_t size,
                uint64_t* set_len = NULL);
    int ltrim(const char* key, int32_t start, int32_t end = -1);
    int lrange(const char* key, 
               int32_t start, 
               int32_t stop,
               std::vector<std::string>* values);
    int hget(const char* key,
             const char* field,
             std::string& value);
    int zcard(const char* key,
                uint64_t* sorted_set_len);
    int zadd(const char* key,
                const char* value,
                uint32_t size,
                int64_t score = 1,
                uint64_t* added_len = NULL);
    int zincr(const char* key,
                const char* value,
                uint32_t size,
                int32_t increment);
    int zscore(const char* key,
                const char* value,
                uint32_t size,
                std::string &score);
    int zrem(const char* key,
                const char* value,
                uint32_t size,
                uint64_t* remed_len = NULL);
    int zrange(const char* key,
                int32_t start,
                int32_t end,
                std::vector<std::string>* value_vec,
                bool with_score = false,
                std::vector<std::string>* score_vec = NULL);

    int zremrangebyrank(const char* key,
                int32_t start,
                int32_t end,
                uint64_t* remed_len = NULL);

private:
    int __execute_command(const char* fmt, ...);
    const char* __get_err_msg(); 

    const char* _host;
    uint32_t _port;
    uint32_t _retry_num;
    long _timeout;
    int _last_err; 

    redisContext* _redis_context;
    redisReply* _redis_reply;

    RedisProxy(const RedisProxy&);
    RedisProxy& operator=(const RedisProxy&);
};

}

#endif  //__REDIS_PROXY_H_

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
