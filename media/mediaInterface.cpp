/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     mediaInterface.cpp
* @version  1.0
* @brief
* @author   hejun
* @date     2017.04.20
* @note
**/

#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <map>
#include "mediaInterface.h"
#include "S1C1Client.h"
#include "A7Client.h"
#include "ClientBase.h"
#include "DataDef.h"

BSTM_DEBUG_DEFINE_MODULE("bstmapp.itv.media");

C_MediaHandle g_current_s1c1_handle = NULL;
MediaStatus_NotifyFunc g_current_callback_func;
//MEDIA_STATUS_E g_media_status;
//std::map < C_MediaHandle, MEDIA_STATUS_E > g_handler_media_status;



void* thread_wait_all_other_thread_exit(void* arg)
{
    BSTM_DEBUG_ENTER();
    S1C1Client* s1c1_client = (S1C1Client*)arg;
	int ret;

    //waiting thread exit
    ret = pthread_join(s1c1_client->get_c1_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread c1 failed!\n");
	}
    ret = pthread_join(s1c1_client->get_s1_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread s1 failed!\n");
	}
    ret = pthread_join(s1c1_client->get_c1_ping_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread c1 ping failed!\n");
	}
    ret = pthread_join(s1c1_client->get_s1_ping_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread s1 ping failed!\n");
	}

    free_handle(s1c1_client);
	return NULL;
}

int create_release_thread(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, thread_wait_all_other_thread_exit, (void*)s1c1_client);
    if (ret != 0)
    {
        MEDIA_PRINTF_E("Create pthread thread_wait_thread_exit error!\n");
        return -1;
    }
    MEDIA_PRINTF_I("Create pthread thread_wait_thread_exit Done!.\n");

    return tid;
}

/*
* poll fd to see whether there are data?
* timeout: second
* */
// return -1 if error, -2 if timeout
int poll_in(int fd, int timeout)
{
    int ret;
    struct pollfd pollit;

    pollit.fd = fd;
    pollit.events = POLLIN | POLLPRI;
    pollit.revents = 0;

    while (1) {
        ret = poll(&pollit, 1, timeout);
        if (ret == 0) { //timeout 
            //MY_PRINTF_D("poll socket(): time out\n");
            return -2;
        }
        else if (ret == -1 && errno == EINTR) {
            MEDIA_PRINTF_E("errno == EINTR\n");
            continue;
        }
        else if (ret < 0) {
            return -1;
        }
        else {
            break;
        }
    }
    return 0;
}

void start_timer_to_send_c1_ping(S1C1Client* s1c1_client)
{
    C_U32 timeout = s1c1_client->get_c1_ping_timeout() / 2;
    signal(SIGALRM, timer_c1_heartBeat);
    s1c1_client->set_c1_heart_beat_timer(timeout);
}


void* thread_send_C1_ping(void* arg)
{
    BSTM_DEBUG_ENTER();
    S1C1Client* s1c1_client = (S1C1Client*)arg;
    int ret;

    while (1)
    {
    	int sleep_time = s1c1_client->get_c1_ping_timeout() / 2;
		s1c1_client->set_c1_ping_sleep_time(sleep_time);
		while (sleep_time > 0)
		{
			sleep(1);
			
			pthread_mutex_lock(&s1c1_client->get_c1_ping_mutex());
			sleep_time = s1c1_client->get_c1_ping_sleep_time();
			sleep_time--;
			s1c1_client->set_c1_ping_sleep_time(sleep_time);
			pthread_mutex_unlock(&s1c1_client->get_c1_ping_mutex());
			
			//if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
			if (MEDIA_CLOSE == s1c1_client->get_media_status())
			{
	            MEDIA_PRINTF_D("thread is: %x, media is closed. exit C1 sending thread... \n", (unsigned int)pthread_self());

	            return NULL;
			}			
		}

        //if (MEDIA_STOP == g_handler_media_status[(C_MediaHandle)s1c1_client])
        if (MEDIA_STOP == s1c1_client->get_media_status())
        {
            continue;
        }
		
		s1c1_client->build_getparam_request(CONNECT_TIMEOUT, true);
		
		ret = s1c1_client->send_getparam_request(true);
        if (ret > 0)
        {
            MEDIA_PRINTF_D("C1: send PING done. seq is %d \n", s1c1_client->get_c1_ping_seq());
        }
		else
		{
            MEDIA_PRINTF_D("thread is: %x, socket error. exit C1 ping thread... \n", (unsigned int)pthread_self());

            s1c1_client->get_callback()((C_MediaHandle)s1c1_client, MediaNotifyType_Other, 0, Media_ServerErr);
            MEDIA_PRINTF_D("send callback done!.\n");
		}
		
        //if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
        if (MEDIA_CLOSE == s1c1_client->get_media_status())
        {
            MEDIA_PRINTF_D("thread is: %x, media is closed. exit C1 sending thread... \n", (unsigned int)pthread_self());

            return NULL;
        }
		
        //waiting ping
        s1c1_client->wait_C1_ping_response();
    }    

	return NULL;
}

pthread_t create_send_C1_ping_thread(S1C1Client* s1c1_client)
{
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, thread_send_C1_ping, (void*)s1c1_client);
    if (ret != 0)
    {
        MEDIA_PRINTF_E("Create C1 pthread error!\n");
        return -1;
    }
    MEDIA_PRINTF_I("Create C1 ping pthread Done!.\n");

    return tid;
}


void* thread_handle_C1_socket_msg(void* arg)
{
    BSTM_DEBUG_ENTER();
    int ret;
    S1C1Client* s1c1_client = (S1C1Client*)arg;    
    
    while (1)
    {
        //if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
        if (MEDIA_CLOSE == s1c1_client->get_media_status())
        {
            MEDIA_PRINTF_D("thread is: %x, media is closed. exit C1 thread... \n", (unsigned int)pthread_self());
            return NULL;
        }

        //waiting C1 announce, PLAY, PAUSE, GET_PARAMETER response, PING(GET_PARAMETER connect_time)
        ret = s1c1_client->recv_C1_Msg();
        if ((0 == ret) || (-1 == ret))
        {
            MEDIA_PRINTF_D("thread is: %x. exit C1 thread... \n", (unsigned int)pthread_self());

            s1c1_client->get_callback()((C_MediaHandle)s1c1_client, MediaNotifyType_Other, 0, Media_ServerErr);
            MEDIA_PRINTF_D("send callback done!.\n");
			//g_handler_media_status[(C_MediaHandle)s1c1_client] = MEDIA_CLOSE;
			s1c1_client->set_media_status(MEDIA_CLOSE);
			return NULL;
        }
    }

    return NULL;
}

pthread_t create_handle_C1_msg_thread(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, thread_handle_C1_socket_msg, (void*)s1c1_client);
    if (ret != 0)
    {
        MEDIA_PRINTF_E("Create C1 pthread error!\n");
        return -1;
    }
    MEDIA_PRINTF_I("Create C1 pthread Done!.\n");

    //pthread_join(tid, NULL);
    //pthread_detach(tid);
    return tid;
}


void* thread_send_S1_ping(void* arg)
{
    BSTM_DEBUG_ENTER();
    S1C1Client* s1c1_client = (S1C1Client*)arg;
    int ret;

    while (1)
    {
    	int counter = s1c1_client->get_s1_ping_timeout() / 2;
		while (counter > 0)
		{
			sleep(1);
			counter--;
			
			//if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
			if (MEDIA_CLOSE == s1c1_client->get_media_status())
			{
	            MEDIA_PRINTF_D("thread is: %x, media is closed. exit S1 sending thread... \n", (unsigned int)pthread_self());

	            return NULL;
			}			
		}

        //if (MEDIA_STOP == g_handler_media_status[(C_MediaHandle)s1c1_client])
        if (MEDIA_STOP == s1c1_client->get_media_status())
        {
            continue;
        }

        s1c1_client->build_S1_ping_request();

        ret = s1c1_client->send_s1_heartBeat_request();
        if (ret > 0)
        {
            MEDIA_PRINTF_D("S1: send PING done. seq is %d \n", s1c1_client->get_s1_ping_seq());
        }
		else
		{
            MEDIA_PRINTF_D("thread is: %x, socket error. exit S1 ping thread... \n", (unsigned int)pthread_self());

            s1c1_client->get_callback()((C_MediaHandle)s1c1_client, MediaNotifyType_Other, 0, Media_ServerErr);
            MEDIA_PRINTF_D("send callback done!.\n");
		}
		
        //if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
        if (MEDIA_CLOSE == s1c1_client->get_media_status())
        {
            MEDIA_PRINTF_D("thread is: %x, media is closed. exit S1 sending thread... \n", (unsigned int)pthread_self());

            return NULL;
        }
		
        //waiting ping
        s1c1_client->wait_S1_ping_response();
    }
	
    return NULL;
}

pthread_t create_send_S1_ping_thread(S1C1Client* s1c1_client)
{
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, thread_send_S1_ping, (void*)s1c1_client);
    if (ret != 0)
    {
        MEDIA_PRINTF_E("Create S1 pthread error!\n");
        return -1;
    }
    MEDIA_PRINTF_I("Create S1 ping pthread Done!.\n");

    return tid;
}

void* thread_handle_S1_socket_msg(void* arg)
{
    BSTM_DEBUG_ENTER();
    int ret;
    S1C1Client* s1c1_client = (S1C1Client*)arg;
    
	//check the announce, teardown, setup, S1 PING
    while (1)
    {
        //if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
        if (MEDIA_CLOSE == s1c1_client->get_media_status())
        {
            MEDIA_PRINTF_D("thread is: %x, media is closed. exit S1 thread... \n", (unsigned int)pthread_self());
            
            return NULL;
        }

        ret = s1c1_client->recv_S1_Msg();
        if ((0 == ret) || (-1 == ret))
        {
            MEDIA_PRINTF_D("thread is: %x. exit S1 thread... \n", (unsigned int)pthread_self());
			
            s1c1_client->get_callback()((C_MediaHandle)s1c1_client, MediaNotifyType_Other, 0, Media_ServerErr);
            MEDIA_PRINTF_D("send callback done!.\n");
			//g_handler_media_status[(C_MediaHandle)s1c1_client] = MEDIA_CLOSE;
			s1c1_client->set_media_status(MEDIA_CLOSE);
            return NULL;
        }
    }
	
    return NULL;
}

pthread_t create_handle_S1_msg_thread(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    pthread_t tid;
    int ret;

    ret = pthread_create(&tid, NULL, thread_handle_S1_socket_msg, (void*)s1c1_client);
    if (ret != 0)
    {
        MEDIA_PRINTF_E("Create S1 pthread error!\n");
        return -1;
    }
    MEDIA_PRINTF_I("Create S1 pthread Done!.\n");
    return tid;
}

void timer_callback_func(int sigNo)
{
    BSTM_DEBUG_ENTER();
    MEDIA_PRINTF_I("callback func timeout expired. incoming sigNo : %d. callback func is called. \n", sigNo);
    g_current_callback_func(g_current_s1c1_handle, MediaNotifyType_Other, 0, Media_ServerErr);
	MEDIA_PRINTF_D("send callback done!.\n");
}

void timer_c1_heartBeat(int sigNo)
{
    BSTM_DEBUG_ENTER();
    int ret;

    S1C1Client* s1c1_client = (S1C1Client*)g_current_s1c1_handle;

    //if (MEDIA_CLOSE == g_handler_media_status[(C_MediaHandle)s1c1_client])
    if (MEDIA_CLOSE == s1c1_client->get_media_status())
    {
        s1c1_client->set_c1_heart_beat_timer(0);

        ret = pthread_join(s1c1_client->get_c1_tid(), NULL);
		if (ret != 0)
		{
			MEDIA_PRINTF_E("join thread c1 failed!\n");
		}
		
        ret = pthread_join(s1c1_client->get_s1_tid(), NULL);
		if (ret != 0)
		{
			MEDIA_PRINTF_E("join thread s1 failed!\n");
		}
		
        ret = pthread_join(s1c1_client->get_s1_ping_tid(), NULL);
		if (ret != 0)
		{
			MEDIA_PRINTF_E("join thread s1 ping failed!\n");
		}

        MEDIA_PRINTF_D("timer_c1_heartBeat(): all child thread exit, free the handler!!\n");
        free_handle(s1c1_client);
        g_current_s1c1_handle = NULL;

        return;
    }

    //if (MEDIA_STOP == g_handler_media_status[(C_MediaHandle)s1c1_client])
    if (MEDIA_STOP == s1c1_client->get_media_status())
    {
        C_U32 timeout = s1c1_client->get_c1_ping_timeout() / 2;
        s1c1_client->set_c1_heart_beat_timer(timeout);
        return;
    }

    s1c1_client->build_getparam_request(CONNECT_TIMEOUT, true);

    ret = s1c1_client->send_getparam_request(true);
    if (ret <= 0)
    {
        MEDIA_PRINTF_D("timer_c1_heartBeat(), socket error. stop c1 heart beat... \n");
        s1c1_client->get_callback()(s1c1_client, MediaNotifyType_Other, 0, Media_ServerErr);
		MEDIA_PRINTF_D("send callback done!.\n");
		return;
    }
    else
    {
        MEDIA_PRINTF_D("C1: send PING done. seq is %d\n", s1c1_client->get_c1_ping_seq());
    }

    //waiting ping
    s1c1_client->wait_C1_ping_response();

    C_U32 timeout = s1c1_client->get_c1_ping_timeout() / 2;
    s1c1_client->set_c1_heart_beat_timer(timeout);
}

int process_a7_msg(A7Client* a7_client)
{
    BSTM_DEBUG_ENTER();
    int ret = 0;
	
	if (0 != a7_client->set_dynamic_data())
	{
		MEDIA_PRINTF_E("set service group failed!\n");
		return -1;
	}

	if ((1 == a7_client->get_playType()) || (2 == a7_client->get_playType()))
	{
		a7_client->build_A7_SelectionStart_msg();
	}
	else if ((3 == a7_client->get_playType()) || (4 == a7_client->get_playType()))
	{
		a7_client->build_A7_ChannelSelectionStart_msg();
	}
	else
	{
		MEDIA_PRINTF_E("play type is not be recognized. can't build A7 msg!\n");
		return -1;
	}
    
    ret = a7_client->send_A7_selection_request();
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("send A7 msg done!\n");

    ret = a7_client->recv_A7_selection_response();
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("recv A7 msg done!\n");

    return ret;
}

int create_a7s1c1_client(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    int ret = 0;

    /*ret = a7_client->create_A7_client();
    if (-1 == ret)
    {
        return -1;
    }
    MEDIA_PRINTF_I("create A7 client done!\n");*/

    ret = s1c1_client->create_S1_client();
    if (-1 == ret)
    {
        return -1;
    }
    MEDIA_PRINTF_I("create S1 client done!\n");

    ret = s1c1_client->create_C1_client();
    if (-1 == ret)
    {
        return -1;
    }
    MEDIA_PRINTF_I("create C1 client done!\n");

    return ret;
}

int process_s1_setup(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    int ret = 0;

	//s1c1_client->set_purchaseToken(a7_client);
    ret = s1c1_client->build_setup_request();
    if (ret < 0)
    {
        return -1;
    }
    ret = s1c1_client->send_setup_request();
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("send SETUP done!\n");

    ret = s1c1_client->wait_S1_response();
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("recv SETUP done!\n");

    return ret;
}

int process_c1_play(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    int ret = 0;
    
    s1c1_client->build_default_play_request();
    ret = s1c1_client->send_C1_request();
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("send default play done!\n");

    ret = s1c1_client->wait_C1_response();
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("recv default play done!\n");

    return ret;
}

int get_connect_timeout(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    int ret;
    s1c1_client->build_getparam_request(CONNECT_TIMEOUT, true);
    ret = s1c1_client->send_getparam_request(true);
    if (ret <= 0)
    {
        return -1;
    }
    MEDIA_PRINTF_I("send getparam CONNECT_TIMEOUT done!\n");

    ret = s1c1_client->wait_C1_ping_response();
    if (ret <= 0)
    {
        return ret;
    }
    MEDIA_PRINTF_I("recv getparam CONNECT_TIMEOUT done!\n");

    return ret;
}

void set_global_info(MediaStatus_NotifyFunc func, C_MediaHandle s1c1_client, C_U32 uTimeOut)
{
    BSTM_DEBUG_ENTER();
    g_current_s1c1_handle = s1c1_client;
    g_current_callback_func = func;

    //A7Client* a7_cli = (A7Client*)a7_client;
    //a7_cli->set_callback_timer(uTimeOut);

    S1C1Client* s1c1_cli = (S1C1Client*)s1c1_client;
    s1c1_cli->set_callback(func);
    s1c1_cli->set_timer_callback_func(uTimeOut);
}

void clear_call_back_func(C_MediaHandle s1c1_client)
{
    BSTM_DEBUG_ENTER();
    S1C1Client* client = (S1C1Client*)s1c1_client;
    client->clear_timer_callback_func();
}

void free_handle(S1C1Client* s1c1_client)
{
    BSTM_DEBUG_ENTER();
    /*if (NULL != a7_client)
    {
        delete a7_client;
        a7_client = NULL;
    }*/

    if (NULL != s1c1_client)
    {
    	//g_handler_media_status[(C_MediaHandle)s1c1_client] = MEDIA_CLOSE;
		s1c1_client->set_media_status(MEDIA_CLOSE);
        delete s1c1_client;
        s1c1_client = NULL;
    }
}

void close_S1C1_socket(S1C1Client* s1c1_client)
{
    s1c1_client->close_client_socket(s1c1_client->get_S1_sockId());
    s1c1_client->close_client_socket(s1c1_client->get_C1_sockId());

    s1c1_client->set_S1_sockId(-1);
    s1c1_client->set_C1_sockId(-1);
}

int play_media_interface(S1C1Client* s1c1_client)
{
    s1c1_client->play_media();

	return 1;
}

void wait_resource_released(S1C1Client* s1c1_client)
{	
	int ret;
	
    s1c1_client->set_c1_heart_beat_timer(0);

    ret = pthread_join(s1c1_client->get_c1_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread c1 ping failed!\n");
	}
	
    ret = pthread_join(s1c1_client->get_s1_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread s1 ping failed!\n");
	}
	
    ret = pthread_join(s1c1_client->get_s1_ping_tid(), NULL);
	if (ret != 0)
	{
		MEDIA_PRINTF_E("join thread s1 ping failed!\n");
	}

    MEDIA_PRINTF_I("all child thread exit, free the handler!!\n");
    free_handle(s1c1_client);
    g_current_s1c1_handle = NULL;

    return;
}

C_MediaHandle client_Media_Open(C_U8* url, MediaStatus_NotifyFunc func, C_U32 uTimeOut)
{
    int ret;
    bool is_suc = false;
    int s1_tid, c1_tid, s1_ping_tid;
    //A7Client* a7_client = NULL;
    S1C1Client* s1c1_client = NULL;
    //g_media_status = MEDIA_NON_SETUP;

	/*
    a7_client = new A7Client();
    if (NULL == a7_client)
    {
        return NULL;
    }
	
	is_suc = a7_client->parse_url((char*)url);
    if (false == is_suc)
    {
        free_handle(a7_client, NULL);
        return NULL;
    }
    */

    s1c1_client = new S1C1Client();
    if (NULL == s1c1_client)
    {
        free_handle(s1c1_client);
        return NULL;
    }

	ret = s1c1_client->init_data(url);
	if (ret < 0)
	{
        free_handle(s1c1_client);
        return NULL;
	}

    //start timer to call callback if timeout
    set_global_info(func, (C_MediaHandle)s1c1_client, uTimeOut);

    ret = create_a7s1c1_client(s1c1_client);
	if (ret < 0)
	{
		clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(s1c1_client);
        return NULL;
	}

    //create thread for receiving msg from S1 socket
    s1_tid = (int)create_handle_S1_msg_thread(s1c1_client);
    if (s1_tid == -1)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(s1c1_client);
        return NULL;
    }
    s1c1_client->set_s1_tid(s1_tid);

    //create thread for receiving msg from C1 socket
    c1_tid = (int)create_handle_C1_msg_thread(s1c1_client);
    if (c1_tid == -1)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(s1c1_client);
        return NULL;
    }
    s1c1_client->set_c1_tid(c1_tid);

	/*
	ret = process_a7_msg(a7_client);
    if (ret < 0)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(a7_client, s1c1_client);
        return NULL;
    }
    */

    ret = process_s1_setup(s1c1_client);
    if (ret < 0)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(s1c1_client);
        return NULL;
    }

    //create thread for send S1 ping
    s1_ping_tid = create_send_S1_ping_thread(s1c1_client);
    if (s1_ping_tid == -1)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(s1c1_client);
        return NULL;
    }
    s1c1_client->set_s1_ping_tid(s1_ping_tid);

    ret = process_c1_play(s1c1_client);
    if (ret < 0)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(s1c1_client);
        return NULL;
    }

    /*ret = get_connect_timeout(s1c1_client);
    if (ret <= 0 && ret != -2)
    {
        clear_call_back_func((C_MediaHandle)s1c1_client);
        free_handle(a7_client, s1c1_client);
        return NULL;
    }*/

    //it is about to return, no need timer to call callback
    clear_call_back_func((C_MediaHandle)s1c1_client);

    start_timer_to_send_c1_ping(s1c1_client);

    //A7 will not be used anymore
    //free_handle(a7_client);

    //call the API to play the media stream
    play_media_interface(s1c1_client);

    //g_handler_media_status[(C_MediaHandle)s1c1_client] = MEDIA_PLAY;
	s1c1_client->set_media_status(MEDIA_PLAY);
	MEDIA_PRINTF_D("client_Media_Open() is done!\n");
    return (C_MediaHandle)s1c1_client;
}

C_RESULT client_Media_ctrl(C_MediaHandle hMediaHandle, C_MediaCtrlType uMediaCtrlType, C_S32 uValue)
{
    int ret;
    S1C1Client* s1c1_client = (S1C1Client*)hMediaHandle;
    MEDIA_STATUS_E cur_status = s1c1_client->get_media_status();
    //MEDIA_STATUS_E cur_status = g_media_status;
    //MEDIA_STATUS_E cur_status = g_handler_media_status[hMediaHandle];

	if (MEDIA_CLOSE== cur_status)
	{
    	MEDIA_PRINTF_D("media has been closed!\n");
		return CLOUD_FAILURE;
	}

    switch (uMediaCtrlType)
    {
    case MediaCtrlType_PLAY:
        if ((cur_status != MEDIA_PAUSE) && (cur_status != MEDIA_STOP))
        {
    		MEDIA_PRINTF_D("media has been played!\n");
			s1c1_client->set_err_code(Media_Unsupport);
            return CLOUD_FAILURE;
        }

        if (MEDIA_STOP == cur_status)
        {
            //1,send SETUP to S1 socket. 2, send ctrl request to C1 socket
            ret = s1c1_client->build_setup_request();
            if (ret < 0)
            {
            return CLOUD_FAILURE;
            }
			
            ret = s1c1_client->send_setup_request();
            if (ret <= 0)
            {
            return CLOUD_FAILURE;
            }

            ret = s1c1_client->wait_S1_response();
            if (ret <= 0)
            {
            return CLOUD_FAILURE;
            }

            s1c1_client->build_media_ctrl_request(uMediaCtrlType, uValue);

            ret = s1c1_client->send_C1_request();
            if (ret <= 0)
            {
            return CLOUD_FAILURE;
            }

            ret = s1c1_client->wait_C1_response();
            if (ret <= 0)
            {
            return CLOUD_FAILURE;
            }

            //call the API to play the media stream
            play_media_interface(s1c1_client);
        }
        else if (MEDIA_PAUSE == cur_status)
        {
            //send ctrl request to C1 socket
            s1c1_client->build_media_ctrl_request(uMediaCtrlType, uValue);

            ret = s1c1_client->send_C1_request();
            if (ret <= 0)
            {
            return CLOUD_FAILURE;
            }

            ret = s1c1_client->wait_C1_response();
            if (ret <= 0)
            {
            return CLOUD_FAILURE;
            }
        }

        //g_media_status = MEDIA_PLAY;
        //g_handler_media_status[hMediaHandle] = MEDIA_PLAY;
		s1c1_client->set_media_status(MEDIA_PLAY);
    	break;

    case MediaCtrlType_PAUSE:
        if (cur_status != MEDIA_PLAY)
        {
    		MEDIA_PRINTF_D("media has been paused!\n");
			s1c1_client->set_err_code(Media_Unsupport);
            return CLOUD_FAILURE;
        }

        //send ctrl request to C1 socket
        s1c1_client->build_media_ctrl_request(uMediaCtrlType, uValue);

        ret = s1c1_client->send_C1_request();
        if (ret <= 0)
        {
        return CLOUD_FAILURE;
        }

        ret = s1c1_client->wait_C1_response();
        if (ret <= 0)
        {
        return CLOUD_FAILURE;
        }

        //g_media_status = MEDIA_PAUSE;
        //g_handler_media_status[hMediaHandle] = MEDIA_PAUSE;
		s1c1_client->set_media_status(MEDIA_PAUSE);
        break;

    case MediaCtrlType_SEEK:
    case MediaCtrlType_SETSCALE:
        if (MEDIA_STOP == cur_status)
        {
    		MEDIA_PRINTF_D("media has been stoped!\n");
			s1c1_client->set_err_code(Media_Unsupport);
            return CLOUD_FAILURE;
        }

        //send ctrl request to C1 socket
        s1c1_client->build_media_ctrl_request(uMediaCtrlType, uValue);

        ret = s1c1_client->send_C1_request();
        if (ret <= 0)
        {
        return CLOUD_FAILURE;
        }

        ret = s1c1_client->wait_C1_response();
        if (ret <= 0)
        {
        return CLOUD_FAILURE;
        }

        //g_media_status = MEDIA_PLAY;
        //g_handler_media_status[hMediaHandle] = MEDIA_PLAY;
        s1c1_client->set_media_status(MEDIA_PLAY);
        break;

    case MediaCtrlType_STOP:
        if (MEDIA_STOP == cur_status)
        {
    		MEDIA_PRINTF_D("media has been stoped!\n");
			s1c1_client->set_err_code(Media_Unsupport);
            return CLOUD_FAILURE;
        }
        //send teardown request
        s1c1_client->build_teardown_request();
        ret = s1c1_client->send_teardown_request();
        if (ret <= 0)
        {
        return CLOUD_FAILURE;
        }

        ret = s1c1_client->wait_S1_response();
        if (ret <= 0)
        {
        return CLOUD_FAILURE;
        }

        //g_media_status = MEDIA_STOP;
        //g_handler_media_status[hMediaHandle] = MEDIA_STOP;
        s1c1_client->set_media_status(MEDIA_STOP);
        break;
    }

    s1c1_client->set_c1_heart_beat_timer(s1c1_client->get_c1_ping_timeout() / 2);
    return CLOUD_OK;
}

C_RESULT client_Media_Close(C_MediaHandle hMediaHandle)
{
    int ret;
    S1C1Client* s1c1_client = (S1C1Client*)hMediaHandle;
    //MEDIA_STATUS_E cur_status = g_handler_media_status[hMediaHandle];
    MEDIA_STATUS_E cur_status = s1c1_client->get_media_status();

	if (MEDIA_CLOSE== cur_status)
	{
    	MEDIA_PRINTF_D("media has been closed!\n");
		return CLOUD_OK;
	}

    //send teardown request
    s1c1_client->build_teardown_request();
    ret = s1c1_client->send_teardown_request();
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("send teardown request done!\n");

    //ret = s1c1_client->recv_RTSP_teardown_response();
    ret = s1c1_client->wait_S1_response();
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("recv teardown response done!\n");
    
    //g_handler_media_status[hMediaHandle] = MEDIA_CLOSE;
	s1c1_client->set_media_status(MEDIA_CLOSE);
    MEDIA_PRINTF_I("set flag MEDIA_CLOSE to ask threads to finish!.\n");

    wait_resource_released(s1c1_client);

    return CLOUD_OK;
}

C_RESULT client_Get_Media_Info(C_MediaHandle hMediaHandle, C_CStbMediaInfo* stMediaInfo)
{
    int ret;
    S1C1Client* s1c1_client = (S1C1Client*)hMediaHandle;
    //MEDIA_STATUS_E cur_status = g_handler_media_status[hMediaHandle];
    MEDIA_STATUS_E cur_status = s1c1_client->get_media_status();

	if ((MEDIA_STOP == cur_status) || (MEDIA_CLOSE== cur_status))
	{
    	MEDIA_PRINTF_D("media has been stoped or closed!\n");
		return CLOUD_FAILURE;
	}
	
    //send get_parameter request : position
    s1c1_client->build_getparam_request(POSITION, false);
    ret = s1c1_client->send_getparam_request(false);
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("send getparam POSITION done!\n");

    ret = s1c1_client->wait_C1_response();
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("recv getparam POSITION done!\n");

    //send get_parameter request : presentation_state
    s1c1_client->build_getparam_request(PRESENTATION_STATE, false);
    ret = s1c1_client->send_getparam_request(false);
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("send getparam presentation_state done!\n");

    ret = s1c1_client->wait_C1_response();
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("recv getparam presentation_state done!\n");

    //send get_parameter request : scale
    s1c1_client->build_getparam_request(SCALE, false);
    ret = s1c1_client->send_getparam_request(false);
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("send getparam SCALE done!\n");

    ret = s1c1_client->wait_C1_response();
    if (ret <= 0)
    {
    return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("recv getparam SCALE done!\n");

    memset(stMediaInfo->PlayUrl, 0, sizeof(C_U8)*MAX_INFO_LEN);
    memcpy(stMediaInfo->PlayUrl, s1c1_client->get_media_info().PlayUrl, strlen((const char*)s1c1_client->get_media_info().PlayUrl));
    stMediaInfo->MediaStatusType = s1c1_client->get_media_info().MediaStatusType;
    stMediaInfo->uCurrentTime = s1c1_client->get_media_info().uCurrentTime;
    stMediaInfo->uPlayScale = s1c1_client->get_media_info().uPlayScale;
    stMediaInfo->uTotleTime = s1c1_client->get_media_info().uTotleTime;
	
    MEDIA_PRINTF_I("media info: status=%u. current_time=%u. scale=%d. total_time=%u.\n", (C_U32)stMediaInfo->MediaStatusType, stMediaInfo->uCurrentTime, stMediaInfo->uPlayScale, stMediaInfo->uTotleTime);

    s1c1_client->set_c1_heart_beat_timer(s1c1_client->get_c1_ping_timeout() / 2);
    return CLOUD_OK;
}

/*
参数url:
url首先应该包括type
type值为2，表示点播，url参数需要包括type，pid，paid；
type值为3，表示回看，url参数需要包括type，pid，paid；
type值为4，表示时移，url参数需要包括type，channelId；

比如点播
char url[] = "rtsp://192.168.1.202:554/?type=2&paid=GTIT0120170426200218&pid=10008"
*/
C_MediaHandle CStb_MediaOpen(C_U8* url, MediaStatus_NotifyFunc func, C_U32 uTimeOut)
{
    C_MediaHandle handler = NULL;

    if (NULL == url || NULL == func)
    {
        MEDIA_PRINTF_E("the input parameter is incorrect! url or func is NULL! \n");
        return NULL;
    }
    MEDIA_PRINTF_I("param url = %s; uTimeOut = %u\n", url, uTimeOut);

    if (NULL != g_current_s1c1_handle)
    {
        MEDIA_PRINTF_E("some media is running! \n");
        return NULL;
    }
	
    handler = client_Media_Open(url, func, uTimeOut);
    MEDIA_PRINTF_I("return handler = 0x%x\n", (unsigned int)handler);
    if (NULL == handler)
    {
        if (NULL != g_current_s1c1_handle)
        {
            func(g_current_s1c1_handle, MediaNotifyType_Other, 0, ((S1C1Client*)g_current_s1c1_handle)->get_err_code());
			g_current_s1c1_handle = NULL;
        }
    	return NULL;
    }
    else
    {
        func(g_current_s1c1_handle, MediaNotifyType_Open, 0, Media_Ok);
    }
	
    return handler;
}

C_RESULT CStb_MediaCtrl(C_MediaHandle hMediaHandle, C_MediaCtrlType uMediaCtrlType, C_S32 uValue)
{
    C_RESULT ret;
    S1C1Client* s1c1_client = (S1C1Client*)hMediaHandle;
	
    MEDIA_PRINTF_I("parameter hMediaHandle = 0x%x; uMediaCtrlType = %d; uValue = %d.\n", (unsigned int)hMediaHandle, (int)uMediaCtrlType, uValue);
    if (NULL == hMediaHandle)
    {
        MEDIA_PRINTF_E("the input parameter is incorrect! hMediaHandle is NULL! \n");
        return CLOUD_FAILURE;
    }

	s1c1_client->set_err_code(Media_ServerInvalid);

	MEDIA_PRINTF_I("parameter uMediaCtrlType is %d, uValue is %d\n", uMediaCtrlType, uValue);
    ret = client_Media_ctrl(hMediaHandle, uMediaCtrlType, uValue);

	MediaStatus_NotifyFunc func = s1c1_client->get_callback();
    if (CLOUD_FAILURE == ret)
    {
    	func(hMediaHandle, MediaNotifyType_Other, 0, (s1c1_client->get_err_code()));
		MEDIA_PRINTF_D("send callback done!.\n");
        return CLOUD_FAILURE;
    }
    
	switch(uMediaCtrlType)
	{
        case MediaCtrlType_PLAY:
    	    func(hMediaHandle, MediaNotifyType_Play, 0, Media_Ok);
            break;
        case MediaCtrlType_PAUSE:
            func(hMediaHandle, MediaNotifyType_Pause, 0, Media_Ok);
            break;
        case MediaCtrlType_SEEK:
            func(hMediaHandle, MediaNotifyType_Seek, uValue, Media_Ok);
            break;
        case MediaCtrlType_SETSCALE:
            func(hMediaHandle, MediaNotifyType_SetScale, uValue, Media_Ok);
            break;
        case MediaCtrlType_STOP:
            func(hMediaHandle, MediaNotifyType_Stop, 0, Media_Ok);
            break;
	}
	MEDIA_PRINTF_D("send callback done!.\n");

    return CLOUD_OK;
}

C_RESULT CStb_MediaClose(C_MediaHandle hMediaHandle)
{
    C_RESULT ret;
    S1C1Client* s1c1_client = (S1C1Client*)hMediaHandle;

	MEDIA_PRINTF_I("CStb_MediaClose is called.\n");
    if (NULL == hMediaHandle)
    {
        MEDIA_PRINTF_E("the input parameter is incorrect! hMediaHandle is NULL! \n");
        return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("param hMediaHandle = 0x%x\n", (unsigned int)hMediaHandle);

	s1c1_client->set_err_code(Media_ServerInvalid);

    ret = client_Media_Close(hMediaHandle);
	
	MediaStatus_NotifyFunc func = s1c1_client->get_callback();
    if (CLOUD_FAILURE == ret)
    {
    	func(hMediaHandle, MediaNotifyType_Other, 0, s1c1_client->get_err_code());
		MEDIA_PRINTF_D("send callback done!.\n");
        return CLOUD_FAILURE;
    }
	
    func(hMediaHandle, MediaNotifyType_Close, 0, Media_Ok);
	MEDIA_PRINTF_D("send callback done!.\n");
    return CLOUD_OK;
}

C_RESULT CStb_GetMediaInfo(C_MediaHandle  hMediaHandle, C_CStbMediaInfo* stMediaInfo)
{
    C_RESULT ret;
    S1C1Client* s1c1_client = (S1C1Client*)hMediaHandle;
	
	MEDIA_PRINTF_I("CStb_GetMediaInfo is called.\n");
    if (NULL == hMediaHandle)
    {
        MEDIA_PRINTF_E("the input parameter is incorrect! hMediaHandle is NULL! \n");
        return CLOUD_FAILURE;
    }
    MEDIA_PRINTF_I("param hMediaHandle = 0x%x\n", (unsigned int)hMediaHandle);

	s1c1_client->set_err_code(Media_ServerInvalid);

    ret = client_Get_Media_Info(hMediaHandle, stMediaInfo);
	
	MediaStatus_NotifyFunc func = s1c1_client->get_callback();
    if (CLOUD_FAILURE == ret)
    {
    	func(hMediaHandle, MediaNotifyType_Other, 0, s1c1_client->get_err_code());
		MEDIA_PRINTF_D("send callback done!.\n");
        return CLOUD_FAILURE;
    }

    return CLOUD_OK;
}




