/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     ClientBase.h
* @version  1.0
* @brief
* @author   hejun
* @date     2017.05.09
* @note
**/


#ifndef _CLIENTBASE_H_
#define _CLIENTBASE_H_

#include "DataDef.h"

class ClientBase
{
public:
    ClientBase();

    virtual ~ClientBase();

    C_S32 create_client_socket(int port, const char* ip);

    int send_client_request(C_S32 socketId, const char* sendBuf);

    int recv_client_response(C_S32 socketId, std::string& res_buf, int timeout);

    int close_client_socket(C_S32 socketId);

	int HexToNibble(char hex);

	int HexToByte(const char* buffer, unsigned char& b);

	std::string PercentDecode(const char* str);
	
	std::string UrlDecode(const char* str);

};


#endif
