#ifndef _DEBUG_TIMER_INFO_H_
#define _DEBUG_TIMER_INFO_H_

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
    class DebugUserTimer: public CTimerInfo
    {
        public:
        /*enum DEBUG_OP
        {
            OP_GET = 1,
            OP_DEL = 2,
            OP_SET = 3,
            OP_END = 4,
        };*/
        
        DebugUserTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~DebugUserTimer();

        int do_next_step(string& req_data);
        int on_debug_user();

        string m_debug_op;
        UserInfo m_userInfo;
    };

    class DebugServiceTimer: public CTimerInfo
    {
        public:
        DebugServiceTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~DebugServiceTimer();

        int do_next_step(string& req_data);
        int on_debug_service();

        string m_debug_op;
        ServiceInfo m_serviceInfo;
    };

}

#endif
