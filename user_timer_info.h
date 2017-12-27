#ifndef _USER_TIMER_INFO_H_
#define _USER_TIMER_INFO_H_

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
    class EchoTimer: public CTimerInfo
    {
        public:
        enum STATE
        {
            STATE_INIT = 0,
            STATE_END  = 255,
        };

        EchoTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~EchoTimer();

        int do_next_step(string& req_data);
        int on_echo();
    };

    class GetUserInfoTimer: public CTimerInfo
    {
        public:
        GetUserInfoTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~GetUserInfoTimer();

        int do_next_step(string& req_data);
        int on_not_online();
        int on_get_userinfo();
    };

    class UserOnlineTimer:public CTimerInfo
    {
        public:
        UserOnlineTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~UserOnlineTimer();

        int do_next_step(string& req_data);
        //int on_resp_cp();
        //int on_already_online();
        void set_user_fields(UserInfo &user);
        int on_user_online();

        string m_status;
    };

    class ConnectServiceTimer:public CTimerInfo
    {
        public:
        ConnectServiceTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {
        }
        ~ConnectServiceTimer();

        int do_next_step(string& req_data);
        int set_data(Json::Value &data);
        int on_already_onqueue();
        int on_no_service();
        int on_reject_enqueue();
        int on_already_inservice();
        int on_service_with_noqueue(bool flag);
        int on_appoint_service_offline();
        int on_send_connect_success(const Session &sess, const ServiceInfo &serv);
        int on_appoint_service();
        int on_queue();
        int on_connect_service();
    };

    class CancelQueueTimer:public CTimerInfo
    {
        public:
        CancelQueueTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~CancelQueueTimer();

        int do_next_step(string& req_data);
        int on_resp_cp();
        int on_not_onqueue();
        int on_cancel_queue();
    };

    class CloseSessionTimer:public CTimerInfo
    {
        public:
        CloseSessionTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~CloseSessionTimer();

        int do_next_step(string& req_data);
        int on_closeSession_reply(const string &oldSessionID);
        int on_not_inservice();
        int on_close_session();

        Session m_session;
        ServiceInfo m_serviceInfo;
    };

    class RefreshUserTimer:public CTimerInfo
    {
        public:
        RefreshUserTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~RefreshUserTimer();
        int  do_next_step(string& req_data);
        int  on_refresh_user();
    };

}

#endif
