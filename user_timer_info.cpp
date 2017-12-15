#include "user_timer_info.h"
#include "statsvr_mcd_proc.h"

#include <algorithm>
#include "jsoncpp/json.h"

using namespace statsvr;

/* just for test */
int EchoTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_echo())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

/*
input: m_appID
*/
int EchoTimer::on_echo()
{
	//遍历用户列表、坐席列表
	Json::Value userListJson, servListJson, sessQueueJson, normalQueueJson, highpriQueueJson, onlineServNumJson;
	Json::Value data;

	LogTrace("Get appID[%s] data structures...", m_appID.c_str());
	
	CAppConfig::Instance()->getUserListJson(m_appID, userListJson);
	CAppConfig::Instance()->getServiceListJson(m_appID, servListJson);
	CAppConfig::Instance()->getSessionQueueJson(m_appID, sessQueueJson);
	CAppConfig::Instance()->getTagNormalQueueJson(m_appID, normalQueueJson);
	CAppConfig::Instance()->getTagHighPriQueueJson(m_appID, highpriQueueJson);
	CAppConfig::Instance()->getOnlineServiceNumJson(m_appID, onlineServNumJson);

	data["userList"] = userListJson;
	data["servList"] = servListJson;
	data["sessionList"] = sessQueueJson;
	data["normalQueue"] = normalQueueJson;
	data["highpriQueue"] = highpriQueueJson;
	data["onlineServNum"] = onlineServNumJson;
	
	return on_send_reply(data);
}

EchoTimer::~EchoTimer()
{
}


int GetUserInfoTimer::do_next_step(string& req_data)
{
    if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}

	if (on_get_userinfo())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int GetUserInfoTimer::on_not_online()
{
	Json::Value data;
	set_user_data(data);
	return on_send_error_reply(ERROR_USER_NOT_ONLINE, "User Not Online", data);
}

/* 
input: m_userID_list 
*/
int GetUserInfoTimer::on_get_userinfo()
{
	Json::Value data = Json::objectValue;
	Json::Value userInfoList;
	userInfoList.resize(0);
	Json::Value userJson;
	
    TagUserQueue* pTagQueues = NULL;
	UserInfo user;
    SessionQueue*  pSessQueue = NULL;
	Session sess;
	int queueRank = 0;

	LogDebug("userID count: %u", m_userID_list.size());
	int i = 0;
	for (set<string>::iterator it = m_userID_list.begin(); it != m_userID_list.end(); it++)
	{
		m_userID   = (*it);
		m_raw_userID = delappID(m_userID);
		LogDebug("try to get userInfo: %s", m_userID.c_str());

		if (CAppConfig::Instance()->GetUser(m_userID, user))
		{
			on_not_online();
			return SS_ERROR;
		}

		// get user queueRank
		if (SS_OK == CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagQueues)
			  && -1 != (queueRank = pTagQueues->find_user(m_userID))
		   )
		{
			LogTrace("user[%s] is on HighPriQueue", m_userID.c_str());
			get_user_json(m_appID, m_userID, user, userJson);
			userJson["status"] = "onQueue";
			userJson["queueRank"] = queueRank;
		}
		else if (SS_OK == CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues)
			  		&& -1 != (queueRank = pTagQueues->find_user(m_userID))
			    )
		{
			LogTrace("user[%s] is on NormalQueue", m_userID.c_str());
			get_user_json(m_appID, m_userID, user, userJson);
			userJson["status"] = "onQueue";
			userJson["queueRank"] = queueRank;
		}
		else if (SS_OK == CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue)
					&& SS_OK == pSessQueue->get(m_userID, sess)
					&& sess.serviceID != "")
		{
			LogTrace("user[%s] is inService", m_userID.c_str());
			construct_user_json(user, sess, userJson);
			userJson["status"]    = "inService";
			userJson["queueRank"] = 0;
		}
		else
		{
			LogTrace("user[%s] is inYiBot", m_userID.c_str());
			get_user_json(m_appID, m_userID, user, userJson);
			userJson["status"]	  = "inYiBot";
			userJson["queueRank"] = 0;
		}
		
		userInfoList[i] = userJson;

		//update session["notified"]
		if (1 == m_notify)
		{
			DO_FAIL(update_session_notified(m_appID, m_userID));
		}
		
		++i;
	}

	data["userInfo"] = userInfoList;
	return on_send_reply(data);
}

GetUserInfoTimer::~GetUserInfoTimer()
{
}


int UserOnlineTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_user_online())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

void UserOnlineTimer::set_user_fields(UserInfo &user)
{
	//don't set user["status"]
	user.atime   = GetCurTimeStamp();
	user.channel = m_channel;
	user.tag     = m_raw_tag;
	if ("" != m_extends)
	{
		user.extends = m_extends;
	}
}

/* 
input: m_userID 
*/
int UserOnlineTimer::on_user_online()
{
	UserInfo user;
	Json::Value data;
	Session sess;
	
	if (SS_OK == CAppConfig::Instance()->GetUser(m_userID, user))
	{
		if (m_cpIP == user.cpIP && m_cpPort == user.cpPort)
		{
			//update user
			LogDebug("user[%s] online in the same CP, update user info.", m_userID.c_str());
			set_user_fields(user);
			DO_FAIL(UpdateUser(m_userID, user));
			//send reply
			DO_FAIL(reply_user_json_A(m_appID, m_userID, user));

			//update session["notified"]
			DO_FAIL(update_session_notified(m_appID, m_userID));
			
			return SS_OK;
		}
		else
		{
			//send kickout msg, if wx, not kick
			LogDebug("user[%s] online in another CP, kick out.", m_userID.c_str());
			set_user_data(data);
			DO_FAIL(on_send_request("kickOut", user.cpIP, user.cpPort, data, false));
			//update user			
			LogDebug("update user[%s]'s info.", m_userID.c_str());
			set_user_fields(user);
			user.cpIP   = m_cpIP;
			user.cpPort = m_cpPort;
			DO_FAIL(UpdateUser(m_userID, user));

			//update session
			LogDebug("update user[%s]'s session.", m_userID.c_str());
			GET_SESS(get_user_session(m_appID, m_userID, &sess));
			sess.cpIP   = m_cpIP;
			sess.cpPort = m_cpPort;
			DO_FAIL(UpdateUserSession(m_appID, m_userID, &sess));

			//send reply
			DO_FAIL(reply_user_json_B(user, sess));

			//update session["notified"]
			DO_FAIL(update_session_notified(m_appID, m_userID));

			return SS_OK;
		}
	}
	else //user first online, create and add a new user
	{
		//create user
		LogDebug("Add new user: %s", m_userID.c_str());
		user = UserInfo(m_data);
		user.status    = "inYiBot";
		user.sessionID = gen_sessionID(m_userID);
		DO_FAIL(AddUser(m_userID, user));
		//create session
		LogDebug("Add session for user: %s", m_userID.c_str());
		sess.sessionID = user.sessionID;
		sess.userID    = m_raw_userID;
		sess.cpIP      = m_cpIP;
		sess.cpPort    = m_cpPort;
		sess.atime     = sess.btime = GetCurTimeStamp();
		sess.serviceID = "";/// no service yet
		sess.notified  = 0;
		DO_FAIL(CreateUserSession(m_appID, m_userID, &sess, MAX_INT, MAX_INT));
		
		//send reply
		DO_FAIL(reply_user_json_B(user, sess));

		//update session["notified"]
		DO_FAIL(update_session_notified(m_appID, m_userID));
		
		return SS_OK;
	}
}

UserOnlineTimer::~UserOnlineTimer()
{
}


int ConnectServiceTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}

	if (on_connect_service())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int ConnectServiceTimer::set_data(Json::Value &data)
{
	data["identity"]  = "user";
	data["userID"]    = m_raw_userID;
	data["sessionID"] = m_sessionID;
}

int ConnectServiceTimer::on_already_onqueue()
{
	Json::Value data;

	data["status"] = "AlreadyOnQueue";
	set_data(data);
	return on_send_error_reply(WARN_ALREADY_ONQUEUE, "Already onQueue", data);
}

int ConnectServiceTimer::on_no_service()
{
	Json::Value data;
	
	data["status"] = "NoService";
	set_data(data);
	//if(m_whereFrom == "wx" || m_whereFrom == "wxpro")
	//{
		data["des"] = CAppConfig::Instance()->getNoServiceOnlineHint(m_appID);
	//}
	return on_send_error_reply(ERROR_NO_SERVICE, "Reject Enqueue", data);
}

int ConnectServiceTimer::on_reject_enqueue()
{
	Json::Value data;

	data["status"] = "FullQueue";
	set_data(data);	
	//if(m_whereFrom == "wx" || m_whereFrom == "wxpro")
	//{
		data["des"] = CAppConfig::Instance()->getQueueUpperLimitHint(m_appID);
	//}
	return on_send_error_reply(ERROR_REJECT_ENQUEUE, "Reject Enqueue", data);
}

int ConnectServiceTimer::on_already_inservice()
{
	Json::Value data;
	
	data["status"] = "inService";
	set_data(data);
	return on_send_error_reply(WARN_ALREADY_INSERVICE, "Already inService", data);
}

int ConnectServiceTimer::on_service_with_noqueue(bool flag)
{
	Json::Value data;

	set_data(data);
	
	if (flag)
	{
		data["status"] = "inServiceNow";
		return on_send_error_reply(SERVICE_WITH_NO_QUEUE, "OK", data);
	}
	else
	{
		data["status"] = "onQueue";
		return on_send_error_reply(ERROR_NO_ERROR, "OK", data);
	}
}

int ConnectServiceTimer::on_appoint_service_offline()
{
	Json::Value data;
	data["userID"]    = m_raw_userID;
	data["serviceID"] = m_raw_appointServiceID;
	data["status"]    = "ServiceOffLine";
	return on_send_error_reply(ERROR_NO_ERROR, "ServiceOffLine", data);
}

int ConnectServiceTimer::on_send_connect_success(const Session &sess, const ServiceInfo &serv)
{
	Json::Value sessData;
    
	LogDebug("==>IN");
	//connectSuccess消息的data字段包含的是service的信息
    sessData["userID"]    = sess.userID;
    sessData["serviceID"] = sess.serviceID;
    sessData["sessionID"] = sess.sessionID;
    sessData["channel"]   = m_channel;
	#if 0
	Json::Value json_extends;
    reader.parse(m_extends, json_extends);
    sessData["extends"]   = json_extends;
	#else
	sessData["extends"]   = Json::objectValue;
	#endif
	
    sessData["serviceName"]   = serv.serviceName;
    sessData["serviceAvatar"] = serv.serviceAvatar;
	//Json::Reader reader;
    //reader.parse(m_userInfo, userInfo);
    //serviceData["userInfo"] = userInfo;

	//发送connectSuccess消息给user
    sessData["identity"] = "user";
	DO_FAIL(on_send_request("connectSuccess", sess.cpIP, sess.cpPort, sessData, true));

	#if 0
    if (m_whereFrom == "websocket" || m_session.whereFrom == "iOS" || m_session.whereFrom == "Android")
    {
        MsgRetransmit::Instance()->SetMsg(strServiceRsp, m_appID, ui2str(m_msg_seq), m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
    }
	#endif
	
	//发送connectSuccess消息给service
    sessData["identity"] = "service";
	DO_FAIL(on_send_request("connectSuccess", serv.cpIP, serv.cpPort, sessData, true));

	LogDebug("==>OUT");
	return SS_OK;
}

int ConnectServiceTimer::on_appoint_service()
{
	if ("" == m_raw_appointServiceID)
	{
		LogTrace("[%s]: appointServiceID is empty, do nothing!", m_appID.c_str());
		return SS_OK;
	}
	
	//无需判定坐席的服务上限，直接服务
	ServiceInfo serv;
	if (CAppConfig::Instance()->GetService(m_appointServiceID, serv)
		|| "offline" == serv.status)
	{
		LogTrace("[%s]: appointService[%s] is offline!", m_appID.c_str(), m_appointServiceID.c_str());
		return on_appoint_service_offline();
	}

	//发送connectService-reply报文，让用户排队
	DO_FAIL(on_service_with_noqueue(false));
	
	//更新session
	Session sess;
	GET_SESS(get_user_session(m_appID, m_userID, &sess));
	sess.serviceID = m_raw_appointServiceID;
	sess.atime 	   = GetCurTimeStamp();
	DO_FAIL(UpdateUserSession(m_appID, m_userID, &sess));
	
	//更新user
	UserInfo user;
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	user.status = "inService";
	user.qtime  = 0;
	user.atime  = GetCurTimeStamp();
	DO_FAIL(UpdateUser(m_userID, user));

	//更新service
	SET_SERV(serv.add_user(m_raw_userID));
	DO_FAIL(UpdateService(m_serviceID, serv));
	LogTrace("Success to create new session: %s", sess.toString().c_str());

	//发送connectSuccess报文
	return on_send_connect_success(sess, serv);
}

int ConnectServiceTimer::on_queue()
{
	unsigned max_conv_num    = 0;
	long long queue_timeout  = 0;
	unsigned long max_queue_num   = 0;
	bool serviceWithNoQueue  = false;
    TagUserQueue* pTagQueues = NULL;
    TagUserQueue* pHighPriTagQueues = NULL;
	unsigned queue_count         = 0;
	unsigned highpri_queue_count = 0;
    unsigned serviceNum = 0;
	long long qtime = 0, expire_time = 0;
	
	LogDebug("==>IN");

	//指定客服
	if (true == m_has_appointServiceID)
	{
		return on_appoint_service();
	}
	
	max_conv_num  = CAppConfig::Instance()->getMaxConvNum(m_appID);
	serviceNum    = CAppConfig::Instance()->GetTagOnlineServiceNumber(m_appID, m_raw_tag);
	max_queue_num = serviceNum * max_conv_num * m_proc->m_cfg._queue_rate;
    LogTrace("[%s] serviceNum:%u, max_conv_num:%u, queue rate:%u, max_queue_num: %lu", 
    		m_appID.c_str(), serviceNum, max_conv_num, m_proc->m_cfg._queue_rate, max_queue_num);

	//无客服在线
	if (serviceNum <= 0)
	{
		return on_no_service();
	}

    if (CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pHighPriTagQueues) ||
    	CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues))
    {
		ON_ERROR_GET_DATA("tag user queues");
	    return SS_ERROR;
    }
	queue_count         = pTagQueues->queue_count(m_raw_tag);
	highpri_queue_count = pHighPriTagQueues->queue_count(m_raw_tag);
    LogTrace("[%s] queue size:%u, highpri queue size:%u", 
    		m_appID.c_str(), queue_count, highpri_queue_count);

	//排队人数已达上限
    if (queue_count + highpri_queue_count >= max_queue_num)
    {
        if (serviceNum <= 0)
			return on_no_service();
        else
			return on_reject_enqueue();
    }

	//该分组下没有用户在排队，可以直接转人工
    if (m_queuePriority != 0)
    {
        if (highpri_queue_count == 0 && max_queue_num > 0)
            serviceWithNoQueue = true;
        else
            serviceWithNoQueue = false;
    }
    else
    {
        if (queue_count == 0 && max_queue_num > 0)
            serviceWithNoQueue = true;
        else
            serviceWithNoQueue = false;
    }

	//所有的坐席都busy或offline时，仍然让用户排队，而不是直接转人工（返回71001）
	if (CAppConfig::Instance()->CanAppOfferService(m_appID))
	{
		LogTrace("====>let user on queue");
		serviceWithNoQueue = false;
	}
	
	//获取user
	UserInfo user;
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
    if ("" != m_channel)
		user.channel       = m_channel;
	if ("" != m_extends)
    {
		LogTrace("====> get user extends: %s", m_extends.c_str());
		user.extends       = m_extends;
	}
	if ("" != m_raw_tag)
    	user.tag           = m_raw_tag;
    if ("" != m_raw_lastServiceID)
    	user.lastServiceID = m_raw_lastServiceID;
    user.priority      = m_priority;
    user.queuePriority = m_queuePriority;
    user.atime = user.qtime = GetCurTimeStamp();///

	//将user插入排队队列
	LogDebug("Go to add user onQueue. tag: %s, queuePriority: %u, user: %s", m_raw_tag.c_str(), m_queuePriority, user.toString().c_str());
	qtime         = (user.qtime / 1000);
	queue_timeout = CAppConfig::Instance()->getDefaultQueueTimeout(m_appID);
	expire_time   = qtime + queue_timeout;
	LogDebug("qtime: %lu, queue_timeout: %lu", qtime, queue_timeout);
	
    if (m_queuePriority != 0)
    {
		LogTrace("====>add user on HighPri Queue.");
		DO_FAIL(pHighPriTagQueues->add_user(m_raw_tag, m_userID, expire_time));
		DO_FAIL(KV_set_queue(m_appID, m_raw_tag, true));
    }
    else
    {
		LogTrace("====>add user on Normal Queue.");
		DO_FAIL(pTagQueues->add_user(m_raw_tag, m_userID, expire_time));
		DO_FAIL(KV_set_queue(m_appID, m_raw_tag, false));
    }
	
	//更新user
	user.status = "onQueue";
	DO_FAIL(UpdateUser(m_userID, user));
	
	LogDebug("==>OUT");
    return on_service_with_noqueue(serviceWithNoQueue);
}

int ConnectServiceTimer::on_connect_service()
{
    TagUserQueue *pTagQueues = NULL;
	TagUserQueue *pTagHighPriQueues = NULL;
	SessionQueue *pSessQueue = NULL;
    Session sess;

	LogDebug("==>IN");

    if ((0 == CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues) && -1 != pTagQueues->find_user(m_userID)) 
		|| (0 == CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagHighPriQueues) && -1 != pTagHighPriQueues->find_user(m_userID)))
    {
        LogDebug("Already onQueue");
		on_already_onqueue();
		return SS_ERROR;
    }
    else if (SS_OK == get_user_session(m_appID, m_userID, &sess) && sess.serviceID != "")
    {
        LogDebug("Already inService");
		on_already_inservice();
		return SS_ERROR;
    }
    else
    {
        LogDebug("inYiBot, go to add user[%s] onqueue", m_userID.c_str());
		return on_queue();
    }
	
	LogDebug("==>OUT");
	return SS_OK;
}

ConnectServiceTimer::~ConnectServiceTimer()
{
}



int CancelQueueTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_cancel_queue())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int CancelQueueTimer::on_resp_cp()
{
	Json::Value data = Json::objectValue;
	return on_send_reply(data);
}

int CancelQueueTimer::on_not_onqueue()
{
	Json::Value data;
	set_user_data(data);
	return on_send_error_reply(WARN_NOT_ONQUEUE, "User Not onQueue", data);
}

int CancelQueueTimer::on_cancel_queue()
{
    TagUserQueue* pTagQueues = NULL;
	UserInfo user;
	int queueRank = -1;

	LogDebug("==>IN");	
	
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));

	if (user.status != "onQueue")
	{
		on_not_onqueue();
		return SS_ERROR;
	}

	if (SS_OK == CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagQueues) && -1 != pTagQueues->find_user(m_userID))
	{
		m_queuePriority = 1;
	}
	else if (SS_OK == CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues) && -1 != pTagQueues->find_user(m_userID))
	{
		m_queuePriority = 0;
	}
	else
	{
		on_not_onqueue();
		return SS_ERROR;
	}

	//delete user from queue
	m_raw_tag = user.tag;
	LogDebug("Delete user %s from tag queue: %s", m_raw_userID.c_str(), m_raw_tag.c_str());
	SET_USER(pTagQueues->del_user(m_raw_tag, m_userID));
	DO_FAIL(KV_set_queue(m_appID, m_raw_tag, m_queuePriority));
	
	//set user.status = "inYiBot"
	user.status = "inYiBot";
	user.qtime  = 0;
	DO_FAIL(UpdateUser(m_userID, user));

	on_resp_cp();
	LogDebug("==>OUT");
	return SS_OK;
}

CancelQueueTimer::~CancelQueueTimer()
{
}


int CloseSessionTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_close_session())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int CloseSessionTimer::on_closeSession_reply(const string &oldSessionID)
{
	Json::Value data;
	Json::Value userInfo;
	Json::Value servInfo;

	userInfo["ID"] = m_raw_userID;
	userInfo["chatProxyIp"] = m_session.cpIP;
	userInfo["chatProxyPort"] = m_session.cpPort;
	
	servInfo["ID"] = m_raw_serviceID;
	servInfo["chatProxyIp"] = m_serviceInfo.cpIP;
	servInfo["chatProxyPort"] = m_serviceInfo.cpPort;

	data["sessionID"] = oldSessionID;
	data["userInfo"] = userInfo;
	data["serviceInfo"] = servInfo;
	return on_send_reply(data);
}

int CloseSessionTimer::on_not_inservice()
{
	Json::Value data;
	set_user_data(data);
	return on_send_error_reply(WARN_NOT_INSERVICE, "Not inService", data);
}


/*
m_userID
*/
int CloseSessionTimer::on_close_session()
{
	UserInfo user;
	string oldSessionID;

	LogDebug("==>IN");	
	
	if (CI->GetUser(m_userID, user) || user.status != "inService")
	{
		on_not_inservice();
		return SS_ERROR;
	}
	
	GET_SESS(get_user_session(m_appID, m_userID, &m_session));
	LogTrace("get user[%s]'s session: %s", m_userID.c_str(), m_session.toString().c_str());
	m_raw_serviceID     = m_session.serviceID;
	m_serviceID         = m_appID + "_" + m_raw_serviceID;
	m_raw_lastServiceID = m_raw_serviceID;
	m_lastServiceID     = m_serviceID;
	
	//delete old session, create new session
	LogTrace("====>Delete old session: %s", m_session.toString().c_str());
	if (m_session.has_refreshed())
	{
		m_session.notified = 0;
	}
	m_session.serviceID = "";
	oldSessionID    = m_session.sessionID;
	user.sessionID  = m_session.sessionID = gen_sessionID(m_userID);
	m_session.atime = m_session.btime = GetCurTimeStamp();
	LogTrace("====>Create new session: %s", m_session.toString().c_str());
	DO_FAIL(DeleteUserSession(m_appID, m_userID));
	DO_FAIL(CreateUserSession(m_appID, m_userID, &m_session, MAX_INT, MAX_INT));

	//update service.userList
	GET_SERV(CI->GetService(m_serviceID, m_serviceInfo));
	SET_SERV(m_serviceInfo.delete_user(m_raw_userID));
	DO_FAIL(UpdateService(m_serviceID, m_serviceInfo));
	
	//update user
	user.status        = "inYiBot";
	user.lastServiceID = m_raw_lastServiceID;
	DO_FAIL(UpdateUser(m_userID, user));
	
	DO_FAIL(on_closeSession_reply(oldSessionID));
	LogDebug("==>OUT");
	return SS_OK;
}

CloseSessionTimer::~CloseSessionTimer()
{
}

