#include "water_log.h"
#include "string.h"
#include <iostream>
#include "common_api.h"

using namespace std;

//class member
CWaterLog* CWaterLog::m_instance = NULL;

CWaterLog* CWaterLog::Instance()
{
    if (NULL == m_instance)
    {
        m_instance = new CWaterLog();
    }
    return m_instance;
}

void CWaterLog::Destance()
{
    if (NULL != m_instance)
    {
        delete m_instance;
    }
    m_instance = NULL;
}

CWaterLog::CWaterLog()
{
}

CWaterLog::~CWaterLog()
{
}

void CWaterLog::WriteLog(timeval& op_time, int op, const char *ip, unsigned port, int ret, const char *data)
{
    string opString;
    if (0 == op)
    {
        opString = string("Recv");
    }
    else if (1 == op)
    {
        opString = string("Send");
    }
    else
    {
        opString = string("What");
    }
    
    #if 0
    m_log.log_p(0, "[%s] | OP:%s | IP:%s | Port:%u | ret:%d | data:%s\n"
              , GetFormatTime(op_time).c_str(), opString.c_str(), ip, port, ret, data);
    #else
    m_log.log_p(0, "%s | %s | %u | %d | %s", opString.c_str(), ip, port, ret, (char*)data);
    #endif
}

int CWaterLog::Init(const string &path, const string &file_prex, unsigned max_size, unsigned max_num)
{
    int log_level = 0;
    int log_type = LOG_TYPE_DAILY;
    return m_log.open(log_level, log_type, (char*)path.c_str(), (char*)file_prex.c_str()
             , max_size, max_num);
}

