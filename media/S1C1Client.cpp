/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     S1C1Client.cpp
* @version  1.0
* @brief
* @author   hejun
* @date     2017.04.20
* @note
**/

#include <map>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h> 
#include <memory.h>
#include "S1C1Client.h"
#include "mediaInterface.h"

BSTM_DEBUG_DEFINE_MODULE("bstmapp.itv.media");

#define REQUEST_BUF (1024)
#define IP_NUM (20)
#define SESSION_NUM (20)
#define TOTAL_TIME_NUM (20)
#define ONDEMANDSESSIONID_NUM (100)
#define PARAM_TYPE_CONTENT_LEN (1024)
#define DEFAULT_MEDIA_PLAY_SYMBOL (6875)

const int default_paly_range = 0;
const int default_play_scale = 1;
const static char* g_seperator = "\r\n";

const static char* g_UserAgent = "User-Agent: Enseo HD-1000";
const static char* g_Version = "Tianshan-Version: 1.0";
const static char* g_RtspUrl = "RtspUrl: %s";
const static char* g_QamName = "QamName: %s";
const static char* g_ClientSessionId = "ClientSessionId: %s";
const static char* g_Require = "Require: %s";
const static char* g_Transport = "Transport: %s";
const static char* g_Cseq = "CSeq: %d";
const static char* g_Range = "Range: npt=%.3f-";
const static char* g_Scale = "Scale: %d";
const static char* g_Session = "Session: %s";
const static char* g_Reason = "Reason: %s";
const static char* g_OnDemandSessionId = "OnDemandSessionId: %s";
const static char* g_Content_Type = "Content-Type: %s";
const static char* g_Conten_Length = "Content-Length: %d";

const static char* g_param_type_Connection_Timeout = "connection_timeout";
const static char* g_param_type_Session_List = "session_list";
const static char* g_param_type_Position = "position";
const static char* g_param_type_Presentation_State = "presentation_state";
const static char* g_param_type_Scale = "scale";
const static char* g_param_type_Connection_Timeout_com = "connection_timeout:";
const static char* g_param_type_Session_List_com = "session_list:";
const static char* g_param_type_Position_com = "position:";
const static char* g_param_type_Presentation_State_com = "presentation_state:";
const static char* g_param_type_Scale_com = "scale:";
const static char* g_ClientSessionId_value = "26400002310264368262";
const static char* g_Require_S1_value = "com.comcast.ngod.s1";
const static char* g_Require_C1_value = "com.comcast.ngod.c1";
const static char* g_Transport_value = "MP2T/DVBC/QAM;unicast;client=17 3230 1675 89;";
const static char* g_Reason_value = "200 \"user pressed stop\"";
const static char* g_Content_Type_value = "text/parameters";
const static char* g_response_code_ok = "200 OK";

const static char* g_server_id = "192.168.1.202";

//extern MEDIA_STATUS_E g_media_status;
//extern std::map < C_MediaHandle, MEDIA_STATUS_E > g_handler_media_status;

int S1C1Client::init_data(const C_U8* url)
{
	int ret;
	MEDIA_PRINTF_I("url=%s.\n", (char*)url);
    s1c1_data.S1_SocketID = -1;
    s1c1_data.C1_SocketID = -1;
    s1c1_data.C1_Req_Buf = "";
    s1c1_data.C1_Res_Buf = "";
    s1c1_data.S1_Req_Buf = "";
    s1c1_data.S1_Res_Buf = "";
    s1c1_data.S1_PING_Req_Buf= "";
    s1c1_data.C1_PING_Req_Buf= "";

    setup_req.SETUP = "";
    setup_req.Cseq = "";
    setup_req.ClientSessionId = "";
    setup_req.Require = "";
    setup_req.Transport = "";

    teardown_req.ClientSessionId = "";
    teardown_req.Cseq = "";
    teardown_req.OnDemandSessionId = "";
    teardown_req.Reason = "";
    teardown_req.Require = "";
    teardown_req.Session = "";
    teardown_req.TEARDOWN = "";

    c1_req.Cseq = "";
    c1_req.C1_URL = "";
    c1_req.Range = "";
    c1_req.Require = "";
    c1_req.Scale = "";
    c1_req.Session = "";

    getPara_req.Content_Length = "";
    getPara_req.Content_Type = "";
    getPara_req.Cseq = "";
    getPara_req.Get_Parameter = "";
    getPara_req.Parameter_Type = "";
    getPara_req.Require = "";
    getPara_req.Session = "";

    s1c1_res.MediaStatusType = MediaStatusType_PLAY;
    s1c1_res.OnDemandSessionId = "";
    s1c1_res.Session = "";
    s1c1_res.TotalTime = 0;
    s1c1_res.uCurrentTime = 0;
    s1c1_res.uPlayScale = 1;
    
    is_c1_thread_exit = false;
    is_s1_thread_exit = false;

    url_t.PurchaseToken = "";
    url_t.Server_Name = "";
    url_t.Server_Port = 0;
    url_t.Server_ID = g_server_id;

    err_code = Media_ServerErr;
//    setup_cseq = 1;
//    teardown_cseq = 1;
	s1_operation_cseq = 0;
    s1_ping_cseq = 0;
    //play_cseq = 1;
    //getparam_cseq = 1;
	c1_operation_cseq = 0;
    c1_ping_cseq = 0;

    c1_ping_time_out = C1_PING_TIMEOUT;
    s1_ping_time_out = S1_PING_TIMEOUT;

    //g_media_status = MEDIA_NON_SETUP;
    media_status = MEDIA_NON_SETUP;
    //g_handler_media_status.insert(std::pair< C_MediaHandle, MEDIA_STATUS_E >(this, MEDIA_NON_SETUP));

	pthread_mutex_init(&c1_ping_sleep_time_mutex, NULL);

	MEDIA_PRINTF_I("url=%s.\n", (char*)url);

	std::string rtsp_url = UrlDecode((char*)url);
	MEDIA_PRINTF_I("after decode,rtsp url=%s.\n", rtsp_url.c_str());

	ret = parse_url(rtsp_url.c_str());
	if (ret < 0)
	{
		MEDIA_PRINTF_E("parse url failed!\n");
		return -1;
	}

    ret = save_url(rtsp_url.c_str());
	if (ret < 0)
	{
		MEDIA_PRINTF_E("save url failed!\n");
		return -1;
	}
    set_ip_addr();
	//set_purchaseToken(a7_client);
    ret = set_ip_port_by_url(rtsp_url.c_str());
	if (ret < 0)
	{
		MEDIA_PRINTF_E("parse url failed!\n");
		return -1;
	}
    
    return 0;
}

std::string S1C1Client::get_value_by_key(std::string text, std::string key)
{
	std::size_t begin;
	std::size_t end;
	std::size_t head;
	std::size_t len;
	
	begin = text.find(key);
	if (std::string::npos == begin)
	{
		MEDIA_PRINTF_D("%s is not found!\n", key.c_str());
		return "not found";
	}
	
	head = begin + strlen((char *)(key.c_str()));
	
	end = text.find("&", head);
	if (std::string::npos == end)
	{
		end = text.find(";", head);
		if (std::string::npos == end)
		{
			MEDIA_PRINTF_D("& or ; is not found after %s!\n", key.c_str());
			return text.substr(head);
		}
		else
		{
			len = end - head;
			return text.substr(head, len);
		}
	}
	else
	{
		len = end - head;
		return text.substr(head, len);
	}
}


int S1C1Client::parse_url(const char* url)
{
	std::string rtsp_url = (char*)url;
	MEDIA_PRINTF_I("rtsp url=%s.\n", rtsp_url.c_str());
	std::string value;

	//set url
	setup_req.RtspUrl = "";
	setup_req.RtspUrl += "RtspUrl: ";	

	//add rtspurl to url
	value = "";
	value = get_value_by_key(rtsp_url, "rtspurl=");	
	if (std::string::npos != value.find("not found"))
	{		
		value = get_value_by_key(rtsp_url, "rtspURL=");	
		if (std::string::npos != value.find("not found"))
		{
			MEDIA_PRINTF_E("rtspurl or rtspURL is not found");
			return -1;
		}
	}
	setup_req.RtspUrl += value;	
	MEDIA_PRINTF_I("RtspUrl=%s.\n", setup_req.RtspUrl.c_str());

	url_t.Rtsp_Url = "";
	url_t.Rtsp_Url += value;
	//add '?' after url
	setup_req.RtspUrl += "?";
	url_t.Rtsp_Url += "?";

	//add asset to url
	setup_req.RtspUrl += "asset=";
	url_t.Rtsp_Url += "asset=";

	value = "";
	value = get_value_by_key(rtsp_url, "asset=");
	if (std::string::npos != value.find("not found"))
	{
		MEDIA_PRINTF_E("asset is not found");
		return -1;
	}	
	setup_req.RtspUrl += value;	
	url_t.Rtsp_Url += value;
	MEDIA_PRINTF_I("RtspUrl=%s.\n", setup_req.RtspUrl.c_str());
	

	
	value = "";
	value = get_value_by_key(rtsp_url, "folderid=");
	if (std::string::npos == value.find("not found"))
	{
		//add '&' after url
		setup_req.RtspUrl += "&";
		url_t.Rtsp_Url += "&";
		MEDIA_PRINTF_D("value.c_str() = %s\n",value.c_str());
		//add folderId to url
		setup_req.RtspUrl += "folderId=";
		url_t.Rtsp_Url += "folderId=";
		setup_req.RtspUrl += value;	
		url_t.Rtsp_Url += value;
		MEDIA_PRINTF_D("RtspUrl=%s.\n", setup_req.RtspUrl.c_str());
	}

	//set Transport
	setup_req.Transport = "";
	setup_req.Transport += "Transport: ";
	
	value = "";
	value = get_value_by_key(rtsp_url, "transport=");	
	if (std::string::npos != value.find("not found"))
	{
		MEDIA_PRINTF_E("transport is not found");
		return -1;
	}
	setup_req.Transport += value;

	//set ServiceGroup
	setup_req.ServiceGroup = "";
	setup_req.ServiceGroup += "TianShan-ServiceGroup: ";
	value = get_value_by_key(rtsp_url, "servicegroup=");
	if (std::string::npos != value.find("not found"))
	{
		MEDIA_PRINTF_E("servicegroup is not found");
		return -1;
	}	
	setup_req.ServiceGroup += value;

	//set AppData
	setup_req.AppData = "";
	setup_req.AppData += "TianShan-AppData: ";
	
	//add smartcard-id to AppData
	setup_req.AppData += "smartcard-id=";
	value = get_value_by_key(rtsp_url, "smartcardid=");
	if (std::string::npos != value.find("not found"))
	{
		MEDIA_PRINTF_E("smartcardid is not found");
		return -1;
	}	
	setup_req.AppData += value;

	//add ";" 
	setup_req.AppData += ";";

	//add device-id to AppData
	setup_req.AppData += "device-id=";
	value = get_value_by_key(rtsp_url, "deviceid=");
	if (std::string::npos != value.find("not found"))
	{
		MEDIA_PRINTF_E("deviceid is not found");
		return -1;
	}
	setup_req.AppData += value;

	//add ";" 
	setup_req.AppData += ";";

	//add home-id to AppData
	setup_req.AppData += "home-id=";
	value = get_value_by_key(rtsp_url, "homeid=");
	if (std::string::npos != value.find("not found"))
	{
		MEDIA_PRINTF_E("homeid is not found");
		return -1;
	}
	setup_req.AppData += value;
	
	//add ";" 
	setup_req.AppData += ";";

	value = get_value_by_key(rtsp_url, "purchaseid=");
	if (std::string::npos == value.find("not found"))
	{
		MEDIA_PRINTF_D("value.c_str() = %s\n",value.c_str());
		//add purchase-id to AppData
		setup_req.AppData += "purchase-id=";
		setup_req.AppData += value;
		//add ";" 
		setup_req.AppData += ";";
	}
	

	return 0;
}


int S1C1Client::save_url(const char* url)
{
	MEDIA_PRINTF_I("url=%s.\n", (char*)url);
    memset(mediaInfo_res.PlayUrl, 0, sizeof(C_U8)*MAX_INFO_LEN);
	if (strlen((char*)url) > MAX_INFO_LEN)
	{
		MEDIA_PRINTF_E("length of url is longer than %d.\n", (int)MAX_INFO_LEN);
		return -1;
	}
    memcpy(mediaInfo_res.PlayUrl, url, strlen((char*)url));
	/*
    s1c1_data.param_url = "";
    s1c1_data.param_url = (char*)url;
	MEDIA_PRINTF_I("s1c1_data.param_url=%s.\n", s1c1_data.param_url.c_str());
	*/
	return 0;
}

int S1C1Client::create_S1_client()
{
    BSTM_DEBUG_ENTER();
    int sockId;
        
    sockId = create_client_socket(ip_addr.S1_Port, ip_addr.GH_IP.c_str());
    s1c1_data.S1_SocketID = sockId;
    if (-1 == sockId)
    {
        MEDIA_PRINTF_E("create S1 client failed! \n");

		//set_err_code(Media_ServerInvalid);
        return -1;
    }

    MEDIA_PRINTF_I("create S1 socket: %d. \n", sockId);
    return sockId;
}

int S1C1Client::send_setup_request()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

	set_s1_recv_msg_status(s1_operation_cseq, RECV_STATUS_WAITING);

	MEDIA_PRINTF_I("send SETUP to S1, buffer = %s \n", s1c1_data.S1_Req_Buf.c_str());
    data_len = send_client_request(s1c1_data.S1_SocketID, s1c1_data.S1_Req_Buf.c_str());
    
    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send SETUP to S1 client failed! \n");
		s1_recv_msg_status.erase(s1_operation_cseq);
        //set_err_code(Media_ServerInvalid);
    }

    return data_len;
}

int S1C1Client::recv_setup_response()
{
    BSTM_DEBUG_ENTER();
    int ret;
    int data_len = -1;
    
    data_len = recv_client_response(s1c1_data.S1_SocketID, s1c1_data.S1_Res_Buf, callback_timer);
    if (data_len <= 0)
    {
        MEDIA_PRINTF_E("recv SETUP response error.\n");
		
        //set_err_code(Media_ServerInvalid);
    }
    else
    {
        ret = handle_setup_response(s1c1_data.S1_Res_Buf);
        if (-1 == ret)
        {
            MEDIA_PRINTF_D("res_buf = %s.\n", s1c1_data.S1_Res_Buf.c_str());
			
			//set_err_code(Media_ServerInvalid);
			return ret;
        }
    }

    return data_len;
}

int S1C1Client::handle_setup_response(std::string &setup_str)
{
    BSTM_DEBUG_ENTER();
    char *ps = NULL;
    bool is_digit_found = false;
    char Session[SESSION_NUM];
    //char OnDemandSessionId[ONDEMANDSESSIONID_NUM];

    //Session
    memset(Session, 0, sizeof(char)*SESSION_NUM);

    ps = (char*)strstr((char*)setup_str.c_str(), g_response_code_ok);
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain \"200 OK\".\n");
        return -1;
    }

    ps = NULL;
    ps = (char*)strstr((char*)setup_str.c_str(), "Session:");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain Session.\n");
        return -1;
    }

    ps += strlen("Session:");

    //skip ' ', find first NON ' '
    while (' ' == *ps)
    {
        ps++;
    }
    for (int i = 0; i < SESSION_NUM; i++)
    {
        if ((ps[i] != ';') && ps[i] != ' ' && ps[i] != '\0' && ps[i] != '\r' && ps[i] != '\n')
        {
            Session[i] = ps[i];
        }
        else
        {
            break;
        }
    }

#if 0
    //OnDemandSessionId
    memset(OnDemandSessionId, 0, sizeof(char)*ONDEMANDSESSIONID_NUM);

    ps = (char*)strstr((char*)setup_str.c_str(), "OnDemandSessionId:");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain OnDemandSessionId.\n");
        return -1;
    }

    ps += strlen("OnDemandSessionId:");

    //skip ' ', find first NON ' '
    while (' ' == *ps)
    {
        ps++;
    }
    for (int i = 0; i < ONDEMANDSESSIONID_NUM; i++)
    {
        if ((ps[i] != ';') && ps[i] != ' ' && ps[i] != '\0' && ps[i] != '\r' && ps[i] != '\n')
        {
            OnDemandSessionId[i] = ps[i];
        }
        else
        {
            break;
        }
    }
#endif
	
#if 0
    //Transport
    ps = (char*)strstr((char*)setup_str.c_str(), "Transport:");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain Transport.\n");
        return -1;
    }

    ps += strlen("Transport");

    //find destination
    while ((*ps != '\r') && (*ps != '\n') && (*ps != '\0'))
    {
        if (strncmp(ps, "destination=", strlen("destination=")) == 0)
        {
            ps += strlen("destination=");
            break;
        }
        ps++;
    }
    if (('\r' == *ps) || ('\n' == *ps) || ('\0' == *ps))
    {
        MEDIA_PRINTF_E("Transport don't contain destination.\n");
        return -1;
    }
    //skip ' ', find first NON ' '
    while (' ' == *ps)
    {
        ps++;
    }
    //set frequency
    s1c1_res.prog_info.freq = 0;
    while (isdigit(*ps) && ('\0' != *ps))
    {
        s1c1_res.prog_info.freq = s1c1_res.prog_info.freq * 10;
        s1c1_res.prog_info.freq += (*ps - '0');
        ps++;
        is_digit_found = true;
    }
	s1c1_res.prog_info.freq = s1c1_res.prog_info.freq/1000;
    if (false == is_digit_found)
    {
        MEDIA_PRINTF_E("destination don't contain valid value.\n");
        return -1;
    }
    //set serviceID
    s1c1_res.prog_info.serviceId = 0;
    if ('.' != *ps)
    {
        MEDIA_PRINTF_E("destination don't contain valid value.\n");
        return -1;
    }
    ps++; //skip '.'

    is_digit_found = false;
    while (isdigit(*ps) && ('\0' != *ps))
    {
        s1c1_res.prog_info.serviceId = s1c1_res.prog_info.serviceId * 10;
        s1c1_res.prog_info.serviceId += (*ps - '0');
        ps++;
        is_digit_found = true;
    }

    if (false == is_digit_found)
    {
        MEDIA_PRINTF_E("destination don't contain valid value.\n");
        return -1;
    }
#else
	//TianShan-Transport
	std::string value;

	//set program-number
	value = "";
	value = get_value_by_key(setup_str, "program-number=");	
	if (value.empty())
	{
        MEDIA_PRINTF_E("res_buf don't contain program-number=.\n");
		return -1;
	}
	s1c1_res.prog_info.serviceId = (unsigned short)atoi((char*)value.c_str());

	//set frequency
	value = "";
	value = get_value_by_key(setup_str, "frequency=");	
	if (value.empty())
	{
        MEDIA_PRINTF_E("res_buf don't contain frequency=.\n");
		return -1;
	}
	s1c1_res.prog_info.freq = (unsigned int)atoi((char*)value.c_str());

#endif
    
    s1c1_res.Session = Session;
    //s1c1_res.OnDemandSessionId = OnDemandSessionId;

    MEDIA_PRINTF_I("Session=%s, freq=%d, serviceID=%d\n", Session, s1c1_res.prog_info.freq, s1c1_res.prog_info.serviceId);

    return 0;
}

int S1C1Client::send_s1_heartBeat_request()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

	set_s1_ping_status(s1_ping_cseq, RECV_STATUS_WAITING);

    MEDIA_PRINTF_I("send PING to S1, buffer = %s \n", s1c1_data.S1_PING_Req_Buf.c_str());
    data_len = send_client_request(s1c1_data.S1_SocketID, s1c1_data.S1_PING_Req_Buf.c_str());

    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send PING to S1 client failed! \n");
		s1_ping_status.erase(s1_ping_cseq);
        //callback(this, MediaNotifyType_Other, 0, Media_ServerInvalid);
    }


    return data_len;

}

/*
int RTSPS1C1Client::recv_s1_heartBeat_response()
{
    BSTM_DEBUG_ENTER();
    int ret;
    int data_len = -1;

    data_len = recv_rtsp_response(s1c1_data.S1_SocketID, s1c1_data.S1_Ping_Res_Buf, callback_timer);
    if (data_len <= 0)
    {
        MY_PRINTF_E("recv PING response error.\n");
        //callback(this, MediaNotifyType_Other, 0, Media_ServerInvalid);
    }
    else
    {
        ret = handle_RTSP_ping_response();
        if (-1 == ret)
        {
            return ret;
        }
    }

    return data_len;
}*/

/*
int RTSPS1C1Client::handle_RTSP_ping_response()
{
    BSTM_DEBUG_ENTER();
    char *ps = NULL;
    
    ps = (char*)strstr((char*)s1c1_data.S1_Ping_Res_Buf.c_str(), g_rtsp_response_code_ok);
    if (NULL == ps)
    {
        MY_PRINTF_E("res_buf don't contain \"200 OK\".\n");
        callback(this, MediaNotifyType_Other, 0, Media_ServerInvalid);
        return -1;
    }

    return 0;
}*/

int S1C1Client::create_C1_client()
{
    BSTM_DEBUG_ENTER();
    int sockId;

    sockId = create_client_socket(ip_addr.C1_Port, ip_addr.GH_IP.c_str());
    s1c1_data.C1_SocketID = sockId;
    if (-1 == sockId)
    {
        MEDIA_PRINTF_E("create C1 client failed! \n");
		
		//set_err_code(Media_ServerInvalid);
    }

    MEDIA_PRINTF_I("create C1 socket: %d. \n", sockId);
    return sockId;
}

int S1C1Client::send_C1_request()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

    set_c1_recv_msg_status(c1_operation_cseq,RECV_STATUS_WAITING);

    MEDIA_PRINTF_I("send request to C1, buffer = %s \n", s1c1_data.C1_Req_Buf.c_str());
    data_len = send_client_request(s1c1_data.C1_SocketID, s1c1_data.C1_Req_Buf.c_str());

    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send request to C1 client failed! \n");
		c1_recv_msg_status.erase(c1_operation_cseq);
		//set_err_code(Media_ServerInvalid);
    }


    return data_len;
}

int S1C1Client::wait_C1_response()
{
    BSTM_DEBUG_ENTER();
    int ret = 0;
    C_U32 counter = 0;

    while (RECV_STATUS_WAITING == get_c1_recv_msg_status(c1_operation_cseq))
    {
        if (counter >= callback_timer / 50)
        {
            break;
        }
        counter++;
        usleep(50*1000);
    }
    if (RECV_STATUS_WAITING == get_c1_recv_msg_status(c1_operation_cseq))
    {
        //timeout, no data received
        MEDIA_PRINTF_D("seq is %d,  timeout\n\n", c1_operation_cseq);
        ret = -2;
    }
    else if (RECV_STATUS_FAULT == get_c1_recv_msg_status(c1_operation_cseq))
    {
        //received incorrect data
        MEDIA_PRINTF_D("seq is %d, error\n\n", c1_operation_cseq);
        ret = -1;
    }
    else if (RECV_STATUS_OK == get_c1_recv_msg_status(c1_operation_cseq))
    {
        MEDIA_PRINTF_D("seq is %d, data received. OK\n\n", c1_operation_cseq);
        ret = 1;
    }	
    
    c1_recv_msg_status.erase(c1_operation_cseq);
    return ret;
}

int S1C1Client::wait_C1_ping_response()
{
    BSTM_DEBUG_ENTER();
    int ret = 0;
    int counter = 0;

    while (RECV_STATUS_WAITING == get_c1_ping_status(c1_ping_cseq))
    {
        if (counter >= WAIT_PING_RESPONSE_TIMER / 50)
        {
            break;
        }
        counter++;
        usleep(50*1000);
    }

    if (RECV_STATUS_WAITING == get_c1_ping_status(c1_ping_cseq))
    {
        //timeout, no data received
        MEDIA_PRINTF_D("seq is %d, timeout\n\n", c1_ping_cseq);
        ret = -2;
    }
    else if (RECV_STATUS_FAULT == get_c1_ping_status(c1_ping_cseq))
    {
        //received incorrect data
        MEDIA_PRINTF_D("seq is %d, error\n\n", c1_ping_cseq);
        ret = -1;
    }
    else if (RECV_STATUS_OK == get_c1_ping_status(c1_ping_cseq))
    {
        MEDIA_PRINTF_D("seq is %d, data received. OK\n\n", c1_ping_cseq);
        ret = 1;
    }

    c1_ping_status.erase(c1_ping_cseq);
    return ret;
}

int S1C1Client::recv_default_play_response()
{
    BSTM_DEBUG_ENTER();
    int ret;
    int data_len = -1;

    data_len = recv_client_response(s1c1_data.C1_SocketID, s1c1_data.C1_Res_Buf, callback_timer);
    if (data_len <= 0)
    {
        MEDIA_PRINTF_E("recv C1 response error.\n");
		
		//set_err_code(Media_ServerInvalid);
    }
    else
    {
        ret = handle_play_response();
        if (-1 == ret)
        {
            MEDIA_PRINTF_D("res_buf = %s.\n", s1c1_data.C1_Res_Buf.c_str());

			//set_err_code(Media_ServerInvalid);
            return ret;
        }
    }

    return data_len;
}

int S1C1Client::handle_play_response()
{
    BSTM_DEBUG_ENTER();
    //check the range, and get the total time, and then set mediaInfo_res
    char *ps = NULL;
    bool is_digit_found = false;
    mediaInfo_res.uTotleTime = 0;

    ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_response_code_ok);
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain \"200 OK\".\n");
        return -1;
    }
    ps = NULL;

    ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), "Range: npt=");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("Range don't contain Range: npt.\n");
        return -1;
    }
    ps += strlen("Range: npt=");

    //find '-'
    while ((*ps != ';') && *ps != '\0' && *ps != '\r' && *ps != '\n')
    {
        if ('-' == *ps)
        {
            break;
        }
        else
        {
            ps++;
        }
    }

    if ('-' == *ps)
    {
        ps++;
        while (isdigit(*ps) && ('\0' != *ps))
        {
            mediaInfo_res.uTotleTime = mediaInfo_res.uTotleTime * 10;
            mediaInfo_res.uTotleTime += (*ps - '0');
            ps++;
            is_digit_found = true;
        }
        
        if (false == is_digit_found)
        {
        	//set_err_code(Media_ServerInvalid);
            MEDIA_PRINTF_D("Range don't contain total time.\n");
			mediaInfo_res.uTotleTime = 0;
            return 0;
        }
    }
	
    MEDIA_PRINTF_I("uTotleTime=%d\n", mediaInfo_res.uTotleTime);
    return 0;
}


int S1C1Client::recv_C1_Msg()
{
    int sock_status = -1;
    std::string recv_buf;

    sock_status = recv_client_response(s1c1_data.C1_SocketID, recv_buf, 1);
    
    //if (MEDIA_CLOSE == g_media_status)
    /*if (MEDIA_CLOSE == g_is_handler_media_closed[(C_MediaHandle)this])
    {
    	BSTM_DEBUG_INFO("recv_RTSP_C1_Msg(): media is closed.\n");
        return -1;
    }*/

    if (sock_status > 0)
    {
        s1c1_data.C1_Res_Buf += recv_buf;

        if ((char*)strstr(s1c1_data.C1_Res_Buf.c_str(), "\r\n\r\n") != NULL)
        {
            handle_C1_Msg();
            s1c1_data.C1_Res_Buf.clear();
        }
    }

    return sock_status;
}


void S1C1Client::handle_C1_Msg()
{
    BSTM_DEBUG_ENTER();
    char* ps = NULL;
	int ret;
	
    //get the cseq
    int seq = get_cseq_value(s1c1_data.C1_Res_Buf);
	if (seq <= 0)
	{
    	MEDIA_PRINTF_E("res_buf don't contain CSeq.res_buf=%s\n",s1c1_data.C1_Res_Buf.c_str());
        
        return;		
	}

    //PLAY
    if ((char*)strstr(s1c1_data.C1_Res_Buf.c_str(), "PLAY") != NULL)
    {
    	ret = handle_play_response();
		if (-1 == ret)
		{
			set_c1_recv_msg_status(seq, RECV_STATUS_FAULT);
            return;
		}
        set_c1_recv_msg_status(seq, RECV_STATUS_OK);
    }
	//PAUSE
	else if ((char*)strstr(s1c1_data.C1_Res_Buf.c_str(), "PAUSE") != NULL)
	{
        ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_response_code_ok);
        if (NULL == ps)
        {
        	MEDIA_PRINTF_E("GET_PARAMETER don't contain \"200 OK\".\n");
			
            //set_err_code(Media_ServerInvalid);
			set_c1_recv_msg_status(seq, RECV_STATUS_FAULT);
            return;
        }
        set_c1_recv_msg_status(seq, RECV_STATUS_OK);
	}
    //GET_PARAMETER
    else if (((char*)strstr(s1c1_data.C1_Res_Buf.c_str(), "text/parameters") != NULL)
		|| ((char*)strstr(s1c1_data.C1_Res_Buf.c_str(), "GET_PARAMETER") != NULL)
		)
    {
        ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_response_code_ok);
        if (NULL == ps)
        {
        	MEDIA_PRINTF_E("GET_PARAMETER don't contain \"200 OK\".\n");
			
            //set_err_code(Media_ServerInvalid);
			set_c1_recv_msg_status(seq, RECV_STATUS_FAULT);
            return;
        }

        ps = NULL;
        ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_param_type_Position_com);
        if (NULL == ps)
        {
            ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_param_type_Presentation_State_com);
            if (NULL == ps)
            {
                ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_param_type_Scale_com);
                if (NULL == ps)
                {
                    //the response of get_parameter connect_timeout don;t contains "connection_timeout"
                    //only check the "200 OK"                    
                    ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), g_response_code_ok);
                    if (NULL == ps)
                    {
                        MEDIA_PRINTF_E("res_buf don't contain valid GET_PARAMETER info.\n");
                        
                        return;
                    }
                    else
                    {
                        //the response of get_parameter connect_timeout don;t contains "connection_timeout", so can't set connect_timeout
                        //handle_RTSP_connectTimeout_response();

                        //only for C1 heart beat
                        set_c1_ping_status(seq, RECV_STATUS_OK);
                    }
                    
                }
                else
                {
                    //save scale value
                    int ret;
                    ret = set_getparam_scale_value();
                    if (-1 == ret)
                    {
						//set_err_code(Media_ServerInvalid);
						set_c1_recv_msg_status(seq, RECV_STATUS_FAULT);
                        return;
                    }
                }
            }
            else
            {
                //save Presentation_State value
                int ret;
                //ret = set_getparam_position_value();
                ret = set_getparam_presentation_state_value();
                if (-1 == ret)
                {
					//set_err_code(Media_ServerInvalid);
					set_c1_recv_msg_status(seq, RECV_STATUS_FAULT);
                    return;
                }
            }
        }
        else
        {
            //save position value
            int ret;
            //ret = set_getparam_presentation_state_value();
            ret = set_getparam_position_value();
            if (-1 == ret)
            {
				//set_err_code(Media_ServerInvalid);
				set_c1_recv_msg_status(seq, RECV_STATUS_FAULT);
                return;
            }
        }
        set_c1_recv_msg_status(seq, RECV_STATUS_OK);
    }
    //ANNOUNCE
    else if ((char*)strstr(s1c1_data.C1_Res_Buf.c_str(), "ANNOUNCE") != NULL)
    {
        ps = (char*)strstr((char*)s1c1_data.C1_Res_Buf.c_str(), "Notice:");
        if (NULL == ps)
        {
        	MEDIA_PRINTF_E("res_buf don't contain valid ANNOUNCE info.\n");
        }
        else
        {
            ps += strlen("Notice: ");
			 
            if (NULL != (strstr(ps, "End-of-Stream Reached")))				
            {
                int ret;

				set_media_status(MEDIA_STOP);
                callback(this, MediaNotifyType_ReachEnd, 0, Media_Ok);
                MEDIA_PRINTF_D("send callback done!.\n");
				
                //build response
                build_c1_announce_response(s1c1_data.C1_Res_Buf.c_str());
                //send response
                ret = send_C1_announce_response();
                if (0 >= ret)
                {
                	MEDIA_PRINTF_E("send announce response failed.\n");
                }
                else
                {
                    MEDIA_PRINTF_D("sent ANNOUNCE MediaNotifyType_ReachEnd response.\n");
                }
            }
            else if ((NULL != (strstr(ps, "Start-of-Stream Reached"))) || (NULL != (strstr(ps, "Beginning-of-Stream Reached"))))				
            {
                int ret;
				
				set_media_status(MEDIA_STOP);
                callback(this, MediaNotifyType_ReachBegin, 0, Media_Ok);
                MEDIA_PRINTF_D("send callback done!.\n");
				
                //build response
                build_c1_announce_response(s1c1_data.C1_Res_Buf.c_str());
                //send response
                ret = send_C1_announce_response();
                if (0 >= ret)
                {
                    MEDIA_PRINTF_E("send announce response failed.\n");
                }
                else
                {
                    MEDIA_PRINTF_D("sent ANNOUNCE MediaNotifyType_ReachBegin response.\n");
                }
            }
			else if (NULL != (strstr(ps, "Client Session Terminated")))				
            {
                int ret;

				set_media_status(MEDIA_STOP);
                callback(this, MediaNotifyType_Other, 0, Media_ServerInvalid);
                MEDIA_PRINTF_D("send callback done!.\n");
				
                //build response
                build_c1_announce_response(s1c1_data.C1_Res_Buf.c_str());
                //send response
                ret = send_C1_announce_response();
                if (0 >= ret)
                {
                    MEDIA_PRINTF_E("send announce response failed.\n");
                }
                else
                {
                    MEDIA_PRINTF_D("sent ANNOUNCE MediaNotifyType_Other response.\n");
                }
                return;
            }
            //else if other value
            else
            {
	            //build response
	            build_c1_announce_response(s1c1_data.C1_Res_Buf.c_str());
	            //send response
	            ret = send_C1_announce_response();
	            if (0 >= ret)
	            {
	            	MEDIA_PRINTF_E("send announce response failed.\n");
	            }
	            else
	            {
	                MEDIA_PRINTF_D("sent ANNOUNCE response.\n");
	            }                
            }
			
        }
    }
    else
    {
    	MEDIA_PRINTF_E("res_buf don't contain any valid info.res_buf=%s\n",s1c1_data.C1_Res_Buf.c_str());
        
        //set_err_code(Media_ServerInvalid);
        return;
    }
    return;
}

int S1C1Client::recv_S1_Msg()
{
    int sock_status = -1;

    std::string recv_buf;
    sock_status = recv_client_response(s1c1_data.S1_SocketID, recv_buf, 1);

    //if (MEDIA_CLOSE == g_media_status)
    /*if (MEDIA_CLOSE == g_is_handler_media_closed[(C_MediaHandle)this])
    {
        return -1;
    }*/

    if (sock_status > 0)
    {
        s1c1_data.S1_Res_Buf += recv_buf;

        if ((char*)strstr(s1c1_data.S1_Res_Buf.c_str(), "\r\n\r\n") != NULL)
        {
            handle_S1_Msg();
            s1c1_data.S1_Res_Buf.clear();
        }
    }

    return sock_status;

}

void S1C1Client::handle_S1_Msg()
{
    BSTM_DEBUG_ENTER();
    char* ps = NULL;

    //get the cseq
    int seq = get_cseq_value(s1c1_data.S1_Res_Buf);
	if (seq <= 0)
	{
    	MEDIA_PRINTF_E("res_buf don't contain CSeq.res_buf=%s\n",s1c1_data.C1_Res_Buf.c_str());
        
        return;		
	}

    //ANNOUNCE
    if ((char*)strstr(s1c1_data.S1_Res_Buf.c_str(), "ANNOUNCE") != NULL)
    {
        ps = (char*)strstr((char*)s1c1_data.S1_Res_Buf.c_str(), "Notice:");
        if (NULL == ps)
        {
        	MEDIA_PRINTF_E("res_buf don't contain valid ANNOUNCE info.\n");

            return;
        }
        else
        {
            ps += strlen("Notice: ");
            if (NULL != (strstr(ps, "Client Session Terminated")))
            {
                int ret;
				
				set_media_status(MEDIA_STOP);
                callback(this, MediaNotifyType_Other, 0, Media_ServerInvalid);
                MEDIA_PRINTF_D("send callback done!.\n");
                //build response
                build_s1_announce_response(s1c1_data.S1_Res_Buf.c_str());
                //send response
                ret = send_S1_announce_response();
                if (0 >= ret)
                {
                    MEDIA_PRINTF_E("send announce MediaNotifyType_Other response failed.\n");
                }

                return;
            }
			else if (NULL != (strstr(ps, "End-of-Stream Reached")))				
            {
                int ret;
				
				set_media_status(MEDIA_STOP);
                callback(this, MediaNotifyType_ReachEnd, 0, Media_Ok);
                MEDIA_PRINTF_D("send callback done!.\n");
				
                //build response
                build_s1_announce_response(s1c1_data.S1_Res_Buf.c_str());
                //send response
                ret = send_S1_announce_response();
                if (0 >= ret)
                {
                	MEDIA_PRINTF_E("send announce response failed.\n");
                }
                else
                {
                    MEDIA_PRINTF_D("sent ANNOUNCE MediaNotifyType_ReachEnd response.\n");
                }
            }
            else if ((NULL != (strstr(ps, "Start-of-Stream Reached"))) || (NULL != (strstr(ps, "Beginning-of-Stream Reached"))))				
            {
                int ret;
				
				set_media_status(MEDIA_STOP);
                callback(this, MediaNotifyType_ReachBegin, 0, Media_Ok);
                MEDIA_PRINTF_D("send callback done!.\n");
				
                //build response
                build_s1_announce_response(s1c1_data.S1_Res_Buf.c_str());
                //send response
                ret = send_S1_announce_response();
                if (0 >= ret)
                {
                    MEDIA_PRINTF_E("send announce response failed.\n");
                }
                else
                {
                    MEDIA_PRINTF_D("sent ANNOUNCE MediaNotifyType_ReachBegin response.\n");
                }
            }
            //else if other value
            else
            {
                int ret;
                //build response
                build_s1_announce_response(s1c1_data.S1_Res_Buf.c_str());
                //send response
                ret = send_S1_announce_response();
                if (0 >= ret)
                {
                    MEDIA_PRINTF_E("send announce response failed.\n");
                }
                else
                {
                    MEDIA_PRINTF_D("sent ANNOUNCE response.\n");
                }
				
                return;
            }
        }
    }
	//SETUP	
    else if ((char*)strstr(s1c1_data.S1_Res_Buf.c_str(), "SETUP") != NULL)
    {
    	int ret;
    	ret = handle_setup_response(s1c1_data.S1_Res_Buf);
        if (-1 == ret)
        {
            MEDIA_PRINTF_D("res_buf = %s.\n", s1c1_data.S1_Res_Buf.c_str());

			set_s1_recv_msg_status(seq, RECV_STATUS_FAULT);
			//set_err_code(Media_ServerInvalid);			
			return;
        }
		
        set_s1_recv_msg_status(seq, RECV_STATUS_OK);
        return;
    }
    //teardown
    else if ((char*)strstr(s1c1_data.S1_Res_Buf.c_str(), "TEARDOWN") != NULL)
    {
        ps = (char*)strstr((char*)s1c1_data.S1_Res_Buf.c_str(), g_response_code_ok);
        if (NULL == ps)
        {
            MEDIA_PRINTF_E("res_buf don't contain \"200 OK\".\n");
			set_s1_recv_msg_status(seq, RECV_STATUS_FAULT);
			//set_err_code(Media_ServerInvalid);			
            return;
        }

        set_s1_recv_msg_status(seq, RECV_STATUS_OK);
        return;
    }
    //PING
    else if ((char*)strstr(s1c1_data.S1_Res_Buf.c_str(), g_response_code_ok) != NULL)
    {
        set_s1_ping_status(seq, RECV_STATUS_OK);
        return;
    }
    else
    {
    	MEDIA_PRINTF_E("res_buf don't contain any valid info. res_buf=%s.\n", s1c1_data.S1_Res_Buf.c_str());
        
		//set_err_code(Media_ServerInvalid);
        return;
    }

    return;
}

int S1C1Client::send_C1_announce_response()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;
	
    MEDIA_PRINTF_I("send announce response to C1, buffer = %s \n", s1c1_data.C1_Req_Buf.c_str());
    data_len = send_client_request(s1c1_data.C1_SocketID, s1c1_data.C1_Req_Buf.c_str());

    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send announce response to C1 client failed! \n");
    }

    return data_len;
}

int S1C1Client::send_S1_announce_response()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

    MEDIA_PRINTF_I("send announce response to S1, buffer = %s \n", s1c1_data.S1_Req_Buf.c_str());
    data_len = send_client_request(s1c1_data.S1_SocketID, s1c1_data.S1_Req_Buf.c_str());

    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send announce response to S1 client failed! \n");
    }

    return data_len;
}

int S1C1Client::send_teardown_request()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

    set_s1_recv_msg_status(s1_operation_cseq, RECV_STATUS_WAITING);

    MEDIA_PRINTF_I("send TEARDOWN to S1, buffer = %s \n", s1c1_data.S1_Req_Buf.c_str());
    data_len = send_client_request(s1c1_data.S1_SocketID, s1c1_data.S1_Req_Buf.c_str());

    if (0 >= data_len)
    {
        MEDIA_PRINTF_E("send TEARDOWN to S1 client failed! \n");
        s1_recv_msg_status.erase(s1_operation_cseq);
		//set_err_code(Media_ServerInvalid);
    }

    return data_len;
}

int S1C1Client::wait_S1_response()
{
    BSTM_DEBUG_ENTER();
    C_U32 ret = 0;
    C_U32 counter = 0;

    while (RECV_STATUS_WAITING == get_s1_recv_msg_status(s1_operation_cseq))
    {
        if (counter >= callback_timer / 50)
        {
            break;
        }
        counter++;
        usleep(50*1000);
    }
    if (RECV_STATUS_WAITING == get_s1_recv_msg_status(s1_operation_cseq))
    {
        //timeout, no data received
        MEDIA_PRINTF_D("seq is %d,  timeout\n", s1_operation_cseq);
        ret = -2;
    }
    else if (RECV_STATUS_FAULT == get_s1_recv_msg_status(s1_operation_cseq))
    {
        //received incorrect data
        MEDIA_PRINTF_D("seq is %d, error\n", s1_operation_cseq);
        ret = -1;
    }
    else if (RECV_STATUS_OK == get_s1_recv_msg_status(s1_operation_cseq))
    {
        MEDIA_PRINTF_D("seq is %d, data received. OK\n", s1_operation_cseq);
        ret = 1;
    }

    s1_recv_msg_status.erase(s1_operation_cseq);
    return ret;
}

int S1C1Client::wait_S1_ping_response()
{
    BSTM_DEBUG_ENTER();
    int ret = 0;
    int counter = 0;

    while (RECV_STATUS_WAITING == get_s1_ping_status(s1_ping_cseq))
    {
        if (counter >= WAIT_PING_RESPONSE_TIMER / 50)
        {
            break;
        }
        counter++;
        usleep(50*1000);
    }
    if (RECV_STATUS_WAITING == get_s1_ping_status(s1_ping_cseq))
    {
        //timeout, no data received
        MEDIA_PRINTF_D("seq is %d, timeout\n\n", s1_ping_cseq);
        ret = -2;
    }
    else if (RECV_STATUS_FAULT == get_s1_ping_status(s1_ping_cseq))
    {
        //received incorrect data
        MEDIA_PRINTF_D("seq is %d, error\n\n", s1_ping_cseq);
        ret = -1;
    }
    else if (RECV_STATUS_OK == get_s1_ping_status(s1_ping_cseq))
    {
        MEDIA_PRINTF_D("seq is %d, data received. OK\n\n", s1_ping_cseq);
        ret = 1;
    }


    s1_ping_status.erase(s1_ping_cseq);
    return ret;
}


int S1C1Client::send_getparam_request(bool is_ping)
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

	if (true == is_ping)
	{
		set_c1_ping_status(c1_ping_cseq, RECV_STATUS_WAITING);
	}
	else
	{
		set_c1_recv_msg_status(c1_operation_cseq, RECV_STATUS_WAITING);
	}   

	if (true == is_ping)
	{
        MEDIA_PRINTF_D("send ping to C1, buffer = %s \n", s1c1_data.C1_PING_Req_Buf.c_str());
        data_len = send_client_request(s1c1_data.C1_SocketID, s1c1_data.C1_PING_Req_Buf.c_str());

		
	    if (0 >= data_len)
	    {
	        MEDIA_PRINTF_E("send ping to C1 client failed! \n");
			c1_ping_status.erase(c1_ping_cseq);
	    }
	}
	else
	{
        data_len = send_client_request(s1c1_data.C1_SocketID, s1c1_data.C1_Req_Buf.c_str());
        MEDIA_PRINTF_D("send GET_PARAMETER to C1, buffer = %s \n", s1c1_data.C1_Req_Buf.c_str());
		
	    if (0 >= data_len)
	    {
	        MEDIA_PRINTF_E("send GET_PARAMETER to C1 client failed! \n");
			c1_recv_msg_status.erase(c1_operation_cseq);
	    }
    }
 

    return data_len;
}

int S1C1Client::recv_connectTimeout_response()
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

    data_len = recv_client_response(s1c1_data.C1_SocketID, s1c1_data.C1_Res_Buf, callback_timer);
    if (data_len <= 0)
    {
        MEDIA_PRINTF_E("recv GET_PARAMETER response error.\n");
        
        //set_err_code(Media_ServerInvalid);
    }
    else
    {
        handle_connectTimeout_response();
    }

    return data_len;
}

int S1C1Client::handle_connectTimeout_response()
{
    BSTM_DEBUG_ENTER();
    set_getparam_connect_timeout_value();

    if (0 == c1_ping_time_out)
    {
    	MEDIA_PRINTF_W("recv GET_PARAMETER response, connect_tiemout is 0, use the default value.\n");
        c1_ping_time_out = C1_PING_TIMEOUT;
    }
    
    return 0;
}


void S1C1Client::set_timer_callback_func(C_U32 uTimeOut)
{
    BSTM_DEBUG_ENTER();
    callback_timer = uTimeOut;
    C_U32 secs = callback_timer / 1000;

    //callback = cb;
    signal(SIGALRM, timer_callback_func);
    alarm(secs);
    MEDIA_PRINTF_I("callback func timer start, timeout is %d seconds.\n", secs);
}


int S1C1Client::set_c1_heart_beat_timer(C_U32 seconds)
{
    int left_scs;
    left_scs = alarm(seconds);

    return left_scs;
}

RECV_MSG_STATUS_E S1C1Client::get_c1_recv_msg_status(int seq)
{
    std::map<int, RECV_MSG_STATUS_E>::iterator it = c1_recv_msg_status.find(seq);
    if (c1_recv_msg_status.end() == it)
    {
        return RECV_STATUS_FAULT;
    }

    return c1_recv_msg_status[seq];
}

void S1C1Client::set_c1_recv_msg_status(int seq, RECV_MSG_STATUS_E status)
{
    if (RECV_STATUS_WAITING == status)
    {
        c1_recv_msg_status[seq] = status;
    }
    else if (RECV_STATUS_WAITING != status)
    {
        if (RECV_STATUS_WAITING == get_c1_recv_msg_status(seq))
        {
            c1_recv_msg_status[seq] = status;
        }
    }
}

RECV_MSG_STATUS_E S1C1Client::get_s1_recv_msg_status(int seq)
{
    std::map<int, RECV_MSG_STATUS_E>::iterator it = s1_recv_msg_status.find(seq);
    if (s1_recv_msg_status.end() == it)
    {
        return RECV_STATUS_FAULT;
    }

    return s1_recv_msg_status[seq];
}

void S1C1Client::set_s1_recv_msg_status(int seq, RECV_MSG_STATUS_E status)
{
    if (RECV_STATUS_WAITING == status)
    {
        s1_recv_msg_status[seq] = status;
    }
    else if (RECV_STATUS_WAITING != status)
    {
        if (RECV_STATUS_WAITING == get_s1_recv_msg_status(seq))
        {
            s1_recv_msg_status[seq] = status;
        }
    }
}

RECV_MSG_STATUS_E S1C1Client::get_c1_ping_status(int seq)
{
    std::map<int, RECV_MSG_STATUS_E>::iterator it = c1_ping_status.find(seq);
    if (c1_ping_status.end() == it)
    {
        return RECV_STATUS_FAULT;
    }

    return c1_ping_status[seq];
}

void S1C1Client::set_c1_ping_status(int seq, RECV_MSG_STATUS_E status)
{
    if (RECV_STATUS_WAITING == status)
    {
        c1_ping_status[seq] = status;
    }
    else if (RECV_STATUS_WAITING != status)
    {
        if (RECV_STATUS_WAITING == get_c1_ping_status(seq))
        {
            c1_ping_status[seq] = status;
        }
    }
}

RECV_MSG_STATUS_E S1C1Client::get_s1_ping_status(int seq)
{
    std::map<int, RECV_MSG_STATUS_E>::iterator it = s1_ping_status.find(seq);
    if (s1_ping_status.end() == it)
    {
        return RECV_STATUS_FAULT;
    }

    return s1_ping_status[seq];
}

void S1C1Client::set_s1_ping_status(int seq, RECV_MSG_STATUS_E status)
{
    if (RECV_STATUS_WAITING == status)
    {
        s1_ping_status[seq] = status;
    }
    else if (RECV_STATUS_WAITING != status)
    {
        if (RECV_STATUS_WAITING == get_s1_ping_status(seq))
        {
            s1_ping_status[seq] = status;
        }
    }
}

void S1C1Client::clear_timer_callback_func()
{
    BSTM_DEBUG_ENTER();
    alarm(0);
}

int S1C1Client::get_cseq_value(const std::string& str)
{
    BSTM_DEBUG_ENTER();
    char* ps = NULL;
    int seq = 0;
    bool is_digit_found = false;

    ps = (char*)strstr(str.c_str(), "CSeq");
    if (NULL == ps)
    {
        return -1;
    }

    ps += strlen("CSeq");

    //skip ' ' and ':'
    while ((' ' == *ps) || (':' == *ps))
    {
        ps++;
    }

    //get cseq
    while (isdigit(*ps) && ('\0' != *ps))
    {
        seq = seq * 10;
        seq += (*ps - '0');
        is_digit_found = true;
        ps++;
    }

    if (false == is_digit_found)
    {
        return -1;
    }

    return seq;
}

int S1C1Client::play_media()
{	
#ifndef STUB_TEST_TURN_ON
    TS_INFO_T ts_info;
    ts_info.freq = s1c1_res.prog_info.freq/1000;
    ts_info.mode = BSTM_QAM_MODE_QAM64;
    ts_info.symb = DEFAULT_MEDIA_PLAY_SYMBOL;
	
    MEDIA_PRINTF_I("ready to play. the freq is: %d. the service id is: %d.\n", ts_info.freq, s1c1_res.prog_info.serviceId);
    programPlayer::getInstance()->play(ts_info, s1c1_res.prog_info.serviceId, 1);
	MEDIA_PRINTF_I("play_media done!\n");
#else
    MEDIA_PRINTF_I("ready to play. the freq is: %d. the service id is: %d.\n", s1c1_res.prog_info.freq/1000, s1c1_res.prog_info.serviceId);
	MEDIA_PRINTF_I("STUB_TEST_TURN_ON is defined. no need to call programPlayer::getInstance()->play!\n");
#endif

    return 0;
}

S1C1Client::S1C1Client()
{
	//MEDIA_PRINTF_I("url=%s.\n", (char*)url);
    //init_data(url, a7_client);
}

S1C1Client::~S1C1Client()
{
    int ret;
    ret = close_client_socket(s1c1_data.S1_SocketID);
    if (-1 != ret)
    {
        MEDIA_PRINTF_D("close S1 socket: %d.\n", s1c1_data.S1_SocketID);
    }
	
    ret = close_client_socket(s1c1_data.C1_SocketID);
    if (-1 != ret)
    {
        MEDIA_PRINTF_D("close C1 socket: %d.\n", s1c1_data.C1_SocketID);
    }
}

std::string S1C1Client::load_text_file(const std::string& path)
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


void S1C1Client::set_ip_addr()
{
    //ip_addr.GH_IP = (a7_client->get_ip_addr()).GH_IP;
    //ip_addr.S1_Port = (a7_client->get_ip_addr()).S1_Port;
    //ip_addr.C1_Port = (a7_client->get_ip_addr()).C1_Port;
    
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



void S1C1Client::set_purchaseToken(A7Client* a7_client)
{
    url_t.PurchaseToken = a7_client->get_purchaseToken();
}


void S1C1Client::build_teardown_request()
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[REQUEST_BUF];
    char s1_port[10] = "";

    //build TEARDOWN
    teardown_req.TEARDOWN.clear();
#if 0
    teardown_req.TEARDOWN += "TEARDOWN rtsp://";
    teardown_req.TEARDOWN += url_t.Server_Name;
    teardown_req.TEARDOWN += ":";
    sprintf(s1_port, "%d", url_t.Server_Port);
    teardown_req.TEARDOWN += s1_port;
    teardown_req.TEARDOWN += " RTSP/1.0";
#else
    teardown_req.TEARDOWN += "TEARDOWN ";
    teardown_req.TEARDOWN += url_t.Rtsp_Url;
    teardown_req.TEARDOWN += " RTSP/1.0";
#endif

    //CSeq
    s1_operation_cseq++;
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Cseq, s1_operation_cseq);
    teardown_req.Cseq = tmp_buf;

    //Require
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Require, g_Require_S1_value);
    teardown_req.Require = tmp_buf;

    //Reason
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Reason, g_Reason_value);
    teardown_req.Reason = tmp_buf;

    //Session
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Session, s1c1_res.Session.c_str());
    teardown_req.Session = tmp_buf;

    //OnDemandSessionId
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_OnDemandSessionId, s1c1_res.OnDemandSessionId.c_str());
    teardown_req.OnDemandSessionId = tmp_buf;

    //ClientSessionId
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_ClientSessionId, g_ClientSessionId_value);
    teardown_req.ClientSessionId = tmp_buf;

    s1c1_data.S1_Req_Buf.clear();
    s1c1_data.S1_Req_Buf += teardown_req.TEARDOWN;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += teardown_req.Cseq;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += teardown_req.Require;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += teardown_req.Reason;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += teardown_req.Session;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += teardown_req.OnDemandSessionId;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += teardown_req.ClientSessionId;

    //add "\r\n\r\n" at end of the message
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += g_seperator;

    //MY_PRINTF_I(" sendbuf is : \n%s \n", s1c1_data.S1_Req_Buf.c_str());
}

void S1C1Client::build_getparam_request(GET_PARAMETER_TYPE_E param_type, bool is_ping)
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[REQUEST_BUF];
    char c1_port[10] = "";
    std::string param_type_content = "";
    std::string build_str;
    int seq = 0;;

    switch (param_type)
    {
    case CONNECT_TIMEOUT:
        param_type_content = g_param_type_Connection_Timeout;
        seq = ++c1_ping_cseq; //CONNECT_TIMEOUT is used to PING C1
    	break;

    case SESSION_LIST:
        param_type_content = g_param_type_Session_List;
        seq = ++c1_operation_cseq;
        break;

    case POSITION:
        param_type_content = g_param_type_Position;
        seq = ++c1_operation_cseq;
        break;

    case PRESENTATION_STATE:
        param_type_content = g_param_type_Presentation_State;
        seq = ++c1_operation_cseq;
        break;

    case SCALE:
        param_type_content = g_param_type_Scale;
        seq = ++c1_operation_cseq;
        break;

    default:
        MEDIA_PRINTF_E("param is incorrect.\n");
        seq = 1;
        break;
    }

    int length = strlen(param_type_content.c_str());

    //build GET_PARAMETER
    getPara_req.Get_Parameter.clear();
    getPara_req.Get_Parameter = "GET_PARAMETER rtsp://";
    getPara_req.Get_Parameter += url_t.Server_Name;
    getPara_req.Get_Parameter += ":";
    sprintf(c1_port, "%d", url_t.Server_Port);
    getPara_req.Get_Parameter += c1_port;
    getPara_req.Get_Parameter += "/";
    getPara_req.Get_Parameter += s1c1_res.Session;
    getPara_req.Get_Parameter += " RTSP/1.0";

    //build CSeq
    getPara_req.Cseq.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Cseq, seq);
    getPara_req.Cseq = tmp_buf;

    //build Require
    getPara_req.Require.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Require, g_Require_C1_value);
    getPara_req.Require = tmp_buf;

    //build Content-Type
    getPara_req.Content_Type = "";;
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Content_Type, g_Content_Type_value);
    getPara_req.Content_Type = tmp_buf;

    //build Session
    getPara_req.Session.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Session, s1c1_res.Session.c_str());
    getPara_req.Session = tmp_buf;

    //build Content-Length
    getPara_req.Content_Length.clear();;
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Conten_Length, length);
    getPara_req.Content_Length = tmp_buf;

    //build param type
    getPara_req.Parameter_Type = param_type_content;

    build_str.clear();
    build_str = getPara_req.Get_Parameter;
    build_str += g_seperator;
    build_str += getPara_req.Cseq;
    build_str += g_seperator;
    build_str += getPara_req.Require;
    build_str += g_seperator;
    build_str += getPara_req.Content_Type;
    build_str += g_seperator;
    build_str += getPara_req.Session;
    build_str += g_seperator;
    build_str += getPara_req.Content_Length;
    build_str += g_seperator;
    //add another "\r\n" before parameter type
    build_str += g_seperator;
    build_str += getPara_req.Parameter_Type;

    //add "\r\n\r\n" at end of the message
    build_str += g_seperator;
    build_str += g_seperator;

	if (true == is_ping)
	{
		s1c1_data.C1_PING_Req_Buf.clear();
		s1c1_data.C1_PING_Req_Buf = build_str;
	}
	else
	{
		s1c1_data.C1_Req_Buf.clear();
		s1c1_data.C1_Req_Buf = build_str;
	}

    //MY_PRINTF_I("sendbuf is : \n");
    //MY_PRINTF_I("%s \n", s1c1_data.C1_Req_Buf.c_str());
}

void S1C1Client::build_media_ctrl_request(C_MediaCtrlType uMediaCtrlType, C_S32 uValue)
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[REQUEST_BUF];
    char c1_port[10] = "";

    switch (uMediaCtrlType)
    {
    case MediaCtrlType_PLAY:
    case MediaCtrlType_SEEK:
    case MediaCtrlType_SETSCALE:
        //build PLAY head
#if 0
        c1_req.C1_URL.clear();
        c1_req.C1_URL += "PLAY rtsp://";
        c1_req.C1_URL += url_t.Server_Name;
        c1_req.C1_URL += ":";
        sprintf(c1_port, "%d", url_t.Server_Port);
        c1_req.C1_URL += c1_port;
        c1_req.C1_URL += "/";
        c1_req.C1_URL += s1c1_res.Session;
        c1_req.C1_URL += " RTSP/1.0";
#else
        c1_req.C1_URL.clear();
        c1_req.C1_URL += "PLAY ";
        c1_req.C1_URL += url_t.Rtsp_Url;
        c1_req.C1_URL += " RTSP/1.0";

#endif
        break;

    case MediaCtrlType_PAUSE:
        //build PAUSE head
#if 0
        c1_req.C1_URL.clear();
        c1_req.C1_URL += "PAUSE rtsp://";
        c1_req.C1_URL += url_t.Server_Name;
        c1_req.C1_URL += ":";
        sprintf(c1_port, "%d", url_t.Server_Port);
        c1_req.C1_URL += c1_port;
        c1_req.C1_URL += "/";
        c1_req.C1_URL += s1c1_res.Session;
        c1_req.C1_URL += " RTSP/1.0";
#else
        c1_req.C1_URL.clear();
        c1_req.C1_URL += "PAUSE ";
        c1_req.C1_URL += url_t.Rtsp_Url;
        c1_req.C1_URL += " RTSP/1.0";
#endif
        break;
        
    default:
        break;
    }

    //build CSeq
    c1_operation_cseq++;
    c1_req.Cseq.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Cseq, c1_operation_cseq);
    c1_req.Cseq = tmp_buf;

    //build require
    c1_req.Require.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Require, g_Require_C1_value);
    c1_req.Require = tmp_buf;

    //build Session
    c1_req.Session.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Session, s1c1_res.Session.c_str());
    c1_req.Session = tmp_buf;

    //build Scale
    c1_req.Scale.clear();
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    if (MediaCtrlType_SETSCALE == uMediaCtrlType)
    {
        sprintf(tmp_buf, g_Scale, uValue);
    }
    else
    {
        sprintf(tmp_buf, g_Scale, default_play_scale);
    }
    c1_req.Scale = tmp_buf;

    if (MediaCtrlType_SEEK == uMediaCtrlType)
    {
        //build Range
        c1_req.Range.clear();
        memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
        sprintf(tmp_buf, g_Range, (float)uValue);
        c1_req.Range = tmp_buf;
    }

    s1c1_data.C1_Req_Buf.clear();
    s1c1_data.C1_Req_Buf += c1_req.C1_URL;
    s1c1_data.C1_Req_Buf += g_seperator;
    s1c1_data.C1_Req_Buf += c1_req.Cseq;
    s1c1_data.C1_Req_Buf += g_seperator;
    if (MediaCtrlType_SEEK == uMediaCtrlType)
    {
        s1c1_data.C1_Req_Buf += c1_req.Range;
        s1c1_data.C1_Req_Buf += g_seperator;
    }
    s1c1_data.C1_Req_Buf += c1_req.Require;
    s1c1_data.C1_Req_Buf += g_seperator;
    s1c1_data.C1_Req_Buf += c1_req.Scale;
    s1c1_data.C1_Req_Buf += g_seperator;
    s1c1_data.C1_Req_Buf += c1_req.Session;

    //add "\r\n\r\n" at end of the message
    s1c1_data.C1_Req_Buf += g_seperator;
    s1c1_data.C1_Req_Buf += g_seperator;

    //MY_PRINTF_I("sendbuf is : \n");
    //MY_PRINTF_I("%s \n", s1c1_data.C1_Req_Buf.c_str());
}

int S1C1Client::build_c1_announce_response(const char* res_buf)
{
    BSTM_DEBUG_ENTER();
    //need get Cseq, Session
    char* ps;
    std::string seq_content = "";
    std::string session_content = "";

    s1c1_data.C1_Req_Buf.clear();

    //find Cseq
    ps = (char*)strstr((char*)res_buf, "CSeq");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain Cseq, response NULL.\n");
        return -1;
    }
    else
    {
        while ((*ps != '\0') && (*ps != '\r') && (*ps != '\n'))
        {
            seq_content += *ps;
            ps++;
        }
    }

    //find Session
    ps = (char*)strstr((char*)res_buf, "Session");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain Session, response NULL.\n");
        return -1;
    }
    else
    {
        while ((*ps != '\0') && (*ps != '\r') && (*ps != '\n'))
        {
            session_content += *ps;
            ps++;
        }
    }

    s1c1_data.C1_Req_Buf += "RTSP/1.0 200 OK";
    s1c1_data.C1_Req_Buf += seq_content;
    s1c1_data.C1_Req_Buf += g_seperator;
    s1c1_data.C1_Req_Buf += session_content;

    //add "\r\n\r\n" at end of the message
    s1c1_data.C1_Req_Buf += g_seperator;
    s1c1_data.C1_Req_Buf += g_seperator;

    return 0;
}

int S1C1Client::build_s1_announce_response(const char* res_buf)
{
    BSTM_DEBUG_ENTER();
    //need get Cseq, Session, OnDemandSessionId
    char* ps;
    std::string seq_content = "";
    std::string session_content = "";
    std::string onDemandsessionid_content = "";

    s1c1_data.S1_Req_Buf.clear();

    //find Cseq
    ps = (char*)strstr((char*)res_buf, "CSeq");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain Cseq, response NULL.\n");
        return -1;
    }
    else
    {
        while ((*ps != '\0') && (*ps != '\r') && (*ps != '\n'))
        {
            seq_content += *ps;
            ps++;
        }
    }

    //find Session
    ps = (char*)strstr((char*)res_buf, "Session");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain Session, response NULL.\n");
        return -1;
    }
    else
    {
        while ((*ps != '\0') && (*ps != '\r') && (*ps != '\n'))
        {
            session_content += *ps;
            ps++;
        }
    }

    //find OnDemandSessionId
    #if 0
	//no need to find OnDemandSessionId
    ps = (char*)strstr((char*)res_buf, "OnDemandSessionId");
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("res_buf don't contain OnDemandSessionId.\n");
        return -1;
    }
    else
    {
        while ((*ps != '\0') && (*ps != '\r') && (*ps != '\n'))
        {
            onDemandsessionid_content += *ps;
            ps++;
        }
    }
	#endif

    s1c1_data.S1_Req_Buf += "RTSP/1.0 200 OK";
    s1c1_data.S1_Req_Buf += seq_content;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += session_content;
    //s1c1_data.S1_Req_Buf += g_seperator;
    //s1c1_data.S1_Req_Buf += onDemandsessionid_content;

    //add "\r\n\r\n" at end of the message
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += g_seperator;

    return 0;

}

int S1C1Client::set_qam_by_rtsp_url(std::string &rtsp_url, std::string &qam)
{
	std::size_t begin = rtsp_url.find("ServiceGroup=");
	if (std::string::npos == begin)
	{
		MEDIA_PRINTF_E("ServiceGroup= is not found!\n");
		return -1;
	}
	printf("************begin is %d.\n", begin);
	std::size_t end = rtsp_url.find("&", begin);
	if (std::string::npos == end)
	{
		MEDIA_PRINTF_E("& is not found after ServiceGroup=!\n");
		return -1;
	}
	printf("************end is %d.\n", end);

	std::size_t head = begin+strlen("ServiceGroup=");
	std::size_t len = end - head;
	qam = rtsp_url.substr(head, len);
	MEDIA_PRINTF_I("head is %d, length is %d, qam=%s.\n",head, len, qam.c_str());
	return 0;
}


int S1C1Client::build_setup_request()
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[REQUEST_BUF];
	int ret;

    //build cseq
    s1_operation_cseq++;
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Cseq, s1_operation_cseq);
    setup_req.Cseq = tmp_buf;

#if 0
    //build clientSessionId
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_ClientSessionId, g_ClientSessionId_value);
    setup_req.ClientSessionId = tmp_buf;

    //build Require
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Require, g_Require_S1_value);
    setup_req.Require = tmp_buf;

    //build Transport
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Transport, g_Transport_value);
    setup_req.Transport = tmp_buf;

	//build RtspUrl
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_RtspUrl, s1c1_data.param_url.c_str());
	setup_req.RtspUrl = tmp_buf;

	//build ServiceGroup
	ret = set_qam_by_rtsp_url(setup_req.RtspUrl, setup_req.ServiceGroup);
	if (ret < 0)
	{
		MEDIA_PRINTF_E("not found ServiceGroup in rtsp url, url is %s.\n", setup_req.RtspUrl.c_str());
		return -1;
	}
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_QamName, setup_req.ServiceGroup.c_str());
	setup_req.ServiceGroup= tmp_buf;
#endif

    build_SETUP_head();

    s1c1_data.S1_Req_Buf.clear();
    s1c1_data.S1_Req_Buf = setup_req.SETUP;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += setup_req.Cseq;
    //s1c1_data.S1_Req_Buf += g_seperator;
    //s1c1_data.S1_Req_Buf += setup_req.ClientSessionId;
    //s1c1_data.S1_Req_Buf += g_seperator;
    //s1c1_data.S1_Req_Buf += setup_req.Require;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += g_UserAgent;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += setup_req.Transport;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += g_Version;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += setup_req.ServiceGroup;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += setup_req.AppData;
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += setup_req.RtspUrl;

    //add "\r\n\r\n" at end of the message
    s1c1_data.S1_Req_Buf += g_seperator;
    s1c1_data.S1_Req_Buf += g_seperator;
	
    //MY_PRINTF_I("sendbuf is : \n");
    //MY_PRINTF_I("%s \n", s1c1_data.S1_Setup_Req_Buf.c_str());

	return 0;
}

void S1C1Client::build_S1_ping_request()
{
    BSTM_DEBUG_ENTER();
    char tmp_buf[REQUEST_BUF];
    char s1_port[10] = "";

    //build TEARDOWN
    ping_req.Ping.clear();
    ping_req.Ping += "PING rtsp://";
    ping_req.Ping += url_t.Server_Name;
    ping_req.Ping += ":";
    sprintf(s1_port, "%d", url_t.Server_Port);
    ping_req.Ping += s1_port;
    ping_req.Ping += " RTSP/1.0";

    //CSeq
    s1_ping_cseq++;
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Cseq, s1_ping_cseq);
    ping_req.Cseq = tmp_buf;

    //Require
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Require, g_Require_S1_value);
    ping_req.Require = tmp_buf;

    //Session
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_Session, s1c1_res.Session.c_str());
    ping_req.Session = tmp_buf;

    //OnDemandSessionId
    memset(tmp_buf, 0, sizeof(char)*REQUEST_BUF);
    sprintf(tmp_buf, g_OnDemandSessionId, s1c1_res.OnDemandSessionId.c_str());
    ping_req.OnDemandSessionId = tmp_buf;

    s1c1_data.S1_PING_Req_Buf.clear();
    s1c1_data.S1_PING_Req_Buf += ping_req.Ping;
    s1c1_data.S1_PING_Req_Buf += g_seperator;
    s1c1_data.S1_PING_Req_Buf += ping_req.Cseq;
    s1c1_data.S1_PING_Req_Buf += g_seperator;
    s1c1_data.S1_PING_Req_Buf += ping_req.Require;
    s1c1_data.S1_PING_Req_Buf += g_seperator;
    s1c1_data.S1_PING_Req_Buf += ping_req.Session;
    s1c1_data.S1_PING_Req_Buf += g_seperator;
    s1c1_data.S1_PING_Req_Buf += ping_req.OnDemandSessionId;

    //add "\r\n\r\n" at end of the message
    s1c1_data.S1_PING_Req_Buf += g_seperator;
    s1c1_data.S1_PING_Req_Buf += g_seperator;
		
    //MY_PRINTF_I("sendbuf is : \n");
    //MY_PRINTF_I("%s \n", s1c1_data.S1_Ping_Req_Buf.c_str());
}

void S1C1Client::set_url_default_value()
{
    url_t.Server_Name = ip_addr.GH_IP;
    url_t.Server_Port = ip_addr.S1_Port;
    url_t.Server_ID = ip_addr.GH_IP;
}

bool S1C1Client::set_Server_ID(const C_U8* url)
{
    char *ps = NULL;
    char serverId[IP_NUM];

    memset(serverId, 0, sizeof(char)*IP_NUM);
    
    ps = (char*)strstr((char*)url, "serverID=");
    if (NULL == ps)
    {
        url_t.Server_ID = ip_addr.GH_IP;
        MEDIA_PRINTF_W("\"serverID=\" is not found, use default value in config file\n");
        MEDIA_PRINTF_W("\"serverID=%s\"\n", url_t.Server_ID.c_str());

        return false;
    }

    ps += strlen("serverID=");

    //skip ' ' 
    while (' ' == *ps)
    {
        ps++;
    }

    for (int i = 0; i < IP_NUM; i++)
    {
        if ((ps[i] != ';') && ps[i] != ' ' && ps[i] != '\0')
        {
            serverId[i] = ps[i];
        }
        else
        {
            break;
        }
    }

    url_t.Server_ID = serverId;

    MEDIA_PRINTF_I("\"serverID=%s\"\n", url_t.Server_ID.c_str());

    return true;
}

void S1C1Client::set_getparam_connect_timeout_value()
{
    char *ps = NULL;
    bool is_digit_found = false;
    c1_ping_time_out = 0;

    ps = (char *)strstr(s1c1_data.C1_Res_Buf.c_str(), g_param_type_Connection_Timeout);
    if (NULL == ps)
    {
    	MEDIA_PRINTF_W("C1_Res_Buf contains no connect_tiemout, use the default value.\n");

        c1_ping_time_out = C1_PING_TIMEOUT;
		return;
    }

    ps += strlen(g_param_type_Connection_Timeout);

    //skip ' ' and ':'
    while ((' ' == *ps) || (':' == *ps))
    {
        ps++;
    }
    //set timeout
    while (isdigit(*ps) && ('\0' != *ps))
    {
        c1_ping_time_out = c1_ping_time_out * 10;
        c1_ping_time_out += (*ps - '0');
        ps++;
        is_digit_found = true;
    }

    if (false == is_digit_found)
    {
        MEDIA_PRINTF_W("C1_Res_Buf contains no connect_tiemout, use the default value.\n");

        c1_ping_time_out = C1_PING_TIMEOUT;
        return;
    }

    MEDIA_PRINTF_I("connect_time_out = %d\n", c1_ping_time_out);
}

int S1C1Client::set_getparam_scale_value()
{
    char *ps = NULL;
    mediaInfo_res.uPlayScale = 0;
    bool is_negative = false;
    bool is_digit_found = false;

    ps = (char *)strstr(s1c1_data.C1_Res_Buf.c_str(), g_param_type_Scale);
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("C1_Res_Buf contains no scale.\n");

        return -1;
    }

    ps += strlen(g_param_type_Scale);

    //find first digit or '-'
    while ((*ps != ';') && *ps != '\0' && *ps != '\r' && *ps != '\n')
    {
        if (isdigit(*ps) || ('-' == *ps))
        {
            break;
        }
        else
        {
            ps++;
        }
    }

    if ('-' == *ps)
    {
        is_negative = true;
        ps++;
    }

    while (isdigit(*ps) && ('\0' != *ps))
    {
        mediaInfo_res.uPlayScale = mediaInfo_res.uPlayScale * 10;
        mediaInfo_res.uPlayScale += (*ps - '0');
        ps++;
        is_digit_found = true;
    }

    if (false == is_digit_found)
    {
        MEDIA_PRINTF_E("C1_Res_Buf contains no valid scale.\n");
    }

    if (true == is_negative)
    {
        mediaInfo_res.uPlayScale = mediaInfo_res.uPlayScale * (-1);
    }

    MEDIA_PRINTF_I("uPlayScale = %d\n", mediaInfo_res.uPlayScale);
    return 0;
}

int S1C1Client::set_getparam_presentation_state_value()
{
    char *ps = NULL;
    char param_type_content[PARAM_TYPE_CONTENT_LEN];
    memset(param_type_content, 0, sizeof(char)*PARAM_TYPE_CONTENT_LEN);

    ps = (char *)strstr(s1c1_data.C1_Res_Buf.c_str(), g_param_type_Presentation_State);
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("C1_Res_Buf contains no presentation state.\n");

        return -1;
    }

    ps += strlen(g_param_type_Presentation_State);
    ps += strlen(":");

    for (int i = 0; i < IP_NUM; i++)
    {
        if ((*ps != ';') && *ps != '\0' && *ps != '\r' && *ps != '\n')
        {
            param_type_content[i] = ps[i];
        }
        else
        {
            break;
        }
    }
    
    ps = NULL;
    ps = (char *)strstr(param_type_content, "play");
    if (NULL == ps)
    {
        ps = (char *)strstr(param_type_content, "pause");
        if (NULL == ps)
        {
            ps = (char *)strstr(param_type_content, "backward");
            if (NULL == ps)
            {
                ps = (char *)strstr(param_type_content, "forward");
                if (NULL == ps)
                {
                    ps = (char *)strstr(param_type_content, "stop");
                    if (NULL == ps)
                    {
                    	MEDIA_PRINTF_E("C1_Res_Buf contains no presentation state value.\n");

                        return -1;
                    }
                    else
                    {
                        mediaInfo_res.MediaStatusType = MediaStatusType_STOP;
                    }
                }
                else
                {
                    mediaInfo_res.MediaStatusType = MediaStatusType_FORWARD;
                }
            }
            else
            {
                mediaInfo_res.MediaStatusType = MediaStatusType_BACKWARD;
            }
        }
        else
        {
            mediaInfo_res.MediaStatusType = MediaStatusType_PAUSE;
        }
    }
    else
    {
        mediaInfo_res.MediaStatusType = MediaStatusType_PLAY;
    }

    MEDIA_PRINTF_I("uPlayScale = %d\n", mediaInfo_res.MediaStatusType);
    return 0;
}

int S1C1Client::set_getparam_position_value()
{
    char *ps = NULL;
    bool is_digit_found = false;
    mediaInfo_res.uCurrentTime = 0;

    ps = (char *)strstr(s1c1_data.C1_Res_Buf.c_str(), g_param_type_Position);
    if (NULL == ps)
    {
        MEDIA_PRINTF_E("C1_Res_Buf contains no position.\n");

        return -1;
    }

    ps += strlen(g_param_type_Position);


    //skip ' ' and ':'
    while ((' ' == *ps) || (':' == *ps))
    {
        ps++;
    }

    //set POSITION
    while (isdigit(*ps) && ('\0' != *ps))
    {
        mediaInfo_res.uCurrentTime = mediaInfo_res.uCurrentTime * 10;
        mediaInfo_res.uCurrentTime += (*ps - '0');
        is_digit_found = true;
        ps++;
    }

    if (false == is_digit_found)
    {
        MEDIA_PRINTF_E("C1_Res_Buf contains no valid position.\n");

        return -1;
    }
    
    MEDIA_PRINTF_I("uCurrentTime = %d\n", mediaInfo_res.uCurrentTime);
    return 0;
}

int S1C1Client::set_ip_port_by_url(const char* url)
{
    const char *str = NULL;
    const char *nextslash = NULL;
    const char *nextcolon = NULL;
    int hostlen;

	str = strstr((char*)url, "rtsp://");

    if (NULL != str)
    {
        str += strlen("rtsp://");
    }
    else
    {
        //set_url_default_value();
        MEDIA_PRINTF_E("not find rtsp, parse failed!\n");
        return -1;
    }

    nextcolon = strchr(str, ':');
    nextslash = strchr(str, '/');

    if (nextslash != 0 || nextcolon != 0)
    {
        if (nextcolon != 0 && (nextcolon < nextslash || nextslash == 0))
        {
            hostlen = nextcolon - str;
            nextcolon++;

			if (!isdigit(*nextcolon))
			{
            	MEDIA_PRINTF_E("server port is not found, parse failed!\n");
				return -1;
			}
			
            url_t.Server_Port = 0;
            while (isdigit(*nextcolon))
            {
                url_t.Server_Port *= 10;
                url_t.Server_Port += (*nextcolon - '0');
                nextcolon++;
            }
            /*if (url_t.Server_Port == 0)
            {
                url_t.Server_Port = ip_addr.S1_Port;
            }*/
        }
        else
        {
            //hostlen = nextslash - str;
            //url_t.Server_Port = ip_addr.S1_Port;
            MEDIA_PRINTF_E("symbol : is not found, parse failed!\n");
			return -1;
        }

        if (hostlen == 0)
        {
            //set_url_default_value();
            MEDIA_PRINTF_E("the length of host is 0, parse failed!\n");
            return -1;
        }

        std::string server_name;

        server_name.assign(str, hostlen);
        url_t.Server_Name = server_name;
    }
    else
    {
        //set_url_default_value();
        MEDIA_PRINTF_E("symbol : and / is not found, parse failed!\n");
    }

    url_t.Server_ID = ip_addr.GH_IP;

    return 0;
}

void S1C1Client::build_SETUP_head()
{
    char s1_port[10] = "";

    setup_req.SETUP.clear();
    setup_req.SETUP += "SETUP rtsp://";
    setup_req.SETUP += url_t.Server_Name;
    setup_req.SETUP += ":";
    sprintf(s1_port, "%d", url_t.Server_Port);
    setup_req.SETUP += s1_port;
    setup_req.SETUP += "/;";
    setup_req.SETUP += "purchaseToken=";
    setup_req.SETUP += url_t.PurchaseToken;
    setup_req.SETUP += ";";
    setup_req.SETUP += "serverID=";
    setup_req.SETUP += url_t.Server_ID;
    setup_req.SETUP += " RTSP/1.0";

}

void S1C1Client::build_default_play_request()
{
    build_media_ctrl_request(MediaCtrlType_SEEK, default_paly_range);
}

