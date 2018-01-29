#ifndef __SS_TIMER_INFO_H__
#define __SS_TIMER_INFO_H__

/* TFC headers */
#include "tfc_cache_proc.h"
#include "tfc_base_http.h"
#include "tfc_base_fast_timer.h"
#include "tfc_debug_log.h"

/* module headers */
#include "debug.h"
#include "common_api.h"
#include "data_model.h"
#include "app_config.h"
#include "statsvr_error.h"
#include "statsvr_mcd_proc.h"
#include "statsvr_kv.h"

using namespace tfc::base;

namespace statsvr
{
    #define mGetUser(app_userID, user) CAppConfig::Instance()->GetUser(app_userID, user)
    #define mGetService(app_servID, serv) CAppConfig::Instance()->GetService(app_servID, serv)
    
    class CMCDProc;

    class CTimerInfo : public tfc::base::CFastTimerInfo
    {
    public:
        CTimerInfo()
        {
        }

        CTimerInfo(CMCDProc* const proc, unsigned msg_seq, const timeval& ccd_time, string ccd_client_ip, 
                    uint64_t ret_flow, uint64_t max_time_gap = 10000)
        {
            gettimeofday(&m_start_time, NULL);
            m_op_start      = m_start_time;
            m_end_time      = m_start_time;
            m_cur_step      = 0;
            m_errno         = 0;
            m_msg_seq       = msg_seq;
            m_proc          = proc;
            m_max_time_gap  = max_time_gap;
            m_ccd_time      = ccd_time;
            m_ret_flow      = ret_flow;
            //m_client_ip     = ccd_client_ip;
            m_errmsg.clear();

            m_cmd.clear();
            m_seq = 0;

            m_raw_userID.clear();
            m_raw_serviceID.clear();
            m_raw_changeServiceID.clear();
            m_raw_lastServiceID.clear();
            m_raw_appointServiceID.clear();

            m_userID.clear();
            m_serviceID.clear();
            m_changeServiceID.clear();
            m_lastServiceID.clear();
            m_appointServiceID.clear();

            m_sessionID.clear();
            m_identity.clear();
            m_priority.clear();
            m_extends.clear();
            //m_content.clear();

            //m_ret_code.clear();
            //m_ret_msg.clear();
            //m_search_no.clear();

            //m_tags.clear();
            m_queuePriority = 0;

            m_userID_list.clear();
            m_serviceID_list.clear();
            m_changeServiceID_list.clear();
        }

        virtual ~CTimerInfo()
        {
        }

        virtual int  do_next_step(string& req_data)=0;
        virtual int  init(string req_data, int datalen);
        virtual int  debug_init(string req_data);
        virtual int  on_error();
        virtual int  on_stat();
        virtual int  on_admin_error();
        
        virtual void AddStatInfo(const char* itemName, timeval* begin, timeval* end, int retcode);
        virtual void FinishStat(const char* itemName);

        virtual void OpStart();
        virtual void OpEnd(const char* itemName, int retcode);

        virtual void on_expire();
        virtual bool on_expire_delete(){ return true; }           

        uint32_t GetMsgSeq() { return m_msg_seq; }
        uint64_t GetTimeGap();

        void on_error_parse_packet(string errmsg);
        void on_error_get_data(string data_name);
        void on_error_set_data(string data_name);
        void on_error_parse_data(string data_name);
        void set_user_data(Json::Value &data);
        void set_service_data(Json::Value & data);
        int on_send_request(string cmd, string ip, unsigned short port, const Json::Value &data, bool with_code = false);
        int on_send_reply(const Json::Value &data);
        int on_send_error_reply(ERROR_TYPE code, string msg, const Json::Value &data);
        void on_parse_extends(const string &extends, Json::Value &data);
        int on_update_addr(const string &appID, const string &identity, const string &raw_id,
                           const string &cpIP, const unsigned &cpPort);

        /***************** never use m_xxx in methods below, keep them stateless **************/

        int get_user_session(const string &appID, const string &app_userID, Session *sess);
        int get_session_timer(const string &appID, const string &app_userID, SessionTimer *st);
        int get_user_queueRank(const string &appID, const string &app_userID);
        void get_user_json(const string &appID, const string &app_userID, const UserInfo &user, Json::Value &userJson, bool withQueueRank = false);
        void construct_user_json(const string &appID, const string &app_userID, const UserInfo &user, const Session &sess, Json::Value &userJson, bool withQueueRank = false);
        int reply_user_json_A(const string &appID, const string &app_userID, const UserInfo &user);
        int reply_user_json_B(const string &appID, const string &app_userID, const UserInfo &user, const Session &sess);
        unsigned get_service_queuenum(const string &appID, const ServiceInfo &serv);
        int get_service_json(const string &appID, const ServiceInfo &serv, Json::Value &servJson);
        int update_user_session(const string &appID, const string &app_userID, Session *sess, long long gap_warn, long long gap_expire, int is_warn = 0);
        int delete_user_session(const string &appID, const string &app_userID);
        int create_user_session(const string &appID, const string &app_userID, Session *sess, long long gap_warn, long long gap_expire, int is_warn = 0);
        string gen_sessionID(const string &app_userID);
        int get_normal_queue(const string &appID, const string &raw_tag, UserQueue **uq);
        int get_highpri_queue(const string &appID, const string &raw_tag, UserQueue **uq);

        int find_random_service_by_tag(const string &app_tag,
                                    const string &old_app_servID, ServiceInfo &target_serv);
        int find_least_service_by_tag(const string &app_tag,
                                    const string &old_app_servID, ServiceInfo &target_serv);
        int find_least_service_by_list(const set<string> &app_servID_list, 
                                    const string &old_app_servID, ServiceInfo &target_serv);

        /********************************* KV methods *************************************/

        int KV_set_userIDList();
        int KV_set_servIDList();
        int KV_set_user(string app_userID, const UserInfo &user, bool isUpdate);
        int KV_del_user(const string &app_userID);
        int KV_set_service(string app_servID, const ServiceInfo &serv, bool isUpdate);
        int KV_del_service(const string &app_servID);
        int KV_set_session(string app_userID, const Session &sess, long long gap_warn, long long gap_expire, int is_warn = 0);
        int KV_del_session(string app_userID);
        int KV_set_queue(string appID, string raw_tag, bool highpri);
        int KV_parse_user(string app_userID);
        int KV_parse_session(string app_userID);
        int KV_parse_service(string app_servID);
        int KV_parse_queue(string app_tag, bool highpri);

        /************************* wrapper methods **********************/
        int AddUser(string app_userID, const UserInfo &user);
        int UpdateUser(string app_userID, const UserInfo &user);
        int DeleteUser(string app_userID);
        int DeleteUserDeep(string app_userID);
        int CheckFixUser(string app_userID, bool fix);
            
        int AddService(string appID, string app_servID, ServiceInfo &serv);
        int UpdateService(string app_servID, const ServiceInfo &serv);
        int DeleteService(string app_servID);
        int DeleteServiceDeep(string app_servID);
        int CheckFixService(string app_servID, bool fix);
        
        int UpdateUserSession(string appID, string app_userID, Session *sess, int is_warn = 0);
        int UpdateSessionNotified(const string &appID, const string &app_userID);
        int DeleteUserSession(string appID, string app_userID);
        int CreateUserSession(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire);

        int AddTagOnlineServNum(string appID, const ServiceInfo &serv);
        int DelTagOnlineServNum(string appID, const ServiceInfo &serv);

    public:
        int32_t         m_errno;
        string          m_errmsg;

    protected:

        CMCDProc*       m_proc;
        
        uint32_t        m_cur_step;            
        uint32_t        m_msg_seq; 
        struct timeval  m_start_time;
        struct timeval  m_end_time;
        struct timeval  m_op_start;
        struct timeval  m_ccd_time;
        uint64_t        m_max_time_gap;
        uint64_t        m_ret_flow;
        //string          m_client_ip;

        string          m_cmd;
        uint32_t        m_seq;
        string          m_raw_tag;
        string          m_tag;
        //timeval         m_cur_time;
        string          m_appID;
        string          m_data;
        string          m_cpIP;
        unsigned        m_cpPort;
        string          m_identity;
        string          m_raw_userID;
        string          m_userID;
        string          m_raw_serviceID;
        string          m_serviceID;
        string          m_sessionID;
        //Json::Value     m_content;
        string          m_raw_changeServiceID;
        string          m_changeServiceID;
        string          m_raw_lastServiceID;
        string          m_lastServiceID;
        bool            m_has_appointServiceID;
        string          m_raw_appointServiceID;
        string          m_appointServiceID;
        //set<string>     m_tags;
        string          m_priority;
        //string          m_ret_code;
        //string          m_ret_msg;
        //string          m_search_no;
        string          m_channel;
        string          m_status;
        string          m_subStatus;
        string          m_extends;
        string          m_serviceName;
        string          m_serviceAvatar;
        unsigned        m_maxUserNum; //客服上限人数
        //string          m_whereFrom;
        //string          m_userInfo;
        unsigned        m_queuePriority;
        unsigned        m_notify;

        set<string> m_userID_list;
        set<string> m_serviceID_list;
        set<string> m_changeServiceID_list;
    };

    /************************* policy functions **********************/
    bool select_session_by_timeout_model(const SessionTimer &st, void *arg);
}
#endif
