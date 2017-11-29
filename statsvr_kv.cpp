#include "tfc_debug_log.h"

#include <sys/file.h>
#include <fstream>
#include <time.h>

#include "kv_define.h"
#include "kv_errno.h"
#include "water_log.h"
#include "feedback_log.h"
#include "longconn_utils.h"
#include "md5.hpp"

#include "kv_proto.h"
#include "statsvr_mcd_proc.h"

#include "statsvr_kv.h"

using namespace std;
using namespace ad::kv_server;

#define BUFF_SIZE       10 * 1024 * 1024

static char kv_server_buffer[BUFF_SIZE];

int KVGetKeyValue(KvCacheCtrl &ckv_cache, const string &key, string &value)
{
	//LogTrace("[KV_GET] key: %s", key.c_str());

    value.clear();
    if (0 == key.length() || key.length() > MAX_KEY_LEN)
    {
		LogError();
        return ERRNO_KV_INVALID_KEY_LEN;
    }

    ssize_t date_len;
    int ret;

    ret = ckv_cache.get((char *)key.c_str() , key.length() , kv_server_buffer , sizeof(kv_server_buffer) , date_len);
    if (ret >= 0)
    {
        value.assign(kv_server_buffer, date_len);
		LogTrace("[KV_GET] success to get key: %s, value:%s", key.c_str(), value.c_str());
        return 0;
    }
    else
    {
		//LogError("[KV_GET] key:%s not exist!", key.c_str());
        return ERRNO_KV_KEY_NOT_EXIST;
    }
}

int KVSetKeyValue(KvCacheCtrl &ckv_cache, const string &key, const string &value)
{
	//LogTrace("[KV_SET] key: %s, value:%s", key.c_str(), value.c_str());
	
    if (0 == key.length() || key.length() > MAX_KEY_LEN)
    {
		LogError("[KV_SET] Invalid key len!");
        return ERRNO_KV_INVALID_KEY_LEN;
    }

    if (0 == value.length())
    {
		LogError("[KV_SET] value is empty, try delete key-value!");
		return KVDelKeyValue(ckv_cache, key);
    }

    int ret = 0;
    ret = ckv_cache.set((char *)key.c_str(), key.length(), (char*)value.c_str(), value.length());
    ret = ret >= 0 ? 0 : -1;

	if (0 == ret)
	{
		LogTrace("Success to set key: %s, value: %s", key.c_str(), value.c_str());
	}
	else
	{
		LogError("Failed to set key: %s, value: %s", key.c_str(), value.c_str());
	}
    return ret;
}

int KVDelKeyValue(KvCacheCtrl &ckv_cache, const string &key)
{
	//LogTrace("[KV_DEL] key: %s", key.c_str());

    if (0 == key.length() || key.length() > MAX_KEY_LEN)
    {
		LogDebug("[KEY_DEL] Invalid key len!");
        return ERRNO_KV_INVALID_KEY_LEN;
    }

    /*if(key.length() > KV_KEY_LEN)
    {
        return -3;
    }*/

    int ret;
    ret = ckv_cache.del((char *)key.c_str() , key.length());
	if (0 == ret)
	{
		LogTrace("Success to del key: %s", key.c_str());
	}
	else
	{
		LogTrace("Failed to del key: %s", key.c_str());
	}
    return ret == 0 ? 0 : ERRNO_KV_DEL_ERR;
}


#if 0
int KVAppendKeyValue(KvCacheCtrl &ckv_cache, const string &key, const string &value)
{
    string old_value;
    KVGetKeyValue(ckv_cache, key, old_value);

    old_value.append(value);
    return KVSetKeyValue(ckv_cache, key, old_value);
}
#endif

