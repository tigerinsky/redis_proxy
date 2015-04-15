 
/**
 * @file redis_proxy.cpp
 * @author way
 * @date 2014/08/28 17:56:34
 * @brief 
 *  
 **/

#include "redis_proxy.h"

#include "hiredis.h"
#include "glog/logging.h"

namespace tis {

RedisProxy::RedisProxy() {
    _host = NULL; 
    _port = 0;
    _retry_num = DEFAULT_RETRY_NUM;
    _timeout = DEFAULT_TIMEOUT;
    _redis_context = NULL;
    _redis_reply = NULL;
    _last_err =  REDIS_OK;
}

RedisProxy::~RedisProxy() {
    close_connection();
}

void RedisProxy::set_retry_num(uint32_t retry_num) {
    _retry_num = retry_num;
}

void RedisProxy::set_timeout(long timeout) {
    _timeout = timeout;
}

RedisProxy* RedisProxy::duplicate() const {
    RedisProxy* new_proxy = new(std::nothrow) RedisProxy;
    if (NULL == new_proxy) {
        return NULL; 
    }
    new_proxy->set_retry_num(get_retry_num());
    new_proxy->set_timeout(get_timeout());
    int ret = new_proxy->connect(get_host(), get_port());
    if (0 != ret) {
        delete new_proxy;
        return NULL; 
    }
    return new_proxy;
}

const char* RedisProxy::__get_err_msg() {
    if (REDIS_ERR_IO == _redis_context->err) {
        return "IO error"; 
    } else {
        return _redis_context->errstr; 
    }
}


int RedisProxy::connect(const char* host, uint32_t port) {
    if (NULL == host || '\0' == host[0]) {
        LOG(WARNING) << "redis proxy: illegal host";
        return 1; 
    }
    if (NULL != _redis_context) {
        LOG(WARNING) << "redis proxy: illegal host";
        return 1; 
    }
    _host = host;
    _port = port;
    _redis_context = redisConnect(host, port);
    if (NULL == _redis_context) {
        LOG(WARNING) << "redis proxy: create redis context error"; 
        return 1;
    }
    if (_redis_context->err) {
        LOG(WARNING) << "redis proxy: init connect error, msg[" << __get_err_msg() <<"]";
        redisFree(_redis_context); 
        _redis_context = NULL;
        return 1;
    }
    struct  timeval tv;
    tv.tv_sec = _timeout / 1000;
    tv.tv_usec = (_timeout % 1000) * 1000000;
    if(REDIS_ERR == redisSetTimeout(_redis_context, tv)) {
        LOG(WARNING) << "redis proxy: set redis timeout error, timeout[" << _timeout << "]"; 
        redisFree(_redis_context); 
        _redis_context = NULL;
        return 1;
    }
    _redis_context->reader->maxbuf = 0;
    return 0;
}

int RedisProxy::__execute_command(const char* fmt, ...) {
    for (uint32_t i = 0; i < _retry_num + 1; ++i) {
        if (REDIS_ERR_IO == _last_err 
                || REDIS_ERR_EOF == _last_err) {
            close_connection(); 
            if (connect(_host, _port)) {
                return REDIS_REQUEST_ERR;
            }
        }
        va_list args;
        va_start(args, fmt);
        _redis_reply = static_cast<redisReply*>(redisvCommand(_redis_context, fmt, args));
        va_end(args);
        if (NULL == _redis_reply) {
            _last_err = _redis_context->err;
            LOG(WARNING) << "redis proxy: get reply failed, time[" << i << "] msg[" << __get_err_msg() << "]"; 
            continue;
        } else {
            _last_err = REDIS_OK; 
        }
        if (REDIS_REPLY_ERROR == _redis_reply->type) {
            LOG(WARNING) << "redis proxy: return erro, msg[" << _redis_reply->str << "]";
            return REDIS_RETURN_ERR; 
        }
        return REDIS_RETURN_OK;
    }
    return REDIS_REQUEST_ERR; 
}

bool RedisProxy::is_alive() {
    bool ret = false;
    if(REDIS_RETURN_OK == __execute_command("PING")
            && REDIS_REPLY_STATUS == _redis_reply->type
            && 0 == strcasecmp(_redis_reply->str, "PONG")) {
        ret = true;
    } else {
        ret = false; 
    }
    freeReplyObject(_redis_reply);
    return ret;
}

int RedisProxy::shutdown() {
    _redis_reply = static_cast<redisReply*>(redisCommand(_redis_context, "SHUTDOWN"));
    if (NULL == _redis_reply) {
        close_connection();
        return 0; 
    } else {
        LOG(WARNING) << "redis proxy: shutdown error, host[" << _host << "] port[" << _port << "]"; 
        freeReplyObject(_redis_reply);
        return 1;
    }
}

void RedisProxy::close_connection() {
    if(NULL != _redis_context) {
        redisFree(_redis_context);
        _redis_context = NULL;
    }
}

int RedisProxy::set(const char* key, const char* value, uint32_t size) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("SET %s %b", key, value, size)
            && REDIS_REPLY_STATUS == _redis_reply->type
            && 0 == strcasecmp(_redis_reply->str, "OK")) {
        ret = REDIS_SET_OK;
    } else {
        ret = REDIS_SET_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::get(const char* key, std::string& value) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("GET %s", key)) {
        if (REDIS_REPLY_NIL == _redis_reply->type) {
            ret = REDIS_GET_NOT_EXIST; 
        }
        if (REDIS_REPLY_STRING == _redis_reply->type) {
            value.assign(_redis_reply->str, _redis_reply->len);
            ret = REDIS_GET_OK; 
        }
    } else {
        ret = REDIS_GET_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::del(const char* key) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("DEL %s", key)) {
        if (REDIS_REPLY_INTEGER == _redis_reply->type) {
            if (0 == _redis_reply->integer) {
                ret = REDIS_DEL_NOT_EXIST; 
            }
            if (1 == _redis_reply->integer) {
                ret = REDIS_DEL_OK; 
            }
        } 
    } else {
        ret = REDIS_DEL_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::exists(const char* key) {
    int ret = REDIS_EXISTS_ERR;
    if (REDIS_RETURN_OK == __execute_command("exists %s", key)) {
        if (REDIS_REPLY_INTEGER == _redis_reply->type) {
            if (0 == _redis_reply->integer) {
                ret = REDIS_EXISTS_NO; 
            } 
            if (1 == _redis_reply->integer) {
                ret = REDIS_EXISTS_YES; 
            } 
        } 
    } else {
        ret = REDIS_EXISTS_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::setex(const char* key, const char* value, uint32_t size, uint64_t expire_time) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("SETEX %s %llu %b", 
                                             key, 
                                             expire_time, 
                                             value, 
                                             size)
            && REDIS_REPLY_STATUS == _redis_reply->type
            && 0 == strcasecmp(_redis_reply->str, "OK")) {
        ret = REDIS_SETEX_OK;
    } else {
        ret = REDIS_SETEX_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::incr(const char* key, int64_t* value) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("INCR %s", key) 
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_INCR_OK;
        if (NULL != value) {
            *value = _redis_reply->integer; 
        }
    } else  {
        ret = REDIS_INCR_ERR; 
    }
    freeReplyObject(_redis_reply);
    return ret;
}

int RedisProxy::lpush(const char* key, 
                        const char* value, 
                        uint32_t size, 
                        uint64_t* list_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("LPUSH %s %b", key, value, size)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_LPUSH_OK;
        if (NULL != list_len){
            *list_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_LPUSH_ERR;
    }
    freeReplyObject(_redis_reply);
    return ret;
}

int RedisProxy::rpush(const char* key,
                        const char* value,
                        uint32_t size,
                        uint64_t* list_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("RPUSH %s %b", key, value, size)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_RPUSH_OK;
        if (NULL != list_len) {
            *list_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_RPUSH_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::smembers(const char* key, std::vector<std::string>* value_vec){
    int ret = -1;
    if (NULL == value_vec){
        return ret;
    }
    if (REDIS_RETURN_OK == __execute_command("SMEMBERS %s", key)
            && REDIS_REPLY_ARRAY == _redis_reply->type) {
        ret = REDIS_SMEMBERS_OK;
        for (size_t i = 0; i < _redis_reply->elements; i++){
            value_vec->push_back(_redis_reply->element[i]->str);
        }
    } else {
        ret = REDIS_SMEMBERS_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::sadd(const char* key,
                    const char* value,
                    uint32_t size,
                    uint64_t* set_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("SADD %s %b", key, value, size)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_SADD_OK;
        if (NULL != set_len){
            *set_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_SADD_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::srem(const char* key,
                    const char* value,
                    uint32_t size,
                    uint64_t* set_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("SREM %s %b", key, value, size)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_SREM_OK;
        if (NULL != set_len){
            *set_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_SREM_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::ltrim(const char* key, int32_t start, int32_t end){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("LTRIM %s %d %d", key, start, end)
            && REDIS_REPLY_STATUS == _redis_reply->type
            && 0 == strcasecmp(_redis_reply->str, "OK")) {
        ret = REDIS_LTRIM_OK;
    } else {
        ret = RDIS_LTRIM_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::lrange(const char* key, 
                       int32_t start, 
                       int32_t end, 
                       std::vector<std::string>* values) {
    int ret = -1;
    if (values 
            && REDIS_RETURN_OK == __execute_command("LRANGE %s %d %d", key, start, end)
            && REDIS_REPLY_ARRAY == _redis_reply->type) {
        ret = REDIS_LRANGE_OK; 
        for (size_t i = 0; i < _redis_reply->elements; i++){
            values->push_back(_redis_reply->element[i]->str);
        }
    } else {
        ret = REDIS_LRANGE_ERR; 
    }
    freeReplyObject(_redis_reply);
    return ret;
}

int RedisProxy::hget(const char* key,
        const char* field,
        std::string& value) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("HGET %s %s", key, field)) {
        if (REDIS_REPLY_NIL == _redis_reply->type) {
            ret = REDIS_HGET_NOT_EXIST; 
        }
        if (REDIS_REPLY_STRING == _redis_reply->type) {
            value.assign(_redis_reply->str, _redis_reply->len);
            ret = REDIS_HGET_OK; 
        }
    } else {
        ret = REDIS_HGET_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::zcard(const char* key,
                    uint64_t* sorted_set_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("ZCARD %s", key)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_ZCARD_OK;
        if (NULL != sorted_set_len){
            *sorted_set_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_ZCARD_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::zadd(const char* key,
                    const char* value,
                    uint32_t size,
                    int64_t score,
                    uint64_t* added_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("ZADD %s %ld %b", key, score, value, size)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_ZADD_OK;
        if (NULL != added_len){
            *added_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_ZADD_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::zincr(const char* key,
                    const char* value,
                    uint32_t size,
                    int32_t increment){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("ZINCRBY %s %d %b", key, increment, value, size)
            && REDIS_REPLY_STRING == _redis_reply->type) {
        ret = REDIS_ZINCR_OK;
    } else {
        ret = REDIS_ZINCR_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::zscore(const char* key,
                    const char* value,
                    uint32_t size,
                    std::string& score) {
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("ZSCORE %s %b", key, value, size)) {
        if (REDIS_REPLY_NIL == _redis_reply->type) {
            ret = REDIS_ZSCORE_NOT_EXIST; 
        }
        if (REDIS_REPLY_STRING == _redis_reply->type) {
            score.assign(_redis_reply->str, _redis_reply->len);
            ret = REDIS_ZSCORE_OK; 
        }
    } else {
        ret = REDIS_ZSCORE_ERR; 
    }
    freeReplyObject(_redis_reply); 
    return ret;
}

int RedisProxy::zrem(const char* key,
                    const char* value,
                    uint32_t size,
                    uint64_t* remed_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("ZREM %s %b", key, value, size)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_ZREM_OK;
        if (NULL != remed_len){
            *remed_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_ZREM_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}

int RedisProxy::zrange(const char* key,
                    int32_t start,
                    int32_t end,
                    std::vector<std::string>* value_vec,
                    bool with_score,
                    std::vector<std::string>* score_vec){
    int ret = -1;
    if (NULL == value_vec){
        return ret;
    }
    const char* command = with_score ? "ZRANGE %s %d %d WITHSCORES" : "ZRANGE %s %d %d";
    if (REDIS_RETURN_OK == __execute_command(command, key, start, end)
            && REDIS_REPLY_ARRAY == _redis_reply->type) {
        ret = REDIS_ZRANGE_OK;
        for (size_t i = 0; i < _redis_reply->elements; i++) {
            if (with_score && (1 == (i % 2))) {
                score_vec->push_back(_redis_reply->element[i]->str);
            } else {
                value_vec->push_back(_redis_reply->element[i]->str);
            }
        }
    } else {
        ret = REDIS_ZRANGE_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;

}

int RedisProxy::zremrangebyrank(const char* key,
                    int32_t start,
                    int32_t stop,
                    uint64_t* remed_len){
    int ret = -1;
    if (REDIS_RETURN_OK == __execute_command("ZREMRANGEBYRANK %s %d %d", key, start, stop)
            && REDIS_REPLY_INTEGER == _redis_reply->type) {
        ret = REDIS_ZREMRANGEBYRANK_OK;
        if (NULL != remed_len){
            *remed_len = _redis_reply->integer;
        }
    } else {
        ret = REDIS_ZREMRANGEBYRANK_ERR;
    }
    freeReplyObject(_redis_reply);

    return ret;
}
}

/* vim: set expandtab ts=4 sw=4 sts=4 tw=100: */
