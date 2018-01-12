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
}

Session::Session(const string& strSession)
{
    Json::Reader reader;
    Json::Value value;
    
    if (!reader.parse(strSession, value) || !value.isObject())
    {
        return;
    }
    
    sessionID = get_value_str(value, SESSION_ID);
    userID    = get_value_str(value, USER_ID);
    serviceID = get_value_str(value, SERV_ID);
    atime     = get_value_uint64(value, ACTIVE_TIME, GetCurTimeStamp());
    btime     = get_value_uint64(value, BUILD_TIME, GetCurTimeStamp());
    cpIP      = get_value_str(value, CP_IP);
    cpPort    = get_value_uint(value, CP_PORT);
    notified  = get_value_int(value, NOTIFIED, 0);
}

void Session::toJson(Json::Value &value) const
{
    value[SESSION_ID]  = sessionID;
    value[USER_ID]     = userID;
    value[SERV_ID]     = serviceID;
    value[ACTIVE_TIME] = atime;
    value[BUILD_TIME]  = btime;
    value[CP_IP]       = cpIP;
    value[CP_PORT]     = cpPort;
    value[NOTIFIED]    = notified;
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
}

UserInfo::UserInfo(const string& strUserInfo)
{
    Json::Reader reader;
    Json::Value value;
    timeval nowTime;
    
    if (!reader.parse(strUserInfo, value) || !value.isObject())
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
    
    atime         = get_value_int64(value, ACTIVE_TIME, GetCurTimeStamp());
    qtime         = get_value_uint64(value, QTIME);
    sessionID     = get_value_str(value, SESSION_ID);
    lastServiceID = get_value_str(value, LAST_SERV_ID);
    priority      = get_value_str(value, PRIO);
    queuePriority = get_value_uint(value, QUEUE_PRIO);
    channel       = get_value_str(value, CHANNEL);

    //LogDebug("construct userInfo: %s", toString().c_str());
}

void UserInfo::toJson(Json::Value &value) const
{
    value[USER_ID]        = userID;
    value[CP_IP]        = cpIP;
    value[CP_PORT]        = cpPort;
    value[USER_TAG]     = tag;
    value[STATUS]        = status;
    value[ACTIVE_TIME]    = atime;
    value[QTIME]        = qtime;
    value[SESSION_ID]    = sessionID;
    value[LAST_SERV_ID] = lastServiceID;
    value[PRIO]         = priority;
    value[QUEUE_PRIO]    = queuePriority;
    value[CHANNEL]        = channel;

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
    subStatus  = SUB_LIXIAN;
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
    subStatus = SUB_LIXIAN;
    //whereFrom.clear();
}

ServiceInfo::ServiceInfo(const string& strServiceInfo, unsigned dft_user_num)
{
    Json::Reader reader;
    Json::Value value;
    
    if (!reader.parse(strServiceInfo, value) || !value.isObject())
    {
        return;
    }

    atime            = get_value_int64(value, ACTIVE_TIME, GetCurTimeStamp());
    serviceID        = get_value_str(value, SERV_ID);
    status           = get_value_str(value, STATUS, DEF_SERV_STATUS);
    cpIP             = get_value_str(value, CP_IP);
    cpPort           = get_value_uint(value, CP_PORT);
    serviceName      = get_value_str(value, SERV_NAME);
    serviceAvatar    = get_value_str(value, SERV_AVATAR);
    maxUserNum       = get_value_uint(value, MAX_USER_NUM_FIELD, dft_user_num);
    subStatus        = get_value_str(value, SUB_STATUS, SUB_LIXIAN);
    //whereFrom      = value["whereFrom"].asString();

    parse_tags(value);
    parse_userList(value);
}

void ServiceInfo::parse_tags(const Json::Value &value)
{
    if (!value.isObject() || !value["tags"].isArray())
    {
        return;
    }
    
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
    if (!value.isObject() || !value["userList"].isArray())
    {
        return;
    }
    
    for (unsigned i = 0; i < value["userList"].size(); ++i)
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
    value[SERV_ID]       = serviceID;
    value[STATUS]        = status;
    value[ACTIVE_TIME] = atime;
    value[CP_IP]       = cpIP;
    value[CP_PORT]     = cpPort;
    value[SERV_NAME]   = serviceName;
    value[SERV_AVATAR] = serviceAvatar;
    value[MAX_USER_NUM_FIELD] = maxUserNum;
    value[SUB_STATUS]  = subStatus;
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
    
    if (!reader.parse(strServiceHeap, value) || !value.isObject() || !value["tagServiceList"].isArray())
    {
        return;
    }
    
    for (int i = 0; i < value["tagServiceList"].size(); ++i)
    {
        if (value["tagServiceList"][i].isString())
        {
            _servlist.insert(value["tagServiceList"][i].asString());
        }
    }
}

string ServiceHeap::toString() const
{
    Json::Value value;
    Json::Value arrayObj;

    arrayObj.resize(0);
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

int ServiceHeap::find_service(const string &app_servID)
{
    if (_servlist.end() != _servlist.find(app_servID))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int ServiceHeap::add_service(const string &app_servID)
{
    pair<set<string>::iterator, bool> ret;
    
    ret = _servlist.insert(app_servID);
    if (true == ret.second)
    {
        return 0;
    }
    else
    {
        LogError("Failed to add new service[%s]!", app_servID.c_str());
        return -1;
    }
}

int ServiceHeap::delete_service(const string &app_servID)
{
    unsigned int ret;
    
    ret = _servlist.erase(app_servID);
    if (ret > 0)
    {
        return 0;
    }
    else
    {
        LogError("Failed to delete service[%s]!", app_servID.c_str());
        return -1;
    }
}

