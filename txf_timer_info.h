#ifndef _TXF_TIMER_INFO_H_
#define _TXF_TIMER_INFO_H_

#include <time.h>
#include <iostream>
#include <string>

#include "statsvr_timer_info.h"

using namespace std;
using namespace tfc;
using namespace tfc::base;

namespace statsvr
{
    class TransferTimer: public CTimerInfo
    {
    public:
        enum STATE
        {
            STATE_INIT        = 0,
            STATE_END        = 255,
        };
        
        TransferTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        ~TransferTimer();
        int do_next_step(string& req_data);
        int on_not_online();
        int on_rsp_cp_addr();
        int on_get_cp_addr();

        UserInfo m_userInfo;
        ServiceInfo m_serviceInfo;
    };
    
}

#endif