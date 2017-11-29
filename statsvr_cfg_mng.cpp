#include "statsvr_cfg_mng.h"
#include "tfc_base_config_file.h"
#include "tfc_base_config.h"
#include "tfc_base_str.h"
#include "tfc_debug_log.h"
#include "common_api.h"

using namespace statsvr;

CStatSvrCfgMng::CStatSvrCfgMng()
{
	// debug log default config
    _log_para.log_level_     = DEF_LOG_LEVEL;
    _log_para.log_type_      = DEF_LOG_TYPE;
    _log_para.path_          = DEF_LOG_PATH;
    _log_para.name_prefix_   = DEF_LOG_PREFIX;
    _log_para.max_file_size_ = DEF_FILE_SIZE;
    _log_para.max_file_no_   = DEF_MIN_LOG_FILE_NUM;

	// stat log default config
    _stat_log_para.log_level_     = DEF_LOG_LEVEL;
    _stat_log_para.log_type_      = DEF_LOG_TYPE;
    _stat_log_para.path_          = DEF_LOG_PATH;
    _stat_log_para.name_prefix_   = DEF_STAT_PREFIX;
    _stat_log_para.max_file_size_ = DEF_FILE_SIZE;
    _stat_log_para.max_file_no_   = DEF_MIN_LOG_FILE_NUM;
    _stat_gap                     = DEF_STAT_GAP;

	// water log default config
    _water_log.log_level_     = DEF_LOG_LEVEL;
    _water_log.log_type_      = DEF_LOG_TYPE;
    _water_log.path_          = DEF_LOG_PATH;
    _water_log.name_prefix_   = DEF_LOG_PREFIX;
    _water_log.max_file_size_ = DEF_FILE_SIZE;
    _water_log.max_file_no_   = DEF_MIN_LOG_FILE_NUM;
}

CStatSvrCfgMng::~CStatSvrCfgMng()
{

}

/******************************** private functions ********************************/

int32_t CStatSvrCfgMng::LoadCacheConfig(CFileConfig& page)
{
	m_conf_cache_size = GetDefault(page, "root\\config_cache\\cache_size", 10);
	m_conf_shmkey     = GetDefault(page, "root\\config_cache\\shmkey", 612500);
	m_node_num        = GetDefault(page, "root\\config_cache\\node_num", 10000);
	m_block_size      = GetDefault(page, "root\\config_cache\\block_size", 100);
	m_read_only       = GetDefault(page, "root\\config_cache\\read_only", 1);
	
	return 0;
}

int CStatSvrCfgMng::loadConfig()
{	
    CFileConfig page;
    page.Init(_config_path);

    // import debug log config
    _log_para.log_level_     = tfc::base::from_str<int>(page["root\\log\\log_level"]);
    _log_para.log_type_      = tfc::base::from_str<int>(page["root\\log\\log_type"]);
    _log_para.path_          = page["root\\log\\path"];
    _log_para.name_prefix_   = page["root\\log\\name_prefix"];
    _log_para.max_file_size_ = tfc::base::from_str<int>(page["root\\log\\max_file_size"]);
    _log_para.max_file_no_   = tfc::base::from_str<int>(page["root\\log\\max_file_no"]);

    // import stat log config
    /*_stat_log_para.log_level_ = tfc::base::from_str<int>(page["root\\stat\\log_level"]);
    _stat_log_para.log_type_    = tfc::base::from_str<int>(page["root\\stat\\log_type"]);*/
    _stat_log_para.path_          = page["root\\stat\\path"];
    _stat_log_para.name_prefix_   = page["root\\stat\\name_prefix"];
    _stat_log_para.max_file_size_ = tfc::base::from_str<int>(page["root\\stat\\max_file_size"]);
    _stat_log_para.max_file_no_   = tfc::base::from_str<int>(page["root\\stat\\max_file_no"]);
    _stat_gap                     = tfc::base::from_str<int>(page["root\\stat\\stat_gap"]);
    _stat_timeout_1               = tfc::base::from_str<int>(page["root\\stat\\time_out_1"]);
    _stat_timeout_2               = tfc::base::from_str<int>(page["root\\stat\\time_out_2"]);
    _stat_timeout_3               = tfc::base::from_str<int>(page["root\\stat\\time_out_3"]);

    // import water log config
    _water_log.path_              = page["root\\water_log\\path"];
    _water_log.name_prefix_       = page["root\\water_log\\name_prefix"];

    _time_out = GetDefault(page, "root\\search_time_out", 5000);

    _config_domin = GetDefault(page, "root\\config_domin", "127.0.0.1");
    struct hostent *hptr;
    if ((hptr = gethostbyname(_config_domin.c_str())) == NULL)
    {
        _config_ip = "127.0.0.1";
    }
    else
    {
        _config_ip.assign(inet_ntoa(*((struct in_addr *)hptr->h_addr)));
    }
    _config_port = GetDefault(page, "root\\config_port", 80);

    _err_push_ip = GetDefault(page, "root\\err_push_ip", "127.0.0.1");
    if (_err_push_ip == "127.0.0.1")
    {
        _err_push_domin = GetDefault(page, "root\\err_push_domin", "127.0.0.1");
        if((hptr = gethostbyname(_err_push_domin.c_str())) != NULL)
        {
            _err_push_ip.assign(inet_ntoa(*((struct in_addr *)hptr->h_addr)));
        }
    }
    _err_push_port = GetDefault(page, "root\\err_push_port", 80);
    _env           = GetDefault(page, "root\\env", "test");
    _local_ip      = GetDefault(page, "root\\local_ip", "127.0.0.1");

    _service_time_gap = GetDefault(page, "root\\service_time_gap", 900); //second

    _queue_rate = GetDefault(page, "root\\operation\\queue_rate", 3);
    _yibot_time = GetDefault(page, "root\\operation\\yibot_time", 480); //min

	// 重发
	_trsf_max_retry_times = GetDefault(page, "root\\policy\\max_retry_times", 2);
	_state_server_ip      = GetDefault(page, "root\\policy\\state_server_ip", "127.0.0.1");
	_state_server_port    = GetDefault(page, "root\\policy\\state_server_port", 0);
	
    LoadCacheConfig(page);
    return 0;
}

int CStatSvrCfgMng::GetDefault(CFileConfig& page, const string& key, int defval)
{
	try
	{
		return tfc::base::from_str<int>(page[key]);
	}
	catch (...)
	{
		return defval;
	}
}

string CStatSvrCfgMng::GetDefault(CFileConfig& page, const string& key, const string& defval)
{
	try
	{
		return page[key];
	}
	catch (...)
	{
		return defval;
	}
}

/******************************** public functions ********************************/

int CStatSvrCfgMng::LoadCfg(const string& cfg_path)
{
	_config_path = cfg_path;
	return loadConfig();
}

int CStatSvrCfgMng::Reload()
{
	return loadConfig();
}

string CStatSvrCfgMng::ToString()
{
	string ret = "\n-----------------config begin-------------------\n";	
	char buf[64];

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "log_level:%d\n", _log_para.log_level_);
	ret += string(buf);

	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf), "timeout:%u\n", _time_out);
	ret += string(buf);
	
	ret += "-----------------config end---------------------\n";
	return ret;
}

