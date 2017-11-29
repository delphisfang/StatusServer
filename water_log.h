#ifndef __WATER_LOG_H__
#define __WATER_LOG_H__

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <sys/time.h>
#include "tfc_debug_log.h"


using namespace std;

class CWaterLog
{
public:
	static CWaterLog* Instance();
	static void Destance ();
	int  Init (const string &path, const string &file_prex, unsigned max_size, unsigned max_num);
	void WriteLog(timeval& op_time, int op, char* ip, unsigned port, int ret, char* data);

private:
	CWaterLog ();
	~CWaterLog ();

private:
	static CWaterLog* m_instance;

  	TFCDebugLog m_log;
};



#endif
