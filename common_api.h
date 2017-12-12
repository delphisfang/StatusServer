#ifndef __COMMON__API__H__
#define __COMMON__API__H__
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <set>
#include "jsoncpp/json.h"


using namespace std;
typedef map<string, string> MapParam;


unsigned get_ip_by_if(const char* ifname);
uint64_t CalcTimeCost_MS(const struct timeval& start, const struct timeval& end);
uint64_t CalcTimeCost_US(const struct timeval& start, const struct timeval& end);
#if 0
bool HitMask(uint64_t mask, uint64_t value);
#endif
int  NetDataType(char * data, unsigned data_len);
char char2hex(char first,char second);
void hex2char(unsigned char what,char *sDest);
void hex2str(const char *what,int len, char *sDest);
void str2hex(const char* what,int len, char *dest);

void LittleStr2hex(const char* what,int len, char *dest);
char LittleChar2hex(char first,char second);

void LittleHex2char(unsigned char what,char *sDest);
void LittleHex2str(const char *what,int len,char *sDest);

string  bin2str(const char *sBinData,int iLen);

string GetTime2(const int iType);

string GetFormatTime(const timeval& tvl);
int MySplitTag(char* pContentLine, const char* pSplitTag, vector<string>& vecContent);
int Split2Set(char* pContentLine, char* pSplitTag, set<string>& setContent);


unsigned INET_aton(const char* ip_str);

string INET_ntoa( unsigned ip );

void string_replace_reverse(string& in);
void string_replace(string& in);

int send_udp_data(const char *ip, short port, const char *data, int data_len);

int get_eth_ip(const char *eth, char ip[16]);


signed  str2int(const signed char* what,int len);
unsigned  str2unsign(const unsigned char* what,int len);

int GetMd5random(char * value, unsigned value_len, unsigned mod_base);

/*
 * make_flow64():		Get a 64 bit flow number from IP, PORT and Index.
 * @ip:					IP.
 * @port:				PORT.
 * @index:				Internal index.
 * Returns:				Always return a flow number.
 */
static inline unsigned long long
                    make_flow64(unsigned ip, unsigned short port, unsigned short index = 0)
{
    unsigned long long	flow = 0, flow_low = 0;

    flow = ip;
    flow = flow << 32;
    flow_low = port;
    flow_low = flow_low << 16;
    flow = flow | flow_low;
    flow_low = index;
    flow = flow | flow_low;

    return flow;
}

inline uint32_t CalcTimeCost_MS(timeval& start, timeval& end) {
    return (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec) / 1000;
}

inline uint32_t CalcTimeCost_US(timeval& start, timeval& end) {
    return (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
}

string get_default(MapParam& mparams, const string& key, const string& def_val);

time_t str2time (const char* date, int defval, const char* format="%Y-%m-%d %H:%M:%S");
uint64_t str2timetick(const char* date, uint64_t defval, const char* format="%Y-%m-%d %H:%M:%S");
uint64_t curr_timetick();

string utf8_2_gbk(const string& u);
void add_docid(string& docids, const string& docid);
void add_doc_question(string& doc_qs, const string& doc_q);


string trim_left(const string &s,const string& filt=" ");
string trim_right(const string &s,const string& filt=" ");
string Trim(const string& s);
int GetKv(const char* pszFilePath, map<string, string>& mConf, const string& split="=");


void set_value(Json::Value& item, const string& keyi, Json::Value& abs, const string& keya, bool convert);
void set_value_uint(Json::Value& item, const string& keyi, Json::Value& abs, const string& keya, uint32_t defval);
void set_value_uint64(Json::Value& item, const string& keyi, Json::Value& abs, const string& keya, uint64_t defval);

int CvtGb2313ToGbk (const string &src_str, string &dst_str);
string urldecode(const string &str_source);

string gbk_2_utf8(const string& g);

string i2str (int n);

string ui2str (unsigned int n);

string l2str (long n);

string ul2str (unsigned long n);

bool IsFileExist(const char* filename);

string getappID(const string& key);

string delappID(string key);

string getDomainIp(const string domainName);

int cancelUserFromString(const string& userID, string& queueString);


string get_value_str(Json::Value &jv, const string &key, const string def_val = "");
int get_value_int(Json::Value &jv, const string &key, const int def_val = 0);
unsigned int get_value_uint(Json::Value &jv, const string &key, const unsigned int def_val = 0);
long long get_value_int64(Json::Value &jv, const string &key, const long long def_val = 0);
unsigned long long get_value_uint64(Json::Value &jv, const string &key, const unsigned long long def_val = 0);

long long GetCurTimeStamp();

#endif

