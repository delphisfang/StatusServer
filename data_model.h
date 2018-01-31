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
        bool has_refreshed() const;
        
        string    sessionID;
        string    userID;
        string    cpIP;         //用户的cpIP
        unsigned short cpPort;  //用户的cpPort
        string    serviceID;    //坐席ID
        long long atime;        //活跃时间，即有消息时间到达就更新
        long long btime;        //建立连接的时间
        int       notified;     //欢迎语通知
        string    lastTalk;     //最后发送消息者
        //bool      toIM;       //标记数据流向，默认为0
        //string      whereFrom;//标记使用http还是websocket
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
        int set_field(const string &field, const string &value);
        
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
        unsigned queuePriority;
        string channel;
        string extends;
    };

    struct ServiceInfo
    {
        ServiceInfo();
        ~ServiceInfo();
        ServiceInfo(const string& strServiceInfo, unsigned dft_user_num);

        void parse_tags(const Json::Value &value);
        void parse_userList(const Json::Value &value);

        void toJson(Json::Value &value) const;
        string toString() const;

        int delete_user(const string &raw_userID);
        int find_user(const string &raw_userID);
        int add_user(const string &raw_userID);
        unsigned user_count() const;
        bool is_available() const;
        bool is_busy() const;
        bool check_tag_exist(const string &raw_tag) const;
        int set_field(const string &field, const string &value);
        
        string serviceID;
        string status;
        long long atime;
        string   cpIP;
        unsigned cpPort;
        set<string> tags;
        set<string> userList;
        string   serviceName;
        string   serviceAvatar;
        unsigned maxUserNum;
        string subStatus;
        //string     whereFrom;
    };

    //仅存储serviceID
    struct ServiceHeap
    {
        ServiceHeap();
        ~ServiceHeap();
        ServiceHeap(const string& strServiceHeap);

        string toString() const;

        unsigned size();
        int find_service(const string &app_servID);
        int add_service(const string &app_servID);
        int delete_service(const string &app_servID);

        //按tag对service进行分组，方便转人工时根据user的tag找到service
        set<string> _servlist;
    };

    //带超时时间的用户信息
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
        
        ~UserQueue()
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
            for (list<UserTimer>::iterator it = _user_list.begin(); it != _user_list.end(); ++it)
            {
                ut = (*it);
                obj["userID"]      = ut.userID;
                obj["expire_time"] = ut.expire_time;

                queueList.append(obj);
            }
            arr["queueList"] = queueList;

            Json::FastWriter writer;
            return writer.write(arr);
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

        // get first user on queue
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

        // get last user on queue
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
                else
                {
                    pos++;
                }
            }
            
            return -1;
        }

        int delete_user(string userID)
        {
            list<UserTimer>::iterator it;

            for (it = _user_list.begin(); it != _user_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    _user_list.erase(it);
                    return 0;
                }
            }

            LogWarn("Failed to delete user[%s] from queue, no such user.", userID.c_str());
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
            //important
            map<string, UserQueue*>::iterator it;
            for (it = _tag_queue.begin(); it != _tag_queue.end(); ++it)
            {
                delete it->second;
                LogTrace("Delete queue[%s].", (it->first).c_str());
            }
            
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

        int find_user(string userID, string &app_tag)
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
                    app_tag = it->first;
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

            LogTrace("====> direct = %d", direct);
            
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
                else            //from queue tail
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
                LogTrace("====> pull out user[%s], expire_time[%ld]", userID.c_str(), expire);
                return 0;
            }
            else
            {
                LogError("====> no matched user!");
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
        int isWarn;
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

    typedef long long (*SESS_GAP_FUNC)(const SessionTimer &st, void *arg);

    class SessionQueue
    {
    public:
        //以expire_time从大到小排序
        list<SessionTimer> _sess_list;
        //以atime从大到小排序
        list<SessionTimer> _yibot_list;
        
        SessionQueue()
        {
            _sess_list.clear();
            _yibot_list.clear();
        }

        //important: free all elements
        ~SessionQueue()
        {
            _sess_list.clear();
            _yibot_list.clear();
        }

        int set(string userID, Session *sess, 
                long long gap_warn = 10*60, long long gap_expire = 20*60, int is_warn = 0)
        {
            //先删除再插入，以保证有序
            if (delete_session(userID)
                || insert(userID, sess, gap_warn, gap_expire, is_warn))
            {
                LogError("Failed to set session: %s!", sess->toString().c_str());
                return -1;
            }
            else
            {
                LogTrace("Success to set session: %s.", sess->toString().c_str());
                return 0;
            }
        }

        int insert(string userID, Session *sess, 
                    long long gap_warn = 10*60, long long gap_expire = 20*60, int is_warn = 0)
        {
            SessionTimer st;
            list<SessionTimer>::iterator it;
            
            st.userID      = userID;
            st.session     = *sess;
            st.isWarn      = is_warn;
            st.warn_time   = (sess->atime/1000) + gap_warn;
            st.expire_time = (sess->atime/1000) + gap_expire;

            LogDebug("Success to insert session: %s", sess->toString().c_str());
            LogDebug("Session atime: %ld, warn_time: %ld, expire_time: %ld, is_warn: %d", 
                    sess->atime, st.warn_time, st.expire_time, st.isWarn);

            if (gap_warn >= MAX_INT && gap_expire >= MAX_INT)
            {
                for (it = _yibot_list.begin(); it != _yibot_list.end(); ++it)
                {
                    if (st.session.atime > (*it).session.atime)
                    {
                        _yibot_list.insert(it, st);
                        return 0;
                    }
                }

                _yibot_list.push_back(st);
                return 0;
            }
            else
            {
                for (it = _sess_list.begin(); it != _sess_list.end(); ++it)
                {
                    if (st.expire_time > (*it).expire_time)
                    {
                        _sess_list.insert(it, st);
                        return 0;
                    }
                }
                
                _sess_list.push_back(st);
                return 0;
            }
        }

        int get_first_timer(SESS_GAP_FUNC pFunc, void *arg, SessionTimer& sessTimer)
        {
            long long nowTime = (long long)time(NULL);
            long long timeGap = 0;

            list<SessionTimer>::reverse_iterator it;
            for (it = _sess_list.rbegin(); it != _sess_list.rend(); ++it)
            {
                if (nowTime < it->expire_time)
                {
                    break;
                }

                timeGap = (*pFunc)(*it, arg);
                if (nowTime >= (it->expire_time + timeGap))
                {
                    sessTimer = (*it);
                    return 0;
                }
            }
            
            LogError("Failed to get an expired session, its size <= 0!");
            return -1;
        }

        int get(string userID, Session& sess)
        {
            list<SessionTimer>::iterator it;
            
            for (it = _sess_list.begin(); it != _sess_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    sess = it->session;
                    return 0;
                }
            }

            for (it = _yibot_list.begin(); it != _yibot_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    sess = it->session;
                    return 0;
                }
            }

            return -1;
        }

        int get_sess_timer(string userID, SessionTimer& st)
        {
            list<SessionTimer>::iterator it;

            for (it = _sess_list.begin(); it != _sess_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    st = (*it);
                    return 0;
                }
            }

            for (it = _yibot_list.begin(); it != _yibot_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    st = (*it);
                    return 0;
                }
            }
            
            return -1;
        }

        int delete_session(string userID)
        {
            list<SessionTimer>::iterator it;
            
            for (it = _sess_list.begin(); it != _sess_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    _sess_list.erase(it);
                    LogDebug("Success to delete human session of user[%s]", userID.c_str());
                    return 0;
                }
            }

            for (it = _yibot_list.begin(); it != _yibot_list.end(); ++it)
            {
                if (it->userID == userID)
                {
                    _yibot_list.erase(it);
                    LogDebug("Success to delete yibot session of user[%s]", userID.c_str());
                    return 0;
                }
            }
            
            return -1;
        }

        int check_warn(SESS_GAP_FUNC pFunc, void *arg, SessionTimer& st)
        {
            long long nowTime = (long long)time(NULL);
            long long timeGap = 0;
            list<SessionTimer>::reverse_iterator it;
            for (it = _sess_list.rbegin(); it != _sess_list.rend(); ++it)
            {
                timeGap = (*pFunc)(*it, arg);

                if (nowTime >= (it->warn_time + timeGap) && 0 == it->isWarn)
                {
                    LogDebug("[nowTime: %ld] Find session timewarn, session:%s", nowTime, it->toString().c_str());
                    st = (*it);
                    it->isWarn = 1;
                    return 0;
                }
            }
            
            return -1;
        }

        //返回超时结束的数量
        int check_expire(SESS_GAP_FUNC pFunc, void *arg)
        {
            int expireNum = 0;
            long long nowTime = (long long)time(NULL);
            long long timeGap = 0;

            list<SessionTimer>::reverse_iterator it;
            for (it = _sess_list.rbegin(); it != _sess_list.rend(); ++it)
            {
                if (nowTime < it->expire_time)
                {
                    return expireNum;
                }

                timeGap = (*pFunc)(*it, arg);
                if (nowTime >= (it->expire_time + timeGap))
                {
                    ++expireNum;
                }
            }
            
            return expireNum;
        }

        void check_expire_yibot(long long gap_expire, vector<string> &app_userID_list)
        {
            app_userID_list.clear();
            long long nowTime = (long long)time(NULL);

            list<SessionTimer>::reverse_iterator it;
            for (it = _yibot_list.rbegin(); it != _yibot_list.rend(); ++it)
            {
                if (nowTime >= ((it->session.atime/1000) + gap_expire))
                {
                    app_userID_list.push_back(it->userID);
                }
                else
                {
                    return;
                }
            }
        }
        
        unsigned size()
        { 
            return _sess_list.size() + _yibot_list.size();
        }

        unsigned get_usernum_in_service()
        {
            return _sess_list.size();
        }
    };
}

#endif //_DATA_MODEL_H_
