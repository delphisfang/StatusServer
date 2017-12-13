#include "app_config.h"
#include "common_api.h"
#include "debug.h"
#include "jsoncpp/json.h"
#include "data_model.h"
#include <string>

using namespace std;
using namespace statsvr;

int statsvr_global;

CAppConfig* CAppConfig::m_instance = NULL;

/************************************************************************************************/
int get_global()
{
	return statsvr_global;
}

void set_global(int glob)
{
	statsvr_global = glob;
}

int CAppConfig::UpdateappIDConf (const Json::Value &push_config_req)
{
	Json::Value configList = push_config_req["appList"];
	int size = configList.size();
	Json::Value appID_conf;
	
	for (int i = 0; i < size; ++i)
	{
		appID_conf = configList[i];
		if (appID_conf["appID"].isNull() || (!appID_conf["appID"].isString() && !appID_conf["appID"].isUInt()))
		{
			LogError("Failed to get <appID> field in request!");
			return -1;
		}

		string appID;
		if (appID_conf["appID"].isString())
		{
	    	appID = appID_conf["appID"].asString();
		}
		else
		{
			appID = ui2str(appID_conf["appID"].asUInt());
		}
		
		string appIDConf = appID_conf.toStyledString();
		appIDConf = Trim(appIDConf);
		
		unsigned version;
		if (!appID_conf["version"].isNull() && appID_conf["version"].isUInt())
		{
			version = appID_conf["version"].asUInt();
		}
		int myVersion = GetVersion(appID);
		if (myVersion >= version)
		{
			continue;
		}
		SetVersion(appID, version);

	    //LogDebug("[UpdateappIDConf] appID:[%s] conf:%s\n", appID.c_str(), appIDConf.c_str());	    
		SetConf(appID, appIDConf);

		if (!appID_conf["configs"].isNull() && appID_conf["configs"].isObject())
		{
			appID_conf = appID_conf["configs"];
		}

		//Intelligence route
		if (!appID_conf["autoTransfer"].isNull() && appID_conf["autoTransfer"].isInt())
		{
			int autoTransfer = appID_conf["autoTransfer"].asInt();
			SetValue (appID, "autoTransfer", autoTransfer);
		}
		if (!appID_conf["dynamicTransfer"].isNull() && appID_conf["dynamicTransfer"].isObject())
		{
			string dynamicTransfer = appID_conf["dynamicTransfer"].toStyledString();
			SetValue (appID, "dynamicTransfer", dynamicTransfer);
		}
		if (!appID_conf["yibotTalk"].isNull() && appID_conf["yibotTalk"].isObject())
		{
			string yibotTalk = appID_conf["yibotTalk"].toStyledString();
			SetValue (appID, "yibotTalk", yibotTalk);
		}
		if (!appID_conf["max_conv_num"].isNull() && appID_conf["max_conv_num"].isInt())
		{
			int max_conv_num = appID_conf["max_conv_num"].asInt();
			SetValue (appID, "max_conv_num", max_conv_num);
		}
		if (!appID_conf["check_user_queue_dir"].isNull() && appID_conf["check_user_queue_dir"].isInt())
		{
			int check_user_queue_dir = appID_conf["check_user_queue_dir"].asInt();
			SetValue (appID, "check_user_queue_dir", check_user_queue_dir);
		}
		if (!appID_conf["check_user_queue_num"].isNull() && appID_conf["check_user_queue_num"].isInt())
		{
			int check_user_queue_num = appID_conf["check_user_queue_num"].asInt();
			SetValue (appID, "check_user_queue_num", check_user_queue_num);
		}
		if (!appID_conf["session_timeout"].isNull() && appID_conf["session_timeout"].isInt())
		{
			int session_timeout = appID_conf["session_timeout"].asInt();
			SetValue (appID, "session_timeout", session_timeout);
		}
		if (!appID_conf["session_timewarn"].isNull() && appID_conf["session_timewarn"].isInt())
		{
			int session_timewarn = appID_conf["session_timewarn"].asInt();
			SetValue (appID, "session_timewarn", session_timewarn);
		}
		if (!appID_conf["queue_timeout"].isNull() && appID_conf["queue_timeout"].isInt())
		{
			int queue_timeout = appID_conf["queue_timeout"].asInt();
			SetValue (appID, "queue_timeout", queue_timeout);
		}
		if (!appID_conf["no_service_online_hint"].isNull() && appID_conf["no_service_online_hint"].isString())
		{
			string no_service_online_hint = appID_conf["no_service_online_hint"].asString();
			SetValue (appID, "no_service_online_hint", no_service_online_hint);
		}
		if (!appID_conf["queue_timeout_hint"].isNull() && appID_conf["queue_timeout_hint"].isString())
		{
			string queue_timeout_hint = appID_conf["queue_timeout_hint"].asString();
			SetValue (appID, "queue_timeout_hint", queue_timeout_hint);
		}
		if (!appID_conf["queue_upper_limit_hint"].isNull() && appID_conf["queue_upper_limit_hint"].isString())
		{
			string queue_upper_limit_hint = appID_conf["queue_upper_limit_hint"].asString();
			SetValue (appID, "queue_upper_limit_hint", queue_upper_limit_hint);
		}
		if (!appID_conf["timeout_warn_hint"].isNull() && appID_conf["timeout_warn_hint"].isString())
		{
			string timeout_warn_hint = appID_conf["timeout_warn_hint"].asString();
			SetValue (appID, "timeout_warn_hint", timeout_warn_hint);
		}
		if (!appID_conf["timeout_end_hint"].isNull() && appID_conf["timeout_end_hint"].isString())
		{
			string timeout_end_hint = appID_conf["timeout_end_hint"].asString();
			SetValue (appID, "timeout_end_hint", timeout_end_hint);
		}
		if (!appID_conf["recommendPre"].isNull() && appID_conf["recommendPre"].isString())
		{
			string recommendPre = appID_conf["recommendPre"].asString();
			SetValue (appID, "recommendPre", recommendPre);
		}
		if (!appID_conf["recommendEnd"].isNull() && appID_conf["recommendEnd"].isString())
		{
			string recommendEnd = appID_conf["recommendEnd"].asString();
			SetValue (appID, "recommendEnd", recommendEnd);
		}
		
		if (!appID_conf["tags"].isNull() && appID_conf["tags"].isArray())
		{
			string tags = "";

			TagUserQueue *pTagQueues = NULL;
			TagUserQueue *pTagHighPriQueues = NULL;
			
			#if 0
			//兼容没收到pingConf只收到updateConf的情况
			SessionQueue *pSessQueue = NULL;
			if (GetTagQueue(appID, pTagQueues))
			{
				DO_FAIL(AddTagQueue(appID));
				GetTagQueue(appID, pTagQueues);
			}
			if (GetTagHighPriQueue(appID, pTagHighPriQueues))
			{
				DO_FAIL(AddTagHighPriQueue(appID));
				GetTagHighPriQueue(appID, pTagHighPriQueues);
			}
			if (GetSessionQueue(appID, pSessQueue))
			{
				DO_FAIL(AddSessionQueue(appID));
				GetSessionQueue(appID, pSessQueue);
			}
			assert(pSessQueue != NULL);
			#else
			DO_FAIL(GetTagQueue(appID, pTagQueues));
			DO_FAIL(GetTagHighPriQueue(appID, pTagHighPriQueues));
			#endif
			
			assert(pTagQueues != NULL);
			assert(pTagHighPriQueues != NULL);
			
			int tagsNum = appID_conf["tags"].size();
			for (int j = 0; j < tagsNum; j++)
			{
				string tag_key = appID + "_" + appID_conf["tags"][j].asString();

				DO_FAIL(AddTagServiceHeap(tag_key));
				DO_FAIL(pTagQueues->add_tag(appID_conf["tags"][j].asString()));
				DO_FAIL(pTagHighPriQueues->add_tag(appID_conf["tags"][j].asString()));
				
				tags += tag_key + ";";
			}
			SetValue(appID, "tags", tags);
		}
		
		#if 0
		if (!appID_conf["changeServiceWord"].isNull() && appID_conf["changeServiceWord"].isArray())
		{
			int wordNum = appID_conf["changeServiceWord"].size();

			for(int i = 0; i < wordNum; i++)
			{
				string wordStr = appID_conf["changeServiceWord"][i].asString();
				AddConnectServiceWord(wordStr);
			}
		}
		#endif
	}

	return 0;
}

int CAppConfig::SetNowappIDList (string& value)
{
	SetValue("0", "appIDlist", value);
	//LogDebug("SetNowappIDList(%s) finish", value.c_str());
	return 0;
}

int CAppConfig::GetNowappIDList(string& value)
{
	GetValue("0", "appIDlist", value);
	return 0;
}

void CAppConfig::DelappID(string appID)
{
	LogDebug("Delete all data structures of appID: %s", appID.c_str());
	DelVersion(appID);
	DelConf(appID);
	#if 0
	DelQueueString(appID);
	DelOfflineHeap(appID);
	#endif
	DelTagQueue(appID);
	DelTagHighPriQueue(appID);
	DelSessionQueue(appID);
	DelTagServiceHeap(appID);
	#if 0
	string key = i2str(appID) + "_";
	DelTagHeap(key);
	#endif
}

int CAppConfig::CheckDel(const map<string, bool>& map_now)
{
	vector<string> dellist;
    Json::Reader reader;
	Json::Value appList;
	string appListString;
	
	GetNowappIDList(appListString);
	//LogDebug("appListString: %s", appListString.c_str());
	
	reader.parse(appListString, appList);
	for (int i = 0; i < appList["appIDList"].size(); i++)
	{
		string appID = appList["appIDList"][i].asString();
		//LogDebug("appID: %s", appID.c_str());
		if (map_now.find(appID) == map_now.end())
		{
			dellist.push_back(appID);
		}
	}
	LogDebug("dellist.size(): %u", dellist.size());
	
	for (uint32_t i = 0; i < dellist.size(); i++)
	{
		string appID = dellist[i];
		DelappID(appID);
	}

	LogDebug("CheckDel finish");
	return (int)dellist.size();
}

int CAppConfig::GetVersion(string appID)
{
	int version;
	GetValue(appID, "version", version);
	//LogDebug("[GetVersion] appID:[%s], version:[%d]\n", appID.c_str() ,version);
	return version;
}

int CAppConfig::SetVersion(string appID, uint32_t version)
{
	SetValue(appID, "version", (int)version);
	//LogDebug("[SetVersion] appID:[%s], version:[%d]\n", appID.c_str() ,version);
	return 0;
}

int CAppConfig::DelVersion(string appID)
{
	DelValue(appID, "version");
	//LogDebug("[DelVersion] appID:[%s]\n", appID.c_str());
	return 0;
}

int CAppConfig::SetConf(string appID, const string& conf)
{
	SetValue(appID, "config", conf);
	return 0;
}

int CAppConfig::DelConf(string appID)
{
	DelValue(appID, "config");
	return 0;
}

int CAppConfig::GetConf(string appID, string& conf)
{
	GetValue(appID, "config", conf);
	return 0;
}

int CAppConfig::DelValue (string appID, const string &key)
{
	string relkey = appID + "_" + key;
	map<string, string>::iterator it1;
	map<string, int>::iterator it2;
	it1 = mapConfigString.find(relkey);
	it2 = mapConfigInt.find(relkey);
	if (it1 != mapConfigString.end())
	{
		mapConfigString.erase(it1);
	}
	if (it2 != mapConfigInt.end())
	{
		mapConfigInt.erase(it2);
	}
	return 0;
}


int CAppConfig::SetValue (string appID, const string &key, int val)
{
	string relkey = appID + "_" + key;
	mapConfigInt[relkey] = val;
	return 0;
}


int CAppConfig::SetValue (string appID, const string &key, const string &val)
{
	string relkey = appID + "_" + key;
	mapConfigString[relkey] = val;
	return 0;
}


int CAppConfig::GetValue (string appID, const string &key, int &val)
{
	map<string, int>::iterator it;
	string relkey = appID + "_" + key;
	it = mapConfigInt.find(relkey);
	if (it == mapConfigInt.end())
	{
		val = 0;
		return -1;
	}
	else
	{
		val = it->second;
		return 0;
	}
}


int CAppConfig::GetValue (string appID, const string &key, string &val)
{
	map<string, string>::iterator it;
	string relkey = appID + "_" + key;
	
	it = mapConfigString.find(relkey);
	if (it == mapConfigString.end())
	{
		val.clear();
		return -1;
	}
	else
	{
		val = it->second;
		return 0;
	}
}


int CAppConfig::GetUser(const string& key, UserInfo &user)
{
	map<string, UserInfo>::iterator it;

	it = _userlist.find(key);
	if (it != _userlist.end())
	{
		user = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::AddUser(const string& key, const UserInfo& user)
{
	pair<map<string, UserInfo>::iterator, bool> ret;

	LogTrace("Add User[%s]:%s", key.c_str(), user.toString().c_str());

	ret = _userlist.insert(pair<string, UserInfo>(key, user));
	if (ret.second)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int CAppConfig::UpdateUser(const string& key, const UserInfo& user)
{
	LogTrace("Update User[%s]:%s", key.c_str(), user.toString().c_str());

	_userlist[key] = user;
	return 0;
}

int CAppConfig::UpdateUser(const string& key, const string& value)
{
	UserInfo user(value);
	LogTrace("Update User[%s]:%s", key.c_str(), user.toString().c_str());

	_userlist[key] = user;
	return 0;
}

int CAppConfig::DelUser(const string& key)
{
	map<string, UserInfo>::iterator it;

	LogTrace("Delete User[%s]", key.c_str());
	
	it = _userlist.find(key);
	if (it != _userlist.end())
	{
		//no need to delete it->second
		_userlist.erase(it);
		return 0;
	}
	return -1;
}

int CAppConfig::UserListToString(string& strUserIDList)
{
	map<string, UserInfo>::iterator it;
	Json::Value arr;
	Json::Value userIDList;
	string userID;

	userIDList.resize(0);
 	for (it = _userlist.begin(); it != _userlist.end(); it++)
	{
		userID = it->first;
		LogDebug("userID: %s", userID.c_str());
		userIDList.append(userID);
	}

	arr["userIDList"] = userIDList;
	strUserIDList = arr.toStyledString();
	return 0;
}

int CAppConfig::AddService(const string &key, ServiceInfo &serv)
{
	pair<map<string, ServiceInfo>::iterator, bool> ret;

	LogTrace("Add Service[%s]:%s", key.c_str(), serv.toString().c_str());

	ret = _servicelist.insert(pair<string, ServiceInfo>(key,serv));
	if (ret.second)
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int CAppConfig::GetService(const string& key, ServiceInfo &serv)
{
	map<string, ServiceInfo>::iterator it;
	
	it = _servicelist.find(key);
	if (it != _servicelist.end())
	{
		serv = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::UpdateService(const string& key, const ServiceInfo& serv)
{
	LogTrace("Update Service[%s]:%s", key.c_str(), serv.toString().c_str());

	_servicelist[key] = serv;
	return 0;
}

int CAppConfig::UpdateService(const string& key, const string& value)
{
	ServiceInfo serv(value);
	LogTrace("Update Service[%s]:%s", key.c_str(), serv.toString().c_str());

	_servicelist[key] = serv;
	return 0;
}

int CAppConfig::DelService(const string& key)
{
	map<string, ServiceInfo>::iterator it;
	
	it = _servicelist.find(key);
	if (it != _servicelist.end())
	{
		LogTrace("Delete Service[%s]:%s", key.c_str(), it->second.toString().c_str());
		_servicelist.erase(it);
	}
	return 0;
}

int CAppConfig::ServiceListToString(string& strServIDList)
{
	map<string, ServiceInfo>::iterator it;
	Json::Value arr;
	Json::Value servIDList;
	string servID;

	servIDList.resize(0);
 	for (it = _servicelist.begin(); it != _servicelist.end(); it++)
	{
		servID = it->first;
		servIDList.append(servID);
	}

	arr["serviceIDList"] = servIDList;
	strServIDList = arr.toStyledString();
	return 0;
}


int CAppConfig::AddTagServiceHeap(const string& key)
{
	map<string, ServiceHeap>::iterator it;
	
	it = tagServiceHeap.find(key);
	if (it == tagServiceHeap.end())
	{
		LogDebug("Add service heap for tag: %s", key.c_str());
		ServiceHeap serviceHeap;
		tagServiceHeap[key] = serviceHeap;
	}
	return 0;
}

int CAppConfig::UpdateTagServiceHeap(const string& key, const string& value)
{
	ServiceHeap serviceHeap(value);
	LogTrace("Update TagServiceHeap[%s]:%s", key.c_str(), serviceHeap.toString().c_str());
	tagServiceHeap[key] = serviceHeap;
	return 0;
}

int CAppConfig::UpdateTagServiceHeap(const string& key, const ServiceHeap& serviceHeap)
{
	LogTrace("Update TagServiceHeap[%s]:%s",key.c_str(), serviceHeap.toString().c_str());
	tagServiceHeap[key] = serviceHeap;
	return 0;
}

int CAppConfig::GetTagServiceHeap(const string& key, ServiceHeap& serviceHeap)
{
	map<string, ServiceHeap>::iterator it;
	it = tagServiceHeap.find(key);
	if (it != tagServiceHeap.end())
	{
		serviceHeap = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::DelTagServiceHeap(const string& appID) //delete a batch of tag related to appID
{
	map<string, ServiceHeap>::iterator it;

	LogTrace("Delete all TagServiceHeaps of appID[%s]", appID.c_str());
	for (it = tagServiceHeap.begin(); it != tagServiceHeap.end();)
	{
		if (string::npos != (it->first).find(appID))
		{
			tagServiceHeap.erase(it++);
		}
		else
		{
			it++;
		}
	}
	return 0;
}

//add serviceID into every tag serviceHeap
int CAppConfig::AddService2Tags(const string &appID, ServiceInfo &serv)
{
	ServiceHeap servHeap;
	string app_serviceID = appID + "_" + serv.serviceID;

	for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
	{
		string app_tag = appID + "_" + (*it);
		if (CAppConfig::Instance()->GetTagServiceHeap(app_tag, servHeap))
		{
			LogError("Fail to find service heap for tag: %s", app_tag.c_str());
			continue;
		}

		servHeap.add_service(app_serviceID);

		if (CAppConfig::Instance()->UpdateTagServiceHeap(app_tag, servHeap))
		{
			LogError("Fail to update service heap for tag: %s", app_tag.c_str());
			continue;
		}
	}

	return 0;
}

int CAppConfig::DelServiceFromTags(const string &appID, ServiceInfo &serv)
{
	ServiceHeap servHeap;
	string app_serviceID = appID + "_" + serv.serviceID;

	for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
	{
		string app_tag = appID + "_" + (*it);
		if (CAppConfig::Instance()->GetTagServiceHeap(app_tag, servHeap))
		{
			LogError("Fail to find service heap for tag: %s", app_tag.c_str());
			continue;
		}

		servHeap.del_service(app_serviceID);

		if (CAppConfig::Instance()->UpdateTagServiceHeap(app_tag, servHeap))
		{
			LogError("Fail to update service heap for tag: %s", app_tag.c_str());
			continue;
		}
	}

	return 0;
}

int CAppConfig::CanOfferService(const ServiceHeap& servHeap, int serverNum)
{
	for (set<string>::iterator i = servHeap._servlist.begin(); i != servHeap._servlist.end(); i++)
	{
		string servID = *i;
		ServiceInfo serv;
		if (SS_OK != GetService(servID, serv))
		{
			continue;
		}
		else if (serv.user_count() < serverNum && "offline" != serv.status)
		{
			return 0;
		}
	}
	return -1;
}

//如果APP下所有坐席都busy或offline，则让用户排队
int CAppConfig::CanAppOfferService(const string& appID)
{
    string strTags;
    vector<string> tags;
	unsigned serverNum = 0;
	
	serverNum = CAppConfig::Instance()->getMaxConvNum(appID);
	CAppConfig::Instance()->GetValue(appID, "tags", strTags);
	//LogDebug("[%s]: strTags:%s", appID.c_str(), strTags.c_str());
    MySplitTag((char *)strTags.c_str(), ";", tags);

	for (int i = 0; i < tags.size(); ++i)
	{
		//LogTrace("====> tags[%d]: %s", i, tags[i].c_str());
		
        ServiceHeap servHeap;
        if (CAppConfig::Instance()->GetTagServiceHeap(tags[i], servHeap))
        {
			LogWarn("Failed to get service heap for tag: %s, find next service heap!", tags[i].c_str());
            continue;
        }

		for (set<string>::iterator it = servHeap._servlist.begin(); it != servHeap._servlist.end(); ++it)
		{
			ServiceInfo serv;

			if (CAppConfig::Instance()->GetService(*it, serv) || "offline" == serv.status || serv.user_count() >= serverNum)
			{
				continue;
			}
			else
			{
				return 0;
			}
		}
	}
	
	return -1;
}

int CAppConfig::AddTagQueue(string appID)
{
	map<string, TagUserQueue*>::iterator it;
	
	it = appTagQueues.find(appID);
	if (it == appTagQueues.end())
	{
		appTagQueues[appID] = new TagUserQueue();
	}
	return 0;
}

int CAppConfig::DelTagQueue(string appID)
{
	map<string, TagUserQueue*>::iterator it;
	
	it = appTagQueues.find(appID);
	if(it != appTagQueues.end())
	{
		delete it->second;
		appTagQueues.erase(it);
		return 0;
	}
	return -1;
}

int CAppConfig::GetTagQueue(string appID, TagUserQueue* &tuq)
{
	map<string, TagUserQueue*>::iterator it;
	
	it = appTagQueues.find(appID);
	if (it != appTagQueues.end())
	{
		tuq = it->second;
		return 0;
	}
	return -1;
}


int CAppConfig::AddTagHighPriQueue(string appID)
{
	map<string, TagUserQueue*>::iterator it;
	
	it = appTagHighPriQueues.find(appID);
	if (it == appTagHighPriQueues.end())
	{
		appTagHighPriQueues[appID] = new TagUserQueue();
	}
	return 0;
}

int CAppConfig::DelTagHighPriQueue(string appID)
{
	map<string, TagUserQueue*>::iterator it;
	
	it = appTagHighPriQueues.find(appID);
	if(it != appTagHighPriQueues.end())
	{
		delete it->second;
		appTagHighPriQueues.erase(it);
		return 0;
	}
	return -1;
}

int CAppConfig::GetTagHighPriQueue(string appID, TagUserQueue* &tuq)
{
	map<string, TagUserQueue*>::iterator it;
	
	it = appTagHighPriQueues.find(appID);
	if (it != appTagHighPriQueues.end())
	{
		tuq = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::AddSessionQueue(string appID)
{
	map<string, SessionQueue*>::iterator it;
	
	it = appSessionQueue.find(appID);
	if (it == appSessionQueue.end())
	{
		appSessionQueue[appID] = new SessionQueue();
	}
	
	return 0;
}

int CAppConfig::UpdateSessionQueue(string appID, string& value)
{
	map<string, SessionQueue*>::iterator it;
	SessionQueue* pSessionQueue = NULL;
	int timeWarn  = 0;
	int timeout = 0;
	
	it = appSessionQueue.find(appID);
	if (it == appSessionQueue.end())
	{
		return -1;
	}
	else
	{
		pSessionQueue = it->second;
	}

	Session session(value);
	// read <timewarn> and <timeout> from config
	if (GetValue(appID, "session_timewarn", timeWarn) || timeWarn == 0)
	{
		timeWarn = 10 * 60;
	}
	else
	{
		timeWarn *= 60;
	}
	
	if (GetValue(appID, "session_timeout", timeout) || timeout == 0)
	{
		timeout = 15 * 60;
	}
	else
	{
		timeout *= 60;
	}
	
	pSessionQueue->insert(session.userID, &session, timeWarn, timeout);
	return 0;
}


int CAppConfig::GetSessionQueue(string appID, SessionQueue* &pSessionQueue)
{
	map<string, SessionQueue*>::iterator it;
	
	it = appSessionQueue.find(appID);
	if (it != appSessionQueue.end())
	{
		pSessionQueue = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::DelSessionQueue(string appID)
{
	map<string, SessionQueue*>::iterator it;
	
	it = appSessionQueue.find(appID);
	if (it != appSessionQueue.end())
	{
		delete it->second;
		appSessionQueue.erase(it);
	}
	return 0;
}

unsigned CAppConfig::GetTagServiceNumber(string appID, string raw_tag)
{
	map<string, ServiceHeap>::iterator it;
	ServiceHeap sh;
	string key = appID + "_" + raw_tag;
	
	it = tagServiceHeap.begin();
	while (it != tagServiceHeap.end())
	{
		if (key == it->first)
		{
			sh = it->second;
			return sh.size();
		}
		it++;
	}
	
	return 0;
}

unsigned CAppConfig::GetServiceNumber(string appID)
{
	map<string, ServiceInfo>::iterator it;
	unsigned servNum = 0;

	for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
	{
		if (appID == getappID(it->first))
		{
			ServiceInfo serv;
			if (0 == GetService(it->first, serv))
			{
				++servNum;
			}
		}
	}
	
	return servNum;
}

//注意，1个service可以属于多个tag，不要重复计算
unsigned CAppConfig::GetOnlineServiceNumber(string appID)
{
	unsigned servNum = 0;

	map<string, ServiceInfo>::iterator it;
	for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
	{
		if (appID == getappID(it->first))
		{
			ServiceInfo serv;
			if (0 == GetService(it->first, serv) && "offline" != serv.status)
			{
				++servNum;
			}
		}
	}
	
	return servNum;
}

int CAppConfig::AddTagOnlineServiceNumber(string appID, string raw_tag)
{
	map<string, unsigned>::iterator it;
	string app_tag = appID + "_" + raw_tag;
	
	it = mapOnlineServiceNumber.find(app_tag);
	if (it == mapOnlineServiceNumber.end())
	{	
		mapOnlineServiceNumber[app_tag] = 1;
	}
	else
	{
		(it->second)++;
	}

	return 0;
}

unsigned CAppConfig::GetTagOnlineServiceNumber(string appID, string raw_tag)
{
	#if 0
	map<string, unsigned>::iterator it;
	string app_tag = appID + "_" + raw_tag;
	
	it = mapOnlineServiceNumber.find(app_tag);
	if (it == mapOnlineServiceNumber.end())
	{
		return 0;
	}
	return it->second;
	#else
	unsigned servNum = 0;

	map<string, ServiceInfo>::iterator it;
	for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
	{
		if (appID == getappID(it->first))
		{
			ServiceInfo &serv = it->second;
			if ("offline" != serv.status)
			{
				set<string>::iterator it2;
				for (it2 = serv.tags.begin(); it2 != serv.tags.end(); ++it2)
				{
					if (*it2 == raw_tag)
					{
						++servNum;
						break;
					}
				}
			}
		}
	}
	
	return servNum;

	#endif
}


int CAppConfig::DelTagOnlineServiceNumber(string appID, string raw_tag)
{
	map<string, unsigned>::iterator it;
	string app_tag = appID + "_" + raw_tag;
	
	it = mapOnlineServiceNumber.find(app_tag);
	if (it == mapOnlineServiceNumber.end())
	{
		mapOnlineServiceNumber[app_tag] = 0;
	}
	else
	{
		(it->second)--;
	}

	return 0;
}


///TODO: 优化性能
int CAppConfig::CheckTimeoutServices(long long time_gap, set<string>& serviceList)
{
	long long nowTime = time(NULL);
	map<string, ServiceInfo>::iterator it;
	
	for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
	{
		ServiceInfo serv = it->second;
		long long atime  = (serv.atime / 1000);
		if (nowTime >= atime + time_gap)
		{
			serviceList.insert(it->first);
		}
	}
	return 0;
}




int CAppConfig::checkAppIDExist(string appID)
{
    Json::Reader reader;
	Json::Value appList;
	string appListString;
	
	if (GetNowappIDList(appListString))
	{
		LogError("get appIDlist failed.");
       	return -1;
	}

    if (!reader.parse(appListString, appList))
	{
		LogError("parse appIDlist to JSON failed:%s", appListString.c_str());
       	return -1;
	}

    for (int i = 0; i < appList["appIDList"].size(); i++)
    {
        if (appID == appList["appIDList"][i].asString())
        {
			return 0;
        }
    }
	return -1;
}

/* tag is with appID prefix */ 
int CAppConfig::checkTagExist(string appID, string tag)
{
	string strTags;
	vector<string> tags;

	if (GetValue(appID, "tags", strTags))
	{
		LogError("[%s]: get appID tags failed.", appID.c_str());
		return -1;
	}

	MySplitTag((char *)strTags.c_str(), ";", tags);
	for (int i = 0; i < tags.size(); i++)
	{
		//LogDebug("tag: %s, tags[i]: %s", tag.c_str(), tags[i].c_str());
		if (tag == tags[i])
		{
			return 0;
		}
	}
	return -1;
}

long long CAppConfig::getDefaultSessionTimeWarn(string appID)
{
	int session_timewarn = 0;
	
	if (CAppConfig::Instance()->GetValue(appID, "session_timewarn", session_timewarn) 
		|| session_timewarn == 0)
	{
		session_timewarn = 10 * 60;
	}
	else
	{
		session_timewarn *= 60;
	}

	return (long long)session_timewarn;
}

long long CAppConfig::getDefaultSessionTimeOut(string appID)
{
	int session_timeout = 0;
	
	if (CAppConfig::Instance()->GetValue(appID, "session_timeout", session_timeout) 
		|| session_timeout == 0)
	{
		session_timeout = 15 * 60;
	}
	else
	{
		session_timeout *= 60;
	}
	return (long long)session_timeout;
}

long long CAppConfig::getDefaultQueueTimeout(string appID)
{
	int queue_timeout = 0;
	
    if (CAppConfig::Instance()->GetValue(appID, "queue_timeout", queue_timeout) || 0 == queue_timeout)
    {
        queue_timeout = 30 * 60;
    }
    else
    {
        queue_timeout *= 60;
    }
	return (long long)queue_timeout;
}

int CAppConfig::getMaxConvNum(string appID)
{
	int max_conv_num        = 0;
	
	if (CAppConfig::Instance()->GetValue(appID, "max_conv_num", max_conv_num) || 0 == max_conv_num)
    {
        max_conv_num = 5;
    }
	
	return max_conv_num;
}

int CAppConfig::getUserQueueNum(string appID)
{
	int user_queue_num = 1;

    if (CAppConfig::Instance()->GetValue(appID, "check_user_queue_num", user_queue_num) || 0 == user_queue_num)
    {
		user_queue_num = 1;
    }

	return user_queue_num;
}

int CAppConfig::getUserQueueDir(string appID)
{
	int user_queue_dir = 1; //默认从队尾拉取

    if (CAppConfig::Instance()->GetValue(appID, "check_user_queue_dir", user_queue_dir))
    {
		user_queue_dir = 1;
    }

	return user_queue_dir;
}

string CAppConfig::getTimeOutHint(string appID)
{
	string hint;
	CAppConfig::Instance()->GetValue(appID, "timeout_end_hint", hint);
	return hint;
}

string CAppConfig::getTimeWarnHint(string appID)
{
	string hint;
	CAppConfig::Instance()->GetValue(appID, "timeout_warn_hint", hint);
	return hint;
}

string CAppConfig::getNoServiceOnlineHint(string appID)
{
	string hint;
	CAppConfig::Instance()->GetValue(appID, "no_service_online_hint", hint);
	return hint;
}

string CAppConfig::getQueueTimeoutHint(string appID)
{
	string hint;
	CAppConfig::Instance()->GetValue(appID, "queue_timeout_hint", hint);
	return hint;
}

string CAppConfig::getQueueUpperLimitHint(string appID)
{
	string hint;
	CAppConfig::Instance()->GetValue(appID, "queue_upper_limit_hint", hint);
	return hint;
}


void CAppConfig::getUserListJson(string appID, Json::Value &userList)
{
	UserInfo user;
	Json::Value userJson;
	string temp_appID;

	userList.resize(0);
	map<string, UserInfo>::iterator it;
	for (it = _userlist.begin(); it != _userlist.end(); it++)
	{
		temp_appID = getappID(it->first);
		if (temp_appID == appID)
		{
			user = it->second;
			user.toJson(userJson);
			userList.append(userJson);
		}
	}
}

void CAppConfig::getServiceListJson(string appID, Json::Value &servList)
{
	ServiceInfo serv;
	Json::Value servJson;
	string temp_appID;

	servList.resize(0);
	map<string, ServiceInfo>::iterator it;
	for (it = _servicelist.begin(); it != _servicelist.end(); it++)
	{
		temp_appID = getappID(it->first);
		if (temp_appID == appID)
		{
			serv = it->second;
			serv.toJson(servJson);
			servList.append(servJson);
		}
	}
}

void CAppConfig::getSessionQueueJson(string appID, Json::Value &sessQueue)
{
	SessionQueue *pSessQueue = NULL;
	SessionTimer sessTimer;
	Session sess;
	Json::Value sessJson;

	sessQueue.resize(0);
	
	map<string, SessionQueue*>::iterator it;
	list<SessionTimer>::iterator it2;
	
	it = appSessionQueue.find(appID);
	if (it != appSessionQueue.end())
	{
		pSessQueue = it->second;
		for (it2 = pSessQueue->_sess_list.begin(); it2 != pSessQueue->_sess_list.end(); it2++)
		{
			sessTimer = (*it2);
			sessJson["warn_time"]   = sessTimer.warn_time;
			sessJson["expire_time"] = sessTimer.expire_time;
			
			sess = sessTimer.session;
			sess.toJson(sessJson);

			sessQueue.append(sessJson);
		}
	}
}

void CAppConfig::getTagQueueJson(string appID, Json::Value &tags, bool isHighPri)
{
	TagUserQueue *pTags = NULL;
	UserQueue *uq = NULL;
	UserTimer ut;

	tags.resize(0);
	
	map<string, TagUserQueue*>::iterator it;
	map<string, UserQueue*>::iterator it2;
	list<UserTimer>::iterator it3;

	if (true == isHighPri)
	{
		it = appTagHighPriQueues.find(appID);
		if (it == appTagHighPriQueues.end())
		{
			return;
		}
	}
	else
	{
		it = appTagQueues.find(appID);
		if (it == appTagQueues.end())
		{
			return;
		}
	}
	
	pTags = it->second;

	for (it2 = pTags->_tag_queue.begin(); it2 != pTags->_tag_queue.end(); it2++)
	{
		Json::Value tagJson;
		tagJson["tag"] = it2->first;

		Json::Value queueJson;
		queueJson.resize(0);

		uq = it2->second;
		for (it3 = uq->_user_list.begin(); it3 != uq->_user_list.end(); it3++)
		{
			ut = (*it3);
			Json::Value userJson;
			userJson["expire_time"] = ut.expire_time;
			userJson["userID"] = ut.userID;

			queueJson.append(userJson);
		}
		tagJson["userQueue"] = queueJson;
		tags.append(tagJson);
	}
}

void CAppConfig::getTagHighPriQueueJson(string appID, Json::Value &tags)
{
	getTagQueueJson(appID, tags, true);
}


void CAppConfig::getTagNormalQueueJson(string appID, Json::Value &tags)
{
	getTagQueueJson(appID, tags, false);
}

void CAppConfig::getOnlineServiceNumJson(string appID, Json::Value &tags)
{
	map<string, unsigned>::iterator it;

	tags = Json::objectValue;
	for (it = mapOnlineServiceNumber.begin(); it != mapOnlineServiceNumber.end(); it++)
	{
		string app_tag = it->first;
		if (appID == getappID(app_tag))
		{
			string raw_tag = delappID(app_tag);
			tags[raw_tag]   = it->second;
		}
	}
}

#if ((((((((0))))))))
#endif
/************************************************************************************************/

#if 0
int CAppConfig::GetVersion (unsigned appID)
{
	int version;
	GetValue(appID, "version", version);
	return version;
}

int CAppConfig::SetVersion (unsigned appID, uint32_t version)
{
	SetValue(appID, "version", (int)version);
	DEBUG_P (LOG_DEBUG, "[SetVersion] appID:[%d], version:[%d]\n", appID ,version);
	return 0;
}

int CAppConfig::DelVersion(unsigned appID)
{
	DelValue(appID, "version");
	DEBUG_P (LOG_DEBUG, "[DelVersion] appID:[%d]\n", appID);
	return 0;
}

int CAppConfig::SetConf(unsigned appID, const string& conf)
{
	SetValue(appID, "config", conf);
	return 0;
}

int CAppConfig::DelConf(unsigned appID)
{
	DelValue(appID, "config");
	return 0;
}

int CAppConfig::GetConf(unsigned appID, string& conf)
{
	GetValue(appID, "config", conf);
	return 0;

}







int CAppConfig::GetHeapNumber(const ServiceHeap &serviceHeap)
{
	int number = 0;
	for(set<string>::iterator i = serviceHeap.tagServiceList.begin(); i != serviceHeap.tagServiceList.end(); i++)
	{
		string tmpService = *i;
		ServiceStatus serviceStatus;
		if(GetServiceStatus(tmpService, serviceStatus))
		{
			continue;
		}
		number += serviceStatus.userList.size();
	}
	return number;
}

//只要没有userList的serviceStatus，都是need del的
int CAppConfig::GetOfflineNeedDelServiceList(const ServiceHeap &serviceHeap)
{
	int number = 0;
	for(set<string>::iterator i = serviceHeap.tagServiceList.begin(); i != serviceHeap.tagServiceList.end(); i++)
	{
		string tmpService = *i;
		ServiceStatus serviceStatus;
		if(GetServiceStatus(tmpService, serviceStatus))
		{
			continue;
		}
		if(serviceStatus.userList.size() == 0)
		{
			number++;
		}	
	}
	return number;
}


int CAppConfig::UpdateKickServiceStatus(const string& key,const ServiceStatus& serviceStatus)
{
	//DEBUG_P(LOG_NORMAL, "Update KickServiceStatus[%s]:%s\n", key.c_str(), serviceStatus.toString().c_str());
	
	mapKickServiceStatus[key] = serviceStatus;
	
	return 0;
}

int CAppConfig::GetKickServiceStatus(const string& key, ServiceStatus &serviceStatus)
{
	map<string, ServiceStatus>::iterator it;
	it = mapKickServiceStatus.find(key);
	if(it != mapKickServiceStatus.end())
	{
		serviceStatus = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::DelKickServiceStatus(const string& key)
{
	map<string, ServiceStatus>::iterator it;
	it = mapKickServiceStatus.find(key);
	if(it != mapKickServiceStatus.end())
	{
		DEBUG_P(LOG_NORMAL, "Del KickServiceStatus[%s]:%s\n", key.c_str(), it->second.toString().c_str());
		mapKickServiceStatus.erase(it);
	}
	return 0;
}


int CAppConfig::AddOfflineHeap(unsigned appID)
{
	map<unsigned, ServiceHeap>::iterator it;
	it = mapOfflineHeap.find(appID);
	if(it == mapOfflineHeap.end())
	{
		ServiceHeap serviceHeap;
		mapOfflineHeap[appID] = serviceHeap;
	}
	return 0;
}

int CAppConfig::UpdateOfflineHeap(unsigned appID, ServiceHeap& serviceHeap)
{
	DEBUG_P(LOG_NORMAL, "Update OfflineHeap[%s]:%s\n", i2str(appID).c_str(), serviceHeap.toString().c_str());
	mapOfflineHeap[appID] = serviceHeap;
	return 0;
}

int CAppConfig::UpdateOfflineHeap(unsigned appID, const string& value)
{
	DEBUG_P(LOG_NORMAL, "Update OfflineHeap[%s]:%s\n", i2str(appID).c_str(), value.c_str());
	ServiceHeap serviceHeap(value);
	mapOfflineHeap[appID] = serviceHeap;
	return 0;
}

int CAppConfig::GetOfflineHeap(unsigned appID, ServiceHeap& value)
{
	map<unsigned, ServiceHeap>::iterator it;
	it = mapOfflineHeap.find(appID);
	if(it != mapOfflineHeap.end())
	{
		value = it->second;
		return 0;
	}
	return -1;
}

int CAppConfig::DelOfflineHeap(unsigned appID)
{
	map<unsigned, ServiceHeap>::iterator it;
	it = mapOfflineHeap.find(appID);
	if(it != mapOfflineHeap.end())
	{
		mapOfflineHeap.erase(it);
		return 0;
	}
	return -1;
}

int CAppConfig::UpdateOnlineService(const string& serviceID)
{
	mapServiceOnlineTime[serviceID] = time(NULL);
}

int CAppConfig::UpdateOnlineService(const string& serviceID, time_t checkTime)
{
	mapServiceOnlineTime[serviceID] = checkTime;
}

int CAppConfig::CheckOnlineService(time_t time_gap, set<string>& serviceList)
{
	time_t nowTime = time(NULL);
	map<string, time_t>::iterator it;
	for(it = mapServiceOnlineTime.begin(); it != mapServiceOnlineTime.end();)
	{
		if(nowTime >= it->second + time_gap)
		{
			serviceList.insert(it->first);
			mapServiceOnlineTime.erase(it++);
		}
		else
		{
			it++;
		}
	}
	return 0;
}

int CAppConfig::DelOnlineService(const string& serviceID)
{
	map<string, time_t>::iterator it;
	it = mapServiceOnlineTime.find(serviceID);
	if(it != mapServiceOnlineTime.end())
	{
		mapServiceOnlineTime.erase(it);
		return 0;
	}
	return -1;
}



void CAppConfig::DelappID(unsigned appID)
{
	LogError("DelApp:appID:%u", appID);
	DelVersion(appID);
	DelConf(appID);
	DelQueue(appID);
	DelHighPriQueue(appID);
	DelQueueString(appID);
	DelOfflineHeap(appID);
	DelSessionQueue(appID);

	string key = i2str(appID) + "_";
	DelTagHeap(key);
}


int CAppConfig::SetQueueString(unsigned appID, const string& queueString)
{
	mapappIDQueueString[appID] = queueString;
	return 0;
}


int CAppConfig::GetQueueString(unsigned appID, string& queueString)
{
	map<unsigned,string>::iterator it;
	it = mapappIDQueueString.find(appID);
	if(it != mapappIDQueueString.end())
	{
		queueString = it->second;
		return 0;
	}
	queueString = string();
	return 0;
}

int CAppConfig::AddQueueString(unsigned appID, const string& queueString)
{
	map<unsigned,string>::iterator it;
	it = mapappIDQueueString.find(appID);
	if(it != mapappIDQueueString.end())
	{
		it->second += queueString + ";";
		return 0;
	}
	return -1;
}

int CAppConfig::CancelQueueString(unsigned appID, string& userID)
{
	string queueString;
	GetQueueString(appID, queueString);
	if(string::npos == queueString.find(userID))
	{
		return 0;
	}
	int pos = queueString.find(userID);
    int pos2 = queueString.find(";", pos);
	if (string::npos == pos2)
	{
		pos2 = queueString.size();
	}
	string tmp = queueString.substr(0, pos + 1);
	int pos1 = tmp.rfind(";");
	if (string::npos == pos1)
	{
	    queueString.replace(0, pos2, "");
	}
    else
    {
	    queueString.replace(pos1, pos2 - pos1, "");
    }
	mapappIDQueueString[appID] = queueString;
	return 0;
}

int CAppConfig::PopQueueString(unsigned appID)
{
	map<unsigned,string>::iterator it;
	it = mapappIDQueueString.find(appID);
	if(it != mapappIDQueueString.end())
	{
		int pos = (it->second).find(';');
		(it->second).replace(0, pos + 1, "");
		return 0;
	}
	return -1;
}

int CAppConfig::DelQueueString(unsigned appID)
{
	map<unsigned,string>::iterator it;
	it = mapappIDQueueString.find(appID);
	if(it != mapappIDQueueString.end())
	{
		mapappIDQueueString.erase(it);
		return 0;
	}
	return -1;
}



int CAppConfig::AddConnectServiceWord(string &word)
{
	list<string>::iterator it;

	for(it = needConnectServiceWord.begin(); it != needConnectServiceWord.end(); it++)
	{
		if(*it == word)
		{
			return -1;
		}
	}

	needConnectServiceWord.push_back(word);
	return 0;
}

int CAppConfig::FindConnectServiceWord(string &msg)
{
	list<string>::iterator it;

	for(it = needConnectServiceWord.begin(); it != needConnectServiceWord.end(); it++)
	{
		if(string::npos != msg.find(*it))
		{
			return 0;
		}
	}

	return -1;
}

int CAppConfig::GetConnectServiceWord(list<string> &words)
{
	words = needConnectServiceWord;
	return 0;
}
#endif
