/* OS headers */
#include <sys/file.h>
#include <fstream>
#include <netdb.h>

/* TFC headers */
#include "tfc_object.h"
#include "tfc_base_fast_timer.h"
#include "tfc_cache_proc.h"
#include "tfc_net_ipc_mq.h"
#include "tfc_net_ccd_define.h"
#include "tfc_net_dcc_define.h"
#include "tfc_debug_log.h"

/* module headers */
#include "debug.h"
#include "statsvr_mcd_proc.h"
#include "statsvr_error.h"
#include "water_log.h"

#include "statsvr_timer_info.h"
#include "admin_timer_info.h"
#include "user_timer_info.h"
#include "service_timer_info.h"
#include "system_timer_info.h"
#include "txf_timer_info.h"

#include "statsvr_kv.h"

using namespace std;
using namespace statsvr;

#define HTTP_PACKET_MAX_LEN (20*1024)

#define BUFF_SIZE (2*1024*1024)

#define ARG_CNT_MAX (32)

static char r_code_200[]   = "200";
static char r_reason_200[] = "OK";

//char BUF[BUFF_SIZE] = {0};

extern "C"
{
    static void disp_ccd(void *papp)
    {
        CMCDProc *app = (CMCDProc*)papp;
        app->DispatchCCD();
    }
    static void disp_dcc(void *papp)
    {
        CMCDProc *app = (CMCDProc*)papp;
        app->DispatchDCC();
    }
    #if 0
    static void disp_dcc_http(void *papp)
    {
        CMCDProc *app = (CMCDProc*)papp;
        app->DispatchDCCHttp();
    }
    #endif
}

void CMCDProc::run(const std::string& conf_file)
{
    LogDebug("#####################################################################\n");
    const char *version = "statsvr@version"
                           MAJOR_VERSION
                           "."
                           MIDDLE_VERSION
                           "."
                           MINOR_VERSION
                           " BUILD DATE:["
                           __DATE__
                           "] BUILD TIME:["
                           __TIME__
                           "]";

    LogDebug("%s\n", version);
    LogDebug("#####################################################################\n");

    if (Init(conf_file) < 0)
    {
        return;
    }

    LogDebug("[%s] server started.....\n", MODULE_NAME);

    GetConfigForIM();

    while (!obj_checkflag.IsStop())
    {
        DispatchUser2Service();
        DispatchSessionTimer();
        //DispatchUserTimeout();
        DispatchServiceTimeout();
        //CAppConfig::Instance()->CheckServiceList();
        run_epoll_4_mq();
        CheckFlag(true);
    }

    LogDebug("[%s] server stopped.....\n", MODULE_NAME);
}

int32_t CMCDProc::Init(const std::string& conf_file)
{
    if (m_cfg.LoadCfg(conf_file) < 0)
    {
        LogError("Failed to LoadCfg()!");
        goto err_out;
    }

    if (InitBuffer() < 0)
    {
        LogError("Failed to InitBuffer()!");
        goto err_out;
    }

    if (InitLog() < 0)
    {
        LogError("Failed to InitLog()!");
        goto err_out;
    }
    
    if (InitStat() < 0)
    {
        LogError("Failed to InitStat()!");
        goto err_out;
    }
    
    if (InitIpc() < 0)
    {
        LogError("Failed to InitIpc()!");
        goto err_out;
    }

    if (InitTemplate())
    {
        LogError("Failed to build http response template!");
        goto err_out;
    }

    if (InitCmdMap())
    {
        LogError("Failed to InitCmdMap()!");
        goto err_out;
    }

    if (InitKV())
    {
        LogError("Failed to InitKV()!");
        goto err_out;
    }
    
    srand((int)time(0));
    m_msg_seq = rand();

    signal(SIGUSR1, sigusr1_handle);
    signal(SIGUSR2, sigusr2_handle);
    return 0;
    
err_out:
    return -1;
}

int32_t CMCDProc::ReloadCfg()
{
    if (m_cfg.Reload() < 0)
    {
        LogError("Failed to ReloadCfg::Reload()!");
        goto err_out;
    }

    if (InitLog())
    {
        LogError("Failed to ReloadCfg::InitLog()!");
        goto err_out;
    }   

    if (InitStat())
    {
        LogError("Failed to ReloadCfg::InitStat()!");
        goto err_out;
    }

    return 0;

err_out:
    return -1;
}

int32_t CMCDProc::InitBuffer()
{
    if (NULL == m_recv_buf)
    {
        m_recv_buf = new char[BUFF_SIZE];
        if (NULL == m_recv_buf)
        {
            return -1;
        }
    }

    if (NULL == m_send_buf)
    {
        m_send_buf = new char[BUFF_SIZE];
        if (NULL == m_send_buf)
        {
            return -1;
        }
    }

    return 0;
}

int32_t CMCDProc::InitLog()
{
    TLogPara *log_para = &(m_cfg._log_para);

    int32_t ret = DEBUG_OPEN(log_para->log_level_, log_para->log_type_,
        log_para->path_, log_para->name_prefix_,
        log_para->max_file_size_, log_para->max_file_no_);

    if (ret < 0)
        return ret;

    log_para = &(m_cfg._water_log);
    ret = CWaterLog::Instance()->Init(log_para->path_, log_para->name_prefix_
                                    , log_para->max_file_size_, log_para->max_file_no_);

    return ret;
}

int32_t CMCDProc::InitStat()
{
    TLogPara* stat_para = &(m_cfg._stat_log_para);

    string stat_file = stat_para->path_ + stat_para->name_prefix_;

    LogTrace("stat_file: %s", stat_file.c_str());
    int32_t ret = m_stat.Inittialize((char*)stat_file.c_str()
                                   , stat_para->max_file_size_
                                   , stat_para->max_file_no_
                                   , m_cfg._stat_timeout_1
                                   , m_cfg._stat_timeout_2
                                   , m_cfg._stat_timeout_3);

    return ret;
}

int32_t CMCDProc::InitIpc()
{
    m_mq_ccd_2_mcd = _mqs["mq_ccd_2_mcd"];
    m_mq_mcd_2_ccd = _mqs["mq_mcd_2_ccd"];
    m_mq_dcc_2_mcd = _mqs["mq_dcc_2_mcd"];
    m_mq_mcd_2_dcc = _mqs["mq_mcd_2_dcc"];
    /*m_mq_dcc_2_mcd_http = _mqs["mq_dcc_2_mcd_http"];
    m_mq_mcd_2_dcc_http = _mqs["mq_mcd_2_dcc_http"];*/

    assert(m_mq_ccd_2_mcd != NULL);
    assert(m_mq_mcd_2_ccd != NULL);
    assert(m_mq_dcc_2_mcd != NULL);
    assert(m_mq_mcd_2_dcc != NULL);
    /*assert(m_mq_dcc_2_mcd_http != NULL);
    assert(m_mq_mcd_2_dcc_http != NULL);*/

    if (add_mq_2_epoll(m_mq_ccd_2_mcd, disp_ccd, this))
    {
        LogErrPrint("Failed to add <mq_ccd_2_mcd> to EPOLL!");
        err_exit();
    }

    if (add_mq_2_epoll(m_mq_dcc_2_mcd, disp_dcc, this))
    {
        LogErrPrint("Failed to add <mq_dcc_2_mcd> to EPOLL!");
        err_exit();
    }

    /*if (add_mq_2_epoll(m_mq_dcc_2_mcd_http, disp_dcc_http, this))
    {
        LogErrPrint("Add mq_dcc_2_mcd to EPOLL fail!");
        err_exit();
    }*/
    
    return 0;
}

int CMCDProc::InitTemplate()
{
    // init http args begin
    m_arg_cnt = 3;
    m_arg_vals = new char*[m_arg_cnt];
    if (!m_arg_vals)
    {
        LogErrPrint("Alloc memory for HTTP template arg values fail!");
        return -1;
    }
    m_arg_vals[2] = m_r_clength;
    // init http args end
    
    char http_head[HTTP_HEAD_MAX];
    char *data = http_head;
    char *head = NULL;
    http_template_arg_t args[ARG_CNT_MAX];
    int head_len;

    data += sprintf(data, "HTTP/1.1 ");
    args[0].offset = data - http_head;
    head = data;
    data += sprintf(data, "    ");
    args[0].max_length = data - head - 1;

    args[1].offset = data - http_head;
    head = data;
    data += sprintf(data, "           \r\n");
    args[1].max_length = data - head - 2;

    data += sprintf(data, "Server: MCP-Simple-HTTP\r\n");
    data += sprintf(data, "Content-Length: ");
    args[2].offset = data - http_head;
    head = data;
    data += sprintf(data, "                                \r\n");
    args[2].max_length = data - head - 2;

    data += sprintf(data, "Cache-Control: no-cache\r\n");
    data += sprintf(data, "Connection: Keep-Alive\r\n");
    data += sprintf(data, "\r\n");
    head_len = data - http_head;

    return m_http_template.Init(http_head, head_len, args, 3);
}

int32_t CMCDProc::InitCmdMap()
{
    m_cmdMap["echo"]                = ECHO;
    m_cmdMap["getUserInfo"]         = GET_USER_INFO;
    m_cmdMap["userOnline"]          = USER_ONLINE;
    m_cmdMap["cancelQueue"]         = CANCEL_QUEUE;
    m_cmdMap["closeSession"]        = CLOSE_SESSION;
    m_cmdMap["connectService"]      = CONNECT_SERVICE;
    
    m_cmdMap["getServiceInfo"]      = GET_SERVICE_INFO;
    m_cmdMap["serviceLogin"]        = SERVICE_LOGIN;
    m_cmdMap["serviceChangeStatus"] = SERVICE_CHANGESTATUS;
    m_cmdMap["changeService"]       = CHANGE_SERVICE;
    m_cmdMap["overload"]            = SERVICE_PULLNEXT;
    m_cmdMap["refreshSession"]      = REFRESH_SESSION;

    m_cmdMap["getAddrByID"] = GET_CP_ADDR;
    
    return 0;
}


int32_t CMCDProc::InitKV()
{
    int ret = SS_ERROR;
    int init_type;
    
    ret = kv_cache.Init(m_cfg.m_conf_cache_size, m_cfg.m_conf_shmkey,
                        m_cfg.m_node_num, m_cfg.m_block_size, 0, &init_type);

    if (ret)
    {
        LogError("Failed to init shared memory! size:[%d], key[%d].", 
                    m_cfg.m_conf_cache_size,  m_cfg.m_conf_shmkey);
        return ret;
    }
    else
    {
        LogTrace("Success to init shared memory. size:[%d], key[%d].", 
                    m_cfg.m_conf_cache_size,  m_cfg.m_conf_shmkey);
        return SS_OK;
    }
}


void CMCDProc::DispatchCCD()
{
    int32_t ret = 0;
    int32_t deal_count = 0;
    unsigned data_len = 0;
    unsigned long long flow = 0;
    TCCDHeader* ccdheader = (TCCDHeader*)m_recv_buf;
    timeval ccd_time;
    
    while (deal_count < 1000)
    {
        data_len = 0;
        ret = m_mq_ccd_2_mcd->try_dequeue(m_recv_buf, BUFF_SIZE, data_len, flow);

        if (ret || data_len < CCD_HEADER_LEN)
        {
            ++deal_count;
            continue;
        }

        //LogDebug("CCD receive a packet");

        uint32_t client_ip     = ccdheader->_ip;
        ccd_time.tv_sec     = ccdheader->_timestamp;
        ccd_time.tv_usec     = ccdheader->_timestamp_msec * 1000;

        if (ccd_rsp_data != ccdheader->_type)
        {
            LogError("Invalid ccdheader->_type! expect: %d, actual: %d, client_ip: %s.",
                    ccd_rsp_data, ccdheader->_type, INET_ntoa(client_ip).c_str());
            ++deal_count;
            continue;
        }

        HandleRequest(m_recv_buf + CCD_HEADER_LEN,
                            data_len - CCD_HEADER_LEN,
                            flow,
                            client_ip,
                            ccd_time);
        ++deal_count;
    }
}

int32_t CMCDProc::HttpGetBu(const char *uri_buf, string &bu_name, string &param_str)
{
    if (uri_buf == NULL)
        return -1;

    string http_url(uri_buf);
    string::size_type start = 1, qm_idx, slash_idx;

    if (':' == http_url[0])
    {
        start = http_url.find_first_of("/", 0);
        if (start == string::npos)
        {
            return -1;
        }
    }  

    slash_idx = http_url.find_first_of("/", start+1);
    bu_name = http_url.substr(start, slash_idx - start);

    qm_idx = http_url.find_first_of("?", start+1);
    if (qm_idx == string::npos)
    {
        param_str = "";
    }else{
        param_str = http_url.substr (qm_idx + 1, http_url.size() - qm_idx - 1);
    }

    return 0;
}

int32_t CMCDProc::HttpParseCmd(const char *data, unsigned data_len, string& outdata, unsigned& out_len)
{
    if (data_len < 5)
    {
        LogError("http data too small: %s", data);
        return -1;
    }

    CHttpParse http_parse;
    int ret = http_parse.Init((void*)((const void*)data), data_len);
    if (ret)
    {
        LogError("Failed to parse http data: %s!", data);
        return -1;
    }

    string request_url = string(http_parse.HttpUri());
    
    unsigned long pos = request_url.find("?");
    if (pos != string::npos)
    {
        string path = request_url.substr (0, pos);
        if (path.find("../") != string::npos)
        {
            LogError( "path has ../: %s", request_url.c_str());
            return -1;
        }
    }

    string m_business_name;
    string req_params;
    HttpGetBu(request_url.c_str(), m_business_name, req_params);
    int m_http_method = http_parse.HttpMethod();
    string identity = "";
    string cmd;
    if (HTTP_POST == m_http_method)
    {
        int body_len = http_parse.BodyLength();
        req_params.assign(data + data_len - body_len, body_len);
        outdata = req_params;
        out_len = req_params.size();

        Json::Reader reader;
        Json::Value root;
        Json::Value rootdata;
        //LogDebug("==>parse root");
        if (!reader.parse(req_params, root))
        {
            LogError("Failed to parse params: %s!", req_params.c_str());
            return -1;
        }
       
        //LogDebug("==>parse seq");
        if (!root["seq"].isNull())
        {
            if (root["seq"].isUInt())
            {
                unsigned int seq_num = root["seq"].asUInt();
                m_seq = ui2str(seq_num);
            }
            else
            {
                m_seq = root["seq"].asString();
            }
        }

        //LogDebug("==>parse method");
        if (!root["method"].isNull())
        {
            cmd  = root["method"].asString();
        }
        else
        {
            cmd  = root["cmd"].asString();
        }
        LogWarn("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
        LogWarn("==>method: %s", cmd.c_str());
        
        if (!root["data"].isNull())
        {
             rootdata = root["data"];
            if (!rootdata["identity"].isNull())
            {
                identity = rootdata["identity"].asString();
            }
        }
        //LogDebug("===>parse data finish");
    }
    else
    {
        return -1;
    }

    //LogDebug("==>identity: %s", identity.c_str());

    if (identity == "admin")
    {
        return 0;
    }
    else if (statsvr::WORKMODE_READY == m_workMode)
    {
        LogError("====> [%s] has not been working yet, m_workMode :%d", MODULE_NAME, m_workMode);
        return -2;
    }
    else
    {
        return m_cmdMap[cmd];
    }
}


int32_t CMCDProc::HandleRequest(const char *data, unsigned data_len, 
                                unsigned long long flow, uint32_t client_ip, timeval& ccd_time)
{    
    string str_client_ip;
    int cmd;
    string outdata;
    unsigned out_len = 0;

    str_client_ip = INET_ntoa(client_ip);
    cmd = HttpParseCmd(data, data_len, outdata, out_len);
    CWaterLog::Instance()->WriteLog(ccd_time, 0, (char *)str_client_ip.c_str(), 0, cmd, (char *)outdata.c_str());

    CTimerInfo* ti = NULL;
    unsigned msg_seq = GetMsgSeq();
    switch (cmd)
    {
        case ECHO:
            ti = new EchoTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;
        
        case ADMIN_CONFIG:
            ti = new AdminConfigTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case GET_USER_INFO:
            ti = new GetUserInfoTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case USER_ONLINE:
            ti = new UserOnlineTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case CANCEL_QUEUE:
            ti = new CancelQueueTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case CLOSE_SESSION:
            ti = new CloseSessionTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case CONNECT_SERVICE:
            ti = new ConnectServiceTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;
        
        case GET_SERVICE_INFO:
            ti = new GetServiceInfoTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case SERVICE_LOGIN:
            ti = new ServiceLoginTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case SERVICE_CHANGESTATUS:
            ti = new ServiceChangeStatusTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case CHANGE_SERVICE:
            ti = new ChangeServiceTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case SERVICE_PULLNEXT:
            ti = new ServicePullNextTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;
        
        case REFRESH_SESSION:
            ti = new RefreshSessionTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;

        case GET_CP_ADDR:
            ti = new TransferTimer(this, msg_seq, ccd_time, str_client_ip, flow, m_cfg._time_out);
            break;
            
        default:
            LogError("Unknown cmd[%d] from IP[%s]!", cmd, str_client_ip.c_str());
            Json::Value resp;
            resp["method"] = "-reply";
            if (cmd == -2)
            {
                resp["code"] = -61001;
                resp["msg"]  = "System not ready";
            }
            else
            {
                resp["code"] = -61003;
                resp["msg"]  = "Unknown command";
            }

            Json::FastWriter writer;
            string strRsp = writer.write(resp);
            EnququeHttp2CCD(flow, (char *)strRsp.c_str(), strRsp.size());
            return -1;
    }

    if (ti != NULL)
    {
        if (ti->do_next_step(outdata) == 0)
        {
            m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
        }
        else
        {
            delete ti;
        }
    }
    
    return 0;
}

void CMCDProc::DispatchDCC()
{
    int ret = 0, deal_count = 0;
    unsigned data_len = 0;
    unsigned long long flow = 0;
    TDCCHeader *dccheader = (TDCCHeader*)m_recv_buf;
    timeval dcc_time;

    while (deal_count < 1000) 
    {
        data_len = 0;

        ret = m_mq_dcc_2_mcd->try_dequeue(m_recv_buf, BUFF_SIZE, data_len, flow);

        if (ret || data_len < DCC_HEADER_LEN)
        {
            deal_count++;
            continue;
        }

        uint32_t down_ip     = dccheader->_ip;
        unsigned down_port  = dccheader->_port;
        dcc_time.tv_sec     = dccheader->_timestamp;
        dcc_time.tv_usec     = dccheader->_timestamp_msec * 1000;

        if (dcc_rsp_data != dccheader->_type)
        {
            LogError("Invalid DCC header type! expect type: %d, actual type: %d, IP: %s.",
                    dcc_rsp_data, dccheader->_type, INET_ntoa(down_ip).c_str());
            deal_count++;
            continue;
        }

        ret = HandleResponse(m_recv_buf + DCC_HEADER_LEN, data_len - DCC_HEADER_LEN, flow, down_ip, down_port, dcc_time);
        if (ret < 0)
        {
            LogError("Failed to HandleResponse() from IP: %s, ret: %d!", INET_ntoa(down_ip).c_str(), ret);
        }

        deal_count++;
    }
}


int32_t CMCDProc::HttpParseResponse(const char *data, unsigned data_len, string& outdata, unsigned& out_len)
{
    if (data_len < 5)
    {
        LogError("http data too small: %s", data);
        return -1;
    }

    CHttpParse http_parse;
    int ret = http_parse.Init((void*)((const void*)data), data_len);
    if (ret)
    {
        LogError("Failed to parse http data: %s!", data);
        return -1;
    }

    int body_len = http_parse.BodyLength();
    string body;
    body.assign(data + data_len - body_len, body_len);

    outdata = body;
    out_len = body.size();
    return 0;
}


int32_t CMCDProc::HandleResponse(const char *data,
                                 unsigned data_len,
                                 unsigned long long flow,
                                 uint32_t down_ip, unsigned down_port, timeval& dcc_time)
{
    string outdata;
    unsigned out_len = 0;
    int ret = HttpParseResponse(data, data_len, outdata, out_len);
    if (ret < 0)
    {
        LogError("Failed to parse http response, ret: %d!", ret);
        return -1;
    }

    Json::Reader reader;
    Json::Value  root;
    if (!reader.parse(outdata, root))
    {
        LogError("Failed to parse data: %s!", outdata.c_str());
        return -1;
    }
    
    if (!root["innerSeq"].isNull() && root["innerSeq"].isUInt())
    {
        //don't handle inner reponse packet
        return 0;
    }

    #if 0
    uint32_t msg_seq = 0;
    if (!root["seq"].isNull() && root["seq"].isString())
    {
        msg_seq = atoi(root["seq"].asString().c_str());
    }
    else
    {
        LogError("Failed to parse seq!");
        return -1;
    }
    LogTrace("====>seq: %u", msg_seq);
    #endif

    timeval ccd_time = {0, 0};    
    CTimerInfo *ti   = new AdminConfigTimer(this, 0, ccd_time, "127.0.0.1", 0, m_cfg._time_out);

    Json::Value configList = root["data"];
    Json::Value req_data;
    req_data["method"] = "getConfigForIM-reply";
    req_data["data"]   = configList;

    Json::FastWriter writer;
    string strReq = writer.write(req_data);
    if (ti->do_next_step(strReq) == 0)
    {
        m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
    }
    else
    {
        delete ti;
    }
    
    #if 0
    CTimerInfo *ti = NULL;
    if (m_timer_queue.get(msg_seq, (CFastTimerInfo**)&ti))
    {
        LogError("Failed to m_timer_queue.get()! seq[%u], IP[%s], port[%u], dcc_time[%s]."
               , msg_seq, INET_ntoa(down_ip).c_str(), down_port, GetFormatTime(dcc_time).c_str());
        return -1;
    }

    if (ti->do_next_step(outdata) == 0)
    {
        m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
    }
    else
    {
        delete ti;
    }
    #endif

    return 0;
}

                                 
int32_t CMCDProc::EnququeHttp2CCD(unsigned long long flow, const char *data, unsigned data_len)
{
    m_arg_vals[0] = r_code_200;
    m_arg_vals[1] = r_reason_200;
    sprintf(m_r_clength, "%u", data_len);

    int   ret_len  = 0;
    char* head_msg = NULL;
    int   head_len = m_http_template.ProduceRef(&head_msg, &ret_len, m_arg_vals, m_arg_cnt);
    if (NULL == head_msg || head_len <= 0)
    {
        LogError("Failed to enqueue HTTP to CCD! flow:%lu, datalen:%u, data:%s", flow, data_len ,data);
        return -1;
    }

    TCCDHeader* header = (TCCDHeader*)m_send_buf;
    char* data_buff    = m_send_buf + CCD_HEADER_LEN;
    unsigned data_max  = BUFF_SIZE- CCD_HEADER_LEN;

    if (head_len + data_len > data_max)
    {
        LogError("head_len+data_len > data_max (%u+%d > %u)!", head_len, data_len, data_max);
        return -1;
    }

    memcpy(data_buff, head_msg, head_len);
    data_buff += head_len;
    memcpy(data_buff, data, data_len);

    header->_type = ccd_req_data;

    int totallen = CCD_HEADER_LEN + head_len + data_len;

    if (m_mq_mcd_2_ccd->enqueue(header, totallen, flow))
    {
        LogError("Failed to enqueue to CCD!");
        timeval nowTime;
        gettimeofday(&nowTime, NULL);
        CWaterLog::Instance()->WriteLog(nowTime, 1, (char *)"", 0, -1, data);
        return -1;
    }
    else
    {
        /*LogDebug("Success to enqueue to CCD, total_len:%d, ccd_header:%d.", totallen, CCD_HEADER_LEN);*/
        timeval nowTime;
        gettimeofday(&nowTime, NULL);
        CWaterLog::Instance()->WriteLog(nowTime, 1, (char *)"", 0, 0, data);
    }

    return 0;
}

int32_t CMCDProc::EnququeHttp2DCCInner(const char *http_header, const char *data, unsigned data_len, const string &domain, const string& ip, unsigned short port)
{
    char uri[HTTP_PACKET_MAX_LEN] = {0};
    snprintf(uri, HTTP_PACKET_MAX_LEN, http_header, domain.c_str(), port, data_len, data);
    unsigned msg_len   = strlen(uri);
    
    TDCCHeader* header = (TDCCHeader*)m_send_buf;
    char* data_buff    = m_send_buf + DCC_HEADER_LEN;
    unsigned data_max  = BUFF_SIZE- DCC_HEADER_LEN;

    if (msg_len > data_max)
    {
        LogError("msg_len > data_max (%u > %u)!", msg_len, data_max);
        return -1;
    }

    memcpy(data_buff, uri, msg_len);

    unsigned iip = INET_aton(ip.c_str());
    unsigned long long flow = make_flow64(iip, port);
    header->_type = dcc_req_send;
    header->_ip   = iip;
    header->_port = port;

    int totallen = DCC_HEADER_LEN + msg_len;
    if (m_mq_mcd_2_dcc->enqueue(header, totallen, flow))
    {
        LogError("Failed to enqueue to DCC!");
        timeval nowTime;
        gettimeofday(&nowTime, NULL);
        CWaterLog::Instance()->WriteLog(nowTime, 1, (char *)ip.c_str(), port, -1, (char*)data);
        return -1;
    }
    else
    {
        LogDebug("Success to enqueue to DCC, total_len:%d, dcc_header:%d.", totallen, DCC_HEADER_LEN);
        timeval nowTime;
        gettimeofday(&nowTime, NULL);
        CWaterLog::Instance()->WriteLog(nowTime, 1, (char *)ip.c_str(), port, 0, (char*)data);
        return 0;
    }
}


int32_t CMCDProc::InitSendPing()
{
    const char *http_header = ""
        "GET /api/v2/configs/ping?name=yiServer HTTP/1.1\r\n"
        "Host:%s:%u\r\n"
        "User-Agent:curl/7.45.0\r\n"
        "Content-Length:%u\r\n"
        "Content-Type:application/json\r\n"
        "Accept:*/*\r\n"
        "\r\n"
        "%s";

    DO_FAIL(EnququeHttp2DCCInner(http_header, "", 0, m_cfg._config_ip, m_cfg._config_ip, m_cfg._config_port));
    return 0;
}


int32_t CMCDProc::GetConfigForIM()
{
    //important: "Content-Type: application/json"
    const char *http_header = ""
        "POST /api/v2/configs/getConfigForIM HTTP/1.1\r\n"
        "Host:%s:%u\r\n"
        "User-Agent:curl/7.45.0\r\n"
        "Content-Length:%u\r\n"
        "Content-Type:application/json\r\n"
        "Accept:*/*\r\n"
        "\r\n"
        "%s";

    Json::Value data;
    data["identity"] = "";
    data["name"]     = "yiServer";
    
    Json::Value req;
    req["cmd"]       = "getConfigForIM";
    req["data"]      = data;
    unsigned msg_seq = GetMsgSeq();
    req["seq"]       = ui2str(msg_seq);

    Json::FastWriter writer;
    string reqStr = writer.write(req);

    DO_FAIL(EnququeHttp2DCCInner(http_header, reqStr.c_str(), reqStr.length(), m_cfg._config_domin, m_cfg._config_ip, m_cfg._config_port));
    return 0;
}


int32_t CMCDProc::EnququeHttp2DCC(const char *data, unsigned data_len, const string& ip, unsigned short port)
{
    const char *http_header = ""
            "POST /chatpass HTTP/1.1\r\n"
            "Host:%s:%u\r\n"
            "Content-Length:%u\r\n"
            "Content-Type:application/json\r\n"
            "User-Agent:curl/7.45.0\r\n"
            "Connection:Keep-Alive\r\n"
            "Accept:*/*\r\n"
            "\r\n"
            "%s";

    DO_FAIL(EnququeHttp2DCCInner(http_header, data, data_len, ip, ip, port));
    return 0;
}

int32_t CMCDProc::EnququeErrHttp2DCC(const char *data, unsigned data_len)
{
    const char *http_header = ""
        "POST /pushError/ HTTP/1.1\r\n"
        "Host:%s:%u\r\n"
        "Content-Type:application/json\r\n"
        "Content-Length:%u\r\n"
        "User-Agent:curl/7.45.0\r\n"
        "Accept:*/*\r\n"
        "\r\n"
        "%s";

    DO_FAIL(EnququeHttp2DCCInner(http_header, data, data_len, m_cfg._err_push_ip, m_cfg._err_push_ip, m_cfg._err_push_port));
    return 0;
}

#if 0
int32_t CMCDProc::PostErrLog(string &data, int type, unsigned appID, unsigned level)
{
    struct timeval cur_time;
    int64_t cur_time_msc;
    Json::Value post_err;
    string err_str;

    gettimeofday(&cur_time, NULL);
    cur_time_msc = cur_time.tv_sec*1000 + cur_time.tv_usec/1000;
    post_err["project"] = "statsvr";
    post_err["module"]  = "statsvr";
    post_err["code"]    = type;
    post_err["desc"]    = data;
    post_err["env"]     = m_cfg._env;
    post_err["ip"]      = m_cfg._local_ip;
    post_err["appid"]   = i2str(appID);
    post_err["time"]    = l2str(cur_time_msc);
    post_err["level"]   = level;
    err_str = post_err.toStyledString();

    LogDebug("[PostErrLog] start post error\n");

    return EnququeErrHttp2DCC((char *)err_str.c_str(), err_str.size());
}

int32_t CMCDProc::Enqueue2DCC(char* data, unsigned data_len, const string& ip, unsigned short port)
{
    TDCCHeader *header = (TDCCHeader*)m_send_buf;
    char *data_buff    = m_send_buf + DCC_HEADER_LEN;
    unsigned data_max  = BUFF_SIZE - DCC_HEADER_LEN;

    if (data_len > data_max)
    {
        LogError("data_len > data_max (%u > %u)!", data_len, data_max);
        return -1;
    }

    unsigned iip = INET_aton(ip.c_str());
    unsigned long long flow = make_flow64(iip, port);

    memcpy(data_buff, data, data_len);
    
    header->_type = dcc_req_send;
    header->_ip   = iip;
    header->_port = port;

    int totallen = DCC_HEADER_LEN + msg_len;
    if (m_mq_mcd_2_dcc->enqueue(header, totallen, flow))
    {
        LogError("Failed to enqueue to DCC!");
        return -2;
    }

    return 0;
}
#endif

void CMCDProc::DispatchUserTimeout()
{
    static struct timeval user_last_check_time = {0, 0};

    if (m_workMode == statsvr::WORKMODE_READY)
    {
        return;
    }
    
    timeval ntv;
    gettimeofday(&ntv, NULL);

    if (ntv.tv_sec - user_last_check_time.tv_sec < 15)
    {
        return;
    }
    else
    {
        user_last_check_time = ntv;
    }

    LogTrace("====> Check user timeout at time: %d", ntv.tv_sec);
    CTimerInfo *ti  = new UserOutTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);
    string req_data = i2str(m_cfg._user_time_gap);

    if (ti->do_next_step(req_data) == 0)
    {
        m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
    }
    else
    {
        delete ti;
    }
}

void CMCDProc::DispatchServiceTimeout()
{
    static struct timeval last_check_time = {0, 0};

    if (m_workMode == statsvr::WORKMODE_READY)
    {
        return;
    }
    
    timeval ntv;
    gettimeofday(&ntv, NULL);

    //check every 15 seconds
    if (ntv.tv_sec - last_check_time.tv_sec < 15)
    {
        return;
    }
    else
    {
        last_check_time = ntv;
    }

    LogTrace("====> Check service timeout at time: %d", ntv.tv_sec);
    CTimerInfo *ti  = new ServiceOutTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);
    string req_data = i2str(m_cfg._service_time_gap);

    if (ti->do_next_step(req_data) == 0)
    {
        m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
    }
    else
    {
        delete ti;
    }
}

void CMCDProc::DispatchUser2Service()
{
    if (m_workMode == statsvr::WORKMODE_READY)
    {
        return;
    }
    
    Json::Reader reader;
    Json::Value appList;
    string appListString;

    if (CAppConfig::Instance()->GetAppIDList(appListString))
    {
        LogError("get appIDlist failed.");
        return;
    }
    if (!reader.parse(appListString, appList))
    {
        //LogError("parse appIDlist to JSON failed: %s", appListString.c_str());
        return;
    }

    for (int i = 0; i < appList["appIDList"].size(); i++)
    {
        string appID = appList["appIDList"][i].asString();
        TagUserQueue* pTagQueues = NULL;
        TagUserQueue* pTagHighPriQueues = NULL;

        if (CAppConfig::Instance()->GetTagQueue(appID, pTagQueues) ||
            CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagHighPriQueues))
        {
            //LogError("Fail to get user queues for appID: %s", appID.c_str());
            continue;
        }

        if (pTagQueues->total_queue_count() <= 0 && pTagHighPriQueues->total_queue_count() <= 0)
        {
            //LogDebug("No user on queue");
            continue;
        }

        string strTags;
        vector<string> tags;
        if (CAppConfig::Instance()->GetValue(appID, "tags", strTags))
        {
            LogError("[%s]: get appID tags failed.", appID.c_str());
               continue;
        }
        //LogDebug("tags: %s", strTags.c_str());
        
        MySplitTag((char *)strTags.c_str(), ";", tags);
        //LogDebug("tags.size(): %d", tags.size());
        for (int i = 0; i < tags.size(); i++)
        {
            int getTag;
            int offServer;
            ServiceHeap serviceHeap;

            //LogTrace("tag: %s", tags[i].c_str());
            getTag    = CAppConfig::Instance()->GetTagServiceHeap(tags[i], serviceHeap);
            offServer = CAppConfig::Instance()->CanOfferService(serviceHeap);
            if (getTag || offServer)
            {
                //LogDebug("No service available");
                continue;
            }
            else
            {
                //LogTrace("[%s]: match user with service...", appID.c_str());
                timeval ntv;
                gettimeofday(&ntv, NULL);
                CTimerInfo* ti = new UserServiceTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);
                string req_data = tags[i];
                if (ti->do_next_step(req_data) == 0)
                {
                    m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
                }
                else
                {
                    delete ti;
                }
                //break;
            }
        }
    }
    return;
}

void CMCDProc::DispatchSessionTimer()
{
    if (m_workMode == statsvr::WORKMODE_READY)
    {
        return;
    }

    Json::Reader reader;
    Json::Value appList;
    string appListString;

    if (CAppConfig::Instance()->GetAppIDList(appListString))
    {
        LogError("Failed to get appIDlist!");
        return;
    }

    if (!reader.parse(appListString, appList))
    {
        //LogError("Failed to parse appIDListString:%s", appListString.c_str());
        return;
    }

    for (int i = 0; i < appList["appIDList"].size(); i++)
    {
        string appID = appList["appIDList"][i].asString();
        DispatchCheckQueue(appID);
        DispatchCheckSession(appID);
        DispatchCheckYiBot(appID);
    }
    return;
}

void CMCDProc::CheckTimeoutQueue(const string &appID, TagUserQueue *pTagQueues, unsigned queuePriority)
{
    UserQueue *uq = NULL;
    int expire_count = 0;
    string req_data;
    timeval ntv;
    string app_tag;
    
    //check every tag queue
    map<string, UserQueue*>::iterator it;
    for (it = pTagQueues->_tag_queue.begin(); it != pTagQueues->_tag_queue.end(); it++)
    {
        uq = it->second;

        app_tag = appID + "_" + (it->first);
        if (CAppConfig::Instance()->CheckTagServiceHeapHasOnline(app_tag))
        {
            expire_count = uq->size();
            if (expire_count > 0)
            {
                LogTrace("====> All services in ServiceHeap[%s] is offline, dequeue all users.", app_tag.c_str(), expire_count);
            }
        }
        else
        {
            expire_count = uq->check_expire();
        }

        for (int i = 0; i < expire_count; ++i)
        {
            LogTrace("[%s]: expire_count[%d] > 0, queue timeout...", appID.c_str(), expire_count);

            gettimeofday(&ntv, NULL);
            CTimerInfo* ti = new QueueOutTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);

            Json::Value data;
            data["appID"]         = appID;
            data["tag"]           = it->first;
            data["queuePriority"] = queuePriority;
            req_data = data.toStyledString();
            LogTrace("====> [timeoutDequeue] req_data: %s", req_data.c_str());
            if (ti->do_next_step(req_data) == 0)
            {
                m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
            }
            else
            {
                delete ti;
            }
        }
    }
}

void CMCDProc::DispatchCheckQueue(string appID)
{
    TagUserQueue *pTagQueues = NULL;
    
    if (0 == CAppConfig::Instance()->GetTagHighPriQueue(appID, pTagQueues) && pTagQueues->total_queue_count() > 0)
    {
        //LogTrace("====> Check HighPriQueue.");
        CheckTimeoutQueue(appID, pTagQueues, 1);
    }

    if (0 == CAppConfig::Instance()->GetTagQueue(appID, pTagQueues) && pTagQueues->total_queue_count() > 0)
    {
        //LogTrace("====> Check NormalQueue.");
        CheckTimeoutQueue(appID, pTagQueues, 0);
    }
}


void CMCDProc::DispatchCheckYiBot(string appID)
{
    #ifndef DISABLE_YIBOT_SESSION_CHECK
    
    SessionQueue* pSessQueue = NULL;
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue) 
        || pSessQueue->size() <= 0)
    {
        return;
    }

    vector<string> app_userID_list;
    pSessQueue->check_expire_yibot(m_cfg._yibot_time_gap, app_userID_list);
    int count = app_userID_list.size();
    if (count <= 0)
    {
        return;
    }
    LogDebug("Num of timeout YiBot session: %d", count);
    
    Json::Value js_userID_list;
    js_userID_list.resize(0);
    for (int i = 0; i < count; ++i)
    {
        js_userID_list.append(app_userID_list[i]);
    }
    
    timeval ntv;
    gettimeofday(&ntv, NULL);
    CTimerInfo* ti = new YiBotOutTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);

    Json::Value data;
    data["userID"]  = js_userID_list;
    string req_data = data.toStyledString();
    //LogTrace("req_data: %s", req_data.c_str());
    if (ti->do_next_step(req_data) == 0)
    {
        m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
    }
    else
    {
        delete ti;
    }
    
    #endif
}

void CMCDProc::DispatchCheckSession(string appID)
{
    SessionQueue* pSessQueue = NULL;
    
    if (CAppConfig::Instance()->GetSessionQueue(appID, pSessQueue) 
        || pSessQueue->size() <= 0)
    {
        return;
    }
    else
    {
        int count = pSessQueue->check_expire();

        for (int i = 0; i < count; i++)
        {
            //LogTrace("[%s]: pop session queue...", appID.c_str());
            timeval ntv;
            gettimeofday(&ntv, NULL);
            CTimerInfo* ti = new SessionOutTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);
            string req_data = appID;
            if (ti->do_next_step(req_data) == 0)
            {
                m_timer_queue.set(ti->GetMsgSeq(), ti, ti->GetTimeGap());
            }
            else
            {
                delete ti;
            }
        }
        
        while (true)
        {
            timeval ntv;
            gettimeofday(&ntv, NULL);
            CTimerInfo* ti = new SessionWarnTimer(this, GetMsgSeq(), ntv, "", 0, m_cfg._time_out);
            string req_data = appID;
            if (ti->do_next_step(req_data) == 0)
            {
                delete ti;
                continue;
            }
            else
            {
                delete ti;
                break;
            }
        }
    }
    return;
}

void CMCDProc::CheckFlag(bool enableReload)
{
    if (enableReload)
    {
        if (obj_checkflag.IsReload())
        {
            obj_checkflag.clear_flag();
            ReloadCfg();
        }
    }
}

extern "C"
{
    CacheProc* create_app()
    {
        return new statsvr::CMCDProc();
    }
}
