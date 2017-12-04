#include "service_timer_info.h"
#include "statsvr_mcd_proc.h"

#include <algorithm>

using namespace statsvr;

int GetServiceInfoTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_get_serviceinfo())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int GetServiceInfoTimer::on_not_online()
{
	Json::Value data;
	set_service_data(data);
	return on_send_error_reply(ERROR_SERVICE_NOT_ONLINE, "Service Not Online", data);
}

int GetServiceInfoTimer::on_get_serviceinfo()
{
	Json::Value data = Json::objectValue;
	Json::Value servInfoList;
	servInfoList.resize(0);
	Json::Value servInfo;
	int i = 0;
	
	ServiceInfo serv;
	UserQueue *uq = NULL, *highpri_uq = NULL;
	unsigned queueNum = 0;
	string app_serviceID;
	
    int maxConvNum = CAppConfig::Instance()->getMaxConvNum(m_appID);

	LogDebug("==>IN");

	for (set<string>::iterator it = m_serviceID_list.begin(); it != m_serviceID_list.end(); it++)
	{
		app_serviceID = (*it);
		if (CAppConfig::Instance()->GetService(app_serviceID, serv))
		{
			m_raw_serviceID = delappID(app_serviceID);
			on_not_online();
			return SS_ERROR;
		}
		//serv.toJson(servInfo);
		get_service_json(m_appID, serv, servInfo);

		//calculate service's queueNumber
		for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
		{
			LogDebug("==>service tag: %s", (*it).c_str());
			
			DO_FAIL(get_normal_queue(m_appID, *it, &uq));
			queueNum += uq->size();
			DO_FAIL(get_highpri_queue(m_appID, *it, &highpri_uq));
			queueNum += highpri_uq->size();
		}
		servInfo["queueNumber"] = queueNum;

		//当前服务人数>=最大会话人数时，返回busy
        if (serv.status == "online" && serv.user_count() >= maxConvNum)
        {
            servInfo["status"] = "busy";
        }
		
		servInfoList[i] = servInfo;
		++i;
	}

	LogDebug("==>OUT");

	data["serviceInfo"] = servInfoList;
	return on_send_reply(data);
}

GetServiceInfoTimer::~GetServiceInfoTimer()
{
}


int ServiceLoginTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_service_login())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int ServiceLoginTimer::on_serviceLogin_reply()
{
	Json::Value data = Json::objectValue;
	return on_send_reply(data);
}

int ServiceLoginTimer::on_already_online()
{
	Json::Value data;
	set_service_data(data);
	return on_send_error_reply(WARN_ALREADY_ONLINE, "Service Already Online", data);
}

void ServiceLoginTimer::set_service_fields(ServiceInfo &serv)
{
	serv.atime = GetCurTimeStamp();
	serv.serviceName = m_serviceName;
	serv.serviceAvatar = m_serviceAvatar;

	Json::Reader reader;
	Json::Value value;
	if (!reader.parse(m_data, value))
	{
		return;
	}
	//clear serv.tags first
	serv.tags.clear();
	unsigned tagsLen = value["tags"].size();
	for (unsigned i = 0; i < tagsLen; i++)
	{
		serv.tags.insert(value["tags"][i].asString());
	}

	//do not change serv.userList
}

int ServiceLoginTimer::on_service_login()
{
	ServiceInfo serv;
	Json::Value data;

	LogDebug("==>IN");
	
    if (SS_OK == CAppConfig::Instance()->GetService(m_serviceID, serv))
    {
		LogDebug("Old service: %s, new cpIP: %s, new cpPort: %u", serv.toString().c_str(), m_cpIP.c_str(), m_cpPort);
    	if (m_cpIP == serv.cpIP && m_cpPort == serv.cpPort)
    	{
			LogDebug("service online in the same CP, update service info.");
			DO_FAIL(CAppConfig::Instance()->DelServiceFromTags(m_appID, serv));
			DO_FAIL(DelTagOnlineServNum(m_appID, serv));
			set_service_fields(serv);
			DO_FAIL(AddTagOnlineServNum(m_appID, serv));
			DO_FAIL(CAppConfig::Instance()->AddService2Tags(m_appID, serv));
			DO_FAIL(UpdateService(m_serviceID, serv));
			return on_serviceLogin_reply();
    	}
		else
		{
			LogDebug("service online in another CP, kick out.");
			set_service_data(data);
			DO_FAIL(on_send_request("kickOut", serv.cpIP, serv.cpPort, data, false));
			
			LogDebug("update service info.");
			DO_FAIL(CAppConfig::Instance()->DelServiceFromTags(m_appID, serv));
			DO_FAIL(DelTagOnlineServNum(m_appID, serv));
			set_service_fields(serv);
			serv.cpIP   = m_cpIP;
			serv.cpPort = m_cpPort;
			DO_FAIL(AddTagOnlineServNum(m_appID, serv));
			DO_FAIL(CAppConfig::Instance()->AddService2Tags(m_appID, serv));
			DO_FAIL(UpdateService(m_serviceID, serv));
    	}
    }
	else //service first online, create and add new service
	{
		serv = ServiceInfo(m_data);
		LogDebug("Add new service: %s", m_serviceID.c_str());
		DO_FAIL(AddService(m_appID, m_serviceID, serv));
		DO_FAIL(AddTagOnlineServNum(m_appID, serv));
		/*unsigned servNum = CAppConfig::Instance()->GetServiceNumber(m_appID);
		LogDebug("total service num: %u", servNum);*/
	}

	DO_FAIL(on_serviceLogin_reply());
	
	LogDebug("==>OUT");
	return SS_OK;
}

ServiceLoginTimer::~ServiceLoginTimer()
{
}


int ServiceChangeStatusTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_service_changestatus())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int ServiceChangeStatusTimer::on_resp_cp()
{
	Json::Value data = Json::objectValue;
	return on_send_reply(data);
}

int ServiceChangeStatusTimer::on_not_online()
{
	Json::Value data;
	set_service_data(data);
	return on_send_error_reply(ERROR_SERVICE_NOT_ONLINE, "Service Not Online", data);
}

/*
m_serviceID, m_status
*/
int ServiceChangeStatusTimer::on_service_changestatus()
{
	ServiceInfo serv;

	LogDebug("==>IN");

	if (CAppConfig::Instance()->GetService(m_serviceID, serv))
	{
		on_not_online();
		return SS_ERROR;
	}

	if (m_status != serv.status)
	{
		LogTrace("Update service[%s] status to %s", m_serviceID.c_str(), m_status.c_str());
    	serv.status = m_status;
		DO_FAIL(UpdateService(m_serviceID, serv));
		if ("online" == serv.status)
		{
			DO_FAIL(AddTagOnlineServNum(m_appID, serv));
		}
		else
		{
			DO_FAIL(DelTagOnlineServNum(m_appID, serv));
		}
		return on_resp_cp();
	}
	else
	{
		LogTrace("service[%s] status[%s] not changed", m_serviceID.c_str(), m_status.c_str());
		return on_resp_cp();
	}
}

ServiceChangeStatusTimer::~ServiceChangeStatusTimer()
{
}



int ChangeServiceTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_change_service())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int ChangeServiceTimer::on_resp_cp()
{
	Json::Value data = Json::objectValue;
	return on_send_reply(data);
}


void ChangeServiceTimer::set_change_service_data(Json::Value &data)
{
	data["identity"]  = "service";
	data["userID"]    = m_raw_userID;
	data["serviceID"] = m_raw_serviceID;
	data["changeServiceID"] = m_raw_changeServiceID;
}

int ChangeServiceTimer::on_service_busy()
{
	Json::Value data;
	set_change_service_data(data);
	return on_send_error_reply(ERROR_SERVICE_BUSY, "Change Service busy", data);
}

int ChangeServiceTimer::on_session_wrong()
{
	Json::Value data;
	set_change_service_data(data);
	return on_send_error_reply(ERROR_SESSION_WRONG, "Session wrong", data);
}

int ChangeServiceTimer::on_service_offline()
{
	Json::Value data;
	set_change_service_data(data);
	return on_send_error_reply(ERROR_SERVICE_OFFLINE, "Change Service offline", data);
}

/*
m_userID, m_serviceID, m_changeServiceID
*/
int ChangeServiceTimer::on_change_service()
{
    int maxUsrNum = 0;
	ServiceInfo src;
	ServiceInfo dst;
	SessionQueue *pSessionQueue = NULL;
	Session sess;

	LogDebug("==>IN");

	//response to ChatProxy first
	on_resp_cp();
	
	if (CAppConfig::Instance()->GetValue(m_appID, "max_conv_num", maxUsrNum) 
		|| 0 == maxUsrNum)
    {
        maxUsrNum = 5;
    }

	if (CAppConfig::Instance()->GetService(m_changeServiceID, dst) 
		|| dst.status == "offline")
	{
		//目的坐席不存在或下线
		return on_service_offline();
	}
	else if (CAppConfig::Instance()->GetService(m_serviceID, src) 
    		|| src.find_user(m_raw_userID))
	{
		//用户当前不在源坐席上
		return on_session_wrong();
    }
	else if (dst.user_count() >= maxUsrNum)
	{
		//目的坐席忙
		return on_service_busy();
	}
	else
	{
        if (CAppConfig::Instance()->GetSessionQueue(m_appID, pSessionQueue) 
			|| pSessionQueue->get(m_userID, sess))
        {
			ON_ERROR_GET_DATA("session");
	    	return SS_ERROR;
        }

		//update session
        sess.serviceID = m_raw_changeServiceID;
        sess.atime     = GetCurTimeStamp();
        m_sessionID    = sess.sessionID;
        //m_channel      = sess.channel;
		SET_SESS(pSessionQueue->set(m_userID, &sess, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));
		DO_FAIL(KV_set_session(m_userID, sess, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));

		//update user
		UserInfo user;
		GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
		user.lastServiceID = m_raw_serviceID;
		//user.sessionID保持不变
		DO_FAIL(UpdateUser(m_userID, user));
		
		//update service1 service2
		SET_SERV(src.delete_user(m_raw_userID));
		DO_FAIL(UpdateService(m_serviceID, src));

        SET_SERV(dst.add_user(m_raw_userID));
		DO_FAIL(UpdateService(m_changeServiceID, dst));

		#if 0
        src.cpIP      = m_cpIP;
        src.cpPort    = m_cpPort;
        //src.whereFrom = m_whereFrom;
		#endif
		
		//send <ChangeSuccess> to user and service
		Json::Value sessData;
		sessData["userID"]        = m_raw_userID;
		sessData["serviceID"]     = m_raw_changeServiceID;
		sessData["sessionID"]     = m_sessionID;
		//sessData["channel"]       = m_channel;
		sessData["serviceName"]   = dst.serviceName;
		sessData["serviceAvatar"] = dst.serviceAvatar;

		sessData["identity"]      = "user";
		DO_FAIL(on_send_request("changeSuccess", sess.cpIP, sess.cpPort, sessData, true));
		
		#if 0
		//if(m_whereFrom == "websocket")
		if(m_session.whereFrom == "websocket" || m_session.whereFrom == "iOS" || m_session.whereFrom == "Android")
		{
			MsgRetransmit::Instance()->SetMsg(strServiceRsp, m_appID, m_seq, m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
		}
		#endif

		sessData["identity"]      = "service";
		DO_FAIL(on_send_request("changeSuccess", dst.cpIP, dst.cpPort, sessData, true));
	}

	LogDebug("==>OUT");
	return SS_OK;
}

ChangeServiceTimer::~ChangeServiceTimer()
{
}


int ServicePullNextTimer::do_next_step(string& req_data)
{
	if (init(req_data, req_data.size()))
	{
		ON_ERROR_PARSE_PACKET();
		return -1;
	}
	
	if (on_pull_next())
	{
		return -1;
	}
	else
	{
		return 1;
	}
}

int ServicePullNextTimer::on_send_connect_success()
{
	Json::Value serviceData;
    //Json::Reader reader;
    //Json::Value json_extends;

    serviceData["userID"]    = m_raw_userID;
    serviceData["serviceID"] = m_raw_serviceID;
    serviceData["sessionID"] = m_sessionID;
    //serviceData["channel"]   = m_userInfo.channel;
#if 0
	reader.parse(m_userInfo.extends, json_extends);
    serviceData["extends"] = json_extends;
#else
	serviceData["extends"] = Json::objectValue;
#endif
    serviceData["serviceName"]   = m_serviceInfo.serviceName;
    serviceData["serviceAvatar"] = m_serviceInfo.serviceAvatar;
    
    //reader.parse(m_userInfo.userInfo, userInfo);
    //serviceData["userInfo"] = userInfo;

	//发送给user端
	serviceData["identity"]      = "user";
	DO_FAIL(on_send_request("connectSuccess", m_session.cpIP, m_session.cpPort, serviceData, true));

#if 0
    if (m_whereFrom == "websocket" || m_session.whereFrom == "iOS" || m_session.whereFrom == "Android")
    {
        MsgRetransmit::Instance()->SetMsg(strServiceRsp, m_appID, ui2str(m_msg_seq), m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
    }
#endif
	
	//发送给service端
	serviceData["identity"]      = "service";
	DO_FAIL(on_send_request("connectSuccess", m_serviceInfo.cpIP, m_serviceInfo.cpPort, serviceData, true));

	return SS_OK;
}


/*
m_serviceID
*/
int ServicePullNextTimer::on_pull_next()
{
    TagUserQueue *pTagQueue = NULL;
    SessionQueue* pSessQueue = NULL;
	UserQueue *uq = NULL;
    int num    = 1;
	int direct = 1; //默认从队尾拉取
	UserInfo user;

	//优先从HighPriQueue拉取
    if (0 == CAppConfig::Instance()->GetTagHighPriQueue(m_appID, pTagQueue)
		&& pTagQueue->total_queue_count() > 0)
    {
		m_queuePriority = 1;
    }
    else if (0 == CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueue)
        	&& pTagQueue->total_queue_count() > 0)
    {
        m_queuePriority = 0;
    }
	else
	{
		LogTrace("no user on queue, overload finish.");
        m_errno  = WARN_NO_USER_QUEUE;
        m_errmsg = "Get userID failed";
        on_error();
		return SS_OK;
	}
	
    GET_SERV(CAppConfig::Instance()->GetService(m_serviceID, m_serviceInfo));

	LogTrace("==========>service[%s]", m_serviceID.c_str());
	//拉取一个user
	num    = CAppConfig::Instance()->getUserQueueNum(m_appID);
	direct = CAppConfig::Instance()->getUserQueueDir(m_appID);
    if (pTagQueue->get_target_user(m_userID, m_serviceID, m_serviceInfo.tags, num, direct))
    {
        m_errno = WARN_NO_USER_QUEUE;
        m_errmsg = "Get userID failed";
        on_error();
        return SS_OK;
    }
	LogTrace("============>pull out user[%s]", m_userID.c_str());
	m_raw_userID = delappID(m_userID);

	//找到user
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, user));
	LogTrace("============>user[%s]'s tag: %s", m_userID.c_str(), user.tag.c_str());
	
	//user出队
	GET_QUEUE(pTagQueue->get_tag(user.tag, uq));
	SET_QUEUE(uq->delete_user(m_userID));
	DO_FAIL(KV_set_queue(m_appID, user.tag, m_queuePriority));
	
	//update user
	user.status = "inService";
	user.qtime  = 0;
	DO_FAIL(UpdateUser(m_userID, user));
	
	//update service
	SET_SERV(m_serviceInfo.add_user(m_raw_userID));
	DO_FAIL(UpdateService(m_serviceID, m_serviceInfo));
	
	//update session
    GET_SESS(CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue));
	GET_SESS(pSessQueue->get(m_userID, m_session));

	m_sessionID = m_session.sessionID;
    m_session.serviceID = m_raw_serviceID;
    m_session.atime     = GetCurTimeStamp();
    SET_SESS(pSessQueue->set(m_userID, &m_session, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));
	DO_FAIL(KV_set_session(m_userID, m_session, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));
	LogWarn("Service[%s] overload user[%s] success.", m_serviceID.c_str(), m_userID.c_str());
	
    return on_send_connect_success();
}


ServicePullNextTimer::~ServicePullNextTimer()
{
}

