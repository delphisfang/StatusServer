#include "water_log.h"
#include "string.h"
#include <iostream>
#include "common_api.h"
//#include "sagitdef.h"

using namespace std;

const unsigned max_buf_length = 2048;
char g_wr_buf[max_buf_length] = {0};

CWaterLog* CWaterLog::m_instance = NULL;


CWaterLog* CWaterLog::Instance()
{
	if (NULL ==  m_instance)
	{
		m_instance = new CWaterLog();
	}
	return m_instance;
}

void CWaterLog::Destance()
{
	if (NULL !=  m_instance)
	{
		delete m_instance;
	}
	m_instance = NULL;
}

CWaterLog::CWaterLog ()
{
}


CWaterLog::~CWaterLog ()
{
}

void CWaterLog::WriteLog(timeval& op_time, int op, char* ip, unsigned port, int ret, char* data)
{
	string opString;
	if(op == 0)
	{
		opString = string("Recv");
	}
	else if(op == 1)
	{
		opString = string("Send");
	}
	else
	{
		opString = string("What");
	}
	m_log.log_p(0,	"[%s] | OP:%s | IP:%s | Port:%u | ret:%d | data:%s\n"
			  , GetFormatTime(op_time).c_str(), opString.c_str(), ip, port, ret, data);
}


int CWaterLog::Init (const string &path, const string &file_prex, unsigned max_size, unsigned max_num)
{
	int log_level = 0;
	int log_type = LOG_TYPE_DAILY;
	return m_log.open(log_level, log_type, (char*)path.c_str(), (char*)file_prex.c_str()
			 , max_size, max_num);
}

