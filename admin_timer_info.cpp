#include "admin_timer_info.h"
#include "statsvr_mcd_proc.h"
#include <algorithm>

//extern char BUF[DATA_BUF_SIZE];

#define MAXSIZE (1024*sizeof(unsigned))

int AdminConfigTimer::do_next_step(string& req_data)
{
    switch (m_cur_step)
    {
        case STATE_INIT:
        {
            if (init(req_data, req_data.size()))
            {
                ON_ERROR_PARSE_PACKET();
                return -1;
            }
            
            if (m_cmd == "pingConf")
            {
                if (on_admin_ping())
                {
                    m_cur_step = STATE_END; 
                    return -1;
                }
                on_stat();
                m_cur_step = STATE_END;
                return 1;
            }
            else if (m_cmd == "getConf")
            {
                if (on_admin_getConf())
                {
                    m_cur_step = STATE_END;
                    return -1;
                }
                on_stat();
                m_cur_step = STATE_END;
                return 1;
            }
            else if (m_cmd == "updateConf" || m_cmd == "getConfigForIM-reply")
            {
                bool isUpdateConf = (m_cmd == "updateConf") ? (true) : (false); 
                if (on_admin_config(isUpdateConf))
                {
                    m_cur_step = STATE_END;
                    return -1;
                }
                on_stat();

                //防止重复重建
                if (m_proc->m_workMode == statsvr::WORKMODE_WORK)
                {
                    LogTrace(">>>>>>>>>>>>>>>>>>>Already in workmode, need not restore.");
                    m_cur_step = STATE_END;
                    return 1;
                }
                
                if (on_admin_restore())
                {
                    m_cur_step = STATE_END;
                    return -1;
                }
                m_cur_step = STATE_END;
                return 1;
            }
            else if (m_cmd == "getServiceStatus")
            {
                if (m_proc->m_workMode == statsvr::WORKMODE_READY)
                {
                    m_errno  = ERROR_NOT_READY;
                    m_errmsg = "System not ready";
                    on_error();
                    m_cur_step = STATE_END;
                    return -1;
                }
                
                if (on_admin_getServiceStatus())
                {
                    m_cur_step = STATE_END;
                    return -1;
                }
                on_stat();
                m_cur_step = STATE_END;
                return 1;
            }
            else if (m_cmd == "getTodayStatus")
            {
                if (m_proc->m_workMode == statsvr::WORKMODE_READY)
                {
                    m_errno  = ERROR_NOT_READY;
                    m_errmsg = "System not ready";
                    on_error();
                    m_cur_step = STATE_END;
                    return -1;
                }
                
                if (on_admin_get_today_status())
                {
                    m_cur_step = STATE_END;
                    return -1;
                }
                on_stat();
                m_cur_step = STATE_END;
                return 1;
            }
            else
            {
                LogError("Error unknown cmd: %s", m_cmd.c_str());
                ON_ERROR_PARSE_PACKET();
                return -1;
            }
        }
        default:
        {
            LogError("error state: %d", m_cur_step);
            return -1;
        }
    }
    return 0;
}

int AdminConfigTimer::on_admin_send_reply(const Json::Value &data)
{
    Json::Value rsp;
    rsp["cmd"]  = m_cmd + "-reply";
    rsp["code"] = 0;
    rsp["msg"]  = "OK";
    rsp["seq"]  = m_seq;
    rsp["data"] = data;

    Json::FastWriter writer;
    string strRsp = writer.write(rsp);
    //LogTrace("Admin send reply: %s", strRsp.c_str());
    if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
    {
        LogError("Failed to enqueue to CCD!");
        m_errno = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send to client";
        on_error();
        return -1;
    }
    return 0;
}

int AdminConfigTimer::on_admin_ping()
{
    Json::Reader reader;
    Json::Value ping_req;
    
    if (!reader.parse(m_data, ping_req))
    {
        LogError("Failed to parse ping data: [%s]", m_data.c_str());
        return -1;
    }

    if (ping_req["appIDList"].isNull() || !ping_req["appIDList"].isArray())
    {
        LogError("Failed to parse appIDlist! data: [%s]", m_data.c_str());
        on_error_parse_packet("Error parse appIDList");
        return -1;
    }

    //参数检查
    unsigned size = ping_req["appIDList"].size();
    if (size > MAXSIZE)
    {
        LogError("size:%d > MAXSIZE:%d", size, MAXSIZE);
        on_error_parse_packet("Error appIDList too long");
        return -1;
    }

    Json::Value appIDlistVer;
    appIDlistVer.resize(0);
    map<string, bool> appIDMap;
    for (unsigned i = 0; i < size; ++i)
    {
        string appID = ping_req["appIDList"][i].asString();

        //若appID不存在，则version返回0
        int version = 0;
        if (CAppConfig::Instance()->GetVersion(appID, version))
        {
            CAppConfig::Instance()->SetVersion(appID, 0);
        }

        Json::Value items;
        items["appID"]   = atoi(appID.c_str());
        items["version"] = version;
        appIDlistVer.append(items);

        /* 为appIDList里的每一个appID创建各种数据结构 */
        CAppConfig::Instance()->AddTagQueue(appID);
        CAppConfig::Instance()->AddTagHighPriQueue(appID);
        CAppConfig::Instance()->AddSessionQueue(appID);
        //暂不调用AddServiceHeap(tag)，tagList需等到updateConf时才确定
        
        appIDMap[appID] = true;
    }
    
    int delnum = CAppConfig::Instance()->CheckDel(appIDMap);

    mSetAppIDListStr(m_data);
    LogTrace("[pingConf] SetAppIDListStr: %s", m_data.c_str());
    
    Json::Value data;
    data["appList"] = appIDlistVer;
    return on_admin_send_reply(data);
}

int AdminConfigTimer::on_admin_getConf()
{
    LogDebug("Receive a getConf request.");

    Json::Reader reader;
    Json::Value appList;
    Json::Value data;
    Json::Value configList;
    configList.resize(0);

    string appListStr;
    mGetAppIDListStr(appListStr);
    
    if (!reader.parse(appListStr, appList))
    {
        LogError("Failed to parse appListStr: %s", appListStr.c_str());
        m_errno  = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error parse appIDList";
        on_error();
        return -1;
    }

    for (int i = 0; i < appList["appIDList"].size(); i++)
    {
        string appID;
        if (appList["appIDList"][i].isString())
        {
            appID = appList["appIDList"][i].asString();
        }
        else
        {
            appID = ui2str(appList["appIDList"][i].asUInt());
        }
        
        int version = 0;
        CAppConfig::Instance()->GetVersion(appID, version);
        
        Json::Value data_rsp;
        data_rsp["appID"]   = atoi(appID.c_str());
        data_rsp["version"] = version;

        string appConf;
        CAppConfig::Instance()->GetConf(appID, appConf);
        Json::Value js_appConf;
        if (!reader.parse(appConf, js_appConf))
        {
            data_rsp["configs"] = appConf;
        }
        else
        {
            data_rsp["configs"] = js_appConf;
        }
        
        configList.append(data_rsp);
    }
    data["appList"] = configList;

    return on_admin_send_reply(data);
}

int AdminConfigTimer::on_admin_config(bool isUpdateConf)
{
    LogDebug("Receive a updateConf request.");

    Json::Reader reader;
    Json::Value push_config;
    if (!reader.parse(m_data, push_config))
    {
        LogError("Failed to parse updateConf data: [%s]", m_data.c_str());
        ON_ERROR_PARSE_PACKET();
        return -1;
    }

    CAppConfig::Instance()->UpdateAppConf(push_config);

    if (isUpdateConf)
    {
        Json::Value data = Json::objectValue;
        return on_admin_send_reply(data);
    }
    else
    {
        return 0;
    }
}

int AdminConfigTimer::on_admin_getServiceStatus()
{
    Json::Value servInfoList;
    servInfoList.resize(0);
    Json::Value servInfo;
    int i = 0;
    
    string app_servID;
    
    for (set<string>::iterator it = m_serviceID_list.begin(); it != m_serviceID_list.end(); it++)
    {
        app_servID = (*it);
        servInfo["serviceID"] = delappID(app_servID);

        ServiceInfo serv;
        if (mGetService(app_servID, serv))
        {
            servInfo["serviceStatus"] = DEF_SERV_STATUS;
            servInfo["userNum"]       = 0;
        }
        else
        {
            servInfo["serviceStatus"]    = serv.status;
            //当前服务人数>=最大会话人数时，返回busy
            if (true == serv.is_busy())
            {
                servInfo["serviceStatus"] = BUSY;
            }
            servInfo["userNum"] = serv.user_count();
        }
        
        servInfoList[i] = servInfo;
        ++i;
    }

    Json::Value data;
    data["services"] = servInfoList;
    data["identity"] = m_identity;

    return on_admin_send_reply(data);
}

int AdminConfigTimer::get_app_today_status(string appID, Json::Value &appList)
{
    unsigned userNumber = 0;
    unsigned queueNumber = 0;
    unsigned onlineServiceNumber = 0;
    SessionQueue *pSessQueue = NULL;
    TagUserQueue *pTagQueues = NULL;
    TagUserQueue *pTagHighPriQueues = NULL;

    //获取某个appID下当前正在会话的用户人数
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue))
    {
        userNumber = 0;
    }
    else
    {
        userNumber = pSessQueue->get_usernum_in_service();
    }

    /* 获取当前正在排队的用户人数 */
    if (0 == CAppConfig::Instance()->GetTagQueue(appID, pTagQueues))
    {
        queueNumber += pTagQueues->total_queue_count();
    }
    if (0 == CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagHighPriQueues))
    {
        queueNumber += pTagHighPriQueues->total_queue_count();
    }

    /* 在线客服人数 */
    onlineServiceNumber = CAppConfig::Instance()->GetOnlineServiceNum(appID);

    Json::Value app_data;
    app_data["appID"]         = atoi(appID.c_str());
    app_data["userNumber"]    = userNumber;
    app_data["queueNumber"]   = queueNumber;
    app_data["serviceNumber"] = onlineServiceNumber;

    appList.append(app_data);
    return 0;
}

/* 获取某个appID下的 (会话用户数，排队用户数，在线客服数) */
int AdminConfigTimer::on_admin_get_today_status()
{
    Json::Reader reader;
    Json::Value req;
    if (!reader.parse(m_data, req))
    {
        LogError("Failed to parse request data. data: [%s]", m_data.c_str());
        return -1;
    }
    
    if (req["appIDList"].isNull() || !req["appIDList"].isArray())
    {
        LogError("Failed to parse appIDlist. data: [%s]", m_data.c_str());
        on_error_parse_packet("Error parse appIDList");
        return -1;
    }

    //参数检查
    unsigned size = req["appIDList"].size();
    if (size > MAXSIZE)
    {
        LogError("size:%d > MAXSIZE:%d", size, MAXSIZE);
        on_error_parse_packet("Error appIDList too long");
        return -1;
    }

    Json::Value appList;
    appList.resize(0);
    for (unsigned i = 0; i < size; ++i)
    {
        string appID = req["appIDList"][i].asString();
        get_app_today_status(appID, appList);
    }

    Json::Value data;
    data["identity"] = m_identity;
    data["appList"]  = appList;
    
    return on_admin_send_reply(data);
}


int AdminConfigTimer::get_id_list(string value, string idListName, vector<string> &idList)
{
    Json::Reader reader;
    Json::Value obj;
    int idNum = 0;
    
    if (!reader.parse(value, obj))
    {
        LogError("Failed to parse value:%s!", value.c_str());
        return 0;
    }
    idNum = obj[idListName].size();
    LogTrace("idNum: %d", idNum);
    for (int i = 0; i < idNum; i++)
    {
        idList.push_back(obj[idListName][i].asString());
    }

    return idNum;
}

int AdminConfigTimer::restore_userList()
{
    Json::Reader reader;
    string val_userIDList;
    int userNum = 0;
    vector<string> userIDList;
    string appID;

    /* 解析userID列表 */
    if (KVGetKeyValue(KV_CACHE, USERLIST_KEY, val_userIDList))
    {
        LogTrace("userIDList is empty!");
        return SS_OK;
    }
    userNum = get_id_list(val_userIDList, "userIDList", userIDList);
    
    /* 解析每个user / session */
    for (int i = 0; i < userNum; ++i)
    {
        /* 解析每个user的详细信息 */
        if (KV_parse_user(userIDList[i]))
        {
            continue;
        }
    
        /* 解析每个user session */
        if (KV_parse_session(userIDList[i]))
        {
            continue;
        }
    }

    return SS_OK;
}

int AdminConfigTimer::restore_serviceList()
{
    string val_servIDList;
    int servNum = 0;
    vector<string> servIDList;
    string appID;

    if (KVGetKeyValue(KV_CACHE, SERVLIST_KEY, val_servIDList))
    {
        LogTrace("servIDList is empty!");
        return SS_OK;
    }

    servNum = get_id_list(val_servIDList, "serviceIDList", servIDList);
    
    /* 解析每个service, 添加到ServiceHeap */
    for (int i = 0; i < servNum; ++i)
    {
        if (KV_parse_service(servIDList[i]))
        {
            continue;
        }
    }

    return SS_OK;
}


int AdminConfigTimer::restore_queue(string appID, vector<string> appID_tags, bool highpri)
{
    for (unsigned i = 0; i < appID_tags.size(); i++)
    {
        if (KV_parse_queue(appID_tags[i], highpri))
        {
            continue;
        }
    }

    return SS_OK;
}


/*
一级数据结构
USERLIST            -> userIDList = ['u1','u2',...]
SERVLIST            -> servIDList = ['s1','s2',...]
QUEUE_<appID>_<tag> -> queueList  = [{userID='u1', expire_time=}, ...] //排队有先后顺序
HIGHQ_<appID>_<tag> -> highqList  = [{userID='u1', expire_time=}, ...]
*/
int AdminConfigTimer::on_admin_restore()
{
    Json::Reader reader;
    Json::Value appList;
    string appListStr;

    /* 获取appID列表 */
    DO_FAIL(mGetAppIDListStr(appListStr));
    if (!reader.parse(appListStr, appList))
    {
        LogError("Failed to parse appListStr: %s!", appListStr.c_str());
        ON_ERROR_PARSE_DATA("appIDList");
        return SS_ERROR;
    }

    /* 重建userList */
    restore_userList();
    
    /* 重建serviceList,   添加到ServiceHeap */
    restore_serviceList();

    /* 遍历appID列表 */
    for (int i = 0; i < appList["appIDList"].size(); i++)
    {
        string appID = appList["appIDList"][i].asString();
        LogTrace("appID: %s", appID.c_str());
        
        string str_appID_tags;
        CAppConfig::Instance()->GetValue(appID, "tags", str_appID_tags);
        vector<string> appID_tags;
        MySplitTag((char *)str_appID_tags.c_str(), ";", appID_tags);

        /* 解析普通队列 - 先解析userList，再解析排队队列 */
        restore_queue(appID, appID_tags, false);

        /* 解析高优先级队列 */
        restore_queue(appID, appID_tags, true);
    }

    m_proc->m_workMode = statsvr::WORKMODE_WORK;
    LogTrace(">>>>>>>>>>>>>>>>>>>>[%s] enter workmode<<<<<<<<<<<<<<<<<<<<", MODULE_NAME);

    return SS_OK;
}

AdminConfigTimer::~AdminConfigTimer()
{
}

