#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "tfc_debug_log.h"

#include <map>
#include <string>
#include "data_model.h"

using namespace std;
using namespace statsvr;

class CAppConfig
{
public:
    static CAppConfig* Instance()
    {
        if (m_instance == NULL)
        {
            m_instance = new CAppConfig();
        }
        return m_instance;
    }

    CAppConfig()
    {
        mapConfigString.clear();
        mapConfigInt.clear();

        _userlist.clear();
        _servicelist.clear();
        tagServiceHeap.clear();
        mapOnlineServiceNumber.clear();
        
        appTagQueues.clear();
        appTagHighPriQueues.clear();
        appSessionQueue.clear();
        
        
        #if 0
        mapappIDQueue.clear();
        mapappIDQueueString.clear();
        tagServiceList.clear();
        mapServiceStatus.clear();
        mapServiceNumber.clear();
        mapOfflineHeap.clear();
        mapappIDSessionQueue.clear();
        mapKickServiceStatus.clear();
        mapHighPriAppIDQueue.clear();
        needConnectServiceWord.clear();
        #endif
    }
    ~CAppConfig()
    {
        mapConfigString.clear();
        mapConfigInt.clear();

        _userlist.clear();
        _servicelist.clear();
        tagServiceHeap.clear();
        mapOnlineServiceNumber.clear();
        
        appTagQueues.clear();
        appTagHighPriQueues.clear();
        appSessionQueue.clear();

        #if 0
        mapappIDQueue.clear();
        mapappIDQueueString.clear();
        tagServiceList.clear();
        mapServiceStatus.clear();
        mapServiceNumber.clear();
        mapOfflineHeap.clear();
        mapappIDSessionQueue.clear();
        mapKickServiceStatus.clear();
        mapHighPriAppIDQueue.clear();
        needConnectServiceWord.clear();
        #endif
    }
    int Init()
    {
        return 0;
    }


public:
    int CheckDel(const map<unsigned, bool>& map_now);
    int CheckDel(const map<string, bool>& map_now);

    int UpdateAppConf(const Json::Value &push_config_req, bool need_set_appIDList);
    int SetAppIDList(string& value);
    int GetAppIDList(string& value);
    
    void DelAppID(string appID);

    int GetVersion(string appID);
    int SetVersion(string appID, uint32_t version);
    int DelVersion(string appID);
    
    int SetConf(string appID, const string& conf);
    int DelConf(string appID);
    int GetConf(string appID, string& conf);

    int DelValue(string appID, const string &key);
    int SetValue(string appID, const string &key, int val);
    int SetValue(string appID, const string &key, const string &val);
    int GetValue(string appID, const string &key, int &val);
    int GetValue(string appID, const string &key, string &val);

    int GetUser(const string& app_userID, UserInfo &user);
    int AddUser(const string& app_userID, const UserInfo& user);
    int UpdateUser(const string& app_userID, const UserInfo& user);
    int UpdateUser(const string& app_userID, const string& value);
    int DelUser(const string& app_userID);
    int UserListToString(string& strUserIDList);

    int AddService(const string &app_serviceID, ServiceInfo &serv);
    int GetService(const string& app_serviceID, ServiceInfo &serv);
    int UpdateService(const string& app_serviceID, const ServiceInfo& serv);
    //int UpdateService(const string& app_serviceID, const string& value);
    int DelService(const string& app_serviceID);
    int CheckServiceList();
    int ServiceListToString(string& strServIDList);

    int AddTagServiceHeap(const string& app_tag);
    int UpdateTagServiceHeap(const string& app_tag, const string& value);
    int UpdateTagServiceHeap(const string& app_tag, const ServiceHeap& serviceHeap);
    int GetTagServiceHeap(const string& app_tag, ServiceHeap& serviceHeap);
    int DelTagServiceHeap(const string& appID);
    int AddService2Tags(const string& appID, ServiceInfo &serv);
    int DelServiceFromTags(const string &appID, ServiceInfo &serv);
    int CanOfferService(const ServiceHeap& servHeap);
    int CanAppOfferService(const string& appID);
    int CheckTagServiceHeapHasOnline(string app_tag);

    int AddTagQueue(string appID);
    int DelTagQueue(string appID);
    int GetTagQueue(string appID, TagUserQueue* &tuq);

    int AddTagHighPriQueue(string appID);
    int DelTagHighPriQueue(string appID);
    int GetTagHighPriQueue(string appID, TagUserQueue* &tuq);
    
    int AddSessionQueue(string appID);
    int GetSessionQueue(string appID, SessionQueue* &pSessionQueue);
    int DelSessionQueue(string appID);

    unsigned GetServiceNum(string appID);
    unsigned GetTagServiceNum(string appID, string raw_tag);
    
    unsigned GetTagOnlineServiceNum(string appID, string raw_tag);
    unsigned GetOnlineServiceNum(string appID);
    int AddTagOnlineServiceNum(string appID, string raw_tag);
    int DelTagOnlineServiceNum(string appID, string raw_tag);

    int GetTimeoutUsers(long long time_gap, set<string>& userList);
    int GetTimeoutServices(long long time_gap, set<string>& serviceList);

    int checkAppIDExist(string appID);
    int checkTagExist(string appID, string tag);
    
    long long getDefaultSessionTimeWarn(string appID);
    long long getDefaultSessionTimeOut(string appID);
    long long getDefaultQueueTimeout(string appID);
    int getMaxConvNum(string appID);
    int getUserQueueNum(string appID);
    int getUserQueueDir(string appID);
    
    string getTimeOutHint(string appID);
    string getTimeWarnHint(string appID);
    string getNoServiceOnlineHint(string appID);
    string getQueueTimeoutHint(string appID);
    string getQueueUpperLimitHint(string appID);

    void getUserListJson(string appID, Json::Value &userList);
    void getServiceListJson(string appID, Json::Value &userList);
    void getSessionQueueJson(string appID, Json::Value &data);
    void getTagQueueJson(string appID, Json::Value &data, bool isHighPri);
    void getTagHighPriQueueJson(string appID, Json::Value &data);
    void getTagNormalQueueJson(string appID, Json::Value &data);
    void getOnlineServiceNumJson(string appID, Json::Value &data);
    
private:
    static CAppConfig *m_instance;
    map<string, string> mapConfigString;
    map<string, int> mapConfigInt;

    /* key: appID_userID */
    map<string, UserInfo> _userlist;
    /* key: appID_servID */
    map<string, ServiceInfo> _servicelist;
    /* key: appID_tag, ServiceHeap仅存储serviceID */
    map<string, ServiceHeap> tagServiceHeap;
    /* key: appID_tag */
    map<string, unsigned> mapOnlineServiceNumber;

    /* key: appID */
    map<string, TagUserQueue*> appTagQueues;
    map<string, TagUserQueue*> appTagHighPriQueues;
    map<string, SessionQueue*> appSessionQueue;
};

#endif
