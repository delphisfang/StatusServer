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
                m_identity.clear();
                m_userID.clear();
                m_serviceID.clear();
                m_content.clear();
                m_changeServiceID.clear();
                m_lastServiceID.clear();
                m_priority.clear();
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
			
			void on_error_parse_packet(string errmsg)
            {
                m_errno  = ERROR_UNKNOWN_PACKET;
                m_errmsg = errmsg;
    			on_error();
			}

			void on_error_get_data(string data_name)
			{
				m_errno  = ERROR_SYSTEM_WRONG;
				m_errmsg = "Error get " + data_name;
				on_error();
			}

			void on_error_set_data(string data_name)
			{
				m_errno  = ERROR_SYSTEM_WRONG;
				m_errmsg = "Error set " + data_name;
				on_error();
			}

			void on_error_parse_data(string data_name)
			{
				m_errno  = ERROR_SYSTEM_WRONG;
				m_errmsg = "Error parse " + data_name;
				on_error();
			}

			void set_user_data(Json::Value &data)
			{
				data["identity"]  = "user";
				data["userID"]    = m_raw_userID;
			}

			void set_service_data(Json::Value & data)
			{
				data["identity"]  = "service";
				data["serviceID"] = m_raw_serviceID;
			}

			void set_system_data(Json::Value &data)
			{
				data["userID"]	  = m_raw_userID;
				data["serviceID"] = m_raw_serviceID;
				data["sessionID"] = m_sessionID;
				data["identity"]  = "system";
			}

			int on_send_request(string cmd, string ip, unsigned short port, const Json::Value &data, bool with_code = false)
            {
				Json::Value req;
				req["appID"]	= m_appID;
				req["method"]	= cmd;
				req["innerSeq"] = m_msg_seq;
				req["data"] 	= data;
				if (with_code)
					req["code"] = 0;
				string strReq   = req.toStyledString();
				LogTrace("send request to <%s, %d>: %s", ip.c_str(), port, strReq.c_str());
				if (m_proc->EnququeHttp2DCC((char *)strReq.c_str(), strReq.size(), ip, port))
				{
					LogError("[%s]: Error send request %s", m_appID.c_str(), cmd.c_str());
					return -1;
				}
				return 0;
			}

			int on_send_reply(const Json::Value &data)
            {
				Json::Value rsp;
				rsp["appID"]	= m_appID;
				rsp["method"]	= m_cmd + "-reply";
				rsp["innerSeq"] = m_seq;
				rsp["code"] 	= 0;
				rsp["data"] 	= data;
				string strRsp   = rsp.toStyledString();
				LogTrace("send response: %s", strRsp.c_str());
				if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
				{
					LogError("searchid[%s]: Failed to SendReply <%s>", m_search_no.c_str(), m_cmd.c_str());
					m_errno  = ERROR_SYSTEM_WRONG;
					m_errmsg = "Error send to ChatProxy";
					on_error();
					return -1;
				}
				else
				{
					on_stat();
					return 0;
				}
			}
			
			int on_send_error_reply(ERROR_TYPE code, string msg, const Json::Value &data)
            {
				Json::Value rsp;
				rsp["appID"]     = m_appID;
				rsp["method"]	 = m_cmd + "-reply";
				rsp["innerSeq"]  = m_seq;
				if (code > 0)
				{
					rsp["code"] = 0;
				}
				else
				{
					rsp["code"] = code;
				}
				rsp["msg"]		 = msg;
				rsp["data"] 	 = data;
				string strRsp    = rsp.toStyledString();
				LogTrace("send ERROR response: %s", strRsp.c_str());
				if (m_proc->EnququeHttp2CCD(m_ret_flow, (char *)strRsp.c_str(), strRsp.size()))
				{
					LogError("searchid[%s]: Failed to SendErrorReply <%s>", m_search_no.c_str(), msg.c_str());
					m_errno  = ERROR_SYSTEM_WRONG;
					m_errmsg = "Error send to ChatProxy";
					on_error();
					return -1;
				}
				else
				{
					on_stat();
					return 0;
				}
			}


			/***************** never use m_xxx in methods below, keep them stateless **************/
			
			int get_user_session(string appID, string app_userID, Session *sess)
			{
				SessionQueue*  pSessQueue = NULL;
				
				if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
					|| pSessQueue->get(app_userID, *sess))
				{
					LogError("Failed to get session of user[%s]", app_userID.c_str());
					return SS_ERROR;
				}

				return SS_OK;
			}

			void get_user_json(string appID, string app_userID, const UserInfo &user, Json::Value &userJson)
			{
				Session sess;
				Json::Value sessJson;

				user.toJson(userJson);
				if (get_user_session(appID, app_userID, &sess))
				{
					userJson["session"] = Json::objectValue;
				}
				else
				{
					sess.toJson(sessJson);
					userJson["session"] = sessJson;
				}
			}

			void construct_user_json(const UserInfo &user, const Session &sess, Json::Value &userJson)
			{
				Json::Value sessInfo;
				user.toJson(userJson);
				sess.toJson(sessInfo);
				userJson["session"] = sessInfo;
			}

			int reply_user_json_A(string appID, string app_userID, const UserInfo &user)
			{
				Json::Value userJson;
				get_user_json(appID, app_userID, user, userJson);
				DO_FAIL(on_send_reply(userJson));
				return SS_OK;
			}

			int reply_user_json_B(const UserInfo &user, const Session &sess)
			{
				Json::Value userJson;
				construct_user_json(user, sess, userJson);
				DO_FAIL(on_send_reply(userJson));
				return SS_OK;
			}

			void get_service_json(string appID, const ServiceInfo &serv, Json::Value &servJson)
			{
				Json::Value arrayUserList;

				serv.toJson(servJson);
				arrayUserList.resize(0);
				for (set<string>::iterator it = serv.userList.begin(); it != serv.userList.end(); it++)
				{
					string userID = appID + "_" + *it;
					UserInfo user;
					Json::Value userJson = Json::objectValue;
					
					userJson["userID"]    = userID;
					if (CAppConfig::Instance()->GetUser(userID, user))
					{
						LogError("Failed to get user: %s!", userID.c_str());
						userJson["sessionID"] = "";
						userJson["channel"]   = "";
					}
					else
					{
						userJson["sessionID"] = user.sessionID;
						userJson["channel"]   = user.channel;
					}
					arrayUserList.append(userJson);
				}
				servJson["userList"].resize(0);
				servJson["userList"] = arrayUserList;
			}
			
			int update_user_session(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
			{
				SessionQueue*  pSessQueue = NULL;
				
				if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
					|| pSessQueue->set(app_userID, sess, gap_warn, gap_expire))
				{
					LogError("Failed to set session of user[%s]", app_userID.c_str());
					return SS_ERROR;
				}

				return SS_OK;
			}

			int create_user_session(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
			{
				SessionQueue*  pSessQueue = NULL;
				
				if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue)
					|| pSessQueue->insert(app_userID, sess, gap_warn, gap_expire))
				{
					LogError("Failed to create session of user[%s]", app_userID.c_str());
					return SS_ERROR;
				}

				return SS_OK;
			}

			string gen_sessionID(string app_userID)
			{
				return app_userID + "_" + l2str(time(NULL));
			}

			int get_normal_queue(string appID, string raw_tag, UserQueue **uq)
			{
				TagUserQueue* pTagQueues = NULL;
				GET_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues), "tag user queues");
				GET_FAIL(pTagQueues->get_tag(raw_tag, *uq), "user queue");
				return SS_OK;
			}

			int get_highpri_queue(string appID, string raw_tag, UserQueue **uq)
			{
				TagUserQueue* pTagQueues = NULL;
				GET_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues), "highpri tag user queues");
				GET_FAIL(pTagQueues->get_tag(raw_tag, *uq), "user queue");
				return SS_OK;
			}

			/********************************* KV methods *************************************/

			int KV_set_userIDList()
			{
				string strUserIDList;
				DO_FAIL(CAppConfig::Instance()->UserListToString(strUserIDList));
				return KVSetKeyValue(KV_CACHE, USERLIST_KEY, strUserIDList);	
			}

			int KV_set_servIDList()
			{
				string strServIDList;
				DO_FAIL(CAppConfig::Instance()->ServiceListToString(strServIDList));
				return KVSetKeyValue(KV_CACHE, SERVLIST_KEY, strServIDList);	
			}

			int KV_set_user(string app_userID, const UserInfo &user)
			{
				string key, value;
				key   = USER_PREFIX+app_userID;
				value = user.toString();
								
				DO_FAIL(KVSetKeyValue(KV_CACHE, key, value));
				DO_FAIL(KV_set_userIDList());
				return SS_OK;
			}

			int KV_set_service(string app_serviceID, const ServiceInfo &serv)
			{
				DO_FAIL(KVSetKeyValue(KV_CACHE, SERV_PREFIX+app_serviceID, serv.toString()));
				DO_FAIL(KV_set_servIDList());
				return SS_OK;
			}

			int KV_set_session(string app_userID, const Session &sess, long long gap_warn, long long gap_expire)
			{
				long long warn_time   = (sess.atime/1000) + gap_warn;
				long long expire_time = (sess.atime/1000) + gap_expire;
				Json::Value sessJson;
				sess.toJson(sessJson);
				sessJson["warn_time"]	= warn_time;
				sessJson["expire_time"] = expire_time;
				return KVSetKeyValue(KV_CACHE, SESS_PREFIX+app_userID, sessJson.toStyledString());
			}

			int KV_set_queue(string appID, string raw_tag, int highpri)
			{
				TagUserQueue *pTagQueues = NULL;
				UserQueue *uq = NULL;

				string prefix        = (highpri) ? (HIGHQ_PREFIX) : (QUEUE_PREFIX);
				string key_queueList = prefix + appID + "_" + raw_tag;

				if (highpri)
				{
					GET_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues), "tag queues");
				}
				else
				{
					GET_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues), "tag highpri queues");
				}
				GET_FAIL(pTagQueues->get_tag(raw_tag, uq), "user queue");

				string val_queueList = uq->toString();
				DO_FAIL(KVSetKeyValue(KV_CACHE, key_queueList, val_queueList));
				return SS_OK;
			}

			int KV_parse_user(string app_userID)
			{
				LogTrace("parse userID:%s", app_userID.c_str());
				
				string user_key = USER_PREFIX + app_userID;
				string user_value;
				DO_FAIL(KVGetKeyValue(KV_CACHE, user_key, user_value));
				
				UserInfo user(user_value);
				SET_USER(CAppConfig::Instance()->AddUser(app_userID, user));
				return SS_OK;
			}

			int KV_parse_session(string app_userID)
			{
				LogTrace("parse session of userID:%s", app_userID.c_str());
				
				Json::Reader reader;
				string sess_key = SESS_PREFIX + app_userID;
				string sess_value;
				DO_FAIL(KVGetKeyValue(KV_CACHE, sess_key, sess_value));
				
				Session sess(sess_value);
				Json::Value obj;
				if (!reader.parse(sess_value, obj))
				{
					return SS_ERROR;
				}
				
				long long warn_time   = obj["warn_time"].asInt64();
				long long expire_time = obj["expire_time"].asInt64();
				string appID = getappID(app_userID);
				SET_SESS(CreateUserSession(appID, app_userID, &sess, warn_time, expire_time));
				return SS_OK;
			}

			int KV_parse_service(string app_serviceID)
			{
				LogTrace("parse serviceID:%s", app_serviceID.c_str());
				
				/*获取*/
				string serv_key = "serv_" + app_serviceID;
				string serv_value;
				DO_FAIL(KVGetKeyValue(KV_CACHE, serv_key, serv_value));
				
				/* 解析每个service的详细信息 */
				ServiceInfo serv(serv_value);
				SET_USER(CAppConfig::Instance()->AddService(app_serviceID, serv));
				
				/* 添加到ServiceHeap */
				string appID = getappID(app_serviceID);
				SET_FAIL(CAppConfig::Instance()->AddService2Tags(appID, serv), "service heap");
				return SS_OK;
			}
			
			int KV_parse_queue(string app_tag, bool highpri)
			{
				LogTrace("parse queue, app_tag: %s", app_tag.c_str());
				
				TagUserQueue *pTagQueues = NULL;
				string key_queueList, val_queueList;

				string appID   = getappID(app_tag);
				string raw_tag = delappID(app_tag);
				
				if (true == highpri)
				{
					DO_FAIL(CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues));
					key_queueList  = QUEUE_PREFIX + app_tag;
				}
				else
				{
					DO_FAIL(CAppConfig::Instance()->GetTagQueue(appID, pTagQueues));
					key_queueList  = HIGHQ_PREFIX + app_tag;
				}

				/*获取*/
				if (KVGetKeyValue(KV_CACHE, key_queueList, val_queueList))
				{
					LogTrace("[statsvr_KV] queue[%s] is empty!", key_queueList.c_str());
					return SS_OK;
				}

				/*解析*/
				Json::Reader reader;
				Json::Value obj, queueNode;
				if (!reader.parse(val_queueList, obj))
				{
					return SS_ERROR;
				}
				int queueNum = obj["queueList"].size();
				for (int i = 0; i < queueNum; i++)
				{
					queueNode = obj["queueList"][i];
					string userID		  = queueNode["userID"].asString();
					long long expire_time = queueNode["expire_time"].asInt64();
					SET_USER(pTagQueues->add_user(raw_tag, userID, expire_time));
				}

				return SS_OK;
			}

			/************************* wrapper methods **********************/
			int AddUser(string app_userID, const UserInfo &user)
			{
				SET_USER(CAppConfig::Instance()->AddUser(app_userID, user));
				DO_FAIL(KV_set_user(app_userID, user));
				return SS_OK;
			}

			int UpdateUser(string app_userID, const UserInfo &user)
			{
				SET_USER(CAppConfig::Instance()->UpdateUser(app_userID, user));
				DO_FAIL(KV_set_user(app_userID, user));
				return SS_OK;
			}

			int AddService(string appID, string app_servID, ServiceInfo &serv)
			{
				SET_SERV(CAppConfig::Instance()->AddService(app_servID, serv));
				//insert service in every tag serviceHeap
				SET_FAIL(CAppConfig::Instance()->AddService2Tags(appID, serv), "service heap");
				DO_FAIL(KV_set_service(app_servID, serv));
				return SS_OK;

			}

			int UpdateService(string app_servID, const ServiceInfo &serv)
			{
				SET_SERV(CAppConfig::Instance()->UpdateService(app_servID, serv));
				DO_FAIL(KV_set_service(app_servID, serv));
				return SS_OK;
			}

			int UpdateUserSession(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
			{
				SET_SESS(update_user_session(appID, app_userID, sess, gap_warn, gap_expire));
				DO_FAIL(KV_set_session(app_userID, *sess, gap_warn, gap_expire));
				return SS_OK;
			}

			int CreateUserSession(string appID, string app_userID, Session *sess, long long gap_warn, long long gap_expire)
			{
				SET_SESS(create_user_session(appID, app_userID, sess, gap_warn, gap_expire));
				DO_FAIL(KV_set_session(app_userID, *sess, gap_warn, gap_expire));
				return SS_OK;
			}
			
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
            string          m_lastServiceID;
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
