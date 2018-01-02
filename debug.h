#ifndef _MY_DEBUG_H_
#define _MY_DEBUG_H_

#define SS_OK (0)
#define SS_ERROR (-1)

#define LogDebug(fmt, ...)    do{\
        DEBUG_P(LOG_DEBUG, "[DEBUG] [%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define LogWarn(fmt, ...)    do{\
        DEBUG_P(LOG_ERROR, "\033[1m\033[33;40m[ERROR] [%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define LogError(fmt, ...)    do{\
        DEBUG_P(LOG_ERROR, "\033[1m\033[31;40m[ERROR] [%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define LogTrace(fmt, ...)    do{\
        DEBUG_P(LOG_TRACE, "\033[1m\033[32;40m[TRACE] [%s:%d] " fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define LogFatal(fmt, ...)    do{\
        DEBUG_P(LOG_FATAL, "[FATAL] [%s:%d] " fmt "\n", __func__, __LINE__, ##__VA_ARGS__);\
    }while(0)

#define LogErrPrint(fmt, ...)   do{\
        LogError("[ERROR] [%s:%d]" fmt "\n", __func__, __LINE__, ##__VA_ARGS__);\
        printf("[ERROR] [%s:%d]" fmt "\n", __func__, __LINE__, ##__VA_ARGS__);\
    }while(0)


#define MAJOR_VERSION  "1"
#define MIDDLE_VERSION "0"
#define MINOR_VERSION  "0"

#define ON_ERROR_PARSE_PACKET() do{\
        LogError("Error parse http data!");\
        on_error_parse_packet("Error unknown packet");\
    }while(0)

#define ON_ERROR_PARSE_DATA(data_name) do{\
        LogError("[%s] Error parse %s!\n", m_search_no.c_str(), data_name);\
        on_error_parse_data(data_name);\
    }while(0)

#define ON_ERROR_GET_DATA(data_name) do{\
        LogError("[%s] Error get %s!", m_search_no.c_str(), data_name);\
        on_error_get_data(data_name);\
    }while(0)

#define ON_ERROR_SET_DATA(data_name) do{\
            LogError("[%s] Error set %s!", m_search_no.c_str(), data_name);\
            on_error_set_data(data_name);\
        }while(0)


#define DO_FAIL(expr) do{\
            if (SS_OK != (expr))\
            {\
                LogError("[ERROR] Failed to call %s!", #expr);\
                return SS_ERROR;\
            }\
        }while(0)

#define GET_FAIL(expr, data_name) do{\
            if (SS_OK != (expr))\
            {\
                LogError("[ERROR] Failed to call %s!", #expr);\
                ON_ERROR_GET_DATA(data_name);\
                return SS_ERROR;\
            }\
        }while(0)

#define SET_FAIL(expr, data_name) do{\
            if (SS_OK != (expr))\
            {\
                LogError("[ERROR] Failed to call %s!", #expr);\
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

#define PROJECT_NAME ("StatusServer")
#define MODULE_NAME  ("StatusServer")

/* user status */
#define IN_YIBOT ("inYiBot")
#define IN_SERVICE ("inService")
#define ON_QUEUE ("onQueue")

/* service status */
#define ONLINE ("online")
#define OFFLINE ("offline")
#define BUSY ("busy")
#define DEF_SERV_STATUS (OFFLINE)
#define SUB_LIXIAN ("离线")

/* session fields */
#define USER_ID ("userID")
#define SERV_ID ("serviceID")
#define SESSION_ID ("sessionID")
#define ACTIVE_TIME ("activeTime")
#define BUILD_TIME ("buildTime")
#define CP_IP ("chatProxyIp")
#define CP_PORT ("chatProxyPort")
#define NOTIFIED ("notified")
#define STATUS ("status")
#define QUEUE_RANK ("queueRank")

/* user fields */
#define USER_TAG ("tag")
#define QTIME ("qtime")
#define LAST_SERV_ID ("lastServiceID")
#define PRIO ("priority")
#define QUEUE_PRIO ("queuePriority")
#define CHANNEL ("channel")

/* service fields */
#define SERV_NAME ("serviceName")
#define SERV_AVATAR ("serviceAvatar")
#define MAX_USER_NUM_FIELD ("maxUserNum")
#define DEF_USER_NUM (CAppConfig::Instance()->getMaxConvNum(m_appID))
#define SUB_STATUS ("subStatus")

#define mGetAppIDListStr(str) CAppConfig::Instance()->GetAppIDListStr(str)
#define mSetAppIDListStr(str) CAppConfig::Instance()->SetAppIDListStr(str)

#endif /* _MY_DEBUG_H_ */

