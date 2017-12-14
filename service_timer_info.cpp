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
	Json::Value servJsonList;
	servJsonList.resize(0);
	Json::Value servJson;
	int i = 0;
	
	ServiceInfo serv;
	string app_serviceID;
	
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
		DO_FAIL(get_service_json(m_appID, serv, servJson));

		//更新坐席atime
		serv.atime = GetCurTimeStamp();
		DO_FAIL(UpdateService(app_serviceID, serv));
		
		servJsonList[i] = servJson;
		++i;
	}

	LogDebug("==>OUT");

	data["serviceInfo"] = servJsonList;
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
	ServiceInfo serv;
	Json::Value servJson = Json::objectValue;
	
	DO_FAIL(CAppConfig::Instance()->GetService(m_serviceID, serv));
	DO_FAIL(get_service_json(m_appID, serv, servJson));
	return on_send_reply(servJson);
}

int ServiceLoginTimer::on_already_online()
{
	Json::Value data;
	set_service_data(data);
	//return on_send_error_reply(WARN_ALREADY_ONLINE, "Service Already Online", data);
	return on_send_error_reply(ERROR_NO_ERROR, "Service Already Online", data);
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
			LogDebug("service login in the same CP, update service info.");
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
			LogDebug("service login in another CP, kick out.");
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
	else //service first login, create and add new service
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
	data["tag"]       = m_raw_tag;
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

int ChangeServiceTimer::on_no_tag_service()
{
	Json::Value data;
	set_change_service_data(data);
	data["status"] = "NoService";
	return on_send_error_reply(ERROR_NO_SERVICE, "No Service Available", data);
}

/*
input: m_userID, m_serviceID, m_changeServiceID, m_tag
*/
int ChangeServiceTimer::on_change_service()
{
	//参数检查
	if ("" == m_raw_changeServiceID && "" == m_raw_tag)
	{
		ON_ERROR_PARSE_PACKET();
		return SS_ERROR;
	}

	//先回复CP
	on_resp_cp();

	//用户不在源坐席上
	if (CAppConfig::Instance()->GetService(m_serviceID, m_src_serviceInfo) 
    	|| m_src_serviceInfo.find_user(m_raw_userID))
	{
		return on_session_wrong();
    }

	m_maxConvNum = CAppConfig::Instance()->getMaxConvNum(m_appID);
	
	if ("" != m_raw_changeServiceID)
	{
		DO_FAIL(on_change_service_by_serviceID());
	}
	else
	{
		DO_FAIL(on_change_service_by_tag());
	}

	DO_FAIL(on_change_session());
	DO_FAIL(on_send_change_success());
}


int ChangeServiceTimer::on_change_service_by_serviceID()
{
	//目标坐席下线
	if (CAppConfig::Instance()->GetService(m_changeServiceID, m_dst_serviceInfo) 
		|| "offline" == m_dst_serviceInfo.status)
	{
		on_service_offline();
		return SS_ERROR;
	}

	//目标坐席忙
	if (m_dst_serviceInfo.user_count() >= m_maxConvNum)
	{
		on_service_busy();
		return SS_ERROR;
	}

	return SS_OK;
}


int ChangeServiceTimer::on_change_service_by_tag()
{
	//不能转给自己
	if (find_least_service_by_tag(m_appID, m_tag, m_serviceID, m_dst_serviceInfo))
	{
		//tag内未找到可用的目标坐席
		on_no_tag_service();
		return SS_ERROR;
	}

	m_raw_changeServiceID = m_dst_serviceInfo.serviceID;
    m_changeServiceID     = m_appID + "_" + m_raw_changeServiceID;
    LogTrace("[ChangeService] userID:%s <-----> tag serviceID:%s", m_userID.c_str(), m_changeServiceID.c_str());
    return SS_OK;
}


int ChangeServiceTimer::on_change_session()
{
	SessionQueue *pSessionQueue = NULL;
	if (CAppConfig::Instance()->GetSessionQueue(m_appID, pSessionQueue) 
		|| pSessionQueue->get(m_userID, m_session))
	{
		ON_ERROR_GET_DATA("session");
		return SS_ERROR;
	}
	
	//update session
	m_session.serviceID = m_raw_changeServiceID;
	m_session.atime 	= GetCurTimeStamp();
	m_sessionID 		= m_session.sessionID;
	//m_channel 		= m_session.channel;
	SET_SESS(pSessionQueue->set(m_userID, &m_session, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));
	DO_FAIL(KV_set_session(m_userID, m_session, DEF_SESS_TIMEWARN, DEF_SESS_TIMEOUT));
	
	//update user
	GET_USER(CAppConfig::Instance()->GetUser(m_userID, m_userInfo));
	m_userInfo.lastServiceID = m_raw_serviceID;
	DO_FAIL(UpdateUser(m_userID, m_userInfo));
	
	//update src_serv
	SET_SERV(m_src_serviceInfo.delete_user(m_raw_userID));
	DO_FAIL(UpdateService(m_serviceID, m_src_serviceInfo));
	//update dst_serv
	SET_SERV(m_dst_serviceInfo.add_user(m_raw_userID));
	DO_FAIL(UpdateService(m_changeServiceID, m_dst_serviceInfo));

	return SS_OK;
}

int ChangeServiceTimer::on_send_change_success()
{
	Json::Value sessData;
	sessData["userID"]        = m_raw_userID;
	sessData["serviceID"]     = m_raw_changeServiceID;
	sessData["sessionID"]     = m_sessionID;
	//sessData["channel"]       = m_channel;
	sessData["serviceName"]   = m_dst_serviceInfo.serviceName;
	sessData["serviceAvatar"] = m_dst_serviceInfo.serviceAvatar;

	//解析extends
	Json::Reader reader;
	Json::Value json_extends;
	if (!reader.parse(m_userInfo.extends, json_extends))
	{
		sessData["extends"] = Json::objectValue;
	}
	else
	{
		sessData["extends"] = json_extends;
	}

	sessData["identity"] = "user";
	DO_FAIL(on_send_request("changeSuccess", m_session.cpIP, m_session.cpPort, sessData, true));
	
	#if 0
	//if(m_whereFrom == "websocket")
	if(m_session.whereFrom == "websocket" || m_session.whereFrom == "iOS" || m_session.whereFrom == "Android")
	{
		MsgRetransmit::Instance()->SetMsg(strServiceRsp, m_appID, m_seq, m_session.userChatproxyIP, m_session.userChatproxyPort, m_proc->m_cfg._re_msg_send_timeout);
	}
	#endif

	sessData["identity"] = "service";
	DO_FAIL(on_send_request("changeSuccess", m_dst_serviceInfo.cpIP, m_dst_serviceInfo.cpPort, sessData, true));	
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

