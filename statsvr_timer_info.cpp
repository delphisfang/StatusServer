#include "statsvr_timer_info.h"
#include <algorithm>

using namespace tfc::base;
using namespace statsvr;

//extern char BUF[DATA_BUF_SIZE];

int CTimerInfo::init(string req_data, int datalen)
{
	Json::Reader reader;
	Json::Value js_req_root;
	Json::Value js_req_data;

	if (!reader.parse(req_data, js_req_root))
	{
		LogError("Failed to parse req_data: %s!", req_data.c_str());
		return -1;
	}

	//m_whereFrom = get_value_str(js_req_root, "whereFrom");

	if (!js_req_root["method"].isNull() && js_req_root["method"].isString())
	{
		m_cmd = get_value_str(js_req_root, "method");
	}
	else
	{
		m_cmd = get_value_str(js_req_root, "cmd");
	}
	//LogDebug("m_cmd: %s", m_cmd.c_str());

	if ((m_cmd != "getUserInfo" && m_cmd != "getServiceInfo")
	/*	|| 0 == access("/home/fht/sskv_10302/debug_switch", F_OK)*/)
	{
		LogDebug("req_data: %s", req_data.c_str());
	}
	
	m_seq = get_value_uint(js_req_root, "innerSeq");

	if (!js_req_root["appID"].isNull() && js_req_root["appID"].isString())
	{
		m_appID = js_req_root["appID"].asString();
	}
	else if (!js_req_root["appID"].isNull() && js_req_root["appID"].isUInt())
	{
		m_appID = ui2str(js_req_root["appID"].asUInt());
	}
	
	if (js_req_root["data"].isNull() || !js_req_root["data"].isObject())
	{
		m_search_no = m_appID + "_" + i2str(m_msg_seq);
		return 0;
	}
	js_req_data = js_req_root["data"];
	m_data = js_req_data.toStyledString();

	/*if (!js_req_data["userInfo"].isNull() && js_req_data["userInfo"].isObject())
	{
		m_userInfo = js_req_data["userInfo"].toStyledString();
	}
	*/

	if (!js_req_data["appID"].isNull() && js_req_data["appID"].isString())
	{
		m_appID = js_req_data["appID"].asString();
	}
	else if (!js_req_data["appID"].isNull() && js_req_data["appID"].isUInt())
	{
		m_appID = ui2str(js_req_data["appID"].asUInt());
	}

	if (m_cmd != "pingConf" && m_cmd != "updateConf" && m_cmd != "getConf"
		&& m_cmd != "getTodayStatus"
		&& CAppConfig::Instance()->checkAppIDExist(m_appID))
	{
		LogError("Unknown appID[%s]!", m_appID.c_str());
		return -1;
	}	
	
	m_identity = get_value_str(js_req_data, "identity");

	m_cpIP = get_value_str(js_req_data, "chatProxyIp");

	m_cpPort = get_value_uint(js_req_data, "chatProxyPort");

	if (!js_req_data["tag"].isNull() && js_req_data["tag"].isString())
	{
		m_raw_tag = js_req_data["tag"].asString();
		m_tag = m_appID + "_" + m_raw_tag;

		if (m_cmd != "pingConf" && m_cmd != "updateConf" && m_cmd != "getConf"
			&& m_cmd != "getTodayStatus"
			&& CAppConfig::Instance()->checkTagExist(m_appID, m_tag))
		{
			LogError("Unknown tag[%s]!", m_tag.c_str());
			return -1;
		}
	}
	
	if (!js_req_data["userID"].isNull() && js_req_data["userID"].isString())
	{
		m_raw_userID = js_req_data["userID"].asString();
		m_userID = m_appID + "_" + m_raw_userID;
	}
	else if (!js_req_data["userID"].isNull() && js_req_data["userID"].isArray())
	{
		Json::Value userID_list = js_req_data["userID"];
		for(int i = 0; i < userID_list.size(); i++)
		{
			m_userID_list.insert(m_appID + "_" + userID_list[i].asString());
		}
	}
	
	if (!js_req_data["serviceID"].isNull() && js_req_data["serviceID"].isString())
	{
		m_raw_serviceID = js_req_data["serviceID"].asString();
		m_serviceID = m_appID + "_" + m_raw_serviceID;
	}
	else if (!js_req_data["serviceID"].isNull() && js_req_data["serviceID"].isArray())
	{
		Json::Value serviceID_list = js_req_data["serviceID"];
		for(int i = 0; i < serviceID_list.size(); i++)
		{
			m_serviceID_list.insert(m_appID + "_" + serviceID_list[i].asString());
		}
	}
	else if (!js_req_data["services"].isNull() && js_req_data["services"].isArray())
	{
		Json::Value serviceID_list = js_req_data["services"];
		for(int i = 0; i < serviceID_list.size(); i++)
		{
			m_serviceID_list.insert(m_appID + "_" + serviceID_list[i].asString());
		}
	}
	
	m_channel = get_value_str(js_req_data, "channel");

	m_status  = get_value_str(js_req_data, "status");
	
	if (!js_req_data["extends"].isNull() && js_req_data["extends"].isObject())
	{
		m_extends = js_req_data["extends"].toStyledString();
	}
	
	m_serviceName = get_value_str(js_req_data, "serviceName");

	m_serviceAvatar = get_value_str(js_req_data, "serviceAvatar");
	
	if (!js_req_data["content"].isNull() && js_req_data["content"].isObject())
	{
		m_content = js_req_data["content"];
	}
	
	if (!js_req_data["changeServiceID"].isNull() && js_req_data["changeServiceID"].isString())
	{
		m_raw_changeServiceID = js_req_data["changeServiceID"].asString();
		m_changeServiceID = m_appID + "_" + m_raw_changeServiceID;
	}
	
	if (!js_req_data["lastServiceID"].isNull() && js_req_data["lastServiceID"].isString())
	{
		m_raw_lastServiceID = js_req_data["lastServiceID"].asString();
		m_lastServiceID = m_appID + "_" + m_raw_lastServiceID;
	}

	if (!js_req_data["appointServiceID"].isNull() && js_req_data["appointServiceID"].isString())
	{
		m_has_appointServiceID = true;
		m_raw_appointServiceID = js_req_data["appointServiceID"].asString();
		m_appointServiceID     = m_appID + "_" + m_raw_appointServiceID;
	}
	else
	{
		m_has_appointServiceID = false;
	}

	//0: normal queue, 1: highpri queue
	m_queuePriority = get_value_uint(js_req_data, "queuePriority");

	if (!js_req_data["tags"].isNull() && js_req_data["tags"].isArray())
	{
		Json::Value jsTags;
		jsTags = js_req_data["tags"];
		for (int i = 0; i < jsTags.size(); i++)
		{
			m_tags.insert(m_appID + "_" + jsTags[i].asString());
		}
	}

	m_priority = get_value_str(js_req_data, "priority");
	
	if (!js_req_data["services"].isNull() && js_req_data["services"].isArray())
	{
		Json::Value services;
		services = js_req_data["services"];
		for(int i = 0; i < services.size(); i++)
		{
			m_checkServices.insert(m_appID + "_" + services[i].asString());
		}
	}

	char id_buf[64];
    snprintf (id_buf, sizeof(id_buf), "%s:%s--%s:%d", m_appID.c_str(), m_serviceID.c_str(), m_userID.c_str(), m_msg_seq);
	m_search_no = string(id_buf);

	if ((m_cmd != "getUserInfo" && m_cmd != "getServiceInfo")
	/*	|| 0 == access("/home/fht/sskv_10302/debug_switch", F_OK)*/)
	{
		LogDebug("Init request data OK! [id:%s,cmd:%s,userID:%s,servID:%s,msg_seq:%u]", 
					m_identity.c_str(), m_cmd.c_str(), m_userID.c_str(), m_serviceID.c_str(), m_msg_seq);
	}

	return 0;
}

int CTimerInfo::on_error()
{
	Json::Value error_rsp;
	error_rsp["method"]   = m_cmd + "-reply";
	error_rsp["innerSeq"] = m_seq;
	error_rsp["code"]     = m_errno;
	error_rsp["msg"]      = m_errmsg;
	string rsp = error_rsp.toStyledString();  
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
	string strRsp;
	
	LogError("[on_expire] searchid[%s]:handle timer timeout, statue[%d].", 
				m_search_no.c_str(), m_cur_step);

	error_rsp["method"]   = m_cmd + "-reply";
	error_rsp["innerSeq"] = m_seq;
	error_rsp["code"]     = ERROR_SYSTEM_WRONG;
	error_rsp["msg"]      = "System handle timeout";
	strRsp = error_rsp.toStyledString();  
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
	data["userID"]	  = m_raw_userID;
}

void CTimerInfo::set_service_data(Json::Value & data)
{
	data["identity"]  = "service";
	data["serviceID"] = m_raw_serviceID;
}

void CTimerInfo::set_system_data(Json::Value &data)
{
	data["userID"]	  = m_raw_userID;
	data["serviceID"] = m_raw_serviceID;
	data["sessionID"] = m_sessionID;
	data["identity"]  = "system";
}

int CTimerInfo::on_send_request(string cmd, string ip, unsigned short port, const Json::Value &data, bool with_code)
{
	Json::Value req;
	req["appID"]	= m_appID;
	req["method"]	= cmd;
	req["innerSeq"] = m_msg_seq;
	req["data"] 	= data;
	if (with_code)
		req["code"] = 0;
	string strReq	= req.toStyledString();
	LogTrace("send request to <%s, %d>: %s", ip.c_str(), port, strReq.c_str());
	if (m_proc->EnququeHttp2DCC((char *)strReq.c_str(), strReq.size(), ip, port))
	{
		LogError("[%s]: Error send request %s", m_appID.c_str(), cmd.c_str());
		return -1;
	}
	return 0;
}

int CTimerInfo::on_send_reply(const Json::Value &data)
{
	Json::Value rsp;
	rsp["appID"]	= m_appID;
	rsp["method"]	= m_cmd + "-reply";
	rsp["innerSeq"] = m_seq;
	rsp["code"] 	= 0;
	rsp["data"] 	= data;
	string strRsp	= rsp.toStyledString();

	if ((m_cmd != "getUserInfo" && m_cmd != "getServiceInfo" && m_cmd != "refreshSession")
	/*	|| 0 == access("/home/fht/sskv_10302/debug_switch", F_OK)*/)
	{
		LogTrace("send response: %s", strRsp.c_str());
	}
	if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
	{
		LogError("searchid[%s]: Failed to SendReply <%s>", m_search_no.c_str(), m_cmd.c_str());
		m_errno  = ERROR_SYSTEM_WRONG;
		m_errmsg = "Error send to ChatProxy";
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
	rsp["appID"]	 = m_appID;
	rsp["method"]	 = m_cmd + "-reply";
	rsp["innerSeq"]  = m_seq;
	rsp["code"]      = code;
	rsp["msg"]		 = msg;
	rsp["data"] 	 = data;
	string strRsp	 = rsp.toStyledString();
	LogTrace("send ERROR response: %s", strRsp.c_str());
	if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
	{
		LogError("searchid[%s]: Failed to SendErrorReply <%s>", m_search_no.c_str(), msg.c_str());
		m_errno  = ERROR_SYSTEM_WRONG;
		m_errmsg = "Error send to ChatProxy";
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

int CTimerInfo::get_user_session(string appID, string app_userID, Session *sess)
{
	SessionQueue*  pSessQueue = NULL;
	Session temp;
	
	if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
		|| pSessQueue->get(app_userID, temp))
	{
		LogError("Failed to get session of user[%s]", app_userID.c_str());
		return SS_ERROR;
	}

	*sess = temp;
	return SS_OK;
}

void CTimerInfo::get_user_json(string appID, string app_userID, const UserInfo &user, Json::Value &userJson)
{
	Session sess;
	Json::Value sessJson;

	user.toJson(userJson);
	if (get_user_session(appID, app_userID, &sess))
	{
		userJson["session"] = Json::objectValue;
	}
	else
	{
		sess.toJson(sessJson);
		userJson["session"] = sessJson;
	}
}

void CTimerInfo::construct_user_json(const UserInfo &user, const Session &sess, Json::Value &userJson)
{
	Json::Value sessInfo;
	user.toJson(userJson);
	sess.toJson(sessInfo);
	userJson["session"] = sessInfo;
}

int CTimerInfo::reply_user_json_A(string appID, string app_userID, const UserInfo &user)
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

int CTimerInfo::get_service_json(string appID, const ServiceInfo &serv, Json::Value &servJson)
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

		userID	   = *it;
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
	UserQueue *uq = NULL, *highpri_uq = NULL;
	unsigned queueNum = 0;
	for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
	{
		LogDebug("==>service tag: %s", (*it).c_str());
		
		DO_FAIL(get_normal_queue(appID, *it, &uq));
		queueNum += uq->size();
		DO_FAIL(get_highpri_queue(appID, *it, &highpri_uq));
		queueNum += highpri_uq->size();
	}
	servJson["queueNumber"] = queueNum;
	
	//当前服务人数>=最大会话人数时，返回busy
	int maxConvNum = CAppConfig::Instance()->getMaxConvNum(appID);
	if ("online" == serv.status && serv.user_count() >= maxConvNum)
	{
		servJson["status"] = "busy";
	}
}

int CTimerInfo::update_user_session(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
{
	SessionQueue*  pSessQueue = NULL;
	
	if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
		|| pSessQueue->set(app_userID, sess, gap_warn, gap_expire))
	{
		LogError("Failed to set session of user[%s]!", app_userID.c_str());
		return SS_ERROR;
	}

	return SS_OK;
}

int CTimerInfo::delete_user_session(string appID, string app_userID)
{
	SessionQueue*  pSessQueue = NULL;
	
	if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
		|| pSessQueue->delete_session(app_userID))
	{
		LogError("Failed to delete session of user[%s]!", app_userID.c_str());
		return SS_ERROR;
	}

	return SS_OK;
}

int CTimerInfo::create_user_session(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
{
	SessionQueue*  pSessQueue = NULL;
	
	if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
		|| pSessQueue->insert(app_userID, sess, gap_warn, gap_expire))
	{
		LogError("Failed to create session of user[%s]!", app_userID.c_str());
		return SS_ERROR;
	}

	return SS_OK;
}

string CTimerInfo::gen_sessionID(string app_userID)
{
	return app_userID + "_" + l2str(time(NULL));
}

int CTimerInfo::get_normal_queue(string appID, string raw_tag, UserQueue **uq)
{
	TagUserQueue* pTagQueues = NULL;
	GET_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues), "tag user queues");
	GET_FAIL(pTagQueues->get_tag(raw_tag, *uq), "user queue");
	return SS_OK;
}

int CTimerInfo::get_highpri_queue(string appID, string raw_tag, UserQueue **uq)
{
	TagUserQueue* pTagQueues = NULL;
	GET_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues), "highpri tag user queues");
	GET_FAIL(pTagQueues->get_tag(raw_tag, *uq), "user queue");
	return SS_OK;
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

int CTimerInfo::KV_set_user(string app_userID, const UserInfo &user)
{
	string key, value;
	key   = USER_PREFIX+app_userID;
	value = user.toString();
					
	DO_FAIL(KVSetKeyValue(KV_CACHE, key, value));
	DO_FAIL(KV_set_userIDList());
	return SS_OK;
}

int CTimerInfo::KV_set_service(string app_serviceID, const ServiceInfo &serv)
{
	DO_FAIL(KVSetKeyValue(KV_CACHE, SERV_PREFIX+app_serviceID, serv.toString()));
	DO_FAIL(KV_set_servIDList());
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
	long long warn_time   = (sess.atime/1000) + gap_warn;
	long long expire_time = (sess.atime/1000) + gap_expire;
	Json::Value sessJson;
	sess.toJson(sessJson);
	sessJson["warn_time"]	= warn_time;
	sessJson["expire_time"] = expire_time;
	return KVSetKeyValue(KV_CACHE, SESS_PREFIX+app_userID, sessJson.toStyledString());
}

int CTimerInfo::KV_del_session(string app_userID)
{
	return KVDelKeyValue(KV_CACHE, SESS_PREFIX+app_userID);
}

int CTimerInfo::KV_set_queue(string appID, string raw_tag, int highpri)
{
	TagUserQueue *pTagQueues = NULL;
	UserQueue *uq = NULL;

	string prefix		 = (highpri) ? (HIGHQ_PREFIX) : (QUEUE_PREFIX);
	string key_queueList = prefix + appID + "_" + raw_tag;

	if (highpri)
	{
		GET_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues), "tag queues");
	}
	else
	{
		GET_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues), "tag highpri queues");
	}
	GET_FAIL(pTagQueues->get_tag(raw_tag, uq), "user queue");

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
	
	long long warn_time   = obj["warn_time"].asInt64();
	long long expire_time = obj["expire_time"].asInt64();
	string appID = getappID(app_userID);
	SET_SESS(CreateUserSession(appID, app_userID, &sess, warn_time, expire_time));
	return SS_OK;
}

int CTimerInfo::KV_parse_service(string app_serviceID)
{
	LogTrace("parse serviceID:%s", app_serviceID.c_str());
	
	/*获取*/
	string serv_key = "serv_" + app_serviceID;
	string serv_value;
	DO_FAIL(KVGetKeyValue(KV_CACHE, serv_key, serv_value));
	
	/* 解析每个service的详细信息 */
	ServiceInfo serv(serv_value);
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
		string userID		  = queueNode["userID"].asString();
		long long expire_time = queueNode["expire_time"].asInt64();
		SET_USER(pTagQueues->add_user(raw_tag, userID, expire_time));
	}

	return SS_OK;
}

/************************* wrapper methods **********************/
int CTimerInfo::AddUser(string app_userID, const UserInfo &user)
{
	SET_USER(CAppConfig::Instance()->AddUser(app_userID, user));
	DO_FAIL(KV_set_user(app_userID, user));
	return SS_OK;
}

int CTimerInfo::UpdateUser(string app_userID, const UserInfo &user)
{
	SET_USER(CAppConfig::Instance()->UpdateUser(app_userID, user));
	DO_FAIL(KV_set_user(app_userID, user));
	return SS_OK;
}

int CTimerInfo::AddService(string appID, string app_servID, ServiceInfo &serv)
{
	SET_SERV(CAppConfig::Instance()->AddService(app_servID, serv));
	//insert service in every tag serviceHeap
	SET_FAIL(CAppConfig::Instance()->AddService2Tags(appID, serv), "service heap");
	DO_FAIL(KV_set_service(app_servID, serv));
	return SS_OK;

}

int CTimerInfo::UpdateService(string app_servID, const ServiceInfo &serv)
{
	SET_SERV(CAppConfig::Instance()->UpdateService(app_servID, serv));
	DO_FAIL(KV_set_service(app_servID, serv));
	return SS_OK;
}

int CTimerInfo::DeleteService(string app_servID)
{
	SET_SERV(CAppConfig::Instance()->DelService(app_servID));
	DO_FAIL(KV_del_service(app_servID));
	return SS_OK;
}

int CTimerInfo::UpdateUserSession(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
{
	SET_SESS(update_user_session(appID, app_userID, sess, gap_warn, gap_expire));
	DO_FAIL(KV_set_session(app_userID, *sess, gap_warn, gap_expire));
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
		DO_FAIL(CAppConfig::Instance()->AddTagOnlineServiceNumber(appID, *it));
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
		DO_FAIL(CAppConfig::Instance()->DelTagOnlineServiceNumber(appID, *it));
	}
	return 0;
}

