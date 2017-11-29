#ifndef _MY_DEBUG_H_
#define _MY_DEBUG_H_

#define SS_OK (0)
#define SS_ERROR (-1)

#define LogDebug(fmt, ...)	do{			\
		DEBUG_P(LOG_DEBUG, "[DEBUG] [%s:%s:%d] " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}while(0)

#define LogWarn(fmt, ...)	do{			\
		DEBUG_P(LOG_ERROR, "\033[1m\033[33;40m[ERROR] [%s:%s:%d] " fmt "\033[0m\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}while(0)

#define LogError(fmt, ...)	do{			\
		DEBUG_P(LOG_ERROR, "\033[1m\033[31;40m[ERROR] [%s:%s:%d] " fmt "\033[0m\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}while(0)

#define LogTrace(fmt, ...)	do{			\
		DEBUG_P(LOG_TRACE, "\033[1m\033[32;40m[TRACE] [%s:%s:%d] " fmt "\033[0m\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}while(0)

#define LogFatal(fmt, ...)	do{			\
		DEBUG_P(LOG_FATAL, "[FATAL] [%s:%s:%d] " fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);	\
	}while(0)

#define LogErrPrint(fmt, ...)   do{         \
        LogError("[ERROR] [%s:%s:%d]" fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__);    \
        printf("[ERROR] [%s:%s:%d]" fmt "\n", __FILE__, __func__, __LINE__, ##__VA_ARGS__); \
    }while(0)


#define MAJOR_VERSION  "1"
#define MIDDLE_VERSION "0"
#define MINOR_VERSION  "0"

#define ON_ERROR_PARSE_PACKET() do{\
		LogError("Error parse http data");\
		on_error_parse_packet("Error unknown packet");\
	}while(0)

#define ON_ERROR_PARSE_DATA(data_name) do{\
		LogError("[%s] Error parse %s\n", m_search_no.c_str(), data_name);\
		on_error_parse_data(data_name);\
	}while(0)

#define ON_ERROR_GET_DATA(data_name) do{\
		LogError("[%s] Error get %s\n", m_search_no.c_str(), data_name);\
		on_error_get_data(data_name);\
	}while(0)

#define ON_ERROR_SET_DATA(data_name) do{\
			LogError("[%s] Error set %s\n", m_search_no.c_str(), data_name);\
			on_error_set_data(data_name);\
		}while(0)


#define DO_FAIL(expr) do{\
			if (SS_OK != expr)\
			{\
				LogError("[ERROR] Failed to call %s", #expr);\
				return SS_ERROR;\
			}\
		}while(0)

#define GET_FAIL(expr, data_name) do{\
			if (SS_OK != expr)\
			{\
				LogError("[ERROR] Failed to call %s", #expr);\
				ON_ERROR_GET_DATA(data_name);\
				return SS_ERROR;\
			}\
		}while(0)

#define SET_FAIL(expr, data_name) do{\
			if (SS_OK != expr)\
			{\
				LogError("[ERROR] Failed to call %s", #expr);\
				ON_ERROR_SET_DATA(data_name);\
				return SS_ERROR;\
			}\
		}while(0)

#define GET_USER(expr) GET_FAIL(expr, "user")
#define GET_SERV(expr) GET_FAIL(expr, "service")
#define GET_SESS(expr) GET_FAIL(expr, "session")
#define GET_QUEUE(expr) GET_FAIL(expr, "queue")

#define SET_USER(expr) SET_FAIL(expr, "user")
#define SET_SERV(expr) SET_FAIL(expr, "service")
#define SET_SESS(expr) SET_FAIL(expr, "session")
#define SET_QUEUE(expr) SET_FAIL(expr, "queue")

#define CI (CAppConfig::Instance())

#define MAX_INT (0x7FFFFFFF)
#define DEF_SESS_TIMEOUT (CAppConfig::Instance()->getDefaultSessionTimeOut(m_appID))
#define DEF_SESS_TIMEWARN (CAppConfig::Instance()->getDefaultSessionTimeWarn(m_appID))

#define CS(x) ((x).c_str())

#define KV_CACHE (m_proc->kv_cache)

#define USER_PREFIX ("USER_")
#define SERV_PREFIX ("SERV_")
#define SESS_PREFIX ("SESS_")
#define USERLIST_KEY ("USERLIST")
#define SERVLIST_KEY ("SERVLIST")
#define QUEUE_PREFIX ("QUEUE_")
#define HIGHQ_PREFIX ("HIGHQ_")

#endif

