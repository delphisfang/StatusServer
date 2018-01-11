#include "system_timer_info.h"
#include "statsvr_mcd_proc.h"

#include <algorithm>

using namespace statsvr;

#ifndef DISABLE_YIBOT_SESSION_CHECK

int YiBotOutTimer::do_next_step(string& req_data)
{
    Json::Reader reader;
    Json::Value data;

    //LogDebug("req_data: %s", req_data.c_str());
    if (!reader.parse(req_data, data))
    {
        LogError("Failed to parse req_data!");
        return -1;
    }

    if (data["userID"].isNull() || !data["userID"].isArray())
    {
        return -1;
    }

    Json::Value js_userID_list = data["userID"];
    for (int i = 0; i < js_userID_list.size(); i++)
    {
        m_userID_list.insert(js_userID_list[i].asString());
    }
    
    if (on_yibot_timeout())
    {
        return -1;
    }
    else
    {
        return 1;///important
    }
}

/*
input: m_appID, m_userID_list
*/
int YiBotOutTimer::on_yibot_timeout()
{
    for (set<string>::iterator it = m_userID_list.begin(); it != m_userID_list.end(); it++)
    {
        m_userID = (*it);
        m_appID  = getappID(m_userID);
        LogTrace("[yibot_timeout] appID: %s, userID: %s", m_appID.c_str(), m_userID.c_str());

        SessionQueue* pSessQueue = NULL;
        DO_FAIL(CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue));
        
        //get session
        Session sess;
        DO_FAIL(pSessQueue->get(m_userID, sess));
        //only update yibot session
        if ("" != sess.serviceID)
        {
            continue;
        }
        LogTrace("[yibot_timeout] sessionID: %s", sess.sessionID.c_str());
        
        //delete old, create new session
        DO_FAIL(DeleteUserSession(m_appID, m_userID));
        if (sess.has_refreshed())
        {
            sess.notified = 0;
        }
        sess.atime = sess.btime = GetCurTimeStamp();
        sess.sessionID = gen_sessionID(m_userID);
        DO_FAIL(CreateUserSession(m_appID, m_userID, &sess, MAX_INT, MAX_INT));
        //update user
        UserInfo user;
        DO_FAIL(mGetUser(m_userID, user));
        user.sessionID = sess.sessionID;
        //don't update user atime
        DO_FAIL(UpdateUser(m_userID, user));
    }
    
    return SS_OK;
}

YiBotOutTimer::~YiBotOutTimer()
{
}
#endif


int UserOutTimer::do_next_step(string& req_data)
{
    m_user_time_gap = atoi(req_data.c_str());

    if (on_user_timeout())
    {
        return -1;
    }
    else
    {
        return 1;///important
    }
}

int UserOutTimer::on_user_timeout()
{
    CAppConfig::Instance()->GetTimeoutUsers(m_user_time_gap, m_userList);
    if (m_userList.size() <= 0)
    {
        LogTrace("no user timeout, do nothing!");
        return 0;
    }
    LogTrace("Num of timeout users: %d", m_userList.size());

    set<string>::iterator it;
    for (it = m_userList.begin(); it != m_userList.end(); it++)
    {
        LogTrace("Goto delete timeout user[%s].", (*it).c_str());
        
        m_userID     = *it;
        m_raw_userID = delappID(m_userID);
        m_appID      = getappID(m_userID);

        //获取user
        UserInfo user;
        if (mGetUser(m_userID, user))
        {
            continue;
        }

        if (IN_YIBOT != user.status)
        {
            continue;
        }

        #if 0
        //从service.userList删除user
        Session sess;
        get_user_session(m_appID, m_userID, &sess);
        if ("" != sess.serviceID)
        {
            m_serviceID = m_appID + "_" + sess.serviceID;
            ServiceInfo serv;
            if (0 == mGetService(m_serviceID, serv))
            {
                serv.delete_user(m_raw_userID);
                UpdateService(m_serviceID, serv);
                LogTrace("Service[%s] Delete user[%s]", m_serviceID.c_str(), m_userID.c_str());
            }
        }
        #endif
        
        //删除user session
        DeleteUserSession(m_appID, m_userID);

        #if 0
        //从排队队列中删除user
        m_raw_tag = user.tag;
        UserQueue *uq = NULL;
        if (0 == get_highpri_queue(m_appID, m_raw_tag, &uq) && NULL != uq)
        {
            if (0 == uq->delete_user(m_userID))
            {
                KV_set_queue(m_appID, m_raw_tag, true);
            }
        }
        if (0 == get_normal_queue(m_appID, m_raw_tag, &uq) && NULL != uq)
        {
            if (0 == uq->delete_user(m_userID))
            {
                KV_set_queue(m_appID, m_raw_tag, false);
            }
        }
        #endif
        
        //最后删除user
        DeleteUser(m_userID);
    }

    return 0;
}

UserOutTimer::~UserOutTimer()
{
}


int ServiceOutTimer::do_next_step(string& req_data)
{
    m_service_time_gap = atoi(req_data.c_str());

    if (on_service_timeout())
    {
        return -1;
    }
    else
    {
        return 1;///important
    }
}

int ServiceOutTimer::on_service_timeout()
{
    CAppConfig::Instance()->GetTimeoutServices(m_service_time_gap, m_serviceList);
    if (m_serviceList.size() <= 0)
    {
        LogTrace("no service timeout, do nothing!");
        return 0;
    }
    LogTrace("Num of timeout services: %d", m_serviceList.size());

    set<string>::iterator it;
    for (it = m_serviceList.begin(); it != m_serviceList.end(); it++)
    {
        string servID = *it;

        #if 0
        LogTrace("Goto delete timeout service[%s]", servID.c_str());
        
        //获取service
        ServiceInfo serv;
        DO_FAIL(mGetService(servID, serv));
        //删除service
        DO_FAIL(DeleteService(servID));
        //更新tagServiceHeap
        m_appID = getappID(servID);
        DO_FAIL(CAppConfig::Instance()->DelServiceFromTags(m_appID, serv));
        //更新onlineServiceNum
        DO_FAIL(DelTagOnlineServNum(m_appID, serv));

        #else
        LogTrace("Goto force-offline timeout-service[%s]", servID.c_str());
        ServiceInfo serv;
        DO_FAIL(mGetService(servID, serv));
        //若service已经offline，无需再强迫下线
        if (OFFLINE == serv.status)
        {
            LogTrace("service[%s] is offline already, no need to force-offline.", servID.c_str());
            continue;
        }
        //先更新onlineServiceNum
        m_appID = getappID(servID);
        DO_FAIL(DelTagOnlineServNum(m_appID, serv));
        //再更新service
        serv.status    = OFFLINE;
        //subStatus设为默认值'离线'
        serv.subStatus = SUB_LIXIAN;
        DO_FAIL(UpdateService(servID, serv));
        #endif
    }

    return 0;
}

ServiceOutTimer::~ServiceOutTimer()
{
}


int SessionOutTimer::do_next_step(string& req_data)
{
    m_appID = req_data;
    if (on_session_timeout())
    {
        return -1;
    }
    else
    {
        return 1;///important
    }
}

int SessionOutTimer::on_send_timeout_msg()
{
    Json::Value data;

    //if(m_session.whereFrom == "wx" || m_session.whereFrom == "wxpro")
    //{
        data["des"] = CAppConfig::Instance()->getTimeOutHint(m_appID);
    //}

    data["userID"]    = m_raw_userID;
    data["serviceID"] = m_raw_serviceID;
    data["sessionID"] = m_sessionID;

    //发送超时断开给坐席
    data["identity"]  = "service";
    DO_FAIL(on_send_request("timeoutEnd", m_serviceInfo.cpIP, m_serviceInfo.cpPort, data, false));

    //发送超时断开给用户
    data["identity"]  = "user";
    DO_FAIL(on_send_request("timeoutEnd", m_userInfo.cpIP, m_userInfo.cpPort, data, false));

    return SS_OK;
}

int SessionOutTimer::on_session_timeout()
{
    SessionQueue* pSessQueue = NULL;
    SessionTimer sessTimer;
    Session sess;
    string new_sessionID;
    
    //choose first session  
    if (SS_OK != CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue) 
        || SS_OK != pSessQueue->get_first_timer(sessTimer))
    {
        //no session timeout yet
        return SS_OK;
    }

    sess = sessTimer.session;
    LogDebug("Choose timeout session: %s", sess.toString().c_str());
    if ("" == sess.serviceID || sessTimer.expire_time >= MAX_INT)
    {
        LogTrace("no need to send timeout");
        return SS_OK;
    }

    LogDebug("==>IN1");
    m_raw_userID    = sess.userID;
    m_raw_serviceID = sess.serviceID;
    m_userID        = m_appID + "_" + m_raw_userID;
    m_serviceID     = m_appID + "_" + m_raw_serviceID;
    if (mGetService(m_serviceID, m_serviceInfo) 
        || m_serviceInfo.find_user(m_raw_userID))
    {
        //LogError("Failed to find user[%s] in service[%s], panic!!!", m_raw_userID.c_str(), m_serviceID.c_str());
        //ON_ERROR_GET_DATA("service"); //need not send packet
        return SS_ERROR;
    }
    GET_USER(mGetUser(m_userID, m_userInfo));

    LogDebug("==>IN2");
    //1.记录旧的sessionID，以发送timeout报文
    //2.set lastServiceID
    m_sessionID              = sess.sessionID;
    m_userInfo.lastServiceID = sess.serviceID;
    //3.删除旧session，创建新session
    if (sess.has_refreshed())
    {
        sess.notified = 0;
    }
    new_sessionID  = gen_sessionID(m_userID);
    sess.sessionID = new_sessionID;
    sess.serviceID = "";
    sess.atime     = sess.btime = GetCurTimeStamp();
    DO_FAIL(DeleteUserSession(m_appID, m_userID));
    DO_FAIL(CreateUserSession(m_appID, m_userID, &sess, MAX_INT, MAX_INT));

    //4.更新user
    m_userInfo.status    = IN_YIBOT;
    m_userInfo.sessionID = new_sessionID;
    DO_FAIL(UpdateUser(m_userID, m_userInfo));
    
    //5.更新service
    SET_SERV(m_serviceInfo.delete_user(m_raw_userID));
    DO_FAIL(UpdateService(m_serviceID, m_serviceInfo));
    
    return on_send_timeout_msg();
}

SessionOutTimer::~SessionOutTimer()
{
}


int SessionWarnTimer::do_next_step(string& req_data)
{
    m_appID = req_data;
    if (on_session_timewarn())
    {
        return -1;
    }
    else
    {
        return 1;///important
    }
}

int SessionWarnTimer::on_send_timewarn_msg()
{
    Json::Value data;

    //if(m_session.whereFrom == "wx" || m_session.whereFrom == "wxpro")
    //{
        data["des"] = CAppConfig::Instance()->getTimeWarnHint(m_appID);
    //}
    
    //发送超时提醒给坐席
    data["userID"]    = m_raw_userID;
    data["serviceID"] = m_raw_serviceID;
    data["sessionID"] = m_sessionID;
    
    data["identity"]  = "service";
    DO_FAIL(on_send_request("timeoutWarn", m_serviceInfo.cpIP, m_serviceInfo.cpPort, data, false));

    //发送超时提醒给用户
    data["identity"]  = "user";
    DO_FAIL(on_send_request("timeoutWarn", m_userInfo.cpIP, m_userInfo.cpPort, data, false));

    return SS_OK;
}

int SessionWarnTimer::on_session_timewarn()
{
    SessionQueue* pSessQueue = NULL;
    Session sess;
    
    if (SS_OK != CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue) 
        || pSessQueue->check_warn(sess))
    {
        //no session timewarn
        return SS_OK;
    }
    LogDebug("Choose timewarn session: %s", sess.toString().c_str());

    if ("" == sess.serviceID)
    {
        LogTrace("no need to send timewarn");
        return SS_OK;
    }

    m_raw_userID    = sess.userID;
    m_raw_serviceID = sess.serviceID;
    m_userID        = m_appID + "_" + m_raw_userID;
    m_serviceID     = m_appID + "_" + m_raw_serviceID;
    m_sessionID     = sess.sessionID;

    ///important
    int is_warn = 1;
    KV_set_session(m_userID, sess, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT, is_warn);

    if (mGetService(m_serviceID, m_serviceInfo) 
        || m_serviceInfo.find_user(m_raw_userID))
    {
        ON_ERROR_GET_DATA("user");
        LogTrace("no need to send timewarn");
        return SS_OK;
    }
    //获取user，以便发送timewarn报文
    GET_USER(mGetUser(m_userID, m_userInfo));

    #if 0
    if (m_whereFrom == "websocket")
    {
        MsgRetransmit::Instance()->SetMsg(strUserRsp, m_appID, ui2str(m_msg_seq), m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
    }
    #endif

    //发送超时提醒
    return on_send_timewarn_msg();
}

SessionWarnTimer::~SessionWarnTimer()
{
}


int QueueOutTimer::do_next_step(string& req_data)
{
    Json::Reader reader;
    Json::Value  data;
    
    LogDebug("req_data: %s", req_data.c_str());
    if (!reader.parse(req_data, data))
    {
        LogError("Failed to parse req_data!");
        return -1;
    }
    
    m_appID         = data["appID"].asString();
    m_raw_tag       = data["tag"].asString();
    m_queuePriority = data["queuePriority"].asUInt();
    LogDebug("m_appID: %s, m_raw_tag: %s, queuePriority: %d", m_appID.c_str(), m_raw_tag.c_str(), m_queuePriority);

    if (on_queue_timeout(req_data))
    {
        return -1;
    }
    else
    {
        return 1;///important
    }
}

int QueueOutTimer::on_queue_timeout(string &req_data)
{
    TagUserQueue *pTagQueues = NULL;
    UserQueue *uq = NULL;
    long long expire_time;
    UserInfo user;

    LogDebug("==>IN");

    if (1 == m_queuePriority)
    {
        DO_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagQueues));
    }
    else
    {
        DO_FAIL(CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues));
    }
    
    //get first user on queue
    if (SS_OK != pTagQueues->get_tag(m_raw_tag, uq)
        || SS_OK != uq->get_first(m_userID, expire_time)
        || SS_OK != mGetUser(m_userID, user))
    {
        LogError("[%s]: Error get queue or empty queue", m_appID.c_str());
        ON_ERROR_GET_DATA("user");
        return SS_ERROR;
    }
    
    //delete user from queue
    SET_USER(pTagQueues->del_user(m_raw_tag, m_userID));
    DO_FAIL(KV_set_queue(m_appID, m_raw_tag, m_queuePriority));
    
    //update user
    user.status = IN_YIBOT;
    user.qtime  = 0;
    DO_FAIL(UpdateUser(m_userID, user));
    
    //send timeoutDequeue msg to user
    Json::Value data;
    data["identity"]  = "user";
    data["userID"]    = delappID(m_userID);
    data["sessionID"] = user.sessionID;
    
    //if(m_whereFrom == "wx" || m_whereFrom == "wxpro")
    //{
        data["des"] = CAppConfig::Instance()->getQueueTimeoutHint(m_appID);
    //}

    LogDebug("==>OUT");

    return on_send_request("timeoutDequeue", user.cpIP, user.cpPort, data, false);
}

QueueOutTimer::~QueueOutTimer()
{
}


int UserServiceTimer::do_next_step(string& req_data)
{
    m_tag     = req_data;
    m_appID   = getappID(req_data);
    m_raw_tag = delappID(req_data); 
    //LogDebug("m_appID: %s, m_raw_tag: %s", m_appID.c_str(), m_raw_tag.c_str());
    
    if (on_offer_service())
    {
        return -1;
    }
    else
    {
        return 1;
    }
}


//分配tag内的一个坐席
int UserServiceTimer::on_user_tag()
{
    LogDebug("==>IN");
    
    ServiceHeap servHeap;
    if (CAppConfig::Instance()->GetTagServiceHeap(m_tag, servHeap))
    {
        LogError("Failed to get ServiceHeap of tag[%s]!", m_tag.c_str());
        return SS_ERROR;
    }

    #if 0
    //如果该tag内有熟客，优先分配tag内熟客
    if (0 == servHeap.find_service(m_lastServiceID) && SS_OK == on_user_lastService())
    {
        return SS_OK;
    }
    #endif

    //使用最小负载策略
    if (find_least_service_by_tag(m_tag, "", m_serviceInfo))
    {
        return SS_ERROR;
    }

    m_raw_serviceID = m_serviceInfo.serviceID;
    m_serviceID     = m_appID + "_" + m_raw_serviceID;
    LogTrace("Connect userID:%s <-----> tag serviceID:%s", m_userID.c_str(), m_serviceID.c_str());
    return SS_OK;
}

//给用户分配熟客
int UserServiceTimer::on_user_lastService()
{
    LogDebug("==>IN");

    DO_FAIL(mGetService(m_lastServiceID, m_serviceInfo));

    if (false == m_serviceInfo.is_available())
    {
        LogError("[%s]: Last service[%s] is offline/busy!", m_appID.c_str(), m_lastServiceID.c_str());
        return SS_ERROR;
    }

    if (false == m_serviceInfo.check_tag_exist(m_raw_tag))
    {
        LogError("[%s]: Last service[%s] has no tag[%s]!", m_appID.c_str(), m_lastServiceID.c_str(), m_raw_tag.c_str());
        return SS_ERROR;
    }
    
    m_serviceID     = m_lastServiceID;
    m_raw_serviceID = m_raw_lastServiceID;
    LogTrace("Connect userID:%s <-----> lastServiceID:%s", m_userID.c_str(), m_lastServiceID.c_str());
    return SS_OK;
}

//随机分配一个坐席
int UserServiceTimer::on_user_common()
{
    LogDebug("==>IN");

    string strTags;
    CAppConfig::Instance()->GetValue(m_appID, "tags", strTags);
    LogDebug("[%s]: strTags:%s", m_appID.c_str(), strTags.c_str());

    vector<string> tags;
    MySplitTag((char *)strTags.c_str(), ";", tags);
    
    srand((unsigned)time(NULL));
    int j = rand() % tags.size();
    for (int i = j; i < tags.size(); )
    {
        LogTrace("==>tags[%d]: %s", i, tags[i].c_str());
        
        ServiceHeap servHeap;
        if (CAppConfig::Instance()->GetTagServiceHeap(tags[i], servHeap))
        {
            LogWarn("Failed to get ServiceHeap of tag: %s, find next ServiceHeap!", tags[i].c_str());
            continue;
        }
        
        for (set<string>::iterator it = servHeap._servlist.begin(); it != servHeap._servlist.end(); it++)
        {
            if (mGetService(*it, m_serviceInfo))
            {
                continue;
            }

            if (false == m_serviceInfo.is_available())
            {
                LogWarn("Service[%s] is offline/busy, find next service!", (*it).c_str());
                continue;
            }

            m_serviceID     = *it;
            m_raw_serviceID = delappID(m_serviceID);
            LogTrace("Connect userID:%s <-----> common serviceID:%s", m_userID.c_str(), m_serviceID.c_str());
            return SS_OK;
        }

        i++;
        if (i == tags.size())
        {
            i = 0;
        }
        
        if (i == j)
        {
            break;
        }
    }

    LogDebug("==>OUT");
    
    return SS_ERROR;
}

int UserServiceTimer::on_dequeue_first_user()
{
    long long expire_time;
    UserQueue *uq = NULL;

    //从m_raw_tag排队队列弹出一个user
    if (0 == get_highpri_queue(m_appID, m_raw_tag, &uq)
        && 0 == uq->get_first(m_userID, expire_time)
        && 0 == mGetUser(m_userID, m_userInfo))
    {
        m_queuePriority = 1;
        LogTrace("[%s]: Dequeue user[%s] from HighPriQueue.", m_appID.c_str(), m_userID.c_str());
    }
    else if (0 == get_normal_queue(m_appID, m_raw_tag, &uq)
            && 0 == uq->get_first(m_userID, expire_time)
            && 0 == mGetUser(m_userID, m_userInfo))
    {
        m_queuePriority = 0;
        LogTrace("[%s]: Dequeue user[%s] from NormalQueue.", m_appID.c_str(), m_userID.c_str());
    }
    else
    {
        LogError("[%s]: Failed to dequeue user!", m_appID.c_str());
        return SS_ERROR;
    }

    //获取user信息
    m_raw_userID        = delappID(m_userID);
    m_sessionID         = m_userInfo.sessionID;
    m_channel           = m_userInfo.channel;
    m_extends           = m_userInfo.extends;
    //get lastServiceID
    m_raw_lastServiceID = m_userInfo.lastServiceID;
    m_lastServiceID     = m_appID + "_" + m_raw_lastServiceID;

    //获取user的session，后续使用
    GET_SESS(get_user_session(m_appID, m_userID, &m_session));

    //排队队列删除user
    SET_USER(uq->delete_user(m_userID));
    DO_FAIL(KV_set_queue(m_appID, m_raw_tag, m_queuePriority));
    
    //更新user
    m_userInfo.status = IN_YIBOT;
    m_userInfo.qtime  = 0;
    DO_FAIL(UpdateUser(m_userID, m_userInfo));

    LogDebug("==>OUT");
    return SS_OK;
}


int UserServiceTimer::on_offer_service()
{
    if (CAppConfig::Instance()->CanAppOfferService(m_appID))
    {
        //LogTrace("====> No need to dequeue user from queue.");
        return SS_OK;
    }

    DO_FAIL(on_dequeue_first_user());

    if ("lastServiceID" == m_userInfo.priority)
    {
        LogDebug("[%s]: 1 try <lastService>", m_appID.c_str());

        //1.分配熟客
        if (on_user_lastService())
        {
            LogDebug("[%s]: 2 lastService is offline/busy, try <tagService>", m_appID.c_str());
            //2.按tag分配
            if (on_user_tag())
            {
                //3.随机分配
                LogDebug("[%s]: 3 tagService not available, try <randService>", m_appID.c_str());
                if (on_user_common())
                    return SS_ERROR;
            }
        }
    }
    else
    {
        LogDebug("[%s]: 1 try <tagService>", m_appID.c_str());
        //1.按tag分配
        if (on_user_tag())
        {
            //2.分配熟客
            LogDebug("[%s]: 2 tagService not available, try <lastService>", m_appID.c_str());
            if (on_user_lastService())
            {
                //3.随机分配
                LogDebug("[%s]: 3 lastService is offline/busy, try <randService>", m_appID.c_str());
                if (on_user_common())
                    return SS_ERROR;
            }
        }
    }

    //创建会话，发送ConnectSuccess消息
    DO_FAIL(on_create_session());
    DO_FAIL(on_send_connect_success());
    return SS_OK;
}

int UserServiceTimer::on_create_session()
{
    //更新service
    SET_SERV(m_serviceInfo.add_user(m_raw_userID));
    DO_FAIL(UpdateService(m_serviceID, m_serviceInfo));

    //更新session
    m_session.serviceID = m_raw_serviceID;
    m_session.atime     = GetCurTimeStamp();
    DO_FAIL(UpdateUserSession(m_appID, m_userID, &m_session));
    
    //更新user
    m_userInfo.status = IN_SERVICE;
    m_userInfo.qtime  = 0;
    m_userInfo.atime  = m_session.atime;
    DO_FAIL(UpdateUser(m_userID, m_userInfo));
    
    LogTrace("Success to create new session: %s", m_session.toString().c_str());
    return SS_OK;
}

int UserServiceTimer::on_send_connect_success()
{
    LogDebug("==>IN");

    //connectSuccess消息的data字段包含的是service的信息
    Json::Value sessData;
    sessData["userID"]    = m_session.userID;
    sessData["serviceID"] = m_raw_serviceID;
    sessData["sessionID"] = m_sessionID;
    sessData["channel"]   = m_channel;

    //解析extends
    Json::Reader reader;
    Json::Value json_extends;
    if (!reader.parse(m_userInfo.extends, json_extends))
    {
        sessData["extends"]   = Json::objectValue;
    }
    else
    {
        sessData["extends"]   = json_extends;
    }
    
    sessData["serviceName"]   = m_serviceInfo.serviceName;
    sessData["serviceAvatar"] = m_serviceInfo.serviceAvatar;

    //发送给user
    sessData["identity"] = "user";
    DO_FAIL(on_send_request("connectSuccess", m_session.cpIP, m_session.cpPort, sessData, true));

    #if 0
    if (m_whereFrom == "websocket" || m_session.whereFrom == "iOS" || m_session.whereFrom == "Android")
    {
        MsgRetransmit::Instance()->SetMsg(strServiceRsp, m_appID, ui2str(m_msg_seq), m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
    }
    #endif
    
    //发送给service
    sessData["identity"] = "service";
    DO_FAIL(on_send_request("connectSuccess", m_serviceInfo.cpIP, m_serviceInfo.cpPort, sessData, true));

    LogDebug("==>OUT");
    return SS_OK;
}

UserServiceTimer::~UserServiceTimer()
{
    
}



int RefreshSessionTimer::do_next_step(string& req_data)
{
    if (init(req_data, req_data.size()))
    {
        ON_ERROR_PARSE_PACKET();
        return -1;
    }
    
    if (on_refresh_session())
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

/*
m_userID
*/
int RefreshSessionTimer::on_refresh_session()
{
    //get session
    Session sess;
    GET_SESS(get_user_session(m_appID, m_userID, &sess));

    //update session.atime
    sess.atime = GetCurTimeStamp();
    DO_FAIL(UpdateUserSession(m_appID, m_userID, &sess));

    //update user.atime
    UserInfo user;
    DO_FAIL(mGetUser(m_userID, user));
    user.atime = sess.atime;
    DO_FAIL(UpdateUser(m_userID, user));

    //update service.atime
    if ("" != sess.serviceID)
    {
        m_serviceID = m_appID + "_" + sess.serviceID;
        ServiceInfo serv;
        DO_FAIL(mGetService(m_serviceID, serv));
        serv.atime = sess.atime;
        DO_FAIL(UpdateService(m_serviceID, serv));
    }
    
    //send reply
    Json::Value data = Json::objectValue;
    DO_FAIL(on_send_reply(data));
    return SS_OK;
}

RefreshSessionTimer::~RefreshSessionTimer()
{
}

