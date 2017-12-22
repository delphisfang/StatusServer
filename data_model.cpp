#include "data_model.h"

using namespace tfc::base;
using namespace statsvr;

Session::Session()
{
	sessionID.clear();
	userID.clear();
	serviceID.clear();
	atime = 0;
	btime = 0;
	cpIP.clear();
	cpPort = 0;
	notified = 0;
	/*toIM = 0;
	whereFrom.clear();
	channel.clear();
	userCount = 0;
	*/
}

Session::~Session()
{
	sessionID.clear();
	userID.clear();
	serviceID.clear();
	atime = 0;
	btime = 0;
	cpIP.clear();
	cpPort = 0;
	notified = 0;
	/*toIM = 0;
	whereFrom.clear();
	channel.clear();
	userCount = 0;
	*/
}

Session::Session(const string& strSession)
{
	Json::Reader reader;
	Json::Value value;
	
	if (!reader.parse(strSession, value))
	{
		return;
	}
	
	sessionID = get_value_str(value, SESSION_ID);
	userID	  = get_value_str(value, USER_ID);
	serviceID = get_value_str(value, SERV_ID);
	atime	  = get_value_uint64(value, ACTIVE_TIME);
	btime     = get_value_uint64(value, BUILD_TIME);
	cpIP      = get_value_str(value, CP_IP);
	cpPort    = get_value_uint(value, CP_PORT);
	notified  = 0;
	
	/*if (!value["toIM"].isNull() && (value["toIM"].isBool() || value["toIM"].isInt()))
	{
		toIM = value["toIM"].asBool();
	}
	if (!value["whereFrom"].isNull() && value["whereFrom"].isString())
	{
		whereFrom = value["whereFrom"].asString();
	}
	if (!value["channel"].isNull() && value["channel"].isString())
	{
		channel = value["channel"].asString();
	}
	if(!value["userCount"].isNull() && value["userCount"].isUInt())
	{
		userCount = value["userCount"].asUInt();
	}*/
}

void Session::toJson(Json::Value &value) const
{
	value[SESSION_ID]  = sessionID;
	value[USER_ID]	   = userID;
	value[SERV_ID]	   = serviceID;
	value[ACTIVE_TIME] = atime;
	value[BUILD_TIME]  = btime;
	value[CP_IP]	   = cpIP;
	value[CP_PORT]	   = cpPort;
	value[NOTIFIED]    = notified;

	//value["toIM"] = toIM;
	//value["whereFrom"] = whereFrom;
	//value["channel"] = channel;
	//value["userCount"] = userCount;
}

string Session::toString() const
{
	Json::Value value;
	toJson(value);

	Json::FastWriter writer;
	return writer.write(value);
}

bool Session::has_refreshed() const
{
	if (atime != btime)
	{
		return true;
	}
	else
	{
		return false;
	}
}

UserInfo::UserInfo()
{
	userID.clear();
	cpIP.clear();
	cpPort = 0;
	tag.clear();
	status = IN_YIBOT;
	atime = 0;
	qtime = 0;
	sessionID.clear();
	lastServiceID.clear();
	priority.clear();
	queuePriority = 0;
	channel.clear();
	extends.clear();
	//userInfo.clear();
}

UserInfo::~UserInfo()
{
	userID.clear();
	cpIP.clear();
	cpPort = 0;
	tag.clear();
	status = IN_YIBOT;
	atime = 0;
	qtime = 0;
	sessionID.clear();
	lastServiceID.clear();
	priority.clear();
	queuePriority = 0;
	channel.clear();
	extends.clear();
	//userInfo.clear();
}

UserInfo::UserInfo(const string& strUserInfo)
{
	Json::Reader reader;
	Json::Value value;
	timeval nowTime;
	
	if (!reader.parse(strUserInfo, value))
	{
		return;
	}

	UserInfo();
	
	//LogDebug("value: %s", value.toStyledString().c_str());
	userID = get_value_str(value, USER_ID);
	cpIP   = get_value_str(value, CP_IP);
	cpPort = get_value_uint(value, CP_PORT);
	tag    = get_value_str(value, USER_TAG);
	status = get_value_str(value, STATUS, IN_YIBOT);
	
	/*if (!value["activeTime"].isNull() && value["activeTime"].isInt64())
	{
		atime = value["activeTime"].asInt64();
	}
	*/

	atime         = GetCurTimeStamp();
	qtime		  = get_value_uint64(value, QTIME);
	sessionID	  = get_value_str(value, SESSION_ID);
	lastServiceID = get_value_str(value, LAST_SERV_ID);
	priority	  = get_value_str(value, PRIO);
	queuePriority = get_value_uint(value, QUEUE_PRIO);
	channel 	  = get_value_str(value, CHANNEL);

	//LogDebug("construct userInfo: %s", toString().c_str());
}

void UserInfo::toJson(Json::Value &value) const
{
	value[USER_ID]		= userID;
	value[CP_IP]		= cpIP;
	value[CP_PORT]		= cpPort;
	value[USER_TAG] 	= tag;
	value[STATUS]		= status;
	value[ACTIVE_TIME]	= atime;
	value[QTIME]		= qtime;
	value[SESSION_ID]	= sessionID;
	value[LAST_SERV_ID] = lastServiceID;
	value[PRIO] 		= priority;
	value[QUEUE_PRIO]	= queuePriority;
	value[CHANNEL]		= channel;
	//value["userInfo"] = userInfo;

	#if 0
	Json::Reader reader;
	Json::Value  obj_extends;
	if (!reader.parse(extends, obj_extends))
	{
		value["extends"] = Json::objectValue;				
	}
	else
	{
		value["extends"] = obj_extends;
	}
	#endif
}

string UserInfo::toString() const
{
	Json::Value value;
	toJson(value);

	Json::FastWriter writer;
	return writer.write(value);
}


ServiceInfo::ServiceInfo()
{
	serviceID.clear();
	status = DEF_SERV_STATUS;
	atime = 0;
	cpIP.clear();
	cpPort = 0;
	tags.clear();
	userList.clear();
	serviceName.clear();
	serviceAvatar.clear();
	maxUserNum = 5;
	//whereFrom.clear();
}

ServiceInfo::~ServiceInfo()
{
	serviceID.clear();
	status = DEF_SERV_STATUS;
	atime = 0;
	cpIP.clear();
	cpPort = 0;
	tags.clear();
	userList.clear();
	serviceName.clear();
	serviceAvatar.clear();
	//whereFrom.clear();
}

ServiceInfo::ServiceInfo(const string& strServiceInfo, unsigned dft_user_num)
{
	Json::Reader reader;
	Json::Value value;
	
	if (!reader.parse(strServiceInfo, value))
	{
		return;
	}

	serviceID 		 = get_value_str(value, SERV_ID);
	status	  		 = DEF_SERV_STATUS;
	atime			 = GetCurTimeStamp();
	cpIP			 = get_value_str(value, CP_IP);
	cpPort			 = get_value_uint(value, CP_PORT);
	serviceName 	 = get_value_str(value, SERV_NAME);
	serviceAvatar    = get_value_str(value, SERV_AVATAR);
	maxUserNum       = get_value_uint(value, MAX_USER_NUM_FIELD, dft_user_num);
	//whereFrom 	 = value["whereFrom"].asString();

	parse_tags(value);
	parse_userList(value);
}

void ServiceInfo::parse_tags(const Json::Value &value)
{
	unsigned tagsLen = value["tags"].size();
	for (unsigned i = 0; i < tagsLen; i++)
	{
		string raw_tag;
		if (get_value_str_safe(value["tags"][i], raw_tag))
		{
			LogError("Error get tags[%d]!", i);
			continue;
		}
		else
		{
			tags.insert(raw_tag);
		}
	}
}

void ServiceInfo::parse_userList(const Json::Value &value)
{
	unsigned userLen = value["userList"].size();
	for (unsigned i = 0; i < userLen; i++)
	{
		string raw_userID;
		if (get_value_str_safe(value["userList"][i], raw_userID))
		{
			LogError("Error get userList[%d]!", i);
			continue;
		}
		else
		{
			userList.insert(raw_userID);
		}
	}
}

void ServiceInfo::toJson(Json::Value &value) const
{
	value[SERV_ID]	   = serviceID;
	value[STATUS] 	   = status;
	value[ACTIVE_TIME] = atime;
	value[CP_IP]       = cpIP;
	value[CP_PORT]     = cpPort;
	value[SERV_NAME]   = serviceName;
	value[SERV_AVATAR] = serviceAvatar;
	value[MAX_USER_NUM_FIELD] = maxUserNum;
	//value["whereFrom"] = whereFrom;

	Json::Value arrayTags;
	arrayTags.resize(0);
	for (set<string>::iterator it = tags.begin(); it != tags.end(); it++)
	{
		arrayTags.append(*it);
	}
	value["tags"] = arrayTags;

	Json::Value arrayUserList;
	arrayUserList.resize(0);
	for (set<string>::iterator it = userList.begin(); it != userList.end(); it++)
	{
		arrayUserList.append(*it);
	}
	value["userList"] = arrayUserList;
}

string ServiceInfo::toString() const
{
	Json::Value value;
	toJson(value);

	Json::FastWriter writer;
	return writer.write(value);
}

int ServiceInfo::delete_user(const string &raw_userID)
{
	set<string>::iterator it = userList.find(raw_userID);
	if (it != userList.end())
	{
		userList.erase(it);
		return 0;
	}
	else
	{
		return -1;
	}
}

int ServiceInfo::find_user(const string &raw_userID)
{
	if (userList.end() != find(userList.begin(), userList.end(), raw_userID))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int ServiceInfo::add_user(const string &raw_userID)
{
	userList.insert(raw_userID);
	return 0;
}

unsigned ServiceInfo::user_count() const
{
	return userList.size();
}

bool ServiceInfo::is_available() const
{
	if (OFFLINE == status || user_count() >= maxUserNum)
	{
		return false;
	}
	else
	{
		return true;
	}
}

bool ServiceInfo::is_busy() const
{
	if (ONLINE == status && user_count() >= maxUserNum)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool ServiceInfo::check_tag_exist(const string &raw_tag) const
{
	set<string>::iterator it;
	for (it = tags.begin(); it != tags.end(); ++it)
	{
		if (raw_tag == *it)
		{
			return true;
		}
	}
	return false;
}


ServiceHeap::ServiceHeap()
{
	_servlist.clear();
}

ServiceHeap::~ServiceHeap()
{
	_servlist.clear();
}

ServiceHeap::ServiceHeap(const string& strServiceHeap)
{
	Json::Reader reader;
	Json::Value value;
	
	if (!reader.parse(strServiceHeap, value))
	{
		return;
	}
	
	int serviceLength = value["tagServiceList"].size();
	for (int i = 0; i < serviceLength; i++)
	{
		_servlist.insert(value["tagServiceList"][i].asString());
	}
}

string ServiceHeap::toString() const
{
	Json::Value value;
	Json::Value arrayObj;
	
	for (set<string>::iterator it = _servlist.begin(); it != _servlist.end(); it++)
	{
		arrayObj.append(*it);
	}
	value["tagServiceList"] = arrayObj;
	
	return value.toStyledString();
}

unsigned ServiceHeap::size()
{
	return _servlist.size();
}

int ServiceHeap::find_service(const string &app_serviceID)
{
	if (_servlist.end() != _servlist.find(app_serviceID))
	{
		return 0;
	}
	else
	{
		return -1;
	}
}

int ServiceHeap::add_service(const string &app_serviceID)
{
	pair<set<string>::iterator, bool> ret;
	
	ret = _servlist.insert(app_serviceID);
	if (true == ret.second)
	{
		return 0;
	}
	else
	{
		LogError("Failed to add new service[%s]!", app_serviceID.c_str());
		return -1;
	}
}

int ServiceHeap::delete_service(const string &app_serviceID)
{
	unsigned int ret;
	
	ret = _servlist.erase(app_serviceID);
	if (ret > 0)
	{
		return 0;
	}
	else
	{
		LogError("Failed to delete service[%s]!", app_serviceID.c_str());
		return -1;
	}
}


