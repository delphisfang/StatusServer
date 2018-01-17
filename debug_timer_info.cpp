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

int DebugUserTimer::on_get_userlist(Json::Value &data)
{
    Json::Value userIDList;
    userIDList.resize(0);
    CAppConfig::Instance()->getUserIDListJson(m_appID, userIDList);
    data["userIDList"] = userIDList;
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
    switch (ret)
    {
        case 0:
            userJson["msg"] = "OK";
            break;
        case -1:
            userJson["msg"] = "no such user";
            break;
        case -2:
            userJson["msg"] = "no session";
            break;
        case -3:
            userJson["msg"] = "invalid service";
            break;
        case -4:
            userJson["msg"] = "user not in service";
            break;
        default:
            userJson["msg"] = "unkown error";
            break;
    }
    
    data["checkInfo"].append(userJson);
    return 0;
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

    //get userid list
    if ("getlist" == m_debug_op)
    {
        on_get_userlist(data);
        return on_send_reply(data);
    }

    set<string>::iterator it;
    for (it = m_userID_list.begin(); it != m_userID_list.end(); ++it)
    {
        m_userID = (*it);
        m_raw_userID = delappID(m_userID);

        //get
        if ("get" == m_debug_op)
        {
            on_get_user(data);
            continue;
        }

        //check
        if ("check" == m_debug_op)
        {
            on_check_fix_user(data, false);
            continue;
        }

        //fix
        if ("fix" == m_debug_op)
        {
            on_check_fix_user(data, true);
            continue;
        }
        
        //delete
        if ("delete" == m_debug_op)
        {
            on_delete_user(data);
            continue;
        }
    }
    
    return on_send_reply(data);
}

DebugUserTimer::~DebugUserTimer()
{
}



int DebugServiceTimer::do_next_step(string& req_data)
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

    if (on_debug_service())
    {
        return -1;
    }
    else
    {
        return 1;
    }
}

int DebugServiceTimer::on_get_servicelist(Json::Value &data)
{
    Json::Value servIDList;
    servIDList.resize(0);
    CAppConfig::Instance()->getServiceIDListJson(m_appID, servIDList);
    data["serviceIDList"] = servIDList;
}

int DebugServiceTimer::on_get_service(Json::Value &data)
{
    if (0 == mGetService(m_serviceID, m_serviceInfo))
    {
        Json::Value servJson;
        m_serviceInfo.toJson(servJson);
        data["serviceInfo"].append(servJson);
    }
    else
    {
        data["serviceInfo"].append(Json::objectValue);
    }
    return 0;
}

int DebugServiceTimer::on_check_fix_service(Json::Value &data, bool fix)
{
    Json::Value servJson = Json::objectValue;
    servJson["serviceID"] = m_raw_serviceID; 
    
    int ret = CheckFixService(m_serviceID, fix);
    switch (ret)
    {
        case 0:
            servJson["msg"] = "OK";
            break;
        case -1:
            servJson["msg"] = "no such service";
            break;
        case -2:
            servJson["msg"] = "invalid user in service";
            break;
        case -3:
            servJson["msg"] = "invalid subStatus";
            break;
        default:
            servJson["msg"] = "unkown error";
            break;
    }
    
    data["checkInfo"].append(servJson);
    return 0;
}

int DebugServiceTimer::on_delete_service(Json::Value &data)
{
    //不判断user的存在性，强制删除
    DeleteServiceDeep(m_serviceID);
    return 0;
}

int DebugServiceTimer::on_debug_service()
{
    Json::Value data = Json::objectValue;

    //get servid list
    if ("getlist" == m_debug_op)
    {
        on_get_servicelist(data);
        return on_send_reply(data);
    }
    
    set<string>::iterator it;
    for (it = m_serviceID_list.begin(); it != m_serviceID_list.end(); ++it)
    {
        m_serviceID = (*it);
        m_raw_serviceID = delappID(m_serviceID);
        
        //get
        if ("get" == m_debug_op)
        {
            on_get_service(data);
            continue;
        }

        //check
        if ("check" == m_debug_op)
        {
            on_check_fix_service(data, false);
            continue;
        }

        //fix
        if ("fix" == m_debug_op)
        {
            on_check_fix_service(data, true);
            continue;
        }
        
        //delete
        if ("delete" == m_debug_op)
        {
            on_delete_service(data);
            continue;
        }
    }
    
    return on_send_reply(data);
}

DebugServiceTimer::~DebugServiceTimer()
{
}

