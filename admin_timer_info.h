#ifndef _ADMIN_TIMER_INFO_H_
#define _ADMIN_TIMER_INFO_H_

#include <time.h>
#include <iostream>
#include <string>

#include "statsvr_timer_info.h"

using namespace std;
using namespace tfc;
using namespace tfc::base;

namespace statsvr
{
    class AdminConfigTimer: public CTimerInfo
    {
    public:
		enum STATE
		{
			STATE_INIT		= 0,
			STATE_END		= 255,
		};
		
        AdminConfigTimer(CMCDProc* const proc
                      , unsigned msg_seq
                      , const timeval& ccd_time
                      , string ccd_client_ip
                      , uint64_t ret_flow
                      , uint64_t max_time_gap) 
                      : CTimerInfo(proc, msg_seq, ccd_time, ccd_client_ip, ret_flow, max_time_gap)
        {}
        int  do_next_step(string& req_data);
        int  on_admin_ping();
		int  on_admin_getConf();
		int  on_admin_config();
		int  on_admin_getServiceStatus();
		int  get_app_today_status(string appID, Json::Value &appList);
		int  on_admin_get_today_status();
		
		int  get_id_list(string value, string idListName, vector<string> &idList);
		int  restore_userList();
		int  restore_serviceList();
		int  restore_queue(string appID, vector<string> appID_tags, bool highpri);
		int  on_admin_restore();
    };
    
}

#endif