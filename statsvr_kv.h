#ifndef _STATSVR_KV_H_
#define _STATSVR_KV_H_

#include "statsvr_error.h"
#include "kv_cache_ctrl.h"

int KVGetKeyValue(ad::kv_server::KvCacheCtrl &ckv_cache, const string &key, string &value);
int KVSetKeyValue(ad::kv_server::KvCacheCtrl &ckv_cache, const string &key, const string &value);
int KVDelKeyValue(ad::kv_server::KvCacheCtrl &ckv_cache, const string &key);
//int KVAppendKeyValue(ad::kv_server::KvCacheCtrl &ckv_cache, const string &key, const string &value);

#endif /* _STATSVR_KV_H_*/
