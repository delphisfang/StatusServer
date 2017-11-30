#include "statsvr_timer_info.h"
#include <algorithm>

using namespace tfc::base;
using namespace statsvr;

extern char BUF[DATA_BUF_SIZE];

string CTimerInfo::get_value_str(Json::Value &jv, const string &key, const string def_val)
{
	if (!jv[key].isNull() && jv[key].isString())
	{
		return jv[key].asString();
	}
	else
	{
		return def_val;
	}
}

unsigned int CTimerInfo::get_value_uint(Json::Value &jv, const string &key, const unsigned int def_val)
{
	if (!jv[key].isNull() && jv[key].isUInt())
	{
		return jv[key].asUInt();
	}
	else
	{
		return def_val;
	}
}

int CTimerInfo::get_value_int(Json::Value &jv, const string &key, const int def_val)
{
	if (!jv[key].isNull() && jv[key].isInt())
	{
		return jv[key].asInt();
	}
	else
	{
		return def_val;
	}
}

int CTimerInfo::init(string req_data, int datalen)
{
	Json::Reader reader;
	Json::Value js_req_root;
	Json::Value js_req_data;

	if (!reader.parse(req_data, js_req_root))
	{
		LogError("Error init SerializeToString Fail: %s\n", req_data.c_str());
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
		|| 0 == access("/home/fht/sskv_10302/debug_switch", F_OK))
	{
		LogTrace("req_data: %s", req_data.c_str());
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
	
	if (m_cmd != "pingConf" && m_cmd != "updateConf" && m_cmd != "getConf"
		&& m_cmd != "getTodayStatus"
		&& CAppConfig::Instance()->checkAppIDExist(m_appID))
	{
		LogError("Unknown appID[%s]!", m_appID.c_str());
		return -1;
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
		m_lastServiceID = m_appID + "_" + js_req_data["lastServiceID"].asString();
	}

	//0: normal queue, 1: highpri queue
	m_queuePriority = get_value_uint(js_req_data, "priority");

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
		|| 0 == access("/home/fht/sskv_10302/debug_switch", F_OK))
	{
		LogTrace("Init request data OK! [id:%s,cmd:%s,userID:%s,servID:%s,msg_seq:%u]", 
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

		post_err["project"] = "StatSvr";
		post_err["module"]  = "statsvr";
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

