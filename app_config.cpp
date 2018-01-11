#include "app_config.h"
#include "common_api.h"
#include "debug.h"
#include "jsoncpp/json.h"
#include "data_model.h"
#include <string>
#include <sstream>

using namespace std;
using namespace statsvr;

//class member
CAppConfig* CAppConfig::m_instance = NULL;

/************************************************************************************************/

int CAppConfig::UpdateSubConf(const string &appID, const Json::Value &appID_conf, 
                            Json::Value &real_conf, ostringstream &ostr)
{
    //允许不传某个字段，不允许类型错误

    if (!appID_conf["max_conv_num"].isNull())
    {
        if (!appID_conf["max_conv_num"].isInt())
        {
            ostr << "Err get app["<<appID<<"] configs[max_conv_num];";
            return -1;
        }
        
        int max_conv_num = appID_conf["max_conv_num"].asInt();
        SetValue(appID, "max_conv_num", max_conv_num);
        real_conf["max_conv_num"] = max_conv_num;
    }
    
    if (!appID_conf["check_user_queue_dir"].isNull())
    {
        if (!appID_conf["check_user_queue_dir"].isInt())
        {
            ostr << "Err get app["<<appID<<"] configs[check_user_queue_dir];";
            return -1;
        }
        int check_user_queue_dir = appID_conf["check_user_queue_dir"].asInt();
        SetValue(appID, "check_user_queue_dir", check_user_queue_dir);
        real_conf["check_user_queue_dir"] = check_user_queue_dir;
    }

    if (!appID_conf["check_user_queue_num"].isNull())
    {
        if (!appID_conf["check_user_queue_num"].isInt())
        {
            ostr << "Err get app["<<appID<<"] configs[check_user_queue_num];";
            return -1;
        }
        int check_user_queue_num = appID_conf["check_user_queue_num"].asInt();
        SetValue(appID, "check_user_queue_num", check_user_queue_num);
        real_conf["check_user_queue_num"] = check_user_queue_num;
    }
    
    if (!appID_conf["session_timeout"].isNull())
    {
        if (!appID_conf["session_timeout"].isInt())
        {
            ostr << "Err get app["<<appID<<"] configs[session_timeout];";
            return -1;
        }
        int session_timeout = appID_conf["session_timeout"].asInt();
        SetValue(appID, "session_timeout", session_timeout);
        real_conf["session_timeout"] = session_timeout;
    }

    if (!appID_conf["session_timewarn"].isNull())
    {
        if (!appID_conf["session_timewarn"].isInt())
        {
            ostr << "Err get app["<<appID<<"] configs[session_timewarn];";
            return -1;
        }
        int session_timewarn = appID_conf["session_timewarn"].asInt();
        SetValue(appID, "session_timewarn", session_timewarn);
        real_conf["session_timewarn"] = session_timewarn;
    }
    
    if (!appID_conf["queue_timeout"].isNull())
    {
        if (!appID_conf["queue_timeout"].isInt())
        {
            ostr << "Err get app["<<appID<<"] configs[queue_timeout];";
            return -1;
        }
        int queue_timeout = appID_conf["queue_timeout"].asInt();
        SetValue(appID, "queue_timeout", queue_timeout);
        real_conf["queue_timeout"] = queue_timeout;
    }

    if (!appID_conf["no_service_online_hint"].isNull())
    {
        if (!appID_conf["no_service_online_hint"].isString())
        {
            ostr << "Err get app["<<appID<<"] configs[no_service_online_hint];";
            return -1;
        }
        string no_service_online_hint = appID_conf["no_service_online_hint"].asString();
        SetValue(appID, "no_service_online_hint", no_service_online_hint);
        real_conf["no_service_online_hint"] = no_service_online_hint;
    }

    if (!appID_conf["queue_timeout_hint"].isNull())
    {
        if (!appID_conf["queue_timeout_hint"].isString())
        {
            ostr << "Err get app["<<appID<<"] configs[queue_timeout_hint];";
            return -1;
        }
        string queue_timeout_hint = appID_conf["queue_timeout_hint"].asString();
        SetValue(appID, "queue_timeout_hint", queue_timeout_hint);
        real_conf["queue_timeout_hint"] = queue_timeout_hint;
    }
    
    if (!appID_conf["queue_upper_limit_hint"].isNull())
    {
        if (!appID_conf["queue_upper_limit_hint"].isString())
        {
            ostr << "Err get app["<<appID<<"] configs[queue_upper_limit_hint];";
            return -1;
        }
        string queue_upper_limit_hint = appID_conf["queue_upper_limit_hint"].asString();
        SetValue(appID, "queue_upper_limit_hint", queue_upper_limit_hint);
        real_conf["queue_upper_limit_hint"] = queue_upper_limit_hint;
    }
    
    if (!appID_conf["timeout_warn_hint"].isNull())
    {
        if(!appID_conf["timeout_warn_hint"].isString())
        {
            ostr << "Err get app["<<appID<<"] configs[timeout_warn_hint];";
            return -1;
        }
        string timeout_warn_hint = appID_conf["timeout_warn_hint"].asString();
        SetValue(appID, "timeout_warn_hint", timeout_warn_hint);
        real_conf["timeout_warn_hint"] = timeout_warn_hint;
    }
    
    if (!appID_conf["timeout_end_hint"].isNull())
    {
        if (!appID_conf["timeout_end_hint"].isString())
        {
            ostr << "Err get app["<<appID<<"] configs[timeout_end_hint];";
            return -1;
        }
        string timeout_end_hint = appID_conf["timeout_end_hint"].asString();
        SetValue(appID, "timeout_end_hint", timeout_end_hint);
        real_conf["timeout_end_hint"] = timeout_end_hint;
    }

    return 0;
}


int CAppConfig::UpdateAppConf(const Json::Value &push_config, string &err_msg)
{
    Json::Value appList;
    Json::Value appIDList;
    //Get old appIDList
    if (mGetAppList(appList))
    {
        appIDList.resize(0);
    }
    else
    {
        appIDList = appList["appIDList"];
    }


    Json::Value configList = push_config["appList"];
    int size = configList.size();
    Json::Value appID_conf;
    ostringstream ostr;

    for (int i = 0; i < size; ++i)
    {
        appID_conf = configList[i];
            
        //check appID
        string appID;
        if (get_value_str_safe(appID_conf["appID"], appID))
        {
            LogError("Err get appID[%d]", i);
            ostr <<  "Err get appID["<<i<<"];";
            continue;
        }

        //check version
        unsigned version;
        if (get_value_uint_safe(appID_conf["version"], version))
        {
            LogError("Err get app[%s] version", appID.c_str());
            ostr <<  "Err get app["<<appID<<"] version;";
            continue;
        }
        int myVersion = 0;
        int appIDExist = GetVersion(appID, myVersion);
        if (myVersion >= version)
        {
            LogWarn("appID: %s, version:%d <= myVersion:%d!", appID.c_str(), version, myVersion);
            ostr << "Err cmp app["<<appID<<"] version;";
            continue;
        }

        //check configs
        if (appID_conf["configs"].isNull() || !appID_conf["configs"].isObject())
        {
            LogError("Err get app[%s] configs", appID.c_str());
            ostr <<  "Err get app["<<appID<<"] configs;";
            continue;
        }
        appID_conf = appID_conf["configs"];

        //check configs["tags"]
        if (appID_conf["tags"].isNull() || !appID_conf["tags"].isArray())
        {
            LogError("Err get app[%s] tags", appID.c_str());
            ostr <<  "Err get app["<<appID<<"] tags;";
            continue;
        }

        //check every tag
        bool check_tag = true;
        int tagsNum = appID_conf["tags"].size();
        for (int j = 0; j < tagsNum; ++j)
        {
            string temp;
            if (get_value_str_safe(appID_conf["tags"][j], temp))
            {
                LogError("Err get app[%s] tags[%d]", appID.c_str(), j);
                ostr <<  "Err get app["<<appID<<"] tags["<<j<<"];";
                check_tag = false;
                break;
            }
        }
        if (false == check_tag)
        {
            continue;
        }

        //get old config
        string real_conf_str = "";
        GetConf(appID, real_conf_str);
        Json::Reader reader;
        Json::Value real_conf = Json::objectValue;
        reader.parse(real_conf_str, real_conf);

        //check and set configs["sub-fields"]
        if (UpdateSubConf(appID, appID_conf, real_conf, ostr))
        {
            LogError("Err get app[%s] configs[xxx]", appID.c_str());
            continue;
        }


        //add appID
        if (-1 == appIDExist)
        {
            LogTrace("====> add new appID: %s", appID.c_str());
            appIDList.append(appID);
        }

        //add appID datastructs
        //兼容没收到pingConf只收到updateConf的情况
        TagUserQueue *pTagQueues = NULL;
        TagUserQueue *pTagHighPriQueues = NULL;
        SessionQueue *pSessQueue = NULL;
        if (GetTagQueue(appID, pTagQueues))
        {
            AddTagQueue(appID);
            GetTagQueue(appID, pTagQueues);
        }
        if (GetTagHighPriQueue(appID, pTagHighPriQueues))
        {
            AddTagHighPriQueue(appID);
            GetTagHighPriQueue(appID, pTagHighPriQueues);
        }
        if (GetSessionQueue(appID, pSessQueue))
        {
            AddSessionQueue(appID);
            GetSessionQueue(appID, pSessQueue);
        }
        assert(pTagQueues != NULL);
        assert(pTagHighPriQueues != NULL);
        assert(pSessQueue != NULL);

        //add new tag datastructs
        string tags = "";
        //allow tags be empty
        for (int j = 0; j < tagsNum; ++j)
        {
            string raw_tag;
            get_value_str_safe(appID_conf["tags"][j], raw_tag);

            pTagQueues->add_tag(raw_tag);
            pTagHighPriQueues->add_tag(raw_tag);
            
            string app_tag = appID + "_" + raw_tag;
            AddTagServiceHeap(app_tag);
            
            tags += app_tag + ";";

            real_conf["tags"].append(raw_tag);
        }

        //set configs["tags"]: app_tag1;app_tag2;app_tag3...
        SetValue(appID, "tags", tags);

        //set configs
        SetConf(appID, real_conf.toStyledString());

        //Finally, set version
        //前端根据version确定某个appID配置是否更新成功
        SetVersion(appID, version);
    }

    //set err_msg
    err_msg.clear();
    err_msg = ostr.str();
    //LogTrace("[updateConf] err_msg: %s", err_msg.c_str());
    
    appList["appIDList"] = appIDList;
    Json::FastWriter writer;
    string appListStr = writer.write(appList);
    mSetAppIDListStr(appListStr);
    LogTrace("[updateConf] SetAppIDListStr: %s", appListStr.c_str());
    
    return 0;
}

int CAppConfig::GetAppConf(Json::Value &data)
{
    Json::Value configList;
    configList.resize(0);

    Json::Value appList;
    GetAppList(appList);
    for (int i = 0; i < appList["appIDList"].size(); ++i)
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
        
        int version = 0;
        GetVersion(appID, version);
        
        Json::Value data_rsp;
        data_rsp["appID"]   = atoi(appID.c_str());
        data_rsp["version"] = version;

        string appConf;
        GetConf(appID, appConf);
        Json::Reader reader;
        Json::Value js_appConf;
        if (!reader.parse(appConf, js_appConf))
        {
            //解析失败，直接返回字符串
            data_rsp["configs"] = appConf;
        }
        else
        {
            data_rsp["configs"] = js_appConf;
        }
        
        configList.append(data_rsp);
    }

    data["appList"] = configList;
    return 0;
}

int CAppConfig::WriteAppConf(const string &conf_file)
{
    Json::Value data;
    GetAppConf(data);

    Json::FastWriter writer;
    string strConf;
    strConf = writer.write(data);

    //write to file
    ofstream out(conf_file.c_str(), ios::out);
    if (!out.is_open())
    {
        LogError("Failed to open file: %s!", conf_file.c_str());
        return -1;
    }
    
    out << strConf;
    out.close();
    LogTrace("Write app config: %s.", strConf.c_str());
    return 0;
}

int CAppConfig::LoadAppConf(const string &conf_file, Json::Value &data)
{
    ifstream in(conf_file.c_str(), ios::in);
    if (!in.is_open())
    {
        LogError("Failed to open file: %s!", conf_file.c_str());
        return -1;
    }
    
    istreambuf_iterator<char> begin(in), end;
    string strConf(begin, end);
    in.close();

    Json::Reader reader;
    if (!reader.parse(strConf, data))
    {
        LogError("Failed to parse app config!");
        return -1;
    }

    if (data["appList"].isNull() || !data["appList"].isArray())
    {
        LogError("Failed to parse <appList>!");
        return -1;
    }
    
    LogTrace("Success to load app config: %s.", strConf.c_str());
    return 0;
}


int CAppConfig::SetAppIDListStr(string& value)
{
    SetValue("0", "appIDListStr", value);
    return 0;
}

int CAppConfig::GetAppIDListStr(string& value)
{
    return GetValue("0", "appIDListStr", value);
}

int CAppConfig::GetAppList(Json::Value &appList)
{
    Json::Reader reader;
    string appListStr;

    if (GetAppIDListStr(appListStr))
    {
        LogError("Failed to get appListStr!");
        return SS_ERROR;
    }

    if (!reader.parse(appListStr, appList))
    {
        //LogError("Failed to parse appIDListStr:%s", appIDListStr.c_str());
        return SS_ERROR;
    }

    return SS_OK;
}

void CAppConfig::DelAppID(string appID)
{
    LogDebug("Delete all Data Structs of appID: %s", appID.c_str());
    DelVersion(appID);
    DelConf(appID);
    
    DelTagQueue(appID);
    DelTagHighPriQueue(appID);
    DelSessionQueue(appID);
    DelAppServiceHeaps(appID);
}

int CAppConfig::CheckDel(const map<string, bool>& appIDMap)
{
    vector<string> delList;

    Json::Value appList;
    GetAppList(appList);
    for (int i = 0; i < appList["appIDList"].size(); i++)
    {
        string appID = appList["appIDList"][i].asString();
        if (appIDMap.find(appID) == appIDMap.end())
        {
            delList.push_back(appID);
        }
    }
    LogDebug("Deleted app count: %u", delList.size());
    
    for (int i = 0; i < delList.size(); ++i)
    {
        DelAppID(delList[i]);
    }

    return (int)delList.size();
}

//若appID不存在，则返回-1
int CAppConfig::GetVersion(string appID, int &version)
{
    return GetValue(appID, "version", version);
}

int CAppConfig::SetVersion(string appID, uint32_t version)
{
    return SetValue(appID, "version", (int)version);
}

int CAppConfig::DelVersion(string appID)
{
    return DelValue(appID, "version");
}

int CAppConfig::SetConf(string appID, const string& conf)
{
    return SetValue(appID, "config", conf);
}

int CAppConfig::DelConf(string appID)
{
    return DelValue(appID, "config");
}

int CAppConfig::GetConf(string appID, string& conf)
{
    return GetValue(appID, "config", conf);
}

int CAppConfig::DelValue(string appID, const string &key)
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


int CAppConfig::SetValue(string appID, const string &key, int val)
{
    string relkey = appID + "_" + key;
    mapConfigInt[relkey] = val;
    return 0;
}


int CAppConfig::SetValue(string appID, const string &key, const string &val)
{
    string relkey = appID + "_" + key;
    mapConfigString[relkey] = val;
    return 0;
}

int CAppConfig::GetValue(string appID, const string &key, int &val)
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

int CAppConfig::GetValue(string appID, const string &key, string &val)
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


int CAppConfig::GetUser(const string &app_userID, UserInfo &user)
{
    map<string, UserInfo>::iterator it;

    it = _userlist.find(app_userID);
    if (it != _userlist.end())
    {
        user = it->second;
        return 0;
    }
    return -1;
}

int CAppConfig::AddUser(const string &app_userID, const UserInfo &user)
{
    pair<map<string, UserInfo>::iterator, bool> ret;

    LogTrace("Add User[%s]: %s", app_userID.c_str(), user.toString().c_str());

    ret = _userlist.insert(pair<string, UserInfo>(app_userID, user));
    if (ret.second)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int CAppConfig::UpdateUser(const string &app_userID, const UserInfo &user)
{
    LogTrace("Update User[%s]: %s", app_userID.c_str(), user.toString().c_str());

    _userlist[app_userID] = user;
    return 0;
}

int CAppConfig::UpdateUser(const string &app_userID, const string &value)
{
    UserInfo user(value);
    LogTrace("Update User[%s]: %s", app_userID.c_str(), user.toString().c_str());

    _userlist[app_userID] = user;
    return 0;
}

int CAppConfig::DelUser(const string &app_userID)
{
    map<string, UserInfo>::iterator it;

    LogTrace("Delete User[%s]", app_userID.c_str());
    
    it = _userlist.find(app_userID);
    if (it != _userlist.end())
    {
        //no need to delete it->second
        _userlist.erase(it);
        return 0;
    }
    return -1;
}

int CAppConfig::UserListToString(string &strUserIDList)
{
    map<string, UserInfo>::iterator it;
    Json::Value arr;
    Json::Value userIDList;
    string userID;

    userIDList.resize(0);
     for (it = _userlist.begin(); it != _userlist.end(); it++)
    {
        userID = it->first;
        //LogDebug("userID: %s", userID.c_str());
        userIDList.append(userID);
    }

    arr["userIDList"] = userIDList;

    Json::FastWriter writer;
    strUserIDList = writer.write(arr);
    return 0;
}

int CAppConfig::AddService(const string &app_servID, ServiceInfo &serv)
{
    pair<map<string, ServiceInfo>::iterator, bool> ret;

    LogTrace("Add Service[%s]: %s", app_servID.c_str(), serv.toString().c_str());

    ret = _servicelist.insert(pair<string, ServiceInfo>(app_servID, serv));
    if (ret.second)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int CAppConfig::GetService(const string &app_servID, ServiceInfo &serv)
{
    map<string, ServiceInfo>::iterator it;
    
    it = _servicelist.find(app_servID);
    if (it != _servicelist.end())
    {
        serv = it->second;
        return 0;
    }
    return -1;
}

int CAppConfig::UpdateService(const string &app_servID, const ServiceInfo &serv)
{
    LogTrace("Update Service[%s]: %s", app_servID.c_str(), serv.toString().c_str());

    _servicelist[app_servID] = serv;
    return 0;
}

#if 0
int CAppConfig::UpdateService(const string &app_servID, const string &value)
{
    ServiceInfo serv(value);
    LogTrace("Update Service[%s]: %s", app_servID.c_str(), serv.toString().c_str());

    _servicelist[app_servID] = serv;
    return 0;
}
#endif

int CAppConfig::DelService(const string &app_servID)
{
    map<string, ServiceInfo>::iterator it;
    
    it = _servicelist.find(app_servID);
    if (it != _servicelist.end())
    {
        LogTrace("Delete Service[%s]: %s", app_servID.c_str(), it->second.toString().c_str());
        _servicelist.erase(it);
    }
    return 0;
}

int CAppConfig::CheckServiceList()
{
    map<string, ServiceInfo>::iterator it;
    
    for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
    {
        string appID      = getappID(it->first);
        ServiceInfo &serv = it->second;

        set<string>::iterator it2;
        for (it2 = serv.userList.begin(); it2 != serv.userList.end(); ++it2)
        {
            string app_userID = appID + "_" + (*it2);
            UserInfo user;
            if (GetUser(app_userID, user))
            {
                LogTrace("====> Service[%s] Delete user[%s]", (it->first).c_str(), app_userID.c_str());
                serv.delete_user(*it2);
            }
        }
    }
    
    return 0;
}

int CAppConfig::ServiceListToString(string &strServIDList)
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

    Json::FastWriter writer;
    strServIDList = writer.write(arr);
    return 0;
}


int CAppConfig::AddTagServiceHeap(const string& app_tag)
{
    map<string, ServiceHeap>::iterator it;
    
    it = tagServiceHeap.find(app_tag);
    if (it == tagServiceHeap.end())
    {
        ServiceHeap serviceHeap;
        tagServiceHeap[app_tag] = serviceHeap;
        LogDebug("Add TagServiceHeap[%s]", app_tag.c_str());
    }
    return 0;
}

#if 0
int CAppConfig::UpdateTagServiceHeap(const string& app_tag, const string& value)
{
    ServiceHeap serviceHeap(value);
    LogTrace("Update TagServiceHeap[%s]: %s", app_tag.c_str(), serviceHeap.toString().c_str());
    tagServiceHeap[app_tag] = serviceHeap;
    return 0;
}
#endif

int CAppConfig::UpdateTagServiceHeap(const string& app_tag, const ServiceHeap& serviceHeap)
{
    LogTrace("Update TagServiceHeap[%s]: %s", app_tag.c_str(), serviceHeap.toString().c_str());
    tagServiceHeap[app_tag] = serviceHeap;
    return 0;
}

int CAppConfig::GetTagServiceHeap(const string& app_tag, ServiceHeap& serviceHeap)
{
    map<string, ServiceHeap>::iterator it;
    it = tagServiceHeap.find(app_tag);
    if (it != tagServiceHeap.end())
    {
        serviceHeap = it->second;
        return 0;
    }
    return -1;
}

int CAppConfig::DelTagServiceHeap(const string &app_tag)
{
    map<string, ServiceHeap>::iterator it;
    it = tagServiceHeap.find(app_tag);
    if (it != tagServiceHeap.end())
    {
        tagServiceHeap.erase(it);
        return 0;
    }
    return -1;
}

//delete a batch of tag related to appID
int CAppConfig::DelAppServiceHeaps(const string& appID)
{
    map<string, ServiceHeap>::iterator it;

    //迭代过程中删除元素
    for (it = tagServiceHeap.begin(); it != tagServiceHeap.end();)
    {
        if (string::npos != (it->first).find(appID))
        {
            tagServiceHeap.erase(it++);
        }
        else
        {
            ++it;
        }
    }

    LogTrace("Delete all TagServiceHeaps of appID[%s]", appID.c_str());
    return 0;
}

//add serviceID into every tag serviceHeap
int CAppConfig::AddService2Tags(const string &appID, ServiceInfo &serv)
{
    ServiceHeap servHeap;
    string app_servID = appID + "_" + serv.serviceID;

    for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
    {
        string app_tag = appID + "_" + (*it);
        if (GetTagServiceHeap(app_tag, servHeap))
        {
            LogError("Fail to get TagServiceHeap[%s]!", app_tag.c_str());
            continue;
        }

        servHeap.add_service(app_servID);

        if (UpdateTagServiceHeap(app_tag, servHeap))
        {
            LogError("Fail to update TagServiceHeap[%s]!", app_tag.c_str());
            continue;
        }
    }

    return 0;
}

int CAppConfig::DelServiceFromTags(const string &appID, ServiceInfo &serv)
{
    ServiceHeap servHeap;
    string app_servID = appID + "_" + serv.serviceID;

    for (set<string>::iterator it = serv.tags.begin(); it != serv.tags.end(); it++)
    {
        string app_tag = appID + "_" + (*it);
        if (GetTagServiceHeap(app_tag, servHeap))
        {
            LogError("Fail to find TagServiceHeap[%s]!", app_tag.c_str());
            continue;
        }

        servHeap.delete_service(app_servID);

        if (UpdateTagServiceHeap(app_tag, servHeap))
        {
            LogError("Fail to update TagServiceHeap[%s]!", app_tag.c_str());
            continue;
        }
    }

    return 0;
}

int CAppConfig::CanOfferService(const ServiceHeap& servHeap)
{
    for (set<string>::iterator i = servHeap._servlist.begin(); i != servHeap._servlist.end(); i++)
    {
        string servID = *i;
        ServiceInfo serv;
        if (SS_OK != GetService(servID, serv))
        {
            continue;
        }
        else if (true == serv.is_available())
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
    
    GetValue(appID, "tags", strTags);
    MySplitTag((char *)strTags.c_str(), ";", tags);

    for (int i = 0; i < tags.size(); ++i)
    {
        ServiceHeap servHeap;
        if (GetTagServiceHeap(tags[i], servHeap))
        {
            LogWarn("Failed to get TagServiceHeap[%s], find next ServiceHeap!", tags[i].c_str());
            continue;
        }

        for (set<string>::iterator it = servHeap._servlist.begin(); it != servHeap._servlist.end(); ++it)
        {
            ServiceInfo serv;

            if (GetService(*it, serv) || false == serv.is_available())
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


int CAppConfig::CheckTagServiceHeapHasOnline(string app_tag)
{
    ServiceHeap servHeap;
    if (GetTagServiceHeap(app_tag, servHeap))
    {
        LogError("Failed to get TagServiceHeap[%s]!", app_tag.c_str());
        return -1;
    }

    set<string>::iterator it;
    for (it = servHeap._servlist.begin(); it != servHeap._servlist.end(); ++it)
    {
        string app_servID = (*it);
        ServiceInfo serv;
        if (SS_OK != GetService(app_servID, serv))
        {
            continue;
        }
        else if (OFFLINE != serv.status)
        {
            return 0;
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
        LogTrace("Delete NormalQueue[appID:%s].", appID.c_str());
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
        LogTrace("Delete HighPriQueue[%s].", appID.c_str());
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
        LogTrace("Delete SessionQueue[%s].", appID.c_str());
    }
    
    return 0;
}

unsigned CAppConfig::GetServiceNum(string appID)
{
    return _servicelist.size();
}


//1个service可以属于多个tag，不要重复计算
unsigned CAppConfig::GetTagServiceNum(string appID, string raw_tag)
{
    string app_tag = appID + "_" + raw_tag;
    
    map<string, ServiceHeap>::iterator it;
    for (it = tagServiceHeap.begin(); it != tagServiceHeap.end(); ++it)
    {
        if (app_tag == it->first)
        {
            return (it->second).size();
        }
    }
    
    return 0;
}


unsigned CAppConfig::GetOnlineServiceNum(string appID)
{
    unsigned servNum = 0;

    map<string, ServiceInfo>::iterator it;
    for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
    {
        if (appID == getappID(it->first))
        {
            if (OFFLINE != (it->second).status)
            {
                ++servNum;
            }
        }
    }
    
    return servNum;
}

int CAppConfig::AddTagOnlineServiceNum(string appID, string raw_tag)
{
    string app_tag = appID + "_" + raw_tag;

    map<string, unsigned>::iterator it;
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

unsigned CAppConfig::GetTagOnlineServiceNum(string appID, string raw_tag)
{
    unsigned servNum = 0;

    map<string, ServiceInfo>::iterator it;
    for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
    {
        if (appID == getappID(it->first))
        {
            ServiceInfo &serv = it->second;
            if (OFFLINE != serv.status)
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
}


int CAppConfig::DelTagOnlineServiceNum(string appID, string raw_tag)
{
    string app_tag = appID + "_" + raw_tag;

    map<string, unsigned>::iterator it = mapOnlineServiceNumber.find(app_tag);
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

int CAppConfig::GetTimeoutUsers(long long time_gap, set<string>& userList)
{
    long long nowTime = time(NULL);
    long long atime;
    
    map<string, UserInfo>::iterator it;
    for (it = _userlist.begin(); it != _userlist.end(); ++it)
    {
        //只选择YiBot状态的用户
        if (IN_YIBOT != (it->second).status)
        {
            continue;
        }
        
        atime = ((it->second).atime / 1000);
        if (nowTime >= atime + time_gap)
        {
            userList.insert(it->first);
        }
    }
    return 0;
}

int CAppConfig::GetTimeoutServices(long long time_gap, set<string>& serviceList)
{
    long long nowTime = time(NULL);
    long long atime;
    
    map<string, ServiceInfo>::iterator it;
    for (it = _servicelist.begin(); it != _servicelist.end(); ++it)
    {
        atime = ((it->second).atime / 1000);
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
    
    if (GetAppIDListStr(appListString))
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


int CAppConfig::checkTagExist(string appID, string app_tag)
{
    string strTags;
    if (GetValue(appID, "tags", strTags))
    {
        LogError("Failed to get tags of appID[%s]!", appID.c_str());
        return -1;
    }

    vector<string> tags;
    MySplitTag((char *)strTags.c_str(), ";", tags);
    for (int i = 0; i < tags.size(); ++i)
    {
        if (app_tag == tags[i])
        {
            return 0;
        }
    }
    return -1;
}


long long CAppConfig::getDefaultSessionTimeWarn(string appID)
{
    int session_timewarn = -1;
    
    if (GetValue(appID, "session_timewarn", session_timewarn) 
        || session_timewarn < 0)
    {
        session_timewarn = 10*60;
    }
    else
    {
        session_timewarn *= 60;
    }

    return (long long)session_timewarn;
}

long long CAppConfig::getDefaultSessionTimeOut(string appID)
{
    int session_timeout = -1;
    
    if (GetValue(appID, "session_timeout", session_timeout) 
        || session_timeout < 0)
    {
        session_timeout = 15*60;
    }
    else
    {
        session_timeout *= 60;
    }
    return (long long)session_timeout;
}

long long CAppConfig::getDefaultQueueTimeout(string appID)
{
    int queue_timeout = -1;
    
    if (GetValue(appID, "queue_timeout", queue_timeout) 
        || queue_timeout < 0)
    {
        queue_timeout = 30*60;
    }
    else
    {
        queue_timeout *= 60;
    }
    return (long long)queue_timeout;
}

int CAppConfig::getMaxConvNum(string appID)
{
    int max_conv_num = -1;
    
    if (GetValue(appID, "max_conv_num", max_conv_num) 
        || max_conv_num < 0)
    {
        max_conv_num = 5;
    }
    
    return max_conv_num;
}

int CAppConfig::getUserQueueNum(string appID)
{
    int user_queue_num = 1;

    if (GetValue(appID, "check_user_queue_num", user_queue_num) 
        || 0 == user_queue_num)
    {
        user_queue_num = 1;
    }

    return user_queue_num;
}

int CAppConfig::getUserQueueDir(string appID)
{
    int user_queue_dir = 1; //默认从队尾拉取

    if (GetValue(appID, "check_user_queue_dir", user_queue_dir))
    {
        user_queue_dir = 1;
    }

    return user_queue_dir;
}

string CAppConfig::getTimeOutHint(string appID)
{
    string hint;
    GetValue(appID, "timeout_end_hint", hint);
    return hint;
}

string CAppConfig::getTimeWarnHint(string appID)
{
    string hint;
    GetValue(appID, "timeout_warn_hint", hint);
    return hint;
}

string CAppConfig::getNoServiceOnlineHint(string appID)
{
    string hint;
    GetValue(appID, "no_service_online_hint", hint);
    return hint;
}

string CAppConfig::getQueueTimeoutHint(string appID)
{
    string hint;
    GetValue(appID, "queue_timeout_hint", hint);
    return hint;
}

string CAppConfig::getQueueUpperLimitHint(string appID)
{
    string hint;
    GetValue(appID, "queue_upper_limit_hint", hint);
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

void CAppConfig::getSessionQueueJson(string appID, Json::Value &data)
{
    SessionQueue *pSessQueue = NULL;
    SessionTimer sessTimer;
    Session sess;
    Json::Value sessJson;

    data.resize(0);
    
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

            data.append(sessJson);
        }
    }
}

void CAppConfig::getTagQueueJson(string appID, Json::Value &data, bool isHighPri)
{
    data.resize(0);

    map<string, TagUserQueue*>::iterator it;
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

    TagUserQueue *pTags = it->second;
    if (NULL == pTags)
    {
        return;
    }
    
    map<string, UserQueue*>::iterator it2;
    UserQueue *uq = NULL;
    for (it2 = pTags->_tag_queue.begin(); it2 != pTags->_tag_queue.end(); ++it2)
    {
        Json::Value tagJson;
        tagJson["tag"] = it2->first;

        Json::Value queueJson;
        queueJson.resize(0);

        uq = it2->second;
        
        for (list<UserTimer>::iterator it3 = uq->_user_list.begin(); it3 != uq->_user_list.end(); ++it3)
        {
            Json::Value userJson;
            userJson["expire_time"] = (*it3).expire_time;
            userJson["userID"]      = (*it3).userID;

            queueJson.append(userJson);
        }
        tagJson["userQueue"] = queueJson;
        data.append(tagJson);
    }
}

void CAppConfig::getTagHighPriQueueJson(string appID, Json::Value &data)
{
    getTagQueueJson(appID, data, true);
}


void CAppConfig::getTagNormalQueueJson(string appID, Json::Value &data)
{
    getTagQueueJson(appID, data, false);
}

void CAppConfig::getOnlineServiceNumJson(string appID, Json::Value &data)
{
    map<string, unsigned>::iterator it;

    data = Json::objectValue;
    for (it = mapOnlineServiceNumber.begin(); it != mapOnlineServiceNumber.end(); it++)
    {
        string app_tag = it->first;
        if (appID == getappID(app_tag))
        {
            string raw_tag = delappID(app_tag);
            data[raw_tag]   = it->second;
        }
    }
}

