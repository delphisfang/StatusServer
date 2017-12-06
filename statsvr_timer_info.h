#ifndef __SS_TIMER_INFO_H__
#define __SS_TIMER_INFO_H__

#include "tfc_cache_proc.h"
#include "tfc_base_http.h"
#include "tfc_base_fast_timer.h"
#include "common_api.h"
#include "tfc_debug_log.h"

#include "debug.h"
#include "data_model.h"
#include "app_config.h"
#include "statsvr_error.h"
//#include "longconn_parse/longconn_parse.h"
#include "statsvr_mcd_proc.h"
#include "statsvr_kv.h"

using namespace tfc::base;

namespace statsvr
{
        const unsigned DATA_BUF_SIZE = 1024 * 1024 * 32;

        class CMCDProc;

        class CTimerInfo : public tfc::base::CFastTimerInfo
        {   
        public:

            CTimerInfo()
            {
            }

            CTimerInfo(CMCDProc* const proc, unsigned msg_seq, const timeval& ccd_time, string ccd_client_ip
                                 , uint64_t ret_flow, uint64_t max_time_gap = 10000)
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
                m_client_ip     = ccd_client_ip;
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
                m_content.clear();

                m_ret_code.clear();
                m_ret_msg.clear();
                m_search_no.clear();

	            m_tags.clear();
                m_checkServices.clear();
                m_queuePriority = 0;
            }

            virtual ~CTimerInfo()
            {
                m_tags.clear();
                m_checkServices.clear();
                m_content.clear();
            }

            virtual int  do_next_step(string& req_data)=0;
            virtual int  init(string req_data, int datalen);
            virtual int  on_error();
            virtual int  on_stat();

            virtual void AddStatInfo(const char* itemName, timeval* begin, timeval* end, int retcode);
            virtual void FinishStat(const char* itemName);

            virtual void OpStart();
            virtual void OpEnd(const char* itemName, int retcode);

            virtual void on_expire();
            virtual bool on_expire_delete(){ return true; }           

            uint32_t GetMsgSeq() { return m_msg_seq; }

			long long GetCurTimeStamp()
            {
				long long ret;
				timeval nowTime;
				gettimeofday(&nowTime, NULL);

				ret = (nowTime.tv_sec*1000 + nowTime.tv_usec / 1000);
				return ret;
			}
			
            uint64_t GetTimeGap()
            {
                struct timeval end;
                gettimeofday(&end, NULL);

                uint64_t timecost = CalcTimeCost_MS(m_start_time, end);
				LogDebug("##### Timer GetTimeGap() max_time_gap[%u], timecost[%u]\n"
					            , m_max_time_gap, timecost);

                if (timecost > m_max_time_gap)
                {
                    m_max_time_gap = 1;
                }
                else
                {
                    m_max_time_gap -= timecost;
                }

                // m_op_start = end;

                return m_max_time_gap;
            }

			string get_value_str(Json::Value &jv, const string &key, const string def_val = "");
			unsigned int get_value_uint(Json::Value &jv, const string &key, const unsigned int def_val = 0);
			int get_value_int(Json::Value &jv, const string &key, const int def_val = 0);
			
			void on_error_parse_packet(string errmsg);
			void on_error_get_data(string data_name);
			void on_error_set_data(string data_name);
			void on_error_parse_data(string data_name);
			void set_user_data(Json::Value &data);
			void set_service_data(Json::Value & data);
			void set_system_data(Json::Value &data);
			int on_send_request(string cmd, string ip, unsigned short port, const Json::Value &data, bool with_code = false);
			int on_send_reply(const Json::Value &data);
			int on_send_error_reply(ERROR_TYPE code, string msg, const Json::Value &data);


			/***************** never use m_xxx in methods below, keep them stateless **************/
			
			int get_user_session(string appID, string app_userID, Session *sess);
			void get_user_json(string appID, string app_userID, const UserInfo &user, Json::Value &userJson);
			void construct_user_json(const UserInfo &user, const Session &sess, Json::Value &userJson);
			int reply_user_json_A(string appID, string app_userID, const UserInfo &user);
			int reply_user_json_B(const UserInfo &user, const Session &sess);
			void get_service_json(string appID, const ServiceInfo &serv, Json::Value &servJson);
			int update_user_session(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire);
			int delete_user_session(string appID, string app_userID);
			int create_user_session(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire);
			string gen_sessionID(string app_userID);
			int get_normal_queue(string appID, string raw_tag, UserQueue **uq);
			int get_highpri_queue(string appID, string raw_tag, UserQueue **uq);

			/********************************* KV methods *************************************/

			int KV_set_userIDList();
			int KV_set_servIDList();
			int KV_set_user(string app_userID, const UserInfo &user);
			int KV_set_service(string app_serviceID, const ServiceInfo &serv);
			int KV_set_session(string app_userID, const Session &sess, long long gap_warn, long long gap_expire);
			int KV_del_session(string app_userID);
			int KV_set_queue(string appID, string raw_tag, int highpri);
			int KV_parse_user(string app_userID);
			int KV_parse_session(string app_userID);
			int KV_parse_service(string app_serviceID);
			int KV_parse_queue(string app_tag, bool highpri);


			/************************* wrapper methods **********************/
			int AddUser(string app_userID, const UserInfo &user);
			int UpdateUser(string app_userID, const UserInfo &user);
			int AddService(string appID, string app_servID, ServiceInfo &serv);
			int UpdateService(string app_servID, const ServiceInfo &serv);
			int UpdateUserSession(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire);
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
            string          m_client_ip;

            string          m_cmd;
            uint32_t        m_seq;
			string          m_raw_tag;
            string          m_tag;
            timeval         m_cur_time;
	        string          m_appID;
            string          m_data;
            string          m_cpIP;	//所连CP的IP地址
            unsigned        m_cpPort;//所连CP的端口
            string          m_identity;     //0-user,1-service
            string          m_raw_userID;
            string          m_userID;       //用户ID
            string          m_raw_serviceID;
            string          m_serviceID;    //坐席ID
            string          m_sessionID;
            Json::Value     m_content;
			string          m_raw_changeServiceID;
            string          m_changeServiceID;
			string          m_raw_lastServiceID;
            string          m_lastServiceID;
			bool            m_has_appointServiceID;
			string          m_raw_appointServiceID;
			string          m_appointServiceID;
            set<string>     m_tags;
            set<string>     m_checkServices;
            string          m_priority;
            string          m_ret_code;
            string          m_ret_msg;
            string          m_search_no;
            string          m_channel;
			string          m_status;
            string          m_extends;
            string          m_serviceName;
            string          m_serviceAvatar;
            //string          m_whereFrom;
            //string          m_userInfo;
            unsigned        m_queuePriority;
            unsigned        m_userCount;

			set<string> m_userID_list;
			set<string> m_serviceID_list;
       };
}
#endif
