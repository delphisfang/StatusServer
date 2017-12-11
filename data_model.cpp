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
	return value.toStyledString();
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
	return value.toStyledString();
}


ServiceInfo::ServiceInfo()
{
	serviceID.clear();
	status = OFFLINE;
	atime = 0;
	cpIP.clear();
	cpPort = 0;
	tags.clear();
	userList.clear();
	serviceName.clear();
	serviceAvatar.clear();
	//whereFrom.clear();
}

ServiceInfo::~ServiceInfo()
{
	serviceID.clear();
	status = OFFLINE;
	atime = 0;
	cpIP.clear();
	cpPort = 0;
	tags.clear();
	userList.clear();
	serviceName.clear();
	serviceAvatar.clear();
	//whereFrom.clear();
}

