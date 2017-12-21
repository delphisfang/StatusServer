#include "txf_timer_info.h"
#include "statsvr_mcd_proc.h"
#include <algorithm>

int TransferTimer::do_next_step(string& req_data)
{
    switch (m_cur_step)
    {
		case STATE_INIT:
		{
    		if (init(req_data, req_data.size()))
    		{
				ON_ERROR_PARSE_PACKET();
				return -1;
    		}
			
            if ("getAddrByID" == m_cmd)
            {
                if (on_get_cp_addr())
				{
					m_cur_step = STATE_END;	
					return -1;
				}
				on_stat();
				m_cur_step = STATE_END;
				return 1;
			}
			else
            {
                LogError("Error unknown cmd: %s", m_cmd.c_str());
				ON_ERROR_PARSE_PACKET();
                return -1;
            }
		}
		default:
		{
			LogError("Wrong TransferTimer state: %d", m_cur_step);
			return -1;
		}
	}
	return 0;
}

int TransferTimer::on_not_online()
{
	Json::Value data;
	data["identity"]  = m_identity;
	data["userID"]    = m_userID;
	data["serviceID"] = m_serviceID;
	
	return on_send_error_reply(ERROR_USER_NOT_ONLINE, "User Not Online", data);
}

int TransferTimer::on_rsp_cp_addr()
{
	Json::Value data;

	data["appID"]    = m_appID;
	data["identity"] = m_identity;
	if ("user" == m_identity)
	{
		data["userID"]        = m_userID;
		data["chatProxyIp"]   = m_userInfo.cpIP;
		data["chatProxyPort"] = m_userInfo.cpPort;

	}
	else
	{
		data["serviceID"]     = m_serviceID;
		data["chatProxyIp"]   = m_serviceInfo.cpIP;
		data["chatProxyPort"] = m_serviceInfo.cpPort;
	}

	Json::Value arr;
	arr.append(data);

	Json::Value outData;
	outData["AddrList"] = arr;
	
	return on_send_reply(outData);
}

int TransferTimer::on_get_cp_addr()
{
	if ("user" == m_identity)
	{
		if (CAppConfig::Instance()->GetUser(m_userID, m_userInfo))
		{
			return on_not_online();
		}
	}
	else if ("service" == m_identity)
	{
		if (CAppConfig::Instance()->GetService(m_serviceID, m_serviceInfo))
		{
			return on_not_online();
		}
	}
	else
	{
		LogError("Unknown identity: %s!", m_identity.c_str());
		ON_ERROR_PARSE_DATA("identity");
		return SS_ERROR;
	}

	return on_rsp_cp_addr();	
}

TransferTimer::~TransferTimer()
{
}
