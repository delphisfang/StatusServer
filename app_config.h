#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "tfc_debug_log.h"

#include <map>
#include <string>
#include "data_model.h"

using namespace std;
using namespace statsvr;

int get_global();

void set_global(int glob);

class CAppConfig
{
public:
	static CAppConfig* Instance ()
	{
		if (m_instance == NULL)
		{
			m_instance = new CAppConfig();
		}
		return m_instance;
	}

	CAppConfig ()
	{
		mapConfigString.clear();
		mapConfigInt.clear();
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
	~CAppConfig ()
	{
		mapConfigString.clear();
		mapConfigInt.clear();
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
	int Init ()
	{
		return 0;
	}


public:
	int CheckDel(const map<unsigned, bool>& map_now);
	int CheckDel(const map<string, bool>& map_now);

	int UpdateappIDConf (const Json::Value &push_config_req);
	int SetNowappIDList(string& value);
	int GetNowappIDList(string& value);
	
	void DelappID(string appID);

	int GetVersion(string appID);
	int SetVersion(string appID, uint32_t version);
	int DelVersion(string appID);
	
	int SetConf(string appID, const string& conf);
	int DelConf(string appID);
	int GetConf(string appID, string& conf);

	int DelValue(string appID, const string &key);
	int SetValue(string appID, const string &key, int val);
	int SetValue(string appID, const string &key, const string &val);
	int GetValue(string appID, const string &key, int &val); //for int value
	int GetValue(string appID, const string &key, string &val); //for string value

	int GetUser(const string& key, UserInfo &ui);
	int AddUser(const string& key, const UserInfo& ui);
	int UpdateUser(const string& key, const UserInfo& ui);
	int UpdateUser(const string& key, const string& value);
	int DelUser(const string& key);
	int UserListToString(string& strUserIDList);

	int AddService(const string &key, ServiceInfo &serv);
	int GetService(const string& key, ServiceInfo &serv);
	int UpdateService(const string& key, const ServiceInfo& serv);
	int UpdateService(const string& key, const string& value);
	int DelService(const string& key);
	int ServiceListToString(string& strServIDList);

	int AddTagServiceHeap(const string& key);
	int UpdateTagServiceHeap(const string& key, const string& value);
	int UpdateTagServiceHeap(const string& key, const ServiceHeap& serviceHeap);
	int GetTagServiceHeap(const string& key, ServiceHeap& serviceHeap);
	int DelTagServiceHeap(const string& appID);
	int AddService2Tags(const string& appID, ServiceInfo &serv);
	int DelServiceFromTags(const string &appID, ServiceInfo &serv);
	int CanOfferService(const ServiceHeap& servHeap, int serverNum);

	int AddTagQueue(string appID);
	int DelTagQueue(string appID);
	int GetTagQueue(string appID, TagUserQueue* &tuq);

	int AddTagHighPriQueue(string appID);
	int DelTagHighPriQueue(string appID);
	int GetTagHighPriQueue(string appID, TagUserQueue* &tuq);
	
	int AddSessionQueue(string appID);
	int UpdateSessionQueue(string appID, string& value);
	int GetSessionQueue(string appID, SessionQueue* &pSessionQueue);
	int DelSessionQueue(string appID);

	unsigned GetServiceNumber(string appID);
	unsigned GetTagServiceNumber(string appID, string raw_tag);
	
	int checkAppIDExist(string appID);
	int checkTagExist(string appID, string tag);
	
	long long getDefaultSessionTimeWarn(string appID);
	long long getDefaultSessionTimeOut(string appID);
	long long getDefaultQueueTimeout(string appID);
	int getMaxConvNum(string appID);
	string getTimeWarnHint(string appID);

	void getUserListJson(string appID, Json::Value &userList);
	void getServiceListJson(string appID, Json::Value &userList);
	void getSessionQueueJson(string appID, Json::Value &sessQueue);
	void getTagQueueJson(string appID, Json::Value &tags, bool isHighPri);
	void getTagHighPriQueueJson(string appID, Json::Value &tags);
	void getTagNormalQueueJson(string appID, Json::Value &tags);
	
private:
	static CAppConfig *m_instance;
	map<string, string> mapConfigString;
	map<string, int> mapConfigInt;

	/* key: appID */
	map<string, TagUserQueue*> appTagQueues;
	map<string, TagUserQueue*> appTagHighPriQueues;
	map<string, SessionQueue*> appSessionQueue;
	#if 0
	/* 以serviceID为索引 */
	map<string, ServiceInfo> _servicelist;
	/* 每个user带有一个session */
	map<string, Session> _map_user_session;
	map<string, UserQueue> _queuelist;
	#endif

	/* key: appID_userID */
	map<string, UserInfo> _userlist;
	/* key: appID_servID */
	map<string, ServiceInfo> _servicelist;
	
	/* key: appID_tag, ServiceHeap仅存储serviceID */
	map<string, ServiceHeap> tagServiceHeap;
};

#endif
