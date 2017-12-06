#ifndef STATSVR_CONFIG_MNG_H_
#define STATSVR_CONFIG_MNG_H_

#include <vector>
#include "tfc_comm.h"
#include "tfc_base_config_file.h"
#include "tfc_debug_log.h"
#include "tfc_base_str.h"

using namespace tfc;
using namespace tfc::base;

namespace statsvr
{

const char * const DEF_LOG_PATH = "../log";
const char * const DEF_LOG_PREFIX = "STATSVR";
const char * const DEF_STAT_PREFIX = "STATSVR_stat";

enum DEFAULT_CFG
{
    DEF_LOG_LEVEL = LOG_NORMAL,
    DEF_LOG_TYPE = LOG_TYPE_CYCLE,
    DEF_MIN_LOG_FILE_NUM = 4,
    DEF_FILE_SIZE = DEFAULT_MAX_FILE_SIZE,
    DEF_STAT_GAP = 300, //5min
    DEF_UPDATE_DB_GAP = 3600,
};

class CStatSvrCfgMng
{
public:
    CStatSvrCfgMng();
    virtual ~CStatSvrCfgMng();
    
    int  LoadCfg(const string& cfg_path);
    int  Reload();
    string ToString();

    TLogPara  _log_para;
    TLogPara  _water_log;
    TLogPara  _feedback_log;
    TLogPara  _stat_log_para;    

    int       _stat_gap;
    int       _stat_timeout_1;
    int       _stat_timeout_2;
    int       _stat_timeout_3;

	/* fff
    int       _re_msg_send_gap;
    int       _re_msg_send_timeout;
    */
    
    int32_t m_conf_cache_size;//MB
	int32_t m_conf_shmkey;
	int32_t m_node_num;
	int32_t m_block_size;
	int32_t m_read_only;

    unsigned  _time_out;

    string    _config_domin;
    string    _config_ip;
    unsigned  _config_port;

    string    _err_push_domin;
    string    _err_push_ip;
    unsigned  _err_push_port;
    string    _env;
    string    _local_ip;
	unsigned  _local_port;
	
    unsigned  _queue_rate;
    unsigned  _yibot_time;
    time_t    _service_time_gap;

	string    _handler_so_path;

	unsigned  _trsf_max_retry_times;
	string    _state_server_ip;
	unsigned short _state_server_port;

	// add end
private:
    int loadConfig();
    int32_t LoadCacheConfig(CFileConfig& page);
	int GetDefault(CFileConfig& page, const string& key, int defval);
	string GetDefault(CFileConfig& page, const string& key, const string& defval);
	
    std::string _config_path;
};

}

#endif /* STATSVR_CONFIG_MNG_H_ */


