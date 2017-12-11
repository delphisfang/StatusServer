#ifndef _DATA_MODEL_H_
#define _DATA_MODEL_H_

/* OS headers */
#include <sys/time.h>
#include <time.h>
#include <map>
#include <list>
#include <set>
#include "list.h"
#include <algorithm>
#include <string>

/* TFC headers */
#include "tfc_object.h"
#include "tfc_base_fast_timer.h"
#include "tfc_base_http.h"
#include "tfc_debug_log.h"

/* module headers */
#include "debug.h"
#include "common_api.h"

using namespace std;

namespace statsvr
{
	struct Session
	{
	    Session();
		~Session();
	    Session(const string& strSession);
		
		void toJson(Json::Value &value) const;
	    string toString() const;
		
		string    sessionID;
		string    userID;
		string    cpIP;			//用户的cpIP
		unsigned short cpPort;	//用户的cpPort
	    string    serviceID;    //坐席ID
	    long long atime;        //活跃时间，即有消息时间到达就更新
		long long btime;        //建立连接的时间
	    //bool      toIM;       //标记数据流向，默认为0
		//string	  whereFrom;	//标记使用http还是websocket
		//string    channel;
		//unsigned  userCount;
	};

	struct UserInfo
	{
	    UserInfo();
		~UserInfo();
	    UserInfo(const string& strUserInfo);
		
		void toJson(Json::Value &value) const;
	    string toString() const;
		
	    string userID;
		string cpIP;
		unsigned short cpPort;
	    string tag;
		string status;
		long long atime;
	    long long qtime;
	    string sessionID;
	    string lastServiceID;
	    string priority;
		//string userInfo;
		unsigned queuePriority;
		string channel;
		string extends;
	};

	struct ServiceInfo
	{
	    ServiceInfo();
		~ServiceInfo();
	
	    ServiceInfo(const string& strServiceInfo)
	    {
	        Json::Reader reader;
	        Json::Value value;
			timeval nowTime;
			
	        if (!reader.parse(strServiceInfo, value))
			{
				return;
			}

			serviceID = value["serviceID"].asString();
			status    = "offline";

			/*atime     = value["activeTime"].asInt64();*/
			gettimeofday(&nowTime, NULL);
			atime = (nowTime.tv_sec*1000 + nowTime.tv_usec / 1000);
			
	        cpIP      = value["chatProxyIp"].asString();
	        cpPort    = value["chatProxyPort"].asUInt();
			serviceName   = value["serviceName"].asString();
			serviceAvatar = value["serviceAvatar"].asString();
			//whereFrom     = value["whereFrom"].asString();
	        unsigned tagsLen = value["tags"].size();

			for (unsigned i = 0; i < tagsLen; i++)
	        {
	            tags.insert(value["tags"][i].asString());
	        }

			unsigned userLen = value["userList"].size();
	        for (unsigned i = 0; i < userLen; i++)
	        {
	            userList.insert(value["userList"][i].asString());
	        }
	    }

		void toJson(Json::Value &value) const
	    {
	        Json::Value arrayTags;
	        Json::Value arrayUserList;

			value["serviceID"]     = serviceID;
			value["status"]        = status;
			value["activeTime"]    = atime;
			value["chatProxyIp"]   = cpIP;
	        value["chatProxyPort"] = cpPort;
			value["serviceName"]   = serviceName;
			value["serviceAvatar"] = serviceAvatar;
			//value["whereFrom"]     = whereFrom;
	       
	        for (set<string>::iterator it = tags.begin(); it != tags.end(); it++)
	        {
	            arrayTags.append(*it);
	        }
	        value["tags"] = arrayTags;

			arrayUserList.resize(0);
	        for (set<string>::iterator it = userList.begin(); it != userList.end(); it++)
	        {
	            arrayUserList.append(*it);
	        }
	        value["userList"] = arrayUserList;
		}
		
	    string toString() const
	    {
			Json::Value value;
			toJson(value);
	        return value.toStyledString();
	    }

		int delete_user(const string &userID)
	    {
			set<string>::iterator it = userList.find(userID);
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

		int find_user(string userID)
		{
			if (userList.end() != find(userList.begin(), userList.end(), userID))
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		
		int add_user(const string &userID)
		{
			userList.insert(userID);
			return 0;
		}

		unsigned user_count() const
		{
			return userList.size();
		}
		
		string serviceID;
		string status;
		long long atime;
		string   cpIP;
	    unsigned cpPort;
	    set<string> tags;
	    set<string> userList;
		string   serviceName;
		string   serviceAvatar;
		//string	 whereFrom;
	};

	//仅存储serviceID
	struct ServiceHeap
	{
	    ServiceHeap()
	    {
			_servlist.clear();
		}

		~ServiceHeap()
	    {
			_servlist.clear();
		}

	    ServiceHeap(const string& strServiceHeap)
	    {
	        Json::Reader reader;
	        Json::Value value;
	        if(!reader.parse(strServiceHeap, value))
			{
				return;
			}
	        int serviceLength = value["tagServiceList"].size();
	        for(int i = 0; i < serviceLength; i++)
	        {
	            _servlist.insert(value["tagServiceList"][i].asString());
	        }
	    }
	    string toString() const
	    {
	        Json::Value value;
	        Json::Value arrayObj;
	        for(set<string>::iterator it = _servlist.begin(); it != _servlist.end(); it++)
	        {
	            arrayObj.append(*it);
	        }
	        value["tagServiceList"] = arrayObj;
	        return value.toStyledString();
	    }

		unsigned size()
	    {
			return _servlist.size();
		}

		int find(const string &serviceID)
		{
			return (_servlist.end() != _servlist.find(serviceID));
		}

		int add_service(const string &serviceID)
		{
			_servlist.insert(serviceID);
		}
		
		int del_service(const string &serviceID)
		{
			_servlist.erase(serviceID);
		}

		//按tag对service进行分组，方便转人工时根据user的tag找到service
	    set<string> _servlist;
	};

	//带计时的用户信息
	struct UserTimer
	{
		string userID;
		long long expire_time;
	};

	//某个tag下的用户排队队列
	struct UserQueue
	{
		list<UserTimer> _user_list;
		
		UserQueue()
		{
			_user_list.clear();
		}
		
		virtual ~UserQueue()
		{
			_user_list.clear();
		}

		string toString()
		{
	        Json::Value obj;
			Json::Value queueList;
			Json::Value arr;
			UserTimer ut;

			queueList.resize(0);
	        for (list<UserTimer>::iterator it = _user_list.begin(); it != _user_list.end(); it++)
	        {
				ut = (*it);
	            obj["userID"]      = ut.userID;
				obj["expire_time"] = ut.expire_time;

				queueList.append(obj);
	        }
			arr["queueList"] = queueList;
			
	        return arr.toStyledString();
		}
		
		int set(string userID, long long expire_time/* seconds */)
		{
			UserTimer ut;

			ut.userID      = userID;
			ut.expire_time = expire_time;
			_user_list.push_back(ut);
			
			LogTrace("Success to add new userID[%s] on queue.", userID.c_str());
			return 0;
		}

		// delete first user
		int pop()
		{
			if (_user_list.size() > 0)
			{
				_user_list.pop_front();
				return 0;
			}
			
			LogError("Failed to pop user from queue, size <= 0!");
			return -1;
		}

		// get first user
		int get_first(string& userID, long long &expire_time)
		{
			list<UserTimer>::iterator it = _user_list.begin();
			
			if (_user_list.size() > 0)
			{
				userID = it->userID;
				expire_time = it->expire_time;
				return 0;
			}
			
			LogError("Failed to get first user from queue, size <= 0!");
			return -1;
		}

		// get last user
		int get_last(string& userID, long long &expire_time)
		{
			list<UserTimer>::reverse_iterator it = _user_list.rbegin();
			
			if (_user_list.size() > 0)
			{
				userID = it->userID;
				expire_time = it->expire_time;
				return 0;
			}

			LogError("Failed to get last user from queue, size <= 0!");
			return -1;
		}

		#if 0
		// return user info according to key <userID>
		int get(string userID, UserInfo& ui)
		{
			list<UserTimer>::iterator it = _user_list.begin();
			
			for (it = _user_list.begin(); it != _user_list.end(); it++)
			{
				if (it->userID == userID)
				{
					ui = it->userInfo;
					return 0;
				}
			}
			return -1;
		}
		#endif
		
		// return number before user
		int find(string userID)
		{
			list<UserTimer>::iterator it;
			int pos = 0;

			for (it = _user_list.begin(); it != _user_list.end(); it++)
			{
				if (it->userID == userID)
				{
					return pos;
				}
				pos++;
			}
			
			return -1;
		}

		// delete user
		int delete_user(string userID)
		{
			list<UserTimer>::iterator it;

			for (it = _user_list.begin(); it != _user_list.end(); it++)
			{
				if (it->userID == userID)
				{
					it = _user_list.erase(it);
					return 0;
				}
			}

			LogError("Failed to delete user[%s] from queue!", userID.c_str());
			return -1;
		}

		//just get number, do not delete
		int check_expire()
		{
			list<UserTimer>::iterator it;
			int expireNum = 0;
			long long nowTime = (long long)time(NULL);

			for (it = _user_list.begin(); it != _user_list.end(); it++)
			{
				if (nowTime >= it->expire_time)
				{
					LogTrace("[nowTime: %ld] Find queue user timeout -- userID: %s, expire_time: %ld", 
								nowTime, it->userID.c_str(), it->expire_time);
					expireNum++;
				}
			}

			return expireNum;
		}

		unsigned size()
		{
			return _user_list.size();
		}
		
		
	};

	class TagUserQueue
	{
	public:
		TagUserQueue()
		{
			_tag_queue.clear();
		}

		~TagUserQueue()
		{
			_tag_queue.clear();
		}
		
		int add_tag(string tag)
		{
			map<string, UserQueue*>::iterator it;
			
			it = _tag_queue.find(tag);
			if (it == _tag_queue.end())
			{
				_tag_queue[tag] = new UserQueue();
				LogTrace("Success to add UserQueue for tag: %s", tag.c_str());
			}
			return 0;
		}
		
		int del_tag(string tag)
		{
			map<string, UserQueue*>::iterator it;
			
			it = _tag_queue.find(tag);
			if (it != _tag_queue.end())
			{
				delete it->second;
				_tag_queue.erase(tag);
				return 0;
			}
			return -1;
		}
		
		int get_tag(string tag, UserQueue* &uq)
		{
			map<string, UserQueue*>::iterator it;
			
			it = _tag_queue.find(tag);
			if(it != _tag_queue.end())
			{
				uq = it->second;
				return 0;
			}
			return -1;
		}

		int add_user(string tag, string userID, long long expire_time/* seconds */)
		{
			map<string, UserQueue*>::iterator it;
			UserQueue *uq = NULL;
			
			it = _tag_queue.find(tag);
			if (it == _tag_queue.end())
			{
				return -1;
			}

			if (NULL == (uq = it->second))
			{
				return -1;
			}

			return uq->set(userID, expire_time);
		}

		#if 0
		// return user rank in tag queue
		int get_user(string userID, UserInfo &user)
		{
			map<string, UserQueue*>::iterator it;
			UserQueue *uq = NULL;
			int pos = -1;
			
			for (it = _tag_queue.begin(); it != _tag_queue.end(); it++)
			{
				uq = it->second;
				if (0 == uq->get(userID, user))
				{
					pos = uq->find(userID);
					return pos;
				}
			}

			return -1;
		}
		#endif
		
		int find_user(string userID)
		{
			map<string, UserQueue*>::iterator it;
			UserQueue *uq = NULL;
			int pos = 0;
			
			for (it = _tag_queue.begin(); it != _tag_queue.end(); it++)
			{
				uq  = it->second;
				pos = uq->find(userID);
				
				if (pos >= 0)
				{
					return pos;
				}
			}

			return -1;
		}

		int del_user(string tag, string userID)
		{
			map<string, UserQueue*>::iterator it;
			UserQueue *uq = NULL;

			it = _tag_queue.find(tag);
			if (it == _tag_queue.end())
			{
				return -1;
			}
			
			if (NULL == (uq = it->second))
			{
				return -1;
			}
			
			return uq->delete_user(userID);
		}

		/*
		tags: service所属的分组
		direct: 从队头还是队尾查找，0代表队头，1代表队尾
		num: 第几个 (TODO:暂时忽略该参数)
		*/
		int get_target_user(string &userID, const string& serviceID, std::set<string> tags, unsigned num, int direct)
		{
			//UserInfo user;
			UserQueue *uq = NULL;
			//bool tag_match = false;
			
			bool found = false;
			string temp_userID;
			long long temp_expire;
			long long expire;

			LogTrace("==========>direct = %d", direct);
			
			map<string, UserQueue*>::iterator it;
			for (it = _tag_queue.begin(); it != _tag_queue.end(); it++)
			{
				//坐席不能拉取不在它服务范围内的用户
				if (tags.end() == tags.find(it->first))
				{
					continue;
				}

				uq = it->second;
				if (direct == 0) //from queue head
				{
					if (0 == uq->get_first(temp_userID, temp_expire))
					{
						if (false == found || temp_expire < expire)
						{
							expire = temp_expire;
							userID = temp_userID;
							found  = true;
						}
					}
				}
				else			//from queue tail
				{
					
					if (0 == uq->get_last(temp_userID, temp_expire))
					{
						if (false == found || temp_expire > expire)
						{
							expire = temp_expire;
							userID = temp_userID;
							found  = true;
						}
					}
				}				
			}

			if (true == found)
			{
				LogTrace("=====> pull out user[%s], expire_time[%ld]", userID.c_str(), expire);
				return 0;
			}
			else
			{
				LogError("=====> not matched user!");
				return -1;
			}
		}

		unsigned total_queue_count()
		{
			map<string, UserQueue*>::iterator it;
			UserQueue *uq = NULL;
			unsigned total = 0;
			
			for (it = _tag_queue.begin(); it != _tag_queue.end(); it++)
			{
				uq  = it->second;
				total += uq->size();
			}

			return total;
		}
		
		unsigned queue_count(string tag)
		{
			map<string, UserQueue*>::iterator it;
			UserQueue *uq = NULL;

			it = _tag_queue.find(tag);
			if (it == _tag_queue.end())
			{
				return 0;
			}
			
			if (NULL == (uq = it->second))
			{
				return 0;
			}

			return uq->size();
		}
		
		map<string, UserQueue*> _tag_queue;
	};

	class SessionTimer
	{
		public:

		string userID;
		Session session;
		bool isWarn;
		long long warn_time;
		long long expire_time;

		void toJson(Json::Value &value) const
    	{
			session.toJson(value);
			value["warn_time"]   = warn_time;
			value["expire_time"] = expire_time;
		}

		string toString() const
		{
			Json::Value value;
			toJson(value);
			return value.toStyledString();
		}
	};
	
	class SessionQueue
	{
	public:
		list<SessionTimer> _sess_list;
		
		SessionQueue()
		{
			_sess_list.clear();	
		};
		
		virtual ~SessionQueue()
		{
			_sess_list.clear();
		};
		
		int set(string userID, Session* sess, 
				long long gap_warn = 10 * 60, long long gap_expire = 20 * 60 /* seconds */)
		{
			list<SessionTimer>::iterator it;

			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if ((*it).userID == userID)
				{
					(*it).session     = *sess;
					(*it).isWarn      = 0;
					(*it).warn_time   = (sess->atime/1000) + gap_warn;
					(*it).expire_time = (sess->atime/1000) + gap_expire;
					//_sess_list.push_back(st);
					LogTrace("Success to set session: %s", sess->toString().c_str());
					return 0;
				}
			}
			
			LogError("Failed to set session: %s!", sess->toString().c_str());
			return -1;
		}

		int insert(string userID, Session* sess, 
					long long gap_warn = 10 * 60, long long gap_expire = 20 * 60 /* seconds */)
		{
			SessionTimer st;
			list<SessionTimer>::iterator it;
			
			st.userID  = userID;
			st.session = *sess;
			st.isWarn  = 0;
			st.warn_time   = (sess->atime/1000) + gap_warn;
			st.expire_time = (sess->atime/1000) + gap_expire;

			LogDebug("Success to insert session: %s", sess->toString().c_str());
			LogDebug("Session atime: %ld, warn_time: %ld, expire_time: %ld", sess->atime, st.warn_time, st.expire_time);
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if (st.expire_time < (*it).expire_time)
				{
					_sess_list.insert(it, st);
					return 0;
				}
			}
			_sess_list.push_back(st);
			return 0;
		}

		int pop()
		{
			if (_sess_list.size() > 0)
			{
				_sess_list.pop_front();
				return 0;
			}
			
			LogError("Failed to pop from SessionQueue, its size <= 0!");
			return -1;
		}

		int get_first_timer(SessionTimer& sessTimer)
		{
			list<SessionTimer>::iterator it;
			long long nowTime = (long long)time(NULL);
			
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if (nowTime >= it->expire_time)
				{
					sessTimer = (*it);
					return 0;
				}
			}
			
			LogError("Failed to get an expired session from SessionQueue, its size <= 0!");
			return -1;
		}

		int get_first(Session& sess)
		{
			list<SessionTimer>::iterator it;
			long long nowTime = (long long)time(NULL);
			
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if (nowTime >= it->expire_time)
				{
					sess = it->session;
					return 0;
				}
			}
			
			LogError("Failed to get an expired session from SessionQueue, its size <= 0!");
			return -1;
		}


		int get(string userID, Session& sess)
		{
			list<SessionTimer>::iterator it;
			
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if (it->userID == userID)
				{
					sess = it->session;
					return 0;
				}
			}
			return -1;
		}

		int delete_session(string userID)
		{
			list<SessionTimer>::iterator it;
			
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if (it->userID == userID)
				{
					it = _sess_list.erase(it);
					LogDebug("Success to delete session of user[%s]", userID.c_str());
					return 0;
				}
			}
			return -1;
		}

		int check_warn(Session& sess)
		{
			list<SessionTimer>::iterator it;
			long long nowTime = (long long)time(NULL);

			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				//it->isWarn==1表示之前提醒过，不需要再提醒
				if (nowTime >= it->warn_time && it->isWarn == 0)
				{
					LogDebug("[nowTime: %ld] Find session timewarn, session:%s", nowTime, it->toString().c_str());
					sess = it->session;
					it->isWarn = 1;
					return 0;
				}
				/*else if (nowTime < it->warn_time)
				{
					return -1;
				}*/
			}
			
			return -1;
		}

		//返回超时断开的数量
		int check_expire()
		{
			list<SessionTimer>::iterator it;
			int expireNum = 0;
			long long nowTime = (long long)time(NULL);
			
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				if (nowTime >= it->expire_time)
				{
					//LogDebug("[nowTime: %ld] Find session timeout, session:%s", nowTime, it->toString().c_str());
					expireNum++;
				}
			}
			return expireNum;
		}

		unsigned size()
		{ 
			return _sess_list.size();
		}

		unsigned get_usernum_in_service()
		{
			list<SessionTimer>::iterator it;
			unsigned userNum = 0;
			
			for (it = _sess_list.begin(); it != _sess_list.end(); it++)
			{
				SessionTimer st = *it;
				if ("" != st.session.serviceID)
				{
					++userNum;
				}
			}
			return userNum;
		}
	};
}

#endif //_DATA_MODEL_H_
