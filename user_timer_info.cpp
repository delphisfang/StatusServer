#include "user_timer_info.h"
#include "statsvr_mcd_proc.h"

#include <algorithm>
#include "jsoncpp/json.h"

using namespace statsvr;

/* just for test */
int EchoTimer::do_next_step(string& req_data)
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

			return on_echo();
        }
    }
	
    return 0;
}

/*
input: m_appID
*/
int EchoTimer::on_echo()
{
	//遍历用户列表、坐席列表
	Json::Value userListJson, servListJson, sessQueueJson, normalQueueJson, highpriQueueJson;
	Json::Value data;

	LogTrace("Get appID[%s] data structures...", m_appID.c_str());
	
	CAppConfig::Instance()->getUserListJson(m_appID, userListJson);
	CAppConfig::Instance()->getServiceListJson(m_appID, servListJson);
	CAppConfig::Instance()->getSessionQueueJson(m_appID, sessQueueJson);
	CAppConfig::Instance()->getTagNormalQueueJson(m_appID, normalQueueJson);
	CAppConfig::Instance()->getTagHighPriQueueJson(m_appID, highpriQueueJson);

	data["userList"] = userListJson;
	data["servList"] = servListJson;
	data["sessionList"] = sessQueueJson;
	data["normalQueue"] = normalQueueJson;
	data["highpriQueue"] = highpriQueueJson;
	
	//遍历排队队列、高优先级队列
	//遍历会话队列
	
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
	LogDebug("==>IN");
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
	int i = 0;

    TagUserQueue* pTagQueues = NULL;
	UserInfo user;
    SessionQueue*  pSessQueue = NULL;
	Session sess;
	int queueRank = 0;
	string app_userID;

	LogDebug("userID count: %u", m_userID_list.size());
	for (set<string>::iterator it = m_userID_list.begin(); it != m_userID_list.end(); it++)
	{
		app_userID = (*it);
		LogDebug("try to get userInfo: %s", app_userID.c_str());

		if (CAppConfig::Instance()->GetUser(app_userID, user))
		{
			on_not_online();
			return SS_ERROR;
		}

		// get user queueRank
		if (SS_OK == CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagQueues)
			  && -1 != (queueRank = pTagQueues->find_user(app_userID))
		   )
		{
			LogTrace("user[%s] is onHighPriQueue", app_userID.c_str());
			get_user_json(m_appID, app_userID, user, userJson);
			userJson["status"] = "onQueue";
			userJson["queueRank"] = queueRank;
		}
		else if (SS_OK == CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues)
			  		&& -1 != (queueRank = pTagQueues->find_user(app_userID))
			    )
		{
			LogTrace("user[%s] is onQueue", app_userID.c_str());
			get_user_json(m_appID, app_userID, user, userJson);
			userJson["status"] = "onQueue";
			userJson["queueRank"] = queueRank;
		}
		else if (SS_OK == CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue)
					&& SS_OK == pSessQueue->get(app_userID, sess)
					&& sess.serviceID != "")
		{
			LogTrace("user[%s] is inService", app_userID.c_str());
			construct_user_json(user, sess, userJson);
			userJson["status"]    = "inService";
			userJson["queueRank"] = 0;
		}
		else
		{
			LogTrace("user[%s] is inYiBot", app_userID.c_str());
			get_user_json(m_appID, app_userID, user, userJson);
			userJson["status"]	  = "inYiBot";
			userJson["queueRank"] = 0;
		}
		
		userInfoList[i] = userJson;
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

/* 
input: m_userID 
*/
int UserOnlineTimer::on_user_online()
{
	UserInfo user;
	Json::Value data;
	Session sess;

	LogDebug("==>IN");

	if (SS_OK == CAppConfig::Instance()->GetUser(m_userID, user))
	{
		if (m_cpIP == user.cpIP && m_cpPort == user.cpPort)
		{
			LogDebug("user[%s] online in the same CP.", m_userID.c_str());
			DO_FAIL(reply_user_json_A(m_appID, m_userID, user));
			return SS_OK;
		}
		else
		{
			LogDebug("user[%s] online in another CP, kick out.", m_userID.c_str());
			//send kickout msg
			//if wx, not kick
			set_user_data(data);
			DO_FAIL(on_send_request("kickOut", user.cpIP, user.cpPort, data, false));
			
			LogDebug("update user[%s]'s <cpIP, cpPort, atime>.", m_userID.c_str());
			user.cpIP   = m_cpIP;
			user.cpPort = m_cpPort;
			user.atime  = GetCurTimeStamp();
			DO_FAIL(UpdateUser(m_userID, user));
			
			LogDebug("update user[%s]'s session.", m_userID.c_str());
			GET_SESS(get_user_session(m_appID, m_userID, &sess));
			sess.cpIP   = m_cpIP;
			sess.cpPort = m_cpPort;
			DO_FAIL(UpdateUserSession(m_appID, m_userID, &sess, MAX_INT, MAX_INT));//超时时间设为无限大
		}
	}
	else //user first online, create and add a new user
	{
		user = UserInfo(m_data);
		LogDebug("Add new user: %s", user.toString().c_str());
		user.status = "inYiBot";
		user.sessionID = sess.sessionID = gen_sessionID(m_userID);
		DO_FAIL(AddUser(m_userID, user));

		LogDebug("Add session for user: %s", m_userID.c_str());
		sess.userID    = m_raw_userID;
		sess.cpIP      = m_cpIP;
		sess.cpPort    = m_cpPort;
		sess.atime     = GetCurTimeStamp();
		sess.btime     = GetCurTimeStamp();		
		sess.serviceID = "";/// no service yet
		DO_FAIL(CreateUserSession(m_appID, m_userID, &sess, MAX_INT, MAX_INT));
	}

	DO_FAIL(reply_user_json_B(user, sess));
	LogDebug("==>OUT");
	return SS_OK;
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
	data["sessionID"] = m_sessionID;
	data["userID"]    = m_raw_userID;
	data["identity"]  = "user";
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
	string no_service_online_hint = "";

	data["status"] = "NoService";
	set_data(data);
	//if(m_whereFrom == "wx" || m_whereFrom == "wxpro")
	//{
		CAppConfig::Instance()->GetValue(m_appID, "no_service_online_hint", no_service_online_hint);
		data["des"] = no_service_online_hint;
	//}
	return on_send_error_reply(ERROR_NO_SERVICE, "Reject Enqueue", data);
}

int ConnectServiceTimer::on_reject_enqueue()
{
	Json::Value data;
	string queue_upper_limit_hint = "";

	data["status"] = "FullQueue";
	set_data(data);	
	//if(m_whereFrom == "wx" || m_whereFrom == "wxpro")
	//{
		CAppConfig::Instance()->GetValue(m_appID, "queue_upper_limit_hint", queue_upper_limit_hint);
		data["des"] = queue_upper_limit_hint;
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
	data["status"] = "onQueue";
	if (flag)
	{
		//data["status"] = "inServiceNow";
		return on_send_error_reply(SERVICE_WITH_NO_QUEUE, "OK", data);
	}
	else
	{
		//data["status"] = "onQueue";
		return on_send_error_reply(ERROR_NO_ERROR, "OK", data);
	}
}

int ConnectServiceTimer::on_queue()
{
	int max_conv_num         = 0;
	long long queue_timeout   = 0;
	unsigned max_queue_num   = 0;
	bool serviceWithNoQueue  = false;
    TagUserQueue* pTagQueues = NULL;
    TagUserQueue* pHighPriTagQueues = NULL;
	unsigned queue_count         = 0;
	unsigned highpri_queue_count = 0;
    unsigned serviceNum = 0;
	UserInfo user;
	long long qtime = 0, expire_time = 0;
	
	LogDebug("==>IN");

	max_conv_num  = CAppConfig::Instance()->getMaxConvNum(m_appID);
	serviceNum    = CAppConfig::Instance()->GetServiceNumber(m_appID);
	max_queue_num = serviceNum * max_conv_num * m_proc->m_cfg._queue_rate;
    LogTrace("[%s] serviceNum:%u, max_conv_num:%d, queue rate:%d, max_queue_num: %d\n", 
    		m_appID.c_str(), serviceNum, max_conv_num, m_proc->m_cfg._queue_rate, max_queue_num);

	
    if (CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagQueues) ||
    	CAppConfig::Instance()->GetTagQueue(m_appID, pHighPriTagQueues))
    {
		ON_ERROR_GET_DATA("tag user queues");
	    return SS_ERROR;
    }
	queue_count         = pTagQueues->queue_count(m_raw_tag);
	highpri_queue_count = pHighPriTagQueues->queue_count(m_raw_tag);
    LogTrace("[%s] queue size:%u, high pri queue size:%u\n", 
    		m_appID.c_str(), queue_count, highpri_queue_count);


    if (queue_count + highpri_queue_count >= max_queue_num)
    {
        if (serviceNum == 0)
			return on_no_service();
        else
			return on_reject_enqueue();
    }

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

	//获取user
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
    //user.userID        = m_raw_userID;
    //user.sessionID     = m_sessionID;
    user.channel       = m_channel;
    user.extends       = m_extends;///
    user.tag           = m_raw_tag;///
    user.lastServiceID = m_lastServiceID;
    user.priority      = m_priority;
    user.queuePriority = m_queuePriority;
    user.atime = user.qtime = GetCurTimeStamp();///

	LogDebug("Go to add user onqueue. tag: %s, user: %s", m_raw_tag.c_str(), user.toString().c_str());
	
	//将user插入排队队列
	qtime = (user.qtime / 1000);
	queue_timeout = CAppConfig::Instance()->getDefaultQueueTimeout(m_appID);
	expire_time = qtime + queue_timeout;
	LogDebug("qtime: %lu, queue_timeout: %lu", qtime, queue_timeout);
	
    if (m_queuePriority != 0)
    {
		DO_FAIL(pHighPriTagQueues->add_user(m_raw_tag, m_userID, expire_time));
		DO_FAIL(KV_set_queue(m_appID, m_raw_tag, true));
    }
    else
    {
		DO_FAIL(pTagQueues->add_user(m_raw_tag, m_userID, expire_time));
		DO_FAIL(KV_set_queue(m_appID, m_raw_tag, false));
    }
	

	//update user
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

	//on_resp_cp();

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

int CloseSessionTimer::on_closeSession_reply()
{
	Json::Value data;
	Json::Value userInfo;
	Json::Value servInfo;

	userInfo["ID"] = m_raw_userID;
	userInfo["chatProxyIp"] = m_session.cpIP;
	userInfo["chatProxyPort"] = m_session.cpPort;
	
	servInfo["ID"] = m_raw_serviceID;
	servInfo["chatProxyIp"] = m_service.cpIP;
	servInfo["chatProxyPort"] = m_service.cpPort;

	data["sessionID"] = m_session.sessionID;
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

	LogDebug("==>IN");	
	
	if (CI->GetUser(m_userID, user) || user.status != "inService")
	{
		on_not_inservice();
		return SS_ERROR;
	}
	
	GET_SESS(get_user_session(m_appID, m_userID, &m_session));
	LogTrace("get user[%s]'s session: %s", m_userID.c_str(), m_session.toString().c_str());
	m_raw_serviceID = m_session.serviceID;
	m_serviceID     = m_appID + "_" + m_raw_serviceID;
	
	//update session
	m_session.serviceID = "";
	DO_FAIL(UpdateUserSession(m_appID, m_userID, &m_session, MAX_INT, MAX_INT));

	//update service.userList
	GET_SERV(CI->GetService(m_serviceID, m_service));
	SET_SERV(m_service.delete_user(m_raw_userID));
	DO_FAIL(UpdateService(m_serviceID, m_service));
	
	//update user
	user.status = "inYiBot"; //do not delete user.tag, user.sessionID
	DO_FAIL(UpdateUser(m_userID, user));
	
	DO_FAIL(on_closeSession_reply());
	LogDebug("==>OUT");
	return SS_OK;
}

CloseSessionTimer::~CloseSessionTimer()
{
}

