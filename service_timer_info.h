#ifndef _SERVICE_TIMER_INFO_H_
#define _SERVICE_TIMER_INFO_H_

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
    class GetServiceInfoTimer: public CTimerInfo
    {
        public:
        GetServiceInfoTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~GetServiceInfoTimer();

        int do_next_step(string& req_data);
		int on_not_online();
        int on_get_serviceinfo();
    };

    class ServiceLoginTimer:public CTimerInfo
    {
        public:
        ServiceLoginTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~ServiceLoginTimer();

        int do_next_step(string& req_data);
		int on_serviceLogin_reply();
		int on_already_online();
		void set_service_fields(ServiceInfo &serv);
		int on_service_login();
    };

    class ServiceChangeStatusTimer:public CTimerInfo
    {
        public:
        ServiceChangeStatusTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~ServiceChangeStatusTimer();

        int do_next_step(string& req_data);
        int on_resp_cp();
		int on_already_online();
		int on_already_offline();
		int on_not_online();
		int on_service_changestatus();
    };

    class ChangeServiceTimer:public CTimerInfo
    {
        public:
        ChangeServiceTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~ChangeServiceTimer();

        int do_next_step(string& req_data);
        int on_resp_cp();
		void set_change_service_data(Json::Value &data);
		int on_service_busy();
		int on_session_wrong();
		int on_service_offline();
		int on_change_service();
    };

    class ServicePullNextTimer:public CTimerInfo
    {
    public:
        ServicePullNextTimer(CMCDProc* const proc
                  , unsigned msg_seq
                  , const timeval& ccd_time
                  , string ccd_client_ip
                  , uint64_t ret_flow
                  , uint64_t max_time_gap) 
                  : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~ServicePullNextTimer();

        int do_next_step(string& req_data);
		int on_send_connect_success();
        int on_pull_next();

		Session m_session;
		ServiceInfo m_serviceInfo;
    };

}

#endif
