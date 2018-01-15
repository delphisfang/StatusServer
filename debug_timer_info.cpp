#include "debug_timer_info.h"
#include "statsvr_mcd_proc.h"

#include <algorithm>
#include "jsoncpp/json.h"

using namespace statsvr;

int DebugUserTimer::do_next_step(string& req_data)
{
    if (init(req_data, req_data.size()))
    {
        ON_ERROR_PARSE_PACKET();
        return -1;
    }

    Json::Reader reader;
    Json::Value js_root;
    
    if (!reader.parse(req_data, js_root) || !js_root.isObject() || !js_root["data"].isObject())
    {
        LogError("Failed to parse req_data: %s!", req_data.c_str());
        return -1;
    }

    m_debug_op = get_value_str(js_root["data"], "op");

    if (on_debug_user())
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

int DebugUserTimer::on_debug_user()
{
    Json::Value data = Json::objectValue;
    
    if ("get" == m_debug_op)
    {
        if (0 == mGetUser(m_userID, m_userInfo))
        {
            Json::Value userJson;
            m_userInfo.toJson(userJson);
            data["userInfo"] = userJson;
        }
        else
        {
            data["userInfo"] = Json::objectValue;
        }
    }
    
    if ("delete" == m_debug_op)
    {
        //不判断user的存在性，强制删除
        DO_FAIL(DeleteUserDeep(m_userID));
    }

    return on_send_reply(data);
}

DebugUserTimer::~DebugUserTimer()
{
}

