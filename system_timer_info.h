#ifndef _SYSTEM_TIMER_INFO_H_
#define _SYSTEM_TIMER_INFO_H_

#include <time.h>
#include <iostream>
#include <string>

#include "statsvr_timer_info.h"

using namespace std;
using namespace tfc;
using namespace tfc::base;
using namespace tfc::cache;

namespace statsvr
{
    #ifndef DISABLE_YIBOT_SESSION_CHECK
    class YiBotOutTimer:public CTimerInfo
    {
        public:
        YiBotOutTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}

        int  do_next_step(string& req_data);
        int  on_yibot_timeout();
    };
    #endif

    class UserOutTimer:public CTimerInfo
    {
        public:
        UserOutTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        
        int  do_next_step(string& req_data);
        int  on_user_timeout();

        int  m_user_time_gap;
        set<string> m_userList;
    };

    class ServiceOutTimer:public CTimerInfo
    {
        public:
        ServiceOutTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        
        int  do_next_step(string& req_data);
        int  on_service_timeout();

        int  m_service_time_gap;
        set<string> m_serviceList;
    };
    
    class SessionOutTimer:public CTimerInfo
    {
        public:
        SessionOutTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}

        int  do_next_step(string& req_data);
        int  on_send_timeout_msg();
        int  on_session_timeout();

        UserInfo m_userInfo;
        ServiceInfo m_serviceInfo;
    };

    class SessionWarnTimer:public CTimerInfo
    {
        public:
        SessionWarnTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}

        int  do_next_step(string& req_data);
        int  on_send_timewarn_msg();
        int  on_session_timewarn();

        UserInfo m_userInfo;
        ServiceInfo m_serviceInfo;
    };

    class QueueOutTimer: public CTimerInfo
    {
        public:
        QueueOutTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}

        int  do_next_step(string& req_data);
        int  on_queue_timeout(string& req_data);
    };

    class UserServiceTimer:public CTimerInfo
    {
        public:
        UserServiceTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}

        int  do_next_step(string& req_data);
        int  on_user_tag();
        int  on_user_lastService();
        int  on_user_common();
        int  on_dequeue_first_user();
        int  on_offer_service();
        int  on_create_session();
        int  on_send_connect_success();

        UserInfo m_userInfo;
        Session m_session;
        ServiceInfo m_serviceInfo;
        int m_serverNum;
    };

    class RefreshSessionTimer:public CTimerInfo
    {
        public:
        RefreshSessionTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}

        int  do_next_step(string& req_data);
        int  on_refresh_session();
    };

}
#endif

