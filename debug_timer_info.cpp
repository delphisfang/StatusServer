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

int DebugUserTimer::on_get_user(Json::Value &data)
{
    if (0 == mGetUser(m_userID, m_userInfo))
    {
        Json::Value userJson;
        m_userInfo.toJson(userJson);
    
        if (0 == get_user_session(m_appID, m_userID, &m_session))
        {
            construct_user_json(m_userInfo, m_session, userJson);
        }
        else
        {
            userJson["session"] = Json::objectValue;
        }
    
        data["userInfo"].append(userJson);
    }
    else
    {
        data["userInfo"].append(Json::objectValue);
    }
    return 0;
}

int DebugUserTimer::on_check_fix_user(Json::Value &data, bool fix)
{
    Json::Value userJson = Json::objectValue;
    userJson["userID"] = m_raw_userID; 
    
    int ret = CheckFixUser(m_userID, fix);
    if (0 == ret)
    {
        userJson["msg"] = "OK";
    }
    else if (-1 == ret)
    {
        userJson["msg"] = "no such user";
    }
    else if (-2 == ret)
    {
        userJson["msg"] = "no session";
    }
    else if (-3 == ret)
    {
        userJson["msg"] = "invalid service";
    }
    else if (-4 == ret)
    {
        userJson["msg"] = "user not in service";
    }
    else
    {
        userJson["msg"] = "unkown error";
    }
    
    data["checkInfo"].append(userJson);
}

int DebugUserTimer::on_delete_user(Json::Value &data)
{
    //不判断user的存在性，强制删除
    DeleteUserDeep(m_userID);
    return 0;
}

int DebugUserTimer::on_debug_user()
{
    Json::Value data = Json::objectValue;

    set<string>::iterator it;
    for (it = m_userID_list.begin(); it != m_userID_list.end(); ++it)
    {
        m_userID = (*it);
        m_raw_userID = delappID(m_userID);
        
        //get
        if ("get" == m_debug_op)
        {
            on_get_user(data);
        }

        //check
        if ("check" == m_debug_op)
        {
            on_check_fix_user(data, false);
        }

        //fix
        if ("fix" == m_debug_op)
        {
            on_check_fix_user(data, true);
        }
        
        //delete
        if ("delete" == m_debug_op)
        {
            on_delete_user(data);
        }
    }
    
    return on_send_reply(data);
}

DebugUserTimer::~DebugUserTimer()
{
}

