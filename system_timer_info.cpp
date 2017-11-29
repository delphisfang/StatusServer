#include "system_timer_info.h"
#include "statsvr_mcd_proc.h"

#include <algorithm>

using namespace statsvr;

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
	UserInfo user;

	data["userID"]    = m_raw_userID;
	data["serviceID"] = m_raw_serviceID;
	data["sessionID"] = m_sessionID;
		
	//发送超时断开给坐席
	data["identity"]  = "service";
	DO_FAIL(on_send_request("timeoutEnd", m_serviceInfo.cpIP, m_serviceInfo.cpPort, data, false));

	//发送超时断开给用户
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	data["identity"]  = "user";
	DO_FAIL(on_send_request("timeoutEnd", user.cpIP, user.cpPort, data, false));

	return SS_OK;
}

int SessionOutTimer::on_session_timeout()
{
    SessionQueue* pSessQueue = NULL;
	Session sess;
	
	//choose first session	
    if (SS_OK != CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue) 
		|| SS_OK != pSessQueue->get_first(sess))
    {
		//no session timeout yet
        return SS_OK;
    }

	LogDebug("==>Choose timeout session: %s", sess.toString().c_str());

	if ("" == sess.serviceID)
	{
		//LogTrace("no need to send timeout");
		return SS_OK;
	}

    m_raw_userID    = sess.userID;
    m_raw_serviceID = sess.serviceID;
	m_userID        = m_appID + "_" + m_raw_userID;
	m_serviceID     = m_appID + "_" + m_raw_serviceID;
    m_sessionID     = sess.sessionID;

    if (CAppConfig::Instance()->GetService(m_serviceID, m_serviceInfo) 
    	|| m_serviceInfo.find_user(m_raw_userID))
    {
        LogError("Failed to find user[%s] in service[%s]", m_raw_userID.c_str(), m_serviceID.c_str());
		ON_ERROR_GET_DATA("service");
        pSessQueue->pop();
        return SS_ERROR;
    }

	LogDebug("==>IN3");

	//更新user，设置熟客ID
	UserInfo user;
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	user.status = "inYiBot";
	user.lastServiceID = sess.serviceID;
	DO_FAIL(UpdateUser(m_userID, user));
	
	//更新service
	//delete user from service.userList
	SET_SERV(m_serviceInfo.delete_user(m_raw_userID));
	DO_FAIL(UpdateService(m_serviceID, m_serviceInfo));
	
	LogDebug("==>IN4");
		
	//删除session or 更新session.serviceID
	#if 0
	pSessQueue->pop();
	#else
	sess.serviceID = "";
	sess.sessionID = m_userID + "_" + l2str(GetCurTimeStamp());
	SET_SESS(pSessQueue->set(m_userID, &sess, MAX_INT, MAX_INT));
	DO_FAIL(KV_set_session(m_userID, sess, MAX_INT, MAX_INT));
	#endif
	
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
	UserInfo user;

	LogDebug("==>IN");
	
	//发送超时提醒给坐席
	data["userID"]    = m_raw_userID;
	data["serviceID"] = m_raw_serviceID;
	data["sessionID"] = m_sessionID;
	
	data["identity"]  = "service";
	DO_FAIL(on_send_request("timeoutWarn", m_serviceInfo.cpIP, m_serviceInfo.cpPort, data, false));

	//发送超时提醒给用户
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	data["identity"]  = "user";
	DO_FAIL(on_send_request("timeoutWarn", user.cpIP, user.cpPort, data, false));

	LogDebug("==>OUT");
	return SS_OK;
}

int SessionWarnTimer::on_session_timewarn()
{
    SessionQueue* pSessQueue = NULL;
	Session sess;
	Json::Value data;
	
	//LogDebug("==>IN");

    if (SS_OK != CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue) 
		|| pSessQueue->check_warn(sess))
    {
		//no session timewarn yet
        return SS_OK;
    }
	LogDebug("==>session: %s", sess.toString().c_str());

	if ("" == sess.serviceID)
	{
		LogTrace("no need to send timewarn");
		return SS_OK;
	}
	
    m_raw_userID = sess.userID;
    m_raw_serviceID = sess.serviceID;
	m_userID     = m_appID + "_" + m_raw_userID;
	m_serviceID  = m_appID + "_" + m_raw_serviceID;
    m_sessionID  = sess.sessionID;

    if (CAppConfig::Instance()->GetService(m_serviceID, m_serviceInfo) 
    	|| m_serviceInfo.find_user(m_raw_userID))
    {
        ON_ERROR_GET_DATA("user");
		LogTrace("no need to send timewarn");
        return SS_OK;   //continue check warn
    }

	//if(m_session.whereFrom == "wx" || m_session.whereFrom == "wxpro")
    //{
        data["des"] = CAppConfig::Instance()->getTimeWarnHint(m_appID);
    //}

	#if 0
    if (m_whereFrom == "websocket")
    {
        MsgRetransmit::Instance()->SetMsg(strUserRsp, m_appID, ui2str(m_msg_seq), m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
    }
	#endif

	LogDebug("==>OUT");

	//发送超时提醒给用户和坐席
	return on_send_timewarn_msg();
}

SessionWarnTimer::~SessionWarnTimer()
{
}


int QueueOutTimer::do_next_step(string& req_data)
{
	LogDebug("req_data: %s", req_data.c_str());
	m_appID   = getappID(req_data);
	m_raw_tag = delappID(req_data);
	LogDebug("m_appID: %s, m_raw_tag: %s", m_appID.c_str(), m_raw_tag.c_str());

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
	Json::Value data;
	string userID;
	UserInfo user;

	LogDebug("==>IN");
	
	//get first user on queue
    if (SS_OK != CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues) 
		|| SS_OK != pTagQueues->get_tag(m_raw_tag, uq)
		|| SS_OK != uq->get_first(userID)
		|| SS_OK != CAppConfig::Instance()->GetUser(userID, user))
    {
        LogError("[%s]: Error get queue or empty queue\n", m_appID.c_str());
		ON_ERROR_GET_DATA("user");
        return SS_ERROR;
    }
	
	//delete user from queue
	SET_USER(pTagQueues->del_user(m_raw_tag, userID));
	///TODO: highpri?
	DO_FAIL(KV_set_queue(m_appID, m_raw_tag, false));
	
	//update user
	user.status = "inYiBot";
	user.qtime  = 0;
	DO_FAIL(UpdateUser(userID, user));
	
	//send timeoutDequeue msg to user
	set_user_data(data);
    //if(m_whereFrom == "wx" || m_whereFrom == "wxpro")
    //{
        string queue_timeout_hint = "";
        CAppConfig::Instance()->GetValue(m_appID, "queue_timeout_hint", queue_timeout_hint);
        data["des"] = queue_timeout_hint;
    //}

	LogDebug("==>OUT");

	return on_send_request("timeoutDequeue", user.cpIP, user.cpPort, data, false);
}

QueueOutTimer::~QueueOutTimer()
{
}


int UserServiceTimer::do_next_step(string& req_data)
{
	m_appID   = getappID(req_data);
	m_raw_tag = delappID(req_data); 
	LogDebug("m_appID: %s, m_raw_tag: %s", m_appID.c_str(), m_raw_tag.c_str());
	
#if 0
	ServiceHeap offlineHeap;
	CAppConfig::Instance()->GetOfflineHeap(m_appID, offlineHeap);
	
	if (offlineHeap.size() == 0 
		|| CAppConfig::Instance()->GetOfflineNeedDelServiceList(offlineHeap) == 0)
	{
		DEBUG_P(LOG_TRACE, "[%u]:Offline Heap have no service to delete:%s", m_appID, offlineHeap.toString().c_str());
		m_cur_step = STATE_END;
		return -1;
	}
#endif
	
	if (on_offer_service())
	{
		return -1;
	}
	else
	{
		return 1; ///important
	}
}

int UserServiceTimer::on_create_session()
{
	LogDebug("==>IN");

	//确定session的serviceID
	m_session.serviceID = delappID(m_serviceID);
	m_session.atime 	= m_session.btime = GetCurTimeStamp();
	DO_FAIL(UpdateUserSession(m_appID, m_userID, &m_session, DEF_SESS_TIMEWARN, DEF_SESS_TIMEWARN));
	
	//更新user状态
	UserInfo user;
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	user.status = "inService";
	user.qtime  = 0;
	user.atime  = GetCurTimeStamp();
	DO_FAIL(UpdateUser(m_userID, user));
	
	LogTrace("Success to create new session: %s", m_session.toString().c_str());
	return SS_OK;
}


//随机分配tag内的一个坐席
int UserServiceTimer::on_user_tag()
{
    ServiceHeap servHeap;
	set<string>::iterator it;

	LogDebug("==>IN");
	
	if (CAppConfig::Instance()->GetTagServiceHeap(m_userInfo.tag, servHeap))
    {
        LogError("Failed to  get ServiceHeap of tag[%s]", m_userInfo.tag.c_str());
        return SS_ERROR;
    }

	//如果有熟客，依然分配熟客
    if (servHeap.find(m_lastServiceID) && SS_OK == on_user_lastService())
    {
        return SS_OK;
    }

	//随机分配一个坐席
    it = servHeap._servlist.begin();
    srand((unsigned)time(NULL));
    int j = rand() % servHeap.size();
    for (int k = 0; k < j; k++)
        it++;
	
    for (int i = j; ; )
    {
        ServiceInfo serv;
		//如果坐席的服务人数已满，就分配下一个坐席
        if (CAppConfig::Instance()->GetService(*it, serv) || serv.user_count() >= m_serverNum)
        {
            i++;
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
            m_serviceID = *it;
            LogTrace("Connect userID:%s <-----> tag serviceID:%s", m_userID.c_str(), m_serviceID.c_str());

			//将user添加到service.userList
            m_serviceInfo = serv;
			SET_SERV(serv.add_user(delappID(m_userID)));
			DO_FAIL(UpdateService(m_serviceID, serv));
			
			//创建会话，发送ConnectSuccess消息
			DO_FAIL(on_create_session());
			DO_FAIL(on_send_connect_success_msg());
            return SS_OK;
        }
    }

	LogDebug("==>OUT");
	
    return SS_ERROR;
}

//给用户分配熟客
int UserServiceTimer::on_user_lastService()
{
    ServiceInfo lastServ;

	LogDebug("==>IN");

    if (SS_OK != CAppConfig::Instance()->GetService(m_userInfo.lastServiceID, lastServ)
    	|| lastServ.status == "offline")
    {
        LogError("[%s]: Last service:%s is offline", m_appID.c_str(), m_userInfo.lastServiceID.c_str());
        return SS_ERROR;
    }

    if (lastServ.user_count() > m_serverNum)
    {
        LogError("[%s]: Last service:%s is busy", m_appID.c_str(), m_userInfo.lastServiceID.c_str());
        return SS_ERROR;
    }

    m_serviceID   = m_userInfo.lastServiceID;
    LogTrace("Connect userID:%s <-----> lastServiceID:%s", m_userID.c_str(), m_serviceID.c_str());

    m_serviceInfo = lastServ;
	LogTrace("m_serviceInfo: %s", m_serviceInfo.toString().c_str());
	SET_SERV(m_serviceInfo.add_user(delappID(m_userID)));
	DO_FAIL(UpdateService(m_serviceID, m_serviceInfo));

	DO_FAIL(on_create_session());
	DO_FAIL(on_send_connect_success_msg());

	LogDebug("==>OUT");

    return SS_OK;
}

//随机分配一个坐席
int UserServiceTimer::on_user_common()
{
    string strTags;
    vector<string> tags;

	LogDebug("==>IN");

	CAppConfig::Instance()->GetValue(m_appID, "tags", strTags);
	LogDebug("[%s]: strTags:%s", m_appID.c_str(), strTags.c_str());
    MySplitTag((char *)strTags.c_str(), ";", tags);
	
	srand((unsigned)time(NULL));
    int j = rand() % tags.size();
    for (int i = j; i < tags.size(); )
    {
		LogTrace("==>tags[%d]: %s", i, tags[i].c_str());
		
        ServiceHeap servHeap;
        if (CAppConfig::Instance()->GetTagServiceHeap(tags[i], servHeap))
        {
			LogWarn("Failed to get service heap for tag: %s, find next service heap!", tags[i].c_str());
            continue;
        }
		
        for (set<string>::iterator it = servHeap._servlist.begin(); it != servHeap._servlist.end(); it++)
        {
            ServiceInfo serv;

			if (CAppConfig::Instance()->GetService(*it, serv) || serv.status == "offline" || serv.user_count() >= m_serverNum)
            {
				LogWarn("Service[%s] is offline or busy, find next service!", serv.serviceID.c_str());
                continue;
            }
			
            m_serviceID = *it;
            LogTrace("Connect userID:%s <-----> random serviceID:%s", m_userID.c_str(), m_serviceID.c_str());

			//将user添加到service.userList
            m_serviceInfo = serv;
			SET_SERV(serv.add_user(delappID(m_userID)));
			DO_FAIL(UpdateService(m_serviceID, serv));

			//创建会话，发送ConnectSuccess消息
			DO_FAIL(on_create_session());
			DO_FAIL(on_send_connect_success_msg());
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
	LogDebug("==>IN");

    LogDebug("[%s]: start connect service", m_appID.c_str());
	string userID;
	UserQueue *uq = NULL;
	
    if (0 == get_highpri_queue(m_appID, m_raw_tag, &uq)
		&& 0 == uq->get_first(userID)
		&& 0 == CAppConfig::Instance()->GetUser(userID, m_userInfo))
    {
		m_queuePriority = 1;
        LogTrace("[%s]: Success to get user from highpri queue.", m_appID.c_str());
    }
    else if (0 == get_normal_queue(m_appID, m_raw_tag, &uq)
			&& 0 == uq->get_first(userID)
			&& 0 == CAppConfig::Instance()->GetUser(userID, m_userInfo))
    {
		m_queuePriority = 0;
        LogTrace("[%s]: Success to get user from normal queue.", m_appID.c_str());
    }
	else
	{
        LogError("[%s]: Failed to get user from highpri and normal queue.", m_appID.c_str());
       	return SS_ERROR;
	}

	LogTrace("dequeue userID: %s", userID.c_str());

    m_userID        = userID;
    m_lastServiceID = m_userInfo.lastServiceID;
    m_sessionID     = m_userInfo.sessionID;
    m_channel       = m_userInfo.channel;
    m_extends       = m_userInfo.extends;

	//获取user的session
	GET_SESS(get_user_session(m_appID, m_userID, &m_session));

    if (CAppConfig::Instance()->GetValue(m_appID, "max_conv_num", m_serverNum) 
		|| 0 == m_serverNum)
    {
        m_serverNum = 5;
    }

	//弹出排队队列的队头，即第一个用户
    //uq->pop();
	SET_USER(uq->delete_user(userID));
	DO_FAIL(KV_set_queue(m_appID, m_raw_tag, m_queuePriority));
	
	//update user status
	UserInfo user;
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	user.status = "inYiBot";
	user.qtime  = 0;
	DO_FAIL(UpdateUser(m_userID, user));
	
	LogDebug("==>OUT");
	
    return SS_OK;
}

int UserServiceTimer::on_offer_service()
{
	LogDebug("==>IN");

	DO_FAIL(on_dequeue_first_user());

    LogDebug("[%s]: Start to try connect service", m_appID.c_str());
	
    if (m_userInfo.priority == "lastServiceID") //用户请求熟客优先
    {
        LogDebug("[%s]: 1 try <lastService>", m_appID.c_str());
        ServiceInfo serv;

		//1.给用户分配熟客
        if (CAppConfig::Instance()->GetService(m_lastServiceID, serv)
        	|| serv.status == "offline"
        	|| serv.user_count() >= m_serverNum 
        	|| on_user_lastService())
        {
            LogDebug("[%s]: 2 lastService is offline or busy, try <tagService>", m_appID.c_str());
            ServiceHeap serv_heap;
			//2.给用户按分组分配
            if (CAppConfig::Instance()->GetTagServiceHeap(m_userInfo.tag, serv_heap) 
            	|| CAppConfig::Instance()->CanOfferService(serv_heap, m_serverNum) 
            	|| on_user_tag())
            {
				//3.给用户随机分配
                LogDebug("[%s]: 3 tagService is not available, try <randService>", m_appID.c_str());
                return on_user_common();
            }
        }
    }
    else //用户请求分组优先
    {
        LogDebug("[%s]: 1 try <tagService>", m_appID.c_str());
        ServiceHeap serv_heap;

		//1.给用户按分组分配
        if (CAppConfig::Instance()->GetTagServiceHeap(m_userInfo.tag, serv_heap) 
        	|| CAppConfig::Instance()->CanOfferService(serv_heap, m_serverNum)
        	|| on_user_tag())
        {
            ServiceInfo serv;

			//2.给用户按照熟客分配
            LogDebug("[%s]: 2 tagService is not available, try <lastService>", m_appID.c_str());
            if (CAppConfig::Instance()->GetService(m_lastServiceID, serv)
            	|| serv.status == "offline"
            	|| serv.user_count() >= m_serverNum 
            	|| on_user_lastService())
            {
				//3.给用户随机分配
                LogDebug("[%s]: 3 lastService is not available, try <randService>", m_appID.c_str());
                return on_user_common();
            }
        }
    }

    return SS_OK;
}

int UserServiceTimer::on_send_connect_success_msg()
{
	Json::Value sessData;
    Json::Reader reader;
    Json::Value userInfo;
    Json::Value json_extends;

	LogDebug("==>IN");
	//connectSuccess消息的data字段包含的是service的信息
    sessData["userID"]    = m_session.userID;
    sessData["serviceID"] = delappID(m_serviceID);
    sessData["sessionID"] = m_sessionID;
    sessData["channel"]   = m_channel;
	#if 0
    reader.parse(m_extends, json_extends);
    sessData["extends"]   = json_extends;
	#else
	sessData["extends"]   = Json::objectValue;
	#endif
	
    sessData["serviceName"]   = m_serviceInfo.serviceName;
    sessData["serviceAvatar"] = m_serviceInfo.serviceAvatar;
    //reader.parse(m_userInfo, userInfo);
    //serviceData["userInfo"] = userInfo;

	//发送connectSuccess消息给user
    sessData["identity"] = "user";
	DO_FAIL(on_send_request("connectSuccess", m_session.cpIP, m_session.cpPort, sessData, true));

	#if 0
    if (m_whereFrom == "websocket" || m_session.whereFrom == "iOS" || m_session.whereFrom == "Android")
    {
        MsgRetransmit::Instance()->SetMsg(strServiceRsp, m_appID, ui2str(m_msg_seq), m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
    }
	#endif
	
	//发送connectSuccess消息给service
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
	Session sess;

	GET_SESS(get_user_session(m_appID, m_userID, &sess));

	//update session.activeTime
	sess.atime = GetCurTimeStamp();
	DO_FAIL(UpdateUserSession(m_appID, m_userID, &sess, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));

	Json::Value data = Json::objectValue;
	DO_FAIL(on_send_reply(data));
	
	return SS_OK;
}

RefreshSessionTimer::~RefreshSessionTimer()
{
}

