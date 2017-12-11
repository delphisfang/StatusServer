#include "common_api.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/limits.h>
#include <fcntl.h>
#include <stdio.h>
#include <net/if.h>
#include <string.h>
//#include "common/encoding/charset_converter.h"
#include "tfc_debug_log.h"
#include "common/crypto/symmetric/aes.h"
#include <iconv.h>
#include <time.h>
#include <sys/time.h>

/*
 * return value -1 -- fail, 0 -- asn,  1 -- longconn
 */

const static unsigned PKT_MAX_LEN = (32<<20);

#if 0
bool HitMask(uint64_t mask, uint64_t value)
{
    return mask & ( 1<<value);
}
#endif
int NetDataType(char* data, unsigned data_len)
{
    if (NULL == data)
    {
        return -1;
    }

    // try parse as longconn
    unsigned total_len = 0;
    if (data_len < 4)
    {
        return 0;
    }
    total_len = *((unsigned*)data);

    total_len = ntohl(total_len);

    if (data_len == total_len)
    {
        return 1;
    }
    return 0;
}
char char2hex(char first,char second)
{
    char digit;

    digit = (first >= 'A' ? (first - 'A')+10 : (first - '0'));
    digit *= 16;
    digit += (second >= 'A' ? (second  - 'A')+10 : (second - '0'));
    return(digit);
}
void hex2char(unsigned char what,char *sDest)
{
    char sHex[20]="0123456789ABCDEF";
    sDest[0]=sHex[what / 16];
    sDest[1]=sHex[what % 16];
}
void hex2str(const char *what,int len,char *sDest)
{
    for(int i=0;i<len;i++)
    {
        hex2char(what[i],sDest+i*2);
    }
}
void str2hex(const char* what,int len, char *dest)
{
    for(int i=0; i<len; i+=2)
    {
        dest[i/2] = char2hex(what[i], what[i+1]);
    }
}

string bin2str(const char* sBinData, int iLen)
{
    string sOut;
    char buf[3]={0};
    for (int i = 0 ; i < iLen ; ++i)
    {
        sprintf(buf, "%02x", (unsigned char)sBinData[i]);
        buf[2]=0;
        sOut+= buf;
    }
    return sOut;
}

string GetTime2(const int iType)
{
    time_t  iCurTime;
    struct tm curr;

    time(&iCurTime);
    curr = *localtime(&iCurTime);

    char gs_szTime[256] = {0};

	if (curr.tm_year > 50)
	{
		if (iType == 0)
		{
			snprintf(gs_szTime, sizeof(gs_szTime), "%04d-%02d-%02d %02d:%02d:%02d",
					curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday,
					curr.tm_hour, curr.tm_min, curr.tm_sec);
		}
		else
		{
			snprintf(gs_szTime, sizeof(gs_szTime), "%04d%02d%02d", curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
		}
	}
	else
	{
		if (iType == 0)
		{
			snprintf(gs_szTime, sizeof(gs_szTime), "%04d-%02d-%02d %02d:%02d:%02d",
					curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday,
					curr.tm_hour, curr.tm_min, curr.tm_sec);
		}
		else
		{
			snprintf(gs_szTime, sizeof(gs_szTime), "%04d%02d%02d", curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
		}
	}
	return gs_szTime;
}

string GetFormatTime(const timeval& tvl)
{
	struct tm curr 	= *localtime(&(tvl.tv_sec));
	int year 		= 0;

	char gs_szTime[32] = {0};

	if (curr.tm_year > 50)
	{
		year = curr.tm_year+1900;
	}
	else
	{
		year = curr.tm_year+2000;
	}

	snprintf(gs_szTime, sizeof(gs_szTime), "%04d-%02d-%02d %02d:%02d:%02d.%3d",
			 year, curr.tm_mon+1, curr.tm_mday,
			 curr.tm_hour, curr.tm_min, curr.tm_sec, int(tvl.tv_usec/1000));
	return string(gs_szTime);
}

int MySplitTag(char* pContentLine, char* pSplitTag, vector<string>& vecContent)
{
	char *pStartPtr = pContentLine;
	int iTagLen = strlen(pSplitTag);

	while(1)
	{
		char* pTagPtr = strstr(pStartPtr,pSplitTag);
		if(!pTagPtr)
		{
			string strTag = pStartPtr;
			vecContent.push_back(strTag);
			break;
		}

		string strTag(pStartPtr,pTagPtr-pStartPtr);

		vecContent.push_back(strTag);
		pStartPtr = pTagPtr + iTagLen;

		if(*pStartPtr == 0)
			break;
	}
	return 0;
}


int Split2Set(char* pContentLine, char* pSplitTag, set<string>& setContent)
{
	char *pStartPtr = pContentLine;
	int iTagLen = strlen(pSplitTag);

	while(1)
	{
		char* pTagPtr = strstr(pStartPtr,pSplitTag);
		if(!pTagPtr)
		{
			string strTag = pStartPtr;
			setContent.insert(strTag);
			break;
		}

		string strTag(pStartPtr,pTagPtr-pStartPtr);

		setContent.insert(strTag);
		pStartPtr = pTagPtr + iTagLen;

		if(*pStartPtr == 0)
			break;
	}
	return 0;
}


unsigned INET_aton(const char* ip_str)
{
	struct in_addr ip;
	inet_aton(ip_str, &ip);
	return ip.s_addr;
}

//string INET_ntoa( long ip )
string INET_ntoa( unsigned ip )
{
	struct in_addr in;
	in.s_addr = ip;
	return inet_ntoa(in);
}

void string_replace_reverse(string& in)
{
	string out("");
	for( unsigned i=0; i<in.length(); i++ )
	{
		if( in[i] == '*')
		{
			out += "/";
		}
		else if( in[i] == '!')
		{
			out += "=";
		}
		else if( in[i] == '.')
		{
			out += "+";
		}
		else
		{
			out += in[i];
		}

	}

	in = out;
}

int send_udp_data(const char *ip, short port, const char *data, int data_len)
{
	sockaddr_in server_addr;
    bzero(&server_addr, sizeof(sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	if (inet_aton(ip, &(server_addr.sin_addr)) != 1)
	{
		return -1;
	}

	int cli_sock = socket(AF_INET, SOCK_DGRAM, 0);
	sendto(cli_sock, data, data_len, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
	close(cli_sock);

	return 0;
}

int get_eth_ip(const char *eth, char ip[16])
{
	register int fd, ret;
	struct ifreq buf;

	if ((fd = socket (AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;

	strcpy(buf.ifr_name, eth);

	ret = ioctl (fd, SIOCGIFADDR, (char *) &buf);
	close(fd);

	if (ret)
		return -1;

	strncpy(ip, inet_ntoa(((struct sockaddr_in*)(&buf.ifr_addr))->sin_addr), 15);
	ip[15] = 0;
	return 0;
}

unsigned  str2unsign(const unsigned char* what,int len)
{
	unsigned ret = 0;
	unsigned char *p = (unsigned char*)&ret;

	for(int i=0; i<len; i++)
	{
		p[i] = what[len - i - 1];
	}
	return ret;
}

signed  str2int(const signed char * what,int len)
{
	signed ret = 0;

	signed char *p = (signed char*)&ret;

	for(int i=0; i<len; i++)
	{
		p[i] = what[len - i - 1];
	}
	unsigned char *first = (unsigned char*)&what[0];
	if (first[0] > 127)
	{
		if (2 == len)
			return (ret-65536);
		if (3 == len)
			return (ret-16777216);
	}
  	return ret;
}

//- Get Version, seq, flow_pos, uin, ip, retcode,
int decode_asn_buf(const char* buf, const unsigned conf_ver, unsigned & version, unsigned & msg_seq, unsigned & flow_pos, unsigned & msg_flow, unsigned & protocol, unsigned & uin, unsigned & ip, signed & flag, signed & retcode)
{
	unsigned body_len = 0;
	unsigned char * tmp = (unsigned char*)&buf[1];
	unsigned second = tmp[0];
	int cur_pos = 2;

	unsigned ver_len = 0;
	unsigned flow_len = 0;
	unsigned seq_len = 0;
	unsigned prot_len = 0;
	unsigned uin_len = 0;
	unsigned ip_len = 0;
	unsigned flag_len = 0;
	unsigned ret_len = 0;

	//- version
	if (second <= 128 && (unsigned)buf[2] == 2)
	{
	//	if (second < 24)
	//		return  _PSA_E_MSG_INVALID;
		cur_pos += 1;
		ver_len =  (unsigned)buf[cur_pos];
		cur_pos += 1;
		unsigned char * p = (unsigned char*)(buf+cur_pos);
		version = str2unsign(p, ver_len);
		cur_pos += ver_len;
		if (version < conf_ver)
			return 0;
	}
	else if (second > 128)
	{
		body_len = second & 127;
		cur_pos += body_len;

		if ((unsigned)buf[cur_pos] == 2)
		{
			cur_pos += 1;
			ver_len =  (unsigned)buf[cur_pos];
			cur_pos += 1;
		    unsigned char * p = (unsigned char*)(buf+cur_pos);
			version = str2unsign(p,ver_len);
			cur_pos += ver_len;
			if (version < conf_ver)
				return 0;
		}
	}
	else
	{
		return  -1;
	}
	//- seq
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		seq_len = (unsigned)buf[cur_pos];
		cur_pos += 1;
		unsigned char* p = (unsigned char*)(buf+cur_pos);
		msg_seq = str2unsign(p, seq_len);
		cur_pos += seq_len;
	}
	else
		return -2;

	//- flow
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		flow_len = (unsigned)buf[cur_pos];
		flow_pos = cur_pos+1;
		cur_pos += 1;
		unsigned char* p = (unsigned char*)(buf+cur_pos);
		msg_flow = str2unsign(p, flow_len);
		cur_pos += flow_len;
	}
	else
		return -3;

	//- protocol
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		prot_len = (unsigned)buf[cur_pos];
		cur_pos += 1;
		unsigned char* p = (unsigned char*)(buf+cur_pos);
		protocol = str2unsign(p, prot_len);
		cur_pos += prot_len;
	}
	else
		return -4;

    //- uin
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		uin_len = (unsigned)buf[cur_pos];
		cur_pos += 1;
		unsigned char* p = (unsigned char*)(buf+cur_pos);
		uin = str2unsign(p, uin_len);
		cur_pos += uin_len;
	}
	else
		return -5;

    //- ip
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		ip_len = (unsigned)buf[cur_pos];
		cur_pos += 1;
		unsigned char* p = (unsigned char*)(buf+cur_pos);
		ip = str2unsign(p, ip_len);
		cur_pos += ip_len;
	}
	else
		return -6;

    //- flag
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		flag_len = (unsigned)buf[cur_pos];
		cur_pos += 1;
		signed char* p = (signed char*)(buf+cur_pos);
		flag = str2int(p, flag_len);
		cur_pos += flag_len;
	}
	else
		return -7;
    //- retcode
	if ((unsigned)buf[cur_pos] == 2)
	{
		cur_pos += 1;
		ret_len = (unsigned)buf[cur_pos];
		cur_pos += 1;
		signed char* p = (signed char*)(buf+cur_pos);
		retcode = str2int(p, ret_len);
//		cur_pos += ret_len;
	}
	else
		return -8;

	return 0;
}


char LittleChar2hex(char first,char second)
{
	char digit;

	digit = (first >= 'a' ? (first - 'a')+10 : (first - '0'));
	digit *= 16;
	digit += (second >= 'a' ? (second  - 'a')+10 : (second - '0'));
	return(digit);
}


void LittleStr2hex(const char* what,int len, char *dest)
{
	for(int i=0; i<len; i+=2)
	{
		dest[i/2] = LittleChar2hex(what[i], what[i+1]);
	}
}

void LittleHex2char(unsigned char what,char *sDest)
{
	char sHex[20]="0123456789abcdef";
	sDest[0]=sHex[what / 16];
	sDest[1]=sHex[what % 16];
}
void LittleHex2str(const char *what,int len,char *sDest)
{
	for(int i=0;i<len;i++)
	{
		LittleHex2char(what[i], sDest + i*2);
	}
}

uint64_t CalcTimeCost_MS(const struct timeval& start, const struct timeval& end) {
    int64_t time_cost =
        (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000;
    if (time_cost < 0) time_cost = 0;
    return time_cost;
}
uint64_t CalcTimeCost_US(const struct timeval& start, const struct timeval& end) {
    int64_t time_cost =
        (end.tv_sec - start.tv_sec)*1000000 + (end.tv_usec - start.tv_usec);
    if (time_cost < 0) time_cost = 0;
    return time_cost;
}

unsigned get_ip_by_if(const char* ifname) {
    register int fd, intrface;
    struct ifreq buf[10];
    struct ifconf ifc;
    unsigned ip = 0;

    if((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) {
        ifc.ifc_len = sizeof(buf);
        ifc.ifc_buf = (caddr_t)buf;
        if(!ioctl(fd, SIOCGIFCONF, (char*)&ifc)) {
            intrface = ifc.ifc_len / sizeof(struct ifreq);
            while(intrface-- > 0)  {
                if(strcmp(buf[intrface].ifr_name, ifname) == 0) {
                    if(!(ioctl(fd, SIOCGIFADDR, (char *)&buf[intrface]))) {
                        ip = (unsigned)((struct sockaddr_in *)(&buf[intrface].ifr_addr))->sin_addr.s_addr;
                    }
                    break;
                }
            }
        }
        ::close(fd);
    }
    return ip;
}

string get_default(MapParam& mparams, const string& key, const string& def_val)
{
	MapParam::iterator it = mparams.find(key);
	if (it != mparams.end())
	{
		return it->second;
	}
	return def_val;
}

time_t str2time (const char* date, int defval, const char* format)
{
	// LogError("str2time: date:%s, format:%s", date, format);
	if (date==NULL || strlen(date) < 1)
	{
		return defval;
	}
	
    struct tm tm;
    strptime(date,format, &tm) ;
    time_t ft=mktime(&tm);
    return ft;
}

uint64_t correct_60(int val)
{
    if (val < 0)return 0;
    if (val > 59)return 59;
    return (uint64_t)val;
}


uint64_t correct_24(int val)
{
    if (val < 0)return 0;
    if (val > 23)return 23; 
    return (uint64_t)val;
}


uint64_t to_tick(struct tm* t)
{
	struct tm& tm = *t;
	return (uint64_t)tm.tm_year*365*86400 + (uint64_t)tm.tm_mon*30*86400 + 
		(uint64_t)tm.tm_mday*86400 + correct_24(tm.tm_hour)*3600 + correct_60(tm.tm_min)*60 + 
		correct_60(tm.tm_sec);
}

uint64_t str2timetick(const char* date, uint64_t defval, const char* format)
{
	//LogError("str2time: date:%s, format:%s", date, format);
	if (date==NULL || strlen(date) < 1)
	{
		return defval;
	}
	
    struct tm tm;
    strptime(date,format, &tm);
    //fprintf (stderr,"[str2timetick] tm:[%d-%d-%d %lu:%lu:%lu]\n", tm.tm_year, tm.tm_mon +1, tm.tm_mday, correct_24(tm.tm_hour), correct_60(tm.tm_min), correct_60(tm.tm_sec));
	return to_tick(&tm);
}

uint64_t curr_timetick()
{
	time_t curr = time(0);
	struct tm* ttm = gmtime(&curr);
    ttm->tm_hour += 8;
    //fprintf (stderr,"[curr_timetick] tm:[%d-%d-%d %d:%d:%d]\n", ttm->tm_year, ttm->tm_mon+1, ttm->tm_mday, ttm->tm_hour, ttm->tm_min, ttm->tm_sec);
	return to_tick(ttm);;
}

int CvtCharSet (const string &from_code, const string to_code, const string &src_str, string &dst_str)
{
	iconv_t m_cd = iconv_open (to_code.c_str(), from_code.c_str());
	if (m_cd == (iconv_t)-1)
	{
		return -255;
	}
	int arg = 1;
	iconvctl (m_cd , ICONV_GET_DISCARD_ILSEQ, &arg);
	if (m_cd == iconv_t(-1))
	{
		//LogError("[CvtGbkToU8] Create convert fd failed!\n");
		return -1;
	}

	//iconv(m_cd, NULL, NULL, NULL, NULL); // reset iconv
	//DEBUG_P (LOG_DEBUG, "[CvtGbkToU8] src_str:[%s]!\n", src_str.c_str());
	char* in_ptr = const_cast<char*>(src_str.c_str());
	size_t in_left = src_str.length();
	size_t out_size = 2 * in_left +1;
	char *out_buf = new char[out_size];
	size_t in_buf_size = in_left;
	char *in_buf = new char[in_buf_size];
	dst_str = "";

	for (;;)
	{
		//size_t org_out_size = 1024;
		size_t out_left = out_size;
		char *out_ptr = out_buf;
		//char* out_ptr = string_as_array(out) + org_out_size;
		size_t converted = iconv(m_cd, &in_ptr, &in_left, &out_ptr, &out_left);
		if (converted != size_t(-1))
		{
			*out_ptr = '\0';
			//cout << "convert success, after convert: " << out_buf << endl;
			dst_str += string(out_buf);
			delete []in_buf;
			delete []out_buf;
			return 0;
		}
		else
		{
			switch (errno)
			{
				case EILSEQ:
					//LogError("[CvtU8ToGbk] Invalid charset, errno:[%d], msg:[%s]!\n", errno, strerror(errno));
					++in_ptr;
					--in_left;
					*out_ptr = '\0';
					dst_str += string(out_buf);
					//delete []in_buf;
					//delete []out_buf;
					//return 0;
					memset (out_buf, 0, out_size);
					if (in_left <= 0)
					{
						return 0;
					}
					//LogError("[CvtU8ToGbk] in_ptr:[%d], in_left:[%d]\n", in_ptr, in_left);
					memcpy (in_buf, in_ptr, in_left);
					in_ptr = in_buf;
					*(in_ptr +  in_left) = 0;
					break;
				default:
					DEBUG_P(LOG_ERROR, "[CvtGbkToU8] other error, errno:[%d], msg:[%s]!\n", errno, strerror(errno));
					*out_ptr = '\0';
					dst_str += string(out_buf);
					delete []in_buf;
					delete []out_buf;
					return 0;
			}
		}
	}
}

int CvtGb2313ToGbk (const string &src_str, string &dst_str)
{
   return CvtCharSet ( "gb2313", "GBK", src_str, dst_str);
}

int CvtU8ToGbk (const string &src_str, string &dst_str)
{
   return CvtCharSet ( "UTF-8", "GBK", src_str, dst_str);
}

int CvtGbkToU8 (const string &src_str, string &dst_str)
{
   return CvtCharSet ( "GBK", "UTF-8", src_str, dst_str);
}


string utf8_2_gbk(const string& u)
{
	string tmp = "";
	if (CvtU8ToGbk (u, tmp) != 0)
		return u;
	return tmp;
}

string gbk_2_utf8(const string& g)
{
	string tmp = "";
	CvtGbkToU8(g, tmp);
	return tmp;
}


void add_docid(string& docids, const string& docid)
{
	if (docids.size() > 0)
	{
		docids += ",";
	}
	docids += docid;
}

void add_doc_question(string& doc_qs, const string& doc_q)
{
	if (doc_qs.size() > 0)
	{
		doc_qs += "||";
	}
	doc_qs += doc_q;
}

string trim_left(const string &s,const string& filt)
{
	char *head=const_cast<char *>(s.c_str());
	char *p=head;
	while(*p) {
		bool b = false;
		for(size_t i=0;i<filt.length();i++) {
			if((unsigned char)*p == (unsigned char)filt.c_str()[i]) {b=true;break;}
		}
		if(!b) break;
		p++;
	}
	return string(p,0,s.length()-(p-head));
}

string trim_right(const string &s,const string& filt)
{
	if(s.length() == 0) return string();
	char *head=const_cast<char *>(s.c_str());
	char *p=head+s.length()-1;
	while(p>=head) {
		bool b = false;
		for(size_t i=0;i<filt.length();i++) {
			if((unsigned char)*p == (unsigned char)filt.c_str()[i]) {b=true;break;}
		}
		if(!b) {break;}
		p--;
	}
	return string(head,0,p+1-head);
}

string Trim(const string& s)
{
	return trim_left(trim_right(s));
}

int GetKv(const char* pszFilePath, map<string, string>& mConf, const string& split)
{
	FILE *pstFile;
	char szBuffer[4096];
	char* pszTemp;

	if((pstFile = fopen(pszFilePath, "r")) == NULL)
	{
			return -1;
	}
	while(fgets(szBuffer, sizeof(szBuffer), pstFile) != NULL)
	{
		szBuffer[strlen(szBuffer)-1] = '\0';
		pszTemp = strstr(szBuffer, split.c_str());
		if(pszTemp == NULL)
			continue;
		*pszTemp='\0';
		mConf[szBuffer] = pszTemp+split.length();
	}

	fclose(pstFile);
	return 0;
}

void set_value(Json::Value& item, const string& keyi, Json::Value& abs, const string& keya, bool convert)
{
	if (convert) item[keyi] = utf8_2_gbk(abs[keya].asString());
	else item[keyi] = abs[keya].asString();
}

void set_value_uint(Json::Value& item, const string& keyi, Json::Value& abs, const string& keya, uint32_t defval)
{
	if (abs[keya].isIntegral())
	{
		item[keyi] = abs[keya].asUInt();
	}
	else
	{
        string n_str = abs[keya].asString();
		item[keyi] = atoi(n_str.c_str());
	}
}

void set_value_uint64(Json::Value& item, const string& keyi, Json::Value& abs, const string& keya, uint64_t defval)
{
	if (abs[keya].isIntegral())
	{
		item[keyi] = abs[keya].asUInt64();
	}
	else
	{
		item[keyi] = (Json::Value::UInt64)defval;
	}
}

int php_htoi(char *s)
{
    int value;
    int c;

    c = ((unsigned char *)s)[0];
    if (isupper(c))
        c = tolower(c);
    value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

    c = ((unsigned char *)s)[1];
    if (isupper(c))
        c = tolower(c);
    value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

    return (value);
}

string urldecode(const string &str_source)
{
    char const *in_str = str_source.c_str();
    int in_str_len = strlen(in_str);
    int out_str_len = 0;
    string out_str;
    char *str;

    str = strdup(in_str);
    char *dest = str;
    char *data = str;

    while (in_str_len--) {
        if (*data == '+') {
            *dest = ' ';
        }
        else if (*data == '%' && in_str_len >= 2 && isxdigit((int) *(data + 1)) 
            && isxdigit((int) *(data + 2))) {
                *dest = (char) php_htoi(data + 1);
                data += 2;
                in_str_len -= 2;
        } else {
            *dest = *data;
        }
        data++;
        dest++;
    }
    *dest = '\0';
    out_str_len =  dest - str;
    out_str = str;
    free(str);
    return out_str;
}

string i2str (int n)
{
	char buf[16];
	snprintf (buf, 16, "%d", n);
	return string(buf);
}

string ui2str (unsigned int n)
{
	char buf[16];
	snprintf (buf, 16, "%u", n);
	return string(buf);
}

string l2str (long n)
{
	char buf[32];
	snprintf (buf, 32, "%ld", n);
	return string(buf);
}

string ul2str (unsigned long n)
{
	char buf[32];
	snprintf (buf, 32, "%lu", n);
	return string(buf);
}

bool IsFileExist(const char* filename) {
  return (access(filename, 0) == 0);
}

string getappID(const string& key)
{
	int pos = key.find('_');
	string strappID = key.substr(0, pos);
	return strappID;
}

string delappID(string key)
{
	int pos = key.find('_');
	if(pos > 0)
	{
		key.replace(0, pos + 1, "");
	}
	return key;
}

string getDomainIp(const string domainName)
{
	char cmd[1024] = {0};

    sprintf(cmd, "dig %s|grep %s|awk '{print $5}'", domainName.c_str(), domainName.c_str());
 
    FILE *pp = popen(cmd, "r");
    if(!pp)
    {
        return string("127.0.0.1");
    }
 
    char result[1024] = {0};
    while(fgets(result, sizeof(result), pp) != NULL)
    {
		if(result[0]>='0' && result[0] <='9')
		{
			return string(result);
		}
    }

	return string("127.0.0.1");
}

int cancelUserFromString(const string& userID, string& queueString)
{
	int pos = queueString.find(userID);
	if(string::npos == pos)
	{
		return -1;
	}
	int pos2 = queueString.find(";", pos);
	if (string::npos == pos2)
	{
		pos2 = queueString.size();
	}
	string tmp = queueString.substr(0, pos + 1);
	int pos1 = tmp.rfind(";");
	string value;
	if (string::npos == pos1)
	{
		value = queueString.substr(0, pos2);
		queueString.replace(0, pos2, "");
	}
	else
	{
		value = queueString.substr(pos1 + 1, pos2 - pos1 - 1);
		queueString.replace(pos1, pos2 - pos1, "");
	}
}



string get_value_str(Json::Value &jv, const string &key, const string def_val)
{
	if (!jv[key].isNull() && jv[key].isString())
	{
		return jv[key].asString();
	}
	else
	{
		return def_val;
	}
}

int get_value_int(Json::Value &jv, const string &key, const int def_val)
{
	if (!jv[key].isNull() && jv[key].isInt())
	{
		return jv[key].asInt();
	}
	else
	{
		return def_val;
	}
}

unsigned int get_value_uint(Json::Value &jv, const string &key, const unsigned int def_val)
{
	if (!jv[key].isNull() && jv[key].isUInt())
	{
		return jv[key].asUInt();
	}
	else
	{
		return def_val;
	}
}

long long get_value_int64(Json::Value &jv, const string &key, const long long def_val)
{
	if (!jv[key].isNull() && jv[key].isInt64())
	{
		return jv[key].asInt64();
	}
	else
	{
		return def_val;
	}
}

unsigned long long get_value_uint64(Json::Value &jv, const string &key, const unsigned long long def_val)
{
	if (!jv[key].isNull() && jv[key].isUInt64())
	{
		return jv[key].asUInt64();
	}
	else
	{
		return def_val;
	}
}

long long GetCurTimeStamp()
{
	timeval nowTime;
	gettimeofday(&nowTime, NULL);
	return (nowTime.tv_sec*1000 + nowTime.tv_usec / 1000);
}

