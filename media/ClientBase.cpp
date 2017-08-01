/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     ClientBase.cpp
* @version  1.0
* @brief
* @author   hejun
* @date     2017.05.09
* @note
**/

#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h> 
#include <memory.h>
#include "mediaInterface.h"
#include "cloud_porting.h"
#include "ClientBase.h"

BSTM_DEBUG_DEFINE_MODULE("bstmapp.itv.media");


ClientBase::ClientBase()
{
}

ClientBase::~ClientBase()
{
}

C_S32 ClientBase::create_client_socket(int port, const char* ip)
{
    BSTM_DEBUG_ENTER();
    struct timeval t;

    int sock_cli = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_cli == -1)
    {
        MEDIA_PRINTF_E("socket client created failed! \n");
        return -1;
    }

    t.tv_sec = 2;
    t.tv_usec = 0;
    if (setsockopt(sock_cli, SOL_SOCKET, SO_SNDTIMEO, &t, sizeof(t)) < 0)
    {
    	MEDIA_PRINTF_E("setsockopt failed! \n");
        close(sock_cli);
        return -1;
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock_cli, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        MEDIA_PRINTF_E("connect socket server failed! \n");
        close(sock_cli);
        return -1;
    }

    return sock_cli;

}

int ClientBase::send_client_request(C_S32 socketId, const char* sendBuf)
{
    BSTM_DEBUG_ENTER();
    int data_len = -1;

    data_len = send(socketId, sendBuf, strlen(sendBuf), 0);
    if (-1 == data_len)
    {
        MEDIA_PRINTF_E("send data to socket server failed! \n");
    }
    else if (0 == data_len)
    {
        MEDIA_PRINTF_E("send data to socket server success. but data length : %d \n", data_len);
    }

    return data_len;
}

int ClientBase::recv_client_response(C_S32 socketId, std::string& res_buf, int timeout)
{
    int poll_ret;
    int data_len = -1;
    int sock_status = -1;
    res_buf.clear();;

    char recvbuf[RECV_BUFF_DEFAULT_LEN * 2];
    memset(recvbuf, 0, (SND_BUFF_DEFAULT_LEN * 2));

    poll_ret = poll_in(socketId, timeout);
    if (-2 == poll_ret)
    {
        //MY_PRINTF_D("poll socket::%d timeout\n", strerror(errno));
        sock_status = -2;
    }
    else if (-1 == poll_ret)
    {
        MEDIA_PRINTF_E("poll socket::%s error occurs\n", strerror(errno));
        sock_status = -1;
    }
    else
    {
        data_len = recv(socketId, recvbuf, sizeof(recvbuf), MSG_NOSIGNAL);
        MEDIA_PRINTF_I("socket %d receive data. data is: \n\"%s\"\n", socketId, recvbuf);

        if (data_len > 0)
        {
            res_buf = recvbuf;
			sock_status = 1;
        }
        else if (data_len == -1)
        {
            MEDIA_PRINTF_E("recv socket msg error.\n");
			sock_status = -1;
        }
        else if (data_len == 0)
        {
            MEDIA_PRINTF_E("socket server is closed.\n");
			sock_status = 0;
        }
    }

    return sock_status;
}

int ClientBase::close_client_socket(C_S32 socketId)
{
    BSTM_DEBUG_ENTER();
    if (-1 == socketId)
    {
        MEDIA_PRINTF_E("socket ID is invalid, can not close it.\n");
        return -1;
    }
    int ret = close(socketId);
    MEDIA_PRINTF_I("close socket id=%x.\n", socketId);

    return ret;
}

int ClientBase::HexToNibble(char hex)
{
    if (hex >= 'a' && hex <= 'f') {
        return ((hex - 'a') + 10);
    } else if (hex >= 'A' && hex <= 'F') {
        return ((hex - 'A') + 10);
    } else if (hex >= '0' && hex <= '9') {
        return (hex - '0');
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------
|   NPT_HexToByte
+---------------------------------------------------------------------*/
int ClientBase::HexToByte(const char* buffer, unsigned char& b)
{
    int nibble_0 = HexToNibble(buffer[0]);
    if (nibble_0 < 0) return -1;
    
    int nibble_1 = HexToNibble(buffer[1]);
    if (nibble_1 < 0) return -1;

    b = (nibble_0 << 4) | nibble_1;
    return 0;
}

std::string ClientBase::PercentDecode(const char* str)
{
    std::string decoded;

    // check args
    if (str == NULL) return decoded;

    // process each character
    while (unsigned char c = *str++) {
        if (c == '%') {
            // needs to be unescaped
            unsigned char unescaped;
            if (0 == HexToByte(str, unescaped)) {
                decoded += unescaped;
                str += 2;
            } else {
                // not a valid escape sequence, just keep the %
                decoded += c;
            }
        } else {
            // no unescaping required
            decoded += c;
        }
    }

    return decoded;
}

std::string ClientBase::UrlDecode(const char* str)
{
    std::string decoded = PercentDecode(str);

	std::string::size_type pos = 0;
	while((pos = decoded.find("+", pos)) != std::string::npos)
	{
		decoded.replace(pos, 1, " ");
		pos++;
	}
    return decoded;
}


