/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     A7Client.cpp
* @version  1.0
* @brief
* @author   hejun
* @date     2017.05.04
* @note
**/

#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include "A7Client.h"
#include "mediaInterface.h"
#ifndef STUB_TEST_TURN_ON
#include "configDb.h"
#endif

BSTM_DEBUG_DEFINE_MODULE("bstmapp.itv.media");

const static char* g_http_selectionStart_request = "POST /SelectionStart HTTP/1.1\r\n";
const static char* g_http_selectionStart_head = "Content-Length: %d\r\nX_Baustem_ServiceGroup: %s\r\nX_Baustem_S1URL: %s\r\n";
const static char* g_seperator = "\r\n";
const static char* g_selectionStart_data = "<SelectionStart titleAssetId=\"%s\" />HTTP/1.1 200 OK\r\n";
const static char* g_http_channelselectionStart_request = "POST /ChannelSelectionStart HTTP/1.1\r\n";

const static char* g_http_channelselectionStart_head = "Content-Length: %d\r\nX_Baustem_ServiceGroup: %s\r\nX_Baustem_S1URL: %s\r\n";
const static char* g_channelselectionStart_data = "<ChannelSelectionStart serviceType=\"%s\" channelId=\"%s\" assetId=\"%s\" />HTTP/1.1 200 OK\r\n";

#define SELECTIONSTART_RESPONSE_LEN (1024)

A7Client::A7Client()
{
    init_data();
    set_ip_addr();
}

A7Client::~A7Client()
{
    close_client_socket(a7_data.SocketID);
    MEDIA_PRINTF_D("close A7 socket client.\n");
}

void A7Client::init_data()
{
    a7_data.SocketID = -1;
    a7_data.Req_Buf = "";
    a7_data.Res_Buf = "";
	a7_data.purchaseToken = "";
	a7_data.ServiceType = "";
	a7_data.ChannelId = "";
	a7_data.AssetId = "";
	a7_data.TitleAssetId = "";
	a7_data.serviceGroup = "";
	a7_data.s1_URL = "";

    ip_addr.GH_IP = "";
    ip_addr.A7_Port = 0;
    ip_addr.S1_Port = 0;
    ip_addr.C1_Port = 0;
    
    return;
}

//in t.gh,only paid and pid is necessary
bool A7Client::parse_url(char* url)
{
    char *ps = NULL;
    char type[purchaseToken_MAX_LEN];
    char pid[purchaseToken_MAX_LEN];
    char paid[purchaseToken_MAX_LEN];
    char serviceType[purchaseToken_MAX_LEN];
    char channelId[purchaseToken_MAX_LEN];
    char s1url[purchaseToken_MAX_LEN];

    a7_data.TitleAssetId = "";

	//set S1 URL
	memset(s1url, 0, sizeof(char)*purchaseToken_MAX_LEN);
	MEDIA_PRINTF_I("param url = %s\n", url);
    char *str = (char *)url;
	
	ps = (char*)strstr((char*)str, "rtsp://");
    if (NULL != ps)
    {
		MEDIA_PRINTF_I("ps = %s\n", ps);
		int i = 0;
		while ( (*ps != '?') && (*ps != '\0'))
		{
			s1url[i] = *ps;
			i++;
			ps++;
		}
		//s1url[i] = '\0';
		MEDIA_PRINTF_I("s1url=%s\n", s1url);
		
    }
	else
	{
		MEDIA_PRINTF_E("rtsp:// is not found!usrl = %s\n", url);
		return false;
	}

	a7_data.s1_URL = "";
	a7_data.s1_URL += s1url;
	MEDIA_PRINTF_I("s1_URL=%s\n", a7_data.s1_URL.c_str());
	

	//set type
    memset(type, 0, sizeof(char)*purchaseToken_MAX_LEN);
    ps = (char*)strstr((char*)url, "type=");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("\"type=\" is not found!\n");
        return false;
    }

    ps += strlen("type=");

    for (int i = 0; i < purchaseToken_MAX_LEN; i++)
    {
        if (ps[i] != ';' && ps[i] != '&' && ps[i] != ' ' && ps[i] != '\0' && ps[i] != '\r' && ps[i] != '\n')
        {
            type[i] = ps[i];
        }
        else
        {
            break;
        }
    }
	
	play_type = atoi(type);
	
	if ((play_type>=1) && (play_type<=4))
	{
	    //get paid
	    memset(paid, 0, sizeof(char)*purchaseToken_MAX_LEN);
	    ps = (char*)strstr((char*)url, "paid=");
	    if (NULL == ps)
	    {
	        MEDIA_PRINTF_E("\"paid=\" is not found!\n");
	        return false;
	    }

	    ps += strlen("paid=");

	    for (int i = 0; i < purchaseToken_MAX_LEN; i++)
	    {
	        if (ps[i] != ';' && ps[i] != '&' && ps[i] != ' ' && ps[i] != '\0' && ps[i] != '\r' && ps[i] != '\n')
	        {
	            paid[i] = ps[i];
	        }
	        else
	        {
	            break;
	        }
	    }


	    //get pid
	    memset(pid, 0, sizeof(char)*purchaseToken_MAX_LEN);
	    ps = (char*)strstr((char*)url, "pid=");
	    if (NULL == ps)
	    {
	        MEDIA_PRINTF_E("\"pid=\" is not found!\n");
	        return false;
	    }

	    ps += strlen("pid=");

	    for (int i = 0; i < purchaseToken_MAX_LEN; i++)
	    {
	        if (ps[i] != ';' && ps[i] != '&' && ps[i] != ' ' && ps[i] != '\0' && ps[i] != '\r' && ps[i] != '\n')
	        {
	            pid[i] = ps[i];
	        }
	        else
	        {
	            break;
	        }
	    }

		//set serviceType
		if (3 == play_type)
		{
			a7_data.ServiceType = "BTV";
		}
		
		//set serviceType
		if (4 == play_type)
		{
			a7_data.ServiceType = "TTV";
		}
		
	}
	else
	{
		MEDIA_PRINTF_E("can't recognize type\n");
		return false;
	}

	//set asset id for selectionStart
	if ((1 == play_type) || (2 == play_type))
	{
		a7_data.TitleAssetId = "-=type=";
    	a7_data.TitleAssetId += type;
	    a7_data.TitleAssetId += "paid=";
	    a7_data.TitleAssetId += paid;
	    a7_data.TitleAssetId += "poid=127";
	    a7_data.TitleAssetId += "pid=";
	    a7_data.TitleAssetId += pid;
	}
	//set asset id for channelSelectionStart
	else if ((3 == play_type) || (4 == play_type))
	{
		a7_data.AssetId = pid;
    	a7_data.AssetId += "-=";
    	a7_data.AssetId += paid;		
	}

    return true;
}


std::string A7Client::load_text_file(const std::string& path)
{
    FILE* f = fopen(path.c_str(), "r");
    if (NULL == f)
    {
        MEDIA_PRINTF_E("can't open file\n");
        return "";
    }

    std::string content;
    char buf[1024] = { 0 };
    while (fgets(buf, sizeof(buf), f)) {
        content += buf;
    }

    fclose(f);
    return content;
}

void A7Client::set_ip_addr()
{
    char cfgPath[256] = "";
    std::string tmp;
    char * p = getenv("SDK_MEDIA_PLAY_CONFIG_PATH");
    if (NULL == p)
    {
        strcpy(cfgPath, "./mediaPlay.ini");
    }
    else
    {
        strncpy(cfgPath, p, strlen(p));
        strcat(cfgPath, "/mediaPlay.ini");
    }


    std::string logContext = load_text_file(cfgPath);
    if (strlen(logContext.c_str()) == 0)
    {
        MEDIA_PRINTF_E("%s is not found.\n", cfgPath);
        return;
    }
    else
    {
        int begin = logContext.find("T_GH_A7_PORT");
        int end = 0;

        //search A7 port 
        if (std::string::npos != (unsigned int)begin)
        {
            end = logContext.find("\n", begin);
            if (std::string::npos != (unsigned int)end)
            {
                tmp = logContext.substr(begin + strlen("T_GH_A7_PORT="), end - begin - strlen("T_GH_A7_PORT="));
                ip_addr.A7_Port = atoi(tmp.c_str());
            }
            else
            {
                MEDIA_PRINTF_E("'%s' no end symbol\r\n", cfgPath);
                return;
            }
            MEDIA_PRINTF_I("A7_Port is %d\r\n", ip_addr.A7_Port);
        }
        else
        {
            MEDIA_PRINTF_E("'%s' no T_GH_A7_PORT=\r\n", cfgPath);
            return;
        }

        //search S1 port 
        begin = logContext.find("T_GH_S1_PORT");
        if (std::string::npos != (unsigned int)begin)
        {
            end = logContext.find("\n", begin);
            if (std::string::npos != (unsigned int)end)
            {
                tmp = logContext.substr(begin + strlen("T_GH_S1_PORT="), end - begin - strlen("T_GH_S1_PORT="));
                ip_addr.S1_Port = atoi(tmp.c_str());
            }
            else
            {
                MEDIA_PRINTF_E("'%s' no end symbol\r\n", cfgPath);
                return;
            }
            MEDIA_PRINTF_I("S1_Port is %d\r\n", ip_addr.S1_Port);

        }
        else
        {
            MEDIA_PRINTF_E("'%s' no T_GH_S1_PORT=\r\n", cfgPath);
            return;
        }


        //search C1 port 
        begin = logContext.find("T_GH_C1_PORT");
        if (std::string::npos != (unsigned int)begin)
        {
            end = logContext.find("\n", begin);
            if (std::string::npos != (unsigned int)end)
            {
                tmp = logContext.substr(begin + strlen("T_GH_C1_PORT="), end - begin - strlen("T_GH_C1_PORT="));
                ip_addr.C1_Port = atoi(tmp.c_str());
            }
            else
            {
                MEDIA_PRINTF_E("'%s' no end symbol\r\n", cfgPath);
                return;
            }
            MEDIA_PRINTF_I("C1_Port is %d\r\n", ip_addr.C1_Port);

        }
        else
        {
            MEDIA_PRINTF_E("'%s' no T_GH_C1_PORT=\r\n", cfgPath);
            return;
        }

        //search IP
        begin = logContext.find("T_GH_LO_IP");
        if (std::string::npos != (unsigned int)begin)
        {
            end = logContext.find("\n", begin);
            if (std::string::npos != (unsigned int)end)
            {
                tmp = logContext.substr(begin + strlen("T_GH_LO_IP="), end - begin - strlen("T_GH_LO_IP="));
                ip_addr.GH_IP = tmp;
            }
            else
            {
                MEDIA_PRINTF_E("'%s' no end symbol\r\n", cfgPath);
                return;
            }
            MEDIA_PRINTF_I("GH_IP is %s\r\n", ip_addr.GH_IP.c_str());

        }
        else
        {
            MEDIA_PRINTF_E("'%s' no T_GH_LO_IP=\r\n", cfgPath);
            return ;
        }

    }


}


IP_ADDR A7Client::get_ip_addr()
{
    return ip_addr;
}


/*std::string A7Client::get_asset_id()
{
    return a7_data.TitleAssetId;
}*/

std::string A7Client::get_purchaseToken()
{
	return a7_data.purchaseToken;
}

void A7Client::set_purchaseToken( std::string token)
{
	a7_data.purchaseToken = token;
}

C_S32 A7Client::get_playType()
{
	return play_type;
}

int A7Client::check_dynamic_url(const char* url)
{
	char* ps = (char*)url;

	//seek first number
	while ((*ps < '0' || *ps > '9') && (*ps!='\0'))
	{
		ps++;
	}

	if ('\0' == *ps)
	{
		return -1;
	}

	while ((*ps >= '0' && *ps <= '9') || ('.' == *ps))
	{
		ps++;
	}

	//check if ":" followed number
	if (':' != *ps)
	{
		return -1;
	}
	ps++;

	//check port
	if (*ps < '0' || *ps > '9')
	{
		return -1;
	}

	return 0;	
}

int A7Client::set_dynamic_data()
{
	unsigned int gourpId = 0;
	char serviceGroup[64];
	std::string s1_url;
	int ret;

	memset(serviceGroup, 0, sizeof(serviceGroup));
	s1_url = "";

	int re;
	#ifndef STUB_TEST_TURN_ON
	ret = configDb::getInstance()->getGroupIdInfo(&gourpId, s1_url);
	#else
	gourpId = 105;
	s1_url = "http://172.16.88.5:554";
	re = 0;
	MEDIA_PRINTF_I("STUB_TEST_TURN_ON is defined. gourpId and s1_url is revalued\n");
	#endif	
	
	sprintf(serviceGroup, "%u", gourpId);
	
	if (0 == re)
	{
		MEDIA_PRINTF_D("configDb::getGroupIdInfo success. groupId=%u. s1_url=%s\n", gourpId, s1_url.c_str());	
		ret = 0;
	}
	else
	{
		MEDIA_PRINTF_E("configDb::getGroupIdInfo falied. groupId=%u. s1_url=%s\n", gourpId, s1_url.c_str());
		ret = -1;
	}

	if (0 != check_dynamic_url(a7_data.s1_URL.c_str()))
	{
		ret = -1;
		MEDIA_PRINTF_E("s1_url is checked failed. s1_url=%s\n", a7_data.s1_URL.c_str());
	}

	a7_data.serviceGroup = serviceGroup;

	return ret;
}


int A7Client::build_A7_SelectionStart_msg()
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[RECV_BUFF_DEFAULT_LEN * 2];

    //build request
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_http_selectionStart_request);
    a7_data.Req_Buf += tmp_buf;

    //count conetent length
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_selectionStart_data, a7_data.TitleAssetId.c_str());
    int content_len = strlen(tmp_buf);

    //build head
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_http_selectionStart_head, content_len, a7_data.serviceGroup.c_str(), a7_data.s1_URL.c_str());
    a7_data.Req_Buf += tmp_buf;

    //add "\r\n" after head
    a7_data.Req_Buf += g_seperator;

    //build body
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_selectionStart_data, a7_data.TitleAssetId.c_str());
    a7_data.Req_Buf += tmp_buf;

    //add "\r\n" at end of the message
    a7_data.Req_Buf += g_seperator;
    
	MEDIA_PRINTF_I("sendbuf is : \n%s \n", a7_data.Req_Buf.c_str());
    return 0;
}

int A7Client::build_A7_ChannelSelectionStart_msg()
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[RECV_BUFF_DEFAULT_LEN * 2];

	
    //build request
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_http_channelselectionStart_request);
    a7_data.Req_Buf += tmp_buf;
	

    //count conetent length
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_channelselectionStart_data, a7_data.ServiceType.c_str(), a7_data.ChannelId.c_str(), a7_data.AssetId.c_str());
    int content_len = strlen(tmp_buf);

    //build head
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
	sprintf(tmp_buf, g_http_channelselectionStart_head, content_len, a7_data.serviceGroup.c_str(), a7_data.s1_URL.c_str());
    a7_data.Req_Buf += tmp_buf;

    //add "\r\n" after head
    a7_data.Req_Buf += g_seperator;

    //build body
    memset(tmp_buf, 0, sizeof(SND_BUFF_DEFAULT_LEN * 2));
    sprintf(tmp_buf, g_channelselectionStart_data, a7_data.ServiceType.c_str(), a7_data.ChannelId.c_str(), a7_data.AssetId.c_str());
    a7_data.Req_Buf += tmp_buf;

    //add "\r\n" at end of the message
    a7_data.Req_Buf += g_seperator;
    
	MEDIA_PRINTF_I("sendbuf is : \n%s \n", a7_data.Req_Buf.c_str());
    return 0;	
}

C_S32 A7Client::create_A7_client()
{
    BSTM_DEBUG_ENTER();
    int sockId;

    sockId = create_client_socket(ip_addr.A7_Port, ip_addr.GH_IP.c_str());
    a7_data.SocketID = sockId;
    
    if (-1 == sockId)
    {
        MEDIA_PRINTF_E("create A7 client failed! \n");
        return -1;
    }

    return sockId;
}


int A7Client::send_A7_selection_request()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

	MEDIA_PRINTF_I("send selection request to A7, buf = %s \n", a7_data.Req_Buf.c_str());
    data_len = send_client_request(a7_data.SocketID, a7_data.Req_Buf.c_str());
    
    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send selection request to A7 client failed! \n");
    }

    return data_len;
}

int A7Client::recv_A7_selection_response()
{
    BSTM_DEBUG_ENTER();
    int sock_status = -1;
    int response_ret = 0;
    std::string recv_buf;

    int counter = (callback_timer / 500) + 1;

    while (counter-- > 0)
    {
        sock_status = recv_client_response(a7_data.SocketID, recv_buf, 500);

        if (sock_status > 0)
        {
            if ((char*)strstr(a7_data.Res_Buf.c_str(), "\r\n\r\n") != NULL)
            {
                a7_data.Res_Buf.clear();
            }
            a7_data.Res_Buf += recv_buf;

            if ((char*)strstr(a7_data.Res_Buf.c_str(), "\r\n\r\n") != NULL)
            {
                response_ret = handle_A7_selection_response();
                if (response_ret <= 0)
                {
                    sock_status = -1;
                }

                break;
            }
        }
        else if (-2 == sock_status)
        {
            //timeout
            continue;
        }
        else
        {
            MEDIA_PRINTF_E("recv selection response error.\n");
            break;
        }
    }

    return sock_status;
}

int A7Client::handle_A7_selection_response()
{
    BSTM_DEBUG_ENTER();
    char *ps = NULL;
	char purchaseToken[SELECTIONSTART_RESPONSE_LEN];

	memset(purchaseToken, 0, sizeof(char)*SELECTIONSTART_RESPONSE_LEN);

    ps = (char*)strstr((char*)a7_data.Res_Buf.c_str(), "purchaseToken=");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain \"purchaseToken=\".\n");
        return -1;
    }

	ps += strlen("purchaseToken=");

	//skip "
	if ('"' == *ps)
	{
		ps++;
	}

	for (int i = 0; i < SELECTIONSTART_RESPONSE_LEN; i++)
	{
		if ((ps[i] != '"') && (ps[i] != ';') && ps[i] != ' ' && ps[i] != '\0' && ps[i] != '\r' && ps[i] != '\n')
		{
			purchaseToken[i] = ps[i];
		}
        else
        {
            break;
        }
	}

	set_purchaseToken(purchaseToken);

	MEDIA_PRINTF_I("\"purchaseToken=%s\".\n", get_purchaseToken().c_str());

    return 1;
}

