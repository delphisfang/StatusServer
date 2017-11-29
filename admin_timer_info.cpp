#include "admin_timer_info.h"
#include "statsvr_mcd_proc.h"
#include <algorithm>

extern char BUF[DATA_BUF_SIZE];

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
            else if (m_cmd == "updateConf")
            {
                if (on_admin_config())
				{
					m_cur_step = STATE_END;
					return -1;
				}
				on_stat();

				//防止重复重建
				if (m_proc->m_workMode == statsvr::WORKMODE_WORK)
				{	
					LogTrace(">>>>>>>>>>>>>>>>>>>Already in workmode, do not restore!");
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
                LogError("Error unknown cmd: %s\n", m_cmd.c_str());
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


int  AdminConfigTimer::on_admin_ping()
{
    Json::Reader reader;
    Json::Value ping_req;
    Json::Value ping_rsp;
	map<string, bool> map_now;
	
    ping_rsp["cmd"]  = m_cmd + "-reply";
    ping_rsp["code"] = 0;
    ping_rsp["msg"]  = "OK";
    ping_rsp["seq"]  = m_seq;

    if (!reader.parse(m_data, ping_req))
    {
    	LogError("Parse ping request error. buf: [%s]\n", m_data.c_str());
        return -1;
    }

    // construct ping response
	if (ping_req["appIDList"].isNull() || !ping_req["appIDList"].isArray())
	{
		LogError("Parse appIDlist error. buf: [%s]\n", m_data.c_str());
		on_error_parse_packet("Error parse appIDList");
        return -1;
	}

    
	string appIDString = m_data;
	LogDebug("appIDString: %s", appIDString.c_str());
	
	//参数检查
	unsigned size = ping_req["appIDList"].size();
	LogDebug("size: %u", size);
	if (size > MAXSIZE)
	{
		LogError("size:%d > MAXSIZE:%d\n", size, MAXSIZE);
		on_error_parse_packet("Error appIDList too long");
		return -1;
	}

    Json::Value appIDlistVer;
    for (unsigned i = 0; i < size; ++i)
    {
        string appID = ping_req["appIDList"][i].asString();
		
		/* 获取app version */
		LogDebug("GetVersion for appID: %s", appID.c_str());
        unsigned version = CAppConfig::Instance()->GetVersion(appID);

		Json::Value items;
        items["appID"]   = atoi(appID.c_str());
        items["version"] = version;
		/* 添加到 appIDlistVer */
		appIDlistVer.append(items);

		/* 核心代码，为appIDList里的每一个appID创建各种数据结构 */
		#if 0
        CAppConfig::Instance()->AddQueue(appID);
		CAppConfig::Instance()->AddOfflineHeap(appID);
		#endif
		DO_FAIL(CAppConfig::Instance()->AddTagQueue(appID));
		DO_FAIL(CAppConfig::Instance()->AddTagHighPriQueue(appID));
		DO_FAIL(CAppConfig::Instance()->AddSessionQueue(appID));
		//此处暂时不调用AddServiceHeap(tag)，taglist需要等到updateConf操作时才知道
		
		map_now[appID] = true;
    }
    ping_rsp["data"] = appIDlistVer;
	LogDebug("Parse data finish");
	
	int delnum = CAppConfig::Instance()->CheckDel(map_now);
	CAppConfig::Instance()->SetNowappIDList(appIDString);
	LogDebug("SetNowappIDList(%s) finish", appIDString.c_str());

	int loglevel = LOG_TRACE;
	if (delnum > 0)
	{
		loglevel = LOG_ERROR;
	}
	DEBUG_P(loglevel, "PingList:%s, delnum:[%d]\n", appIDString.c_str(), delnum);

	LogDebug("Send ping resp");
    string ping_rsp_str = ping_rsp.toStyledString();
    if (m_proc->EnququeHttp2CCD (m_ret_flow, (char *)ping_rsp_str.c_str(), ping_rsp_str.size()))
    {
    	LogError("enqueue_2_ccd failed.");
		m_errno = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send to client";
        on_error();
        return -1;
    }
    return 0;
}

int AdminConfigTimer::on_admin_getConf()
{
	LogTrace("Receive a new get config request.");

	Json::Reader reader;
	Json::Value appList;
	Json::Value data;
	Json::Value configList;
	configList.resize(0);

	string appListString;
	if (CAppConfig::Instance()->GetNowappIDList(appListString))
	{
		LogError("get appIDList failed.");
		m_errno = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error get appIDList";
        on_error();
       	return -1;
	}
	
	if (!reader.parse(appListString, appList))
	{
		LogError("parse appIDlist to JSON failed:%s", appListString.c_str());
		m_errno = ERROR_SYSTEM_WRONG;
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
		
		int version = CAppConfig::Instance()->GetVersion(appID);
		string appConf;
		CAppConfig::Instance()->GetConf(appID, appConf);
		Json::Value data_rsp;
		data_rsp["appID"]   = atoi(appID.c_str());
		data_rsp["configs"] = appConf;
		data_rsp["version"] = version;
		configList.append(data_rsp);
	}
	data["appList"] = configList;

	Json::Value get_cfg_rsp;
	get_cfg_rsp["cmd"]  = m_cmd + "-reply";
    get_cfg_rsp["code"] = 0;
    get_cfg_rsp["msg"]  = "OK";
	get_cfg_rsp["seq"]  = m_seq;
	get_cfg_rsp["data"] = data;
	
	string get_cfg_rsp_str = get_cfg_rsp.toStyledString();
    if (m_proc->EnququeHttp2CCD (m_ret_flow, (char *)get_cfg_rsp_str.c_str(), get_cfg_rsp_str.size()) != 0)
    {
    	LogError("enqueue_2_dcc failed.");
		m_errno = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send to client";
        on_error();
       	return -1;
    }
	
    return 0;
}

int AdminConfigTimer::on_admin_config()
{
	LogTrace("Receive a new push config request.");

    Json::Reader reader;
    Json::Value push_config_req;
    if (!reader.parse(m_data, push_config_req))
    {
		LogError("Parse config request error. buf: [%s]\n", m_data.c_str());
		ON_ERROR_PARSE_PACKET();
        return -1;
    }
    CAppConfig::Instance()->UpdateappIDConf(push_config_req);

    //construct response and then send it.
    Json::Value push_cfg_rsp;
    push_cfg_rsp["cmd"]  = m_cmd + "-reply";
    push_cfg_rsp["code"] = 0;
    push_cfg_rsp["msg"]  = "OK";
    push_cfg_rsp["seq"]  = m_seq;

	LogDebug("Send config resp");
    string push_cfg_rsp_str = push_cfg_rsp.toStyledString();
    if (m_proc->EnququeHttp2CCD (m_ret_flow, (char *)push_cfg_rsp_str.c_str(), push_cfg_rsp_str.size()) != 0)
    {
    	DEBUG_P (LOG_ERROR, "enqueue_2_ccd failed. \n");
		m_errno  = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send to client";
        on_error();
       	return -1;
    }
    return 0;
}

/* 获取某个appID下的 (会话用户数，排队用户数，在线客服数) */
int AdminConfigTimer::on_admin_get_today_status()
{
	unsigned userNumber = 0;
	unsigned queueNumber = 0;
	unsigned onlineServiceNumber = 0;
	SessionQueue* pSessQueue = NULL;
	TagUserQueue* pTagQueues = NULL;

	//获取某个appID下当前正在会话的用户人数
	if (CAppConfig::Instance()->GetSessionQueue(m_appID, pSessQueue))
	{
		userNumber = 0;
	}
	else
	{
		userNumber = pSessQueue->size();
	}

	/* 获取当前正在排队的用户人数 */
	if (CAppConfig::Instance()->GetTagQueue(m_appID, pTagQueues))
	{
		queueNumber = 0;
	}
	else
	{
		queueNumber = pTagQueues->total_queue_count();
	}

	/* 在线客服人数 */
	onlineServiceNumber = CAppConfig::Instance()->GetServiceNumber(m_appID);

	Json::Value admin_data;
	Json::Value admin_rsp;

	admin_data["appID"]         = atoi(m_appID.c_str());
	admin_data["userNumber"]    = userNumber;
	admin_data["queueNumber"]   = queueNumber;
	admin_data["serviceNumber"] = onlineServiceNumber;

    admin_rsp["cmd"]  = m_cmd + "-reply";
    admin_rsp["code"] = 0;
    admin_rsp["msg"]  = "OK";
    admin_rsp["seq"]  = m_seq;
	admin_rsp["data"] = admin_data;

	string admin_rsp_str = admin_rsp.toStyledString();
	if (m_proc->EnququeHttp2CCD (m_ret_flow, (char *)admin_rsp_str.c_str(), admin_rsp_str.size()) != 0)
    {
    	LogError("enqueue_2_dcc failed.");
		m_errno = ERROR_SYSTEM_WRONG;
        m_errmsg = "Error send to client";
        on_error();
       	return -1;
    }
	
    return 0;
}




int AdminConfigTimer::get_id_list(string value, string idListName, vector<string> &idList)
{
	Json::Reader reader;
	Json::Value obj;
	int idNum = 0;
	
	if (!reader.parse(value, obj))
	{
		LogError("parse value to JSON failed:%s", value.c_str());
		return 0;
	}
	idNum = obj[idListName].size();
	LogTrace("idNum: %d", idNum);
	for (int i = 0; i < idNum; i++)
	{
		//idList.insert(obj[idListName][i].asString());
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
		LogTrace("userIDlist is empty!");
		return SS_OK;
	}
	userNum = get_id_list(val_userIDList, "userIDList", userIDList);
	
	/* 解析每个user / session */
	for (int i = 0; i < userNum; ++i)
	{
		/* 解析每个user的详细信息 */
		DO_FAIL(KV_parse_user(userIDList[i]));
	
		/* 解析每个user session */
		DO_FAIL(KV_parse_session(userIDList[i]));
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

	servNum = get_id_list(val_servIDList, "servIDList", servIDList);
	
	/* 解析每个service, 	添加到ServiceHeap */
	for (int i = 0; i < servNum; ++i)
	{
		DO_FAIL(KV_parse_service(servIDList[i]));
	}

	return SS_OK;
}


int AdminConfigTimer::restore_queue(string appID, vector<string> appID_tags, bool highpri)
{
	for (unsigned j = 0; j < appID_tags.size(); j++)
	{
		DO_FAIL(KV_parse_queue(appID_tags[j], highpri));
	}

	return SS_OK;
}


/*
一级数据结构
USERLIST	        -> userIDList = ['u1','u2',...]
SERVLIST	        -> servIDList = ['s1','s2',...]
QUEUE_<appID>_<tag> -> queueList  = [{userID='u1', expire_time=}, ...] //排队有先后顺序
HIGHQ_<appID>_<tag> -> highqList  = [{userID='u1', expire_time=}, ...]
*/
int AdminConfigTimer::on_admin_restore()
{
	Json::Reader reader;
	Json::Value appList;
	string appListString;

	/* 获取appID列表 */
	GET_FAIL(CAppConfig::Instance()->GetNowappIDList(appListString), "appIDList");
	if (!reader.parse(appListString, appList))
	{
		LogError("parse appIDlist to JSON failed:%s", appListString.c_str());
		ON_ERROR_PARSE_DATA("appIDList");
       	return SS_ERROR;
	}

	/* 重建userList */
	DO_FAIL(restore_userList());
	
	/* 重建serviceList,	添加到ServiceHeap */
	DO_FAIL(restore_serviceList());

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
		DO_FAIL(restore_queue(appID, appID_tags, false));
		/* 解析高优先级队列 */
		DO_FAIL(restore_queue(appID, appID_tags, true));
	}

	m_proc->m_workMode = statsvr::WORKMODE_WORK;
	LogTrace(">>>>>>>>>>>>>>>>>>>>Status Server enter workmode<<<<<<<<<<<<<<<<<<<<");

	return SS_OK;
}



