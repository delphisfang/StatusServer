#include "statsvr_timer_info.h"
#include <algorithm>

using namespace tfc::base;
using namespace statsvr;


int CTimerInfo::init(string req_data, int datalen)
{
    Json::Reader reader;
    Json::Value js_root;
    
    if (!reader.parse(req_data, js_root))
    {
        LogError("Failed to parse req_data: %s!", req_data.c_str());
        return -1;
    }

    if (!js_root["method"].isNull() && js_root["method"].isString())
    {
        m_cmd = get_value_str(js_root, "method");
    }
    else
    {
        m_cmd = get_value_str(js_root, "cmd");
    }

    if ("getUserInfo" != m_cmd && "getServiceInfo" != m_cmd && "getConfigForIM-reply" != m_cmd)
    {
        LogDebug("req_data: %s", req_data.c_str());
    }
    
    m_seq = get_value_uint(js_root, "innerSeq");

    get_value_str_safe(js_root["appID"], m_appID);
    
    if (js_root["data"].isNull() || !js_root["data"].isObject())
    {
        m_search_no = m_appID + "_" + i2str(m_msg_seq);
        return 0;
    }

    Json::Value js_data = js_root["data"];
    m_data = js_data.toStyledString();

    if (!js_data["appID"].isNull() && js_data["appID"].isString())
    {
        m_appID = js_data["appID"].asString();
    }
    else if (!js_data["appID"].isNull() && js_data["appID"].isUInt())
    {
        m_appID = ui2str(js_data["appID"].asUInt());
    }

    if (m_cmd != "pingConf" && m_cmd != "updateConf" && m_cmd != "getConfigForIM-reply"
        && m_cmd != "getConf" && m_cmd != "getTodayStatus"
        && CAppConfig::Instance()->checkAppIDExist(m_appID))
    {
        LogError("Unknown appID[%s]!", m_appID.c_str());
        return -1;
    }
    
    m_identity = get_value_str(js_data, "identity");
    m_cpIP     = get_value_str(js_data, "chatProxyIp");
    m_cpPort   = get_value_uint(js_data, "chatProxyPort");

    if (!js_data["tag"].isNull() && js_data["tag"].isString())
    {
        m_raw_tag = js_data["tag"].asString();
        m_tag     = m_appID + "_" + m_raw_tag;

        if (m_cmd != "pingConf" && m_cmd != "updateConf" && m_cmd != "getConfigForIM-reply"
            && m_cmd != "getConf" && m_cmd != "getTodayStatus" && m_cmd != "changeService"
            && CAppConfig::Instance()->checkTagExist(m_appID, m_tag))
        {
            LogError("Unknown tag[%s]!", m_tag.c_str());
            return -1;
        }
    }

    if (!js_data["userID"].isNull() && js_data["userID"].isString())
    {
        m_raw_userID = js_data["userID"].asString();
        m_userID     = m_appID + "_" + m_raw_userID;
    }
    else if (!js_data["userID"].isNull() && js_data["userID"].isArray())
    {
        Json::Value userID_list = js_data["userID"];
        for (int i = 0; i < userID_list.size(); i++)
        {
            m_userID_list.insert(m_appID + "_" + userID_list[i].asString());
        }
    }
    
    if (!js_data["serviceID"].isNull() && js_data["serviceID"].isString())
    {
        m_raw_serviceID = js_data["serviceID"].asString();
        m_serviceID     = m_appID + "_" + m_raw_serviceID;
    }
    else if (!js_data["serviceID"].isNull() && js_data["serviceID"].isArray())
    {
        Json::Value serviceID_list = js_data["serviceID"];
        for (int i = 0; i < serviceID_list.size(); i++)
        {
            m_serviceID_list.insert(m_appID + "_" + serviceID_list[i].asString());
        }
    }
    else if (!js_data["services"].isNull() && js_data["services"].isArray())
    {
        Json::Value serviceID_list = js_data["services"];
        for (int i = 0; i < serviceID_list.size(); i++)
        {
            m_serviceID_list.insert(m_appID + "_" + serviceID_list[i].asString());
        }
    }
    
    m_channel = get_value_str(js_data, "channel");
    m_status  = get_value_str(js_data, "status");
    
    if (!js_data["extends"].isNull() && js_data["extends"].isObject())
    {
        m_extends = js_data["extends"].toStyledString();
    }
    
    m_serviceName   = get_value_str(js_data, "serviceName");
    m_serviceAvatar = get_value_str(js_data, "serviceAvatar");
    m_maxUserNum    = get_value_uint(js_data, "maxUserNum", DEF_USER_NUM);
    
    if (!js_data["changeServiceID"].isNull() && js_data["changeServiceID"].isString())
    {
        m_raw_changeServiceID = js_data["changeServiceID"].asString();
        m_changeServiceID = m_appID + "_" + m_raw_changeServiceID;
    }
    
    if (!js_data["lastServiceID"].isNull() && js_data["lastServiceID"].isString())
    {
        m_raw_lastServiceID = js_data["lastServiceID"].asString();
        m_lastServiceID = m_appID + "_" + m_raw_lastServiceID;
    }

    if (!js_data["appointServiceID"].isNull() && js_data["appointServiceID"].isString())
    {
        m_has_appointServiceID = true;
        m_raw_appointServiceID = js_data["appointServiceID"].asString();
        m_appointServiceID     = m_appID + "_" + m_raw_appointServiceID;
    }
    else
    {
        m_has_appointServiceID = false;
    }

    m_queuePriority = get_value_uint(js_data, "queuePriority");

    #if 0
    if (!js_data["tags"].isNull() && js_data["tags"].isArray())
    {
        for (int i = 0; i < js_data["tags"].size(); i++)
        {
            string raw_tag;
            if (get_value_str_safe(js_data["tags"][i], raw_tag))
            {
                LogError("Error get tags[%d]!", i);
                continue;
            }

            string app_tag = m_appID + "_" + raw_tag;
            if (CAppConfig::Instance()->checkTagExist(m_appID, app_tag))
            {
                LogError("Unknown tags[%d]:%s!", i, raw_tag.c_str());
                continue;
            }
            
            m_tags.insert(app_tag);
        }
    }
    #endif
    
    m_priority = get_value_str(js_data, "priority");
    m_notify   = get_value_uint(js_data, "notify");

    char id_buf[64];
    snprintf(id_buf, sizeof(id_buf), "%s:%s:%s:%d", m_appID.c_str(), m_serviceID.c_str(), m_userID.c_str(), m_msg_seq);
    m_search_no = string(id_buf);
    
    LogDebug("Init request data OK! [id:%s,cmd:%s,userID:%s,servID:%s,msg_seq:%u]", 
                m_identity.c_str(), m_cmd.c_str(), m_userID.c_str(), m_serviceID.c_str(), m_msg_seq);

    return 0;
}

int CTimerInfo::on_error()
{
    Json::Value error_rsp;
    error_rsp["method"]   = m_cmd + "-reply";
    error_rsp["innerSeq"] = m_seq;
    error_rsp["code"]     = m_errno;
    error_rsp["msg"]      = m_errmsg;

    Json::FastWriter writer;
    string rsp = writer.write(error_rsp);
    
    m_proc->EnququeHttp2CCD(m_ret_flow, (char*)rsp.c_str(), rsp.size());

    if (m_errno < 0)
    {
        Json::Value post_data;
        Json::Value post_array;
        Json::Value post_err;
        string err_str;
        string tmp_str;
        timeval nowTime;
        gettimeofday(&nowTime, NULL);

        post_err["project"] = PROJECT_NAME;
        post_err["module"]  = MODULE_NAME;
        post_err["code"]    = m_errno;
        post_err["desc"]    = m_data;
        post_err["env"]     = m_proc->m_cfg._env;
        post_err["ip"]      = m_proc->m_cfg._local_ip;
        post_err["appid"]   = m_appID;
        post_err["timestamp"] = l2str(nowTime.tv_sec*1000 + nowTime.tv_usec / 1000);

        if (ERROR_NOT_READY == m_errno || ERROR_SESSION_WRONG  == m_errno)
        {
            post_err["level"] = 30;
        }
        else
        {
            post_err["level"] = 20;
        }

        post_array["headers"] = post_err;
        post_data.append(post_array);

        err_str = post_err.toStyledString();

        DEBUG_P(LOG_NORMAL, "just test which line\n");

        m_proc->EnququeErrHttp2DCC((char *)err_str.c_str(), err_str.size());
    }
    
    on_stat();
    return 0;
}

int CTimerInfo::on_stat()
{
    gettimeofday(&m_end_time, NULL);
    string staticEntry = m_identity + "_" + m_cmd;
    m_proc->AddStat(m_errno, staticEntry.c_str(), &m_start_time, &m_end_time);

    int32_t cur_errno = m_errno;
    if (errno < 0)
    {
        if (m_identity == "user")
        {
            m_proc->AddErrCmdMsg(m_appID, m_cmd, m_identity, m_userID, m_start_time, cur_errno);
        }
        else
        {
            m_proc->AddErrCmdMsg(m_appID, m_cmd, m_identity, m_serviceID, m_start_time, cur_errno);
        }
    }
}

void CTimerInfo::on_expire()
{
    Json::Value error_rsp;
    
    LogError("searchid[%s]: timer handle timeout, status[%d].", m_search_no.c_str(), m_cur_step);

    error_rsp["method"]   = m_cmd + "-reply";
    error_rsp["innerSeq"] = m_seq;
    error_rsp["code"]     = ERROR_SYSTEM_WRONG;
    error_rsp["msg"]      = "System handle timeout";

    Json::FastWriter writer;
    string strRsp = writer.write(error_rsp);
    m_proc->EnququeHttp2CCD(m_ret_flow, (char*)strRsp.c_str(), strRsp.size());
    return;
}

void CTimerInfo::OpStart()
{
    gettimeofday(&m_op_start, NULL);
}

void CTimerInfo::OpEnd(const char* itemName, int retcode)
{
    struct timeval end;
    gettimeofday(&end, NULL);
    m_proc->AddStat(retcode, itemName, &m_op_start, &end);
}

void CTimerInfo::AddStatInfo(const char* itemName, timeval* begin, timeval* end, int retcode)
{
    m_proc->AddStat(retcode, itemName, begin, end);
}

void CTimerInfo::FinishStat(const char* itemName)
{
    m_proc->AddStat(m_errno, itemName, &m_start_time, &m_end_time);

    char buff[24] = {0};
    snprintf(buff, sizeof(buff), "ccd_%s", itemName);
    m_proc->AddStat(0, buff, &m_ccd_time, &m_end_time);
}

uint64_t CTimerInfo::GetTimeGap()
{
    struct timeval end;
    gettimeofday(&end, NULL);

    uint64_t timecost = CalcTimeCost_MS(m_start_time, end);
    LogDebug("##### Timer GetTimeGap() max_time_gap[%u], timecost[%u].", m_max_time_gap, timecost);

    if (timecost > m_max_time_gap)
    {
        m_max_time_gap = 1;
    }
    else
    {
        m_max_time_gap -= timecost;
    }

    // m_op_start = end;

    return m_max_time_gap;
}

void CTimerInfo::on_error_parse_packet(string errmsg)
{
    m_errno  = ERROR_UNKNOWN_PACKET;
    m_errmsg = errmsg;
    on_error();
}

void CTimerInfo::on_error_get_data(string data_name)
{
    m_errno  = ERROR_SYSTEM_WRONG;
    m_errmsg = "Error get " + data_name;
    on_error();
}

void CTimerInfo::on_error_set_data(string data_name)
{
    m_errno  = ERROR_SYSTEM_WRONG;
    m_errmsg = "Error set " + data_name;
    on_error();
}

void CTimerInfo::on_error_parse_data(string data_name)
{
    m_errno  = ERROR_SYSTEM_WRONG;
    m_errmsg = "Error parse " + data_name;
    on_error();
}

void CTimerInfo::set_user_data(Json::Value &data)
{
    data["identity"]  = "user";
    data["userID"]    = m_raw_userID;
}

void CTimerInfo::set_service_data(Json::Value &data)
{
    data["identity"]  = "service";
    data["serviceID"] = m_raw_serviceID;
}

int CTimerInfo::on_send_request(string cmd, string ip, unsigned short port, const Json::Value &data, bool with_code)
{
    Json::Value req;
    req["appID"]    = m_appID;
    req["method"]    = cmd;
    req["innerSeq"] = m_msg_seq;
    req["data"]     = data;
    if (with_code)
        req["code"] = 0;

    Json::FastWriter writer;
    string strReq = writer.write(req);
    LogTrace("send request to <%s, %d>: %s", ip.c_str(), port, strReq.c_str());

    if (m_proc->EnququeHttp2DCC((char *)strReq.c_str(), strReq.size(), ip, port))
    {
        LogError("[%s]: Failed to send request <%s>!", m_appID.c_str(), cmd.c_str());
        return -1;
    }
    return 0;
}

int CTimerInfo::on_send_reply(const Json::Value &data)
{
    Json::Value rsp;
    rsp["appID"]    = m_appID;
    rsp["method"]    = m_cmd + "-reply";
    rsp["innerSeq"] = m_seq;
    rsp["code"]     = 0;
    rsp["data"]     = data;
    
    Json::FastWriter writer;
    string strRsp = writer.write(rsp);
    LogTrace("send response: %s", strRsp.c_str());

    if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
    {
        LogError("searchid[%s]: Failed to SendReply <%s>", m_search_no.c_str(), m_cmd.c_str());
        m_errno  = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send reply";
        on_error();
        return -1;
    }
    else
    {
        on_stat();
        return 0;
    }
}

int CTimerInfo::on_send_error_reply(ERROR_TYPE code, string msg, const Json::Value &data)
{
    Json::Value rsp;
    rsp["appID"]     = m_appID;
    rsp["method"]     = m_cmd + "-reply";
    rsp["innerSeq"]  = m_seq;
    rsp["code"]      = code;
    rsp["msg"]         = msg;
    rsp["data"]      = data;

    Json::FastWriter writer;
    string strRsp = writer.write(rsp);
    LogTrace("send ERROR response: %s", strRsp.c_str());
    
    if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
    {
        LogError("searchid[%s]: Failed to SendErrorReply <%s>", m_search_no.c_str(), msg.c_str());
        m_errno  = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send error reply";
        on_error();
        return -1;
    }
    else
    {
        on_stat();
        return 0;
    }
}

/***************** never use m_xxx in methods below, keep them stateless **************/

string CTimerInfo::gen_sessionID(const string &app_userID)
{
    return app_userID + "_" + l2str(time(NULL));
}

int CTimerInfo::get_user_session(const string &appID, const string &app_userID, Session *sess)
{
    if (NULL == sess)
        return SS_ERROR;
    
    SessionQueue* pSessQueue = NULL;
    Session temp;
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
        || pSessQueue->get(app_userID, temp))
    {
        LogError("Failed to get session of user[%s]!", app_userID.c_str());
        return SS_ERROR;
    }

    *sess = temp;
    return SS_OK;
}

int CTimerInfo::update_user_session(const string &appID, const string &app_userID, Session *sess, long long gap_warn, long long gap_expire)
{
    if (NULL == sess)
        return SS_ERROR;
    
    SessionQueue* pSessQueue = NULL;
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
        || pSessQueue->set(app_userID, sess, gap_warn, gap_expire))
    {
        LogError("Failed to update session of user[%s]!", app_userID.c_str());
        return SS_ERROR;
    }

    return SS_OK;
}

int CTimerInfo::delete_user_session(const string &appID, const string &app_userID)
{
    SessionQueue* pSessQueue = NULL;
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
        || pSessQueue->delete_session(app_userID))
    {
        LogError("Failed to delete session of user[%s]!", app_userID.c_str());
        return SS_ERROR;
    }

    return SS_OK;
}

int CTimerInfo::create_user_session(const string &appID, const string &app_userID, Session *sess, long long gap_warn, long long gap_expire)
{
    if (NULL == sess)
        return SS_ERROR;
    
    SessionQueue* pSessQueue = NULL;
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
        || pSessQueue->insert(app_userID, sess, gap_warn, gap_expire))
    {
        LogError("Failed to create session of user[%s]!", app_userID.c_str());
        return SS_ERROR;
    }

    return SS_OK;
}

void CTimerInfo::get_user_json(const string &appID, const string &app_userID, const UserInfo &user, Json::Value &userJson)
{
    user.toJson(userJson);

    Session sess;
    if (get_user_session(appID, app_userID, &sess))
    {
        userJson["session"] = Json::objectValue;
    }
    else
    {
        Json::Value sessJson;
        sess.toJson(sessJson);
        userJson["session"] = sessJson;
    }
}

void CTimerInfo::construct_user_json(const UserInfo &user, const Session &sess, Json::Value &userJson)
{
    user.toJson(userJson);

    Json::Value sessJson;
    sess.toJson(sessJson);
    userJson["session"] = sessJson;
}

int CTimerInfo::reply_user_json_A(const string &appID, const string &app_userID, const UserInfo &user)
{
    Json::Value userJson;
    get_user_json(appID, app_userID, user, userJson);
    DO_FAIL(on_send_reply(userJson));
    return SS_OK;
}

int CTimerInfo::reply_user_json_B(const UserInfo &user, const Session &sess)
{
    Json::Value userJson;
    construct_user_json(user, sess, userJson);
    DO_FAIL(on_send_reply(userJson));
    return SS_OK;
}

unsigned CTimerInfo::get_service_queuenum(const string &appID, const ServiceInfo &serv)
{
    unsigned queueNum = 0;

    UserQueue *uq = NULL, *highpri_uq = NULL;
    for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); ++it)
    {
        if (get_normal_queue(appID, *it, &uq))
            continue;

        if (get_highpri_queue(appID, *it, &highpri_uq))
            continue;

        queueNum += uq->size();
        queueNum += highpri_uq->size();
    }

    return queueNum;
}

int CTimerInfo::get_service_json(const string &appID, const ServiceInfo &serv, Json::Value &servJson)
{
    Json::Value arrayUserList;
    string userID;
    string app_userID;
    
    serv.toJson(servJson);

    //获取userList
    arrayUserList.resize(0);
    for (set<string>::iterator it = serv.userList.begin(); it != serv.userList.end(); it++)
    {
        UserInfo user;
        Json::Value userJson = Json::objectValue;

        userID       = *it;
        app_userID = appID + "_" + userID;
    
        userJson["userID"] = userID;
        if (CAppConfig::Instance()->GetUser(app_userID, user))
        {
            LogError("Failed to get user: %s!", app_userID.c_str());
            userJson["sessionID"] = "";
            userJson["channel"]   = "";
        }
        else
        {
            userJson["sessionID"] = user.sessionID;
            userJson["channel"]   = user.channel;
        }
        arrayUserList.append(userJson);
    }
    servJson["userList"].resize(0);
    servJson["userList"] = arrayUserList;

    //获取排队人数
    servJson["queueNumber"] = get_service_queuenum(appID, serv);
    
    if (true == serv.is_busy())
    {
        servJson["status"] = "busy";
    }
}


int CTimerInfo::get_normal_queue(const string &appID, const string &raw_tag, UserQueue **uq)
{
    if (NULL == uq)
        return SS_ERROR;
    
    TagUserQueue *pTagQueues = NULL;
    GET_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues), "tag queues");
    GET_FAIL(pTagQueues->get_tag(raw_tag, *uq), "queue");
    return SS_OK;
}

int CTimerInfo::get_highpri_queue(const string &appID, const string &raw_tag, UserQueue **uq)
{
    if (NULL == uq)
        return SS_ERROR;
    
    TagUserQueue *pTagQueues = NULL;
    GET_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues), "highpri tag queues");
    GET_FAIL(pTagQueues->get_tag(raw_tag, *uq), "queue");
    return SS_OK;
}

int CTimerInfo::find_random_service_by_tag(const string &appID, const string &app_tag, 
                                    const string &old_app_serviceID, ServiceInfo &target_serv)
{
    ServiceHeap servHeap;
    if (CAppConfig::Instance()->GetTagServiceHeap(app_tag, servHeap))
    {
        LogError("Failed to get ServiceHeap of tag[%s]!", app_tag.c_str());
        return SS_ERROR;
    }

    //随机分配一个坐席
    srand((unsigned)time(NULL));
    int j = rand() % servHeap.size();

    set<string>::iterator it;
    it = servHeap._servlist.begin();
    for (int k = 0; k < j; ++k)
    {
        ++it;
    }
    
    for (int i = j; ; )
    {
        //排除old_app_serviceID
        //如果坐席的服务人数已满，就分配下一个坐席
        if (old_app_serviceID == (*it) || CAppConfig::Instance()->GetService(*it, target_serv) || !target_serv.is_available())
        {
            if (old_app_serviceID == (*it))
            {
                LogWarn("skip old_serviceID: %s", old_app_serviceID.c_str());
            }
            else
            {
                LogWarn("service[%s]: user_count[%d], maxConvNum[%d]", target_serv.serviceID.c_str(), target_serv.user_count(), target_serv.maxUserNum);
            }

            ++it;
            ++i;
            if (i == servHeap.size())
            {
                i = 0;
            }
            
            if (i == j)
            {
                break;
            }
        }
        else
        {
            LogTrace("[%s] Find tag serviceID: %s.", appID.c_str(), target_serv.serviceID.c_str());
            return SS_OK;
        }
    }

    LogError("[%s] Failed to find tag service!", appID.c_str());
    return SS_ERROR;
}

int CTimerInfo::find_least_service_by_tag(const string &appID, const string &app_tag,
                                        const string &old_app_serviceID, ServiceInfo &target_serv)
{
    ServiceHeap servHeap;
    if (CAppConfig::Instance()->GetTagServiceHeap(app_tag, servHeap))
    {
        LogError("Failed to get ServiceHeap of tag[%s]!", app_tag.c_str());
        return SS_ERROR;
    }

    double service_load = 0.0, min_load = 1.0; //important
    set<string>::iterator target_it = servHeap._servlist.end();
    
    set<string>::iterator it;
    ServiceInfo serv;
    for (it = servHeap._servlist.begin(); it != servHeap._servlist.end(); ++it)
    {
        //排除old_app_serviceID
        if (old_app_serviceID == (*it) || CAppConfig::Instance()->GetService(*it, serv) || !serv.is_available())
        {
            if (old_app_serviceID == (*it))
            {
                LogWarn("skip old_serviceID: %s", old_app_serviceID.c_str());
            }
            else
            {
                LogWarn("service[%s]: user_count[%d], maxConvNum[%d]", serv.serviceID.c_str(), serv.user_count(), serv.maxUserNum);
            }
            
            continue;
        }
        else
        {
            service_load = ((double)serv.user_count()) / ((double)serv.maxUserNum);
            if (service_load < min_load)
            {
                min_load    = service_load;
                target_it   = it;
                target_serv = serv;
            }
        }
    }

    if (target_it != servHeap._servlist.end())
    {
        LogTrace("[%s] Find tag serviceID: %s.", appID.c_str(), target_serv.serviceID.c_str());
        return SS_OK;
    }
    else
    {
        LogError("[%s] Failed to find tag service!", appID.c_str());
        return SS_ERROR;
    }
}

/********************************* KV methods *************************************/

int CTimerInfo::KV_set_userIDList()
{
    string strUserIDList;
    DO_FAIL(CAppConfig::Instance()->UserListToString(strUserIDList));
    return KVSetKeyValue(KV_CACHE, USERLIST_KEY, strUserIDList);    
}

int CTimerInfo::KV_set_servIDList()
{
    string strServIDList;
    DO_FAIL(CAppConfig::Instance()->ServiceListToString(strServIDList));
    return KVSetKeyValue(KV_CACHE, SERVLIST_KEY, strServIDList);    
}

int CTimerInfo::KV_set_user(string app_userID, const UserInfo &user, bool isUpdate)
{
    DO_FAIL(KVSetKeyValue(KV_CACHE, USER_PREFIX+app_userID, user.toString()));

    if (false == isUpdate)
    {
        DO_FAIL(KV_set_userIDList());
    }
    return SS_OK;
}

int CTimerInfo::KV_del_user(const string &app_userID)
{
    DO_FAIL(KVDelKeyValue(KV_CACHE, USER_PREFIX+app_userID));
    DO_FAIL(KV_set_userIDList());
    return SS_OK;
}

int CTimerInfo::KV_set_service(string app_serviceID, const ServiceInfo &serv, bool isUpdate)
{
    DO_FAIL(KVSetKeyValue(KV_CACHE, SERV_PREFIX+app_serviceID, serv.toString()));

    if (false == isUpdate)
    {
        DO_FAIL(KV_set_servIDList());
    }
    return SS_OK;
}

int CTimerInfo::KV_del_service(const string &app_serviceID)
{
    DO_FAIL(KVDelKeyValue(KV_CACHE, SERV_PREFIX+app_serviceID));
    DO_FAIL(KV_set_servIDList());
    return SS_OK;
}

int CTimerInfo::KV_set_session(string app_userID, const Session &sess, long long gap_warn, long long gap_expire)
{
    Json::Value sessJson;
    sess.toJson(sessJson);
    sessJson["gap_warn"]   = gap_warn;
    sessJson["gap_expire"] = gap_expire;
    return KVSetKeyValue(KV_CACHE, SESS_PREFIX+app_userID, sessJson.toStyledString());
}

int CTimerInfo::KV_del_session(string app_userID)
{
    return KVDelKeyValue(KV_CACHE, SESS_PREFIX+app_userID);
}

int CTimerInfo::KV_set_queue(string appID, string raw_tag, bool highpri)
{
    TagUserQueue *pTagQueues = NULL;
    UserQueue *uq = NULL;

    string prefix         = (highpri) ? (HIGHQ_PREFIX) : (QUEUE_PREFIX);
    string key_queueList  = prefix + appID + "_" + raw_tag;

    if (highpri)
    {
        DO_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues));
    }
    else
    {
        DO_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues));
    }
    DO_FAIL(pTagQueues->get_tag(raw_tag, uq));

    string val_queueList = uq->toString();
    DO_FAIL(KVSetKeyValue(KV_CACHE, key_queueList, val_queueList));
    return SS_OK;
}

int CTimerInfo::KV_parse_user(string app_userID)
{
    LogTrace("parse userID:%s", app_userID.c_str());
    
    string user_key = USER_PREFIX + app_userID;
    string user_value;
    DO_FAIL(KVGetKeyValue(KV_CACHE, user_key, user_value));
    
    UserInfo user(user_value);
    SET_USER(CAppConfig::Instance()->AddUser(app_userID, user));
    return SS_OK;
}

int CTimerInfo::KV_parse_session(string app_userID)
{
    LogTrace("parse session of userID:%s", app_userID.c_str());
    
    Json::Reader reader;
    string sess_key = SESS_PREFIX + app_userID;
    string sess_value;
    DO_FAIL(KVGetKeyValue(KV_CACHE, sess_key, sess_value));
    
    Session sess(sess_value);
    Json::Value obj;
    if (!reader.parse(sess_value, obj))
    {
        return SS_ERROR;
    }
    
    long long gap_warn   = obj["gap_warn"].asInt64();
    long long gap_expire = obj["gap_expire"].asInt64();
    string appID = getappID(app_userID);
    SET_SESS(CreateUserSession(appID, app_userID, &sess, gap_warn, gap_expire));
    return SS_OK;
}

int CTimerInfo::KV_parse_service(string app_serviceID)
{
    LogTrace("parse serviceID:%s", app_serviceID.c_str());
    
    /*获取*/
    string serv_key = SERV_PREFIX + app_serviceID;
    string serv_value;
    DO_FAIL(KVGetKeyValue(KV_CACHE, serv_key, serv_value));
    
    /* 解析每个service的详细信息 */
    ServiceInfo serv(serv_value, DEF_USER_NUM);
    SET_USER(CAppConfig::Instance()->AddService(app_serviceID, serv));
    
    /* 添加到ServiceHeap */
    string appID = getappID(app_serviceID);
    SET_FAIL(CAppConfig::Instance()->AddService2Tags(appID, serv), "service heap");

    /* 更新OnlineServiceNum */
    if ("online" == serv.status)
    {
        DO_FAIL(AddTagOnlineServNum(appID, serv));
    }
    return SS_OK;
}

int CTimerInfo::KV_parse_queue(string app_tag, bool highpri)
{
    LogTrace("parse queue of app_tag: %s", app_tag.c_str());
    
    TagUserQueue *pTagQueues = NULL;
    string key_queueList, val_queueList;

    string appID   = getappID(app_tag);
    string raw_tag = delappID(app_tag);
    
    if (true == highpri)
    {
        if (CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues))
        {
            LogError("Failed to GetTagHighPriQueue(appID: %s)! But we force KV Restore continues...", appID.c_str());
            return SS_OK;
        }
        key_queueList  = HIGHQ_PREFIX + app_tag;
    }
    else
    {
        if (CAppConfig::Instance()->GetTagQueue(appID, pTagQueues))
        {
            LogError("Failed to GetTagQueue(appID: %s)! But we force KV Restore continues...", appID.c_str());
            return SS_OK;
        }
        key_queueList  = QUEUE_PREFIX + app_tag;
    }

    /*获取*/
    if (KVGetKeyValue(KV_CACHE, key_queueList, val_queueList))
    {
        LogTrace("queue[%s] is empty!", key_queueList.c_str());
        return SS_OK;
    }

    /*解析*/
    Json::Reader reader;
    Json::Value obj, queueNode;
    if (!reader.parse(val_queueList, obj))
    {
        return SS_ERROR;
    }
    int queueNum = obj["queueList"].size();
    for (int i = 0; i < queueNum; i++)
    {
        queueNode = obj["queueList"][i];
        string userID         = queueNode["userID"].asString();
        long long expire_time = queueNode["expire_time"].asInt64();
        SET_USER(pTagQueues->add_user(raw_tag, userID, expire_time));
    }

    return SS_OK;
}

/************************* wrapper methods **********************/
int CTimerInfo::AddUser(string app_userID, const UserInfo &user)
{
    SET_USER(CAppConfig::Instance()->AddUser(app_userID, user));
    DO_FAIL(KV_set_user(app_userID, user, false));
    return SS_OK;
}

int CTimerInfo::UpdateUser(string app_userID, const UserInfo &user)
{
    SET_USER(CAppConfig::Instance()->UpdateUser(app_userID, user));
    DO_FAIL(KV_set_user(app_userID, user, true));
    return SS_OK;
}

int CTimerInfo::DeleteUser(string app_userID)
{
    SET_SERV(CAppConfig::Instance()->DelUser(app_userID));
    DO_FAIL(KV_del_user(app_userID));
    return SS_OK;
}

int CTimerInfo::AddService(string appID, string app_servID, ServiceInfo &serv)
{
    SET_SERV(CAppConfig::Instance()->AddService(app_servID, serv));
    //insert service in every tag serviceHeap
    SET_FAIL(CAppConfig::Instance()->AddService2Tags(appID, serv), "service heap");
    DO_FAIL(KV_set_service(app_servID, serv, false));
    return SS_OK;

}

int CTimerInfo::UpdateService(string app_servID, const ServiceInfo &serv)
{
    SET_SERV(CAppConfig::Instance()->UpdateService(app_servID, serv));
    DO_FAIL(KV_set_service(app_servID, serv, true));
    return SS_OK;
}

int CTimerInfo::DeleteService(string app_servID)
{
    SET_SERV(CAppConfig::Instance()->DelService(app_servID));
    DO_FAIL(KV_del_service(app_servID));
    return SS_OK;
}

int CTimerInfo::UpdateUserSession(string appID, string app_userID, Session *sess)
{
    if (NULL == sess)
        return SS_ERROR;
    
    long long gap_warn   = ("" != sess->serviceID) ? (DEF_SESS_TIMEWARN) : (MAX_INT);
    long long gap_expire = ("" != sess->serviceID) ? (DEF_SESS_TIMEOUT) : (MAX_INT);
    
    SET_SESS(update_user_session(appID, app_userID, sess, gap_warn, gap_expire));
    DO_FAIL(KV_set_session(app_userID, *sess, gap_warn, gap_expire));
    return SS_OK;
}

int CTimerInfo::UpdateSessionNotified(const string &appID, const string &app_userID)
{
    Session sess;
    GET_SESS(get_user_session(appID, app_userID, &sess));

    if (0 == sess.notified)
    {
        sess.notified = 1;
        DO_FAIL(UpdateUserSession(appID, app_userID, &sess));
    }
    return SS_OK;
}

int CTimerInfo::DeleteUserSession(string appID, string app_userID)
{
    SET_SESS(delete_user_session(appID, app_userID));
    DO_FAIL(KV_del_session(app_userID));
    return SS_OK;
}

int CTimerInfo::CreateUserSession(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
{
    if (NULL == sess)
        return SS_ERROR;
    
    SET_SESS(create_user_session(appID, app_userID, sess, gap_warn, gap_expire));
    DO_FAIL(KV_set_session(app_userID, *sess, gap_warn, gap_expire));
    return SS_OK;
}

int CTimerInfo::AddTagOnlineServNum(string appID, const ServiceInfo &serv)
{
    if ("online" != serv.status)
    {
        string app_servID = appID + "_" + serv.serviceID;
        LogWarn("Service[%s] is not online, just return.", app_servID.c_str());
        return 0;
    }

    for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
    {
        DO_FAIL(CAppConfig::Instance()->AddTagOnlineServiceNum(appID, *it));
    }
    return 0;
}

int CTimerInfo::DelTagOnlineServNum(string appID, const ServiceInfo &serv)
{
    if ("online" != serv.status)
    {
        string app_servID = appID + "_" + serv.serviceID;
        LogWarn("Service[%s] is not online, just return.", app_servID.c_str());
        return 0;
    }
    
    for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
    {
        DO_FAIL(CAppConfig::Instance()->DelTagOnlineServiceNum(appID, *it));
    }
    return 0;
}

