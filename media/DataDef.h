/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     DataDef.h
* @version  1.0
* @brief
* @author   hejun
* @date     2017.04.20
* @note
**/

#ifndef _DATADEF_H_
#define _DATADEF_H_

#ifdef __cplusplus
extern "C" {
#endif


#include <sys/socket.h>
#include <arpa/inet.h>
#include <string>
#include "cloud_porting.h"

#define MEDIA_PRINTF_TURN_ON
//#define STUB_TEST_TURN_ON

#define purchaseToken_MAX_LEN (128)
#define URL_MAX_LEN (1024)
#define SND_BUFF_DEFAULT_LEN (2048)
#define RECV_BUFF_DEFAULT_LEN (2048)

//#define RECV_MSG_POLL_TIME (15000) //ms
#define WAIT_PING_RESPONSE_TIMER (10000) //ms
#define C1_PING_TIMEOUT (60) //s
#define S1_PING_TIMEOUT (60) //s

enum RECV_MSG_STATUS_E
{
    RECV_STATUS_WAITING = 0,
    RECV_STATUS_FAULT,
    RECV_STATUS_OK
};

enum GET_PARAMETER_TYPE_E
{
    CONNECT_TIMEOUT =0,
    SESSION_LIST,
    POSITION,
    PRESENTATION_STATE,
    SCALE
};

enum MEDIA_STATUS_E
{
    MEDIA_NON_SETUP = 0,
    MEDIA_PLAY,
    MEDIA_PAUSE,
    MEDIA_STOP,
    MEDIA_CLOSE
};

typedef struct{

    std::string GH_IP;

    int A7_Port;

    int S1_Port;

    int C1_Port;

}IP_ADDR; //for creating socket

typedef struct  
{
    unsigned int freq;

    unsigned short serviceId;
}PROGRAM_INFO;

typedef struct {
	
    std::string     S1_PING_Req_Buf;

    std::string     C1_PING_Req_Buf;

    std::string     C1_Req_Buf;

    std::string     C1_Res_Buf;

    std::string     S1_Req_Buf;

    std::string     S1_Res_Buf;

    C_S32           S1_SocketID;

    C_S32           C1_SocketID;

	std::string 	param_url;

	std::string 	service_group;
}S1C1_Data_t;

typedef struct {

    //std::string		portalId;

    //std::string     client;

    //std::string     caRegionCode;

    //std::string     account;

    //std::string     titleProviderId;

    //std::string     format;

    //std::string     serviceId;

    std::string		TitleAssetId;

    std::string		AssetId;

    std::string		ServiceType;

    std::string		ChannelId;

    std::string     Req_Buf;

    std::string     Res_Buf;

    std::string		purchaseToken;
	
    std::string		serviceGroup;
	
	std::string 	s1_URL;	

    C_S32           SocketID;
}A7_Data_t;

typedef struct  
{
    std::string     Server_Name; 

    int             Server_Port;

    std::string     PurchaseToken;

    std::string     Server_ID;

	std::string		Rtsp_Url;
}URL_t;

typedef struct
{
    std::string SETUP; //use the input URL to build Setup

    std::string Cseq;

    std::string ClientSessionId;

    std::string Require;

    std::string Transport;

    std::string RtspUrl;

    std::string ServiceGroup;

	std::string AppData;
}Setup_Request_t;

typedef struct
{
    std::string Ping;

    std::string Cseq;

    std::string OnDemandSessionId;

    std::string Require;

    std::string Session;
}PING_Request_t;

typedef struct
{
    std::string TEARDOWN; 

    std::string Cseq;

    std::string Require;

    std::string Reason;

    std::string Session;

    std::string OnDemandSessionId;

    std::string ClientSessionId;
}TearDown_Request_t;

typedef struct
{
    std::string Get_Parameter;

    std::string Cseq;

    std::string Require;

    std::string Content_Type;

    std::string Session;

    std::string Content_Length;

    std::string Parameter_Type;
}GetParameter_Request_t;

typedef struct
{
    std::string     Session;            //from response of SETUP

    std::string     OnDemandSessionId;  //from response of SETUP

    PROGRAM_INFO    prog_info;          //from response of SETUP, in transport

    C_U32     TotalTime;                // 节目总时长，单位：秒  //from response of PLAY

    C_MediaStatusType  MediaStatusType; // 当前播放状态  //from response of GET_PARAMETER

    C_S8   uPlayScale;   				// 当前播放倍数  //from response of GET_PARAMETER

    C_U32  uCurrentTime; 				// 当前播放时间，单位：秒  //from response of GET_PARAMETER
}RTSP_Response_t;

typedef struct
{
    std::string     Cseq;

    std::string     Session;
}Announuce_Response_t;

typedef struct  
{
    std::string C1_URL;

    std::string Cseq;

    std::string Range;

    std::string Require;

    std::string Scale;

    std::string Session;
}C1_Request_t;

#ifdef __cplusplus
} // extern "C"
#endif


#endif

