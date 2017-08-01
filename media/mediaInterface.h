/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     mediaInterface.h
* @version  1.0
* @brief
* @author   hejun
* @date     2017.04.20
* @note
**/

#ifndef _MEDIAINTERFACE_H_
#define _MEDIAINTERFACE_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h> 
#include "cloud_porting.h"
#include "S1C1Client.h"
#include "A7Client.h"
#ifdef __cplusplus
extern "C"{
#endif
#include "bstm_debug.h"
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef MEDIA_PRINTF_TURN_ON
#define MEDIA_PRINTF_I(fmt,args...) printf("[BSTMAPP_ITV_MEDIA][info]%s::%d: "fmt, __FUNCTION__, __LINE__ , ##args)
#define MEDIA_PRINTF_D(fmt,args...) printf("[BSTMAPP_ITV_MEDIA][debug]%s::%d: "fmt, __FUNCTION__, __LINE__ , ##args)
#define MEDIA_PRINTF_W(fmt,args...) printf("[BSTMAPP_ITV_MEDIA][warning]%s::%d: "fmt, __FUNCTION__, __LINE__ , ##args)
#define MEDIA_PRINTF_E(fmt,args...) printf("[BSTMAPP_ITV_MEDIA][error]%s::%d: "fmt, __FUNCTION__, __LINE__ , ##args)
#else
#define MEDIA_PRINTF_I(fmt,args...) BSTM_DEBUG_INFO("[BSTMAPP_ITV_MEDIA]"fmt, ##args)
#define MEDIA_PRINTF_D(fmt,args...) BSTM_DEBUG_DEBUG("[BSTMAPP_ITV_MEDIA]"fmt, ##args)
#define MEDIA_PRINTF_W(fmt,args...) BSTM_DEBUG_WARN("[BSTMAPP_ITV_MEDIA]"fmt, ##args)
#define MEDIA_PRINTF_E(fmt,args...) BSTM_DEBUG_ERROR("[BSTMAPP_ITV_MEDIA]"fmt, ##args)
#endif


void* thread_wait_all_other_thread_exit(void* arg);

int create_release_thread(S1C1Client* s1c1_client);

int poll_in(int fd, int timeout);

void* thread_send_C1_ping(void* arg);

pthread_t create_send_C1_ping_thread(S1C1Client* s1c1_client);

void start_timer_to_send_c1_ping(S1C1Client* s1c1_client);

void* thread_handle_C1_socket_msg(void* arg);

pthread_t create_handle_C1_msg_thread(S1C1Client* s1c1_client);

void* thread_send_S1_ping(void* arg);

pthread_t create_send_S1_ping_thread(S1C1Client* s1c1_client);

void* thread_handle_S1_socket_msg(void* arg);

pthread_t create_handle_S1_msg_thread(S1C1Client* s1c1_client);

void timer_callback_func(int sigNo);

void timer_c1_heartBeat(int sigNo);

int process_a7_msg(A7Client* a7_client);

int create_a7s1c1_client(S1C1Client* s1c1_client);

int process_s1_setup(S1C1Client* s1c1_client);

int process_c1_play(S1C1Client* s1c1_client);

int get_connect_timeout(S1C1Client* s1c1_client);

void set_global_info(MediaStatus_NotifyFunc func, C_MediaHandle s1c1_client, C_U32 uTimeOut);

void clear_call_back_func(C_MediaHandle s1c1_client);

void free_handle(S1C1Client* s1c1_client = NULL);

void close_S1C1_socket(S1C1Client* s1c1_client);

int play_media_interface(S1C1Client* s1c1_client);

void wait_resource_released(S1C1Client* s1c1_client);

C_MediaHandle client_Media_Open(C_U8* url, MediaStatus_NotifyFunc func, C_U32 uTimeOut);

C_RESULT client_Media_ctrl(C_MediaHandle hMediaHandle, C_MediaCtrlType uMediaCtrlType, C_S32 uValue);

C_RESULT client_Media_Close(C_MediaHandle hMediaHandle);

C_RESULT client_Get_Media_Info(C_MediaHandle  hMediaHandle, C_CStbMediaInfo* stMediaInfo);

#ifdef __cplusplus
} // extern "C"
#endif

#endif

