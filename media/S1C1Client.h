/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     S1C1Client.h
* @version  1.0
* @brief
* @author   hejun
* @date     2017.04.20
* @note
**/


#ifndef _S1C1CLIENT_H_
#define _S1C1CLIENT_H_

#include <map>
#include <string>
#include <pthread.h> 
#include "DataDef.h"
#include "A7Client.h"
#include "ClientBase.h"
#ifndef STUB_TEST_TURN_ON
#include "programPlayer.h"
#endif

class S1C1Client : public ClientBase
{
public:

    S1C1Client();

    virtual ~S1C1Client();

    int init_data(const C_U8* url);

	std::string get_value_by_key(std::string text, std::string key);

	int parse_url(const char* url);

    int save_url(const char* url);
	
	std::string load_text_file(const std::string& path);

    void set_ip_addr();

	void set_purchaseToken(A7Client* a7_client);

    void set_url_default_value();

    bool set_Server_ID(const C_U8* url);

    void set_getparam_connect_timeout_value();

    int set_getparam_scale_value();

    int set_getparam_presentation_state_value();

    int set_getparam_position_value();

    int set_ip_port_by_url(const char* url);

    void build_SETUP_head();

	int set_qam_by_rtsp_url(std::string &rtsp_url, std::string &qam);

    int build_setup_request();

    void build_S1_ping_request();

    void build_default_play_request();

    void build_teardown_request();

    void build_getparam_request(GET_PARAMETER_TYPE_E param_type, bool is_ping);

    void build_media_ctrl_request(C_MediaCtrlType uMediaCtrlType, C_S32 uValue);

    int build_c1_announce_response(const char* res_buf);

    int build_s1_announce_response(const char* res_buf);

    //for S1 setup
    int create_S1_client();

    int send_setup_request();

    int recv_setup_response();

    int handle_setup_response(std::string &setup_str);

    //for S1 Ping
    int send_s1_heartBeat_request();

    //int recv_s1_heartBeat_response();

    //int handle_RTSP_ping_response();

    //for C1
    int create_C1_client();

    //for C1 play, pause, seek, set scale
    int send_C1_request();

    //wait the result from anothre thread
    int wait_C1_response();

    int wait_C1_ping_response();

    //only for default PLAY, other C1 response should be received at another thread 
    int recv_default_play_response();  //only be called when first PLAY response is received 

    int handle_play_response();  //only be called when first PLAY response is received 

    //for S1 teardown
    int send_teardown_request();

    //int recv_RTSP_teardown_response();
    //wait the result from anothre thread
    int wait_S1_response();

    int wait_S1_ping_response();

    //int handle_RTSP_teardown_response();

    //for C1 get_parameter
    int send_getparam_request(bool is_ping);

    //only for first time to get connectio_timeout, other GET_PARAMETER response should be received at another thread 
    int recv_connectTimeout_response();

    int handle_connectTimeout_response();

    //create a dead loop, receive the signal from C1 socket repeatedly    
    int recv_C1_Msg();

    void handle_C1_Msg();

    //create a dead loop, receive the signal from S1 socket repeatedly    
    int recv_S1_Msg();

    void handle_S1_Msg();

    //for announce response
    int send_C1_announce_response();

    int send_S1_announce_response();

    void clear_timer_callback_func();

    int get_cseq_value(const std::string& str);

    int play_media();

    //handle member data
    S1C1_Data_t get_s1_client_setup() const { return s1c1_data; }

    void set_s1_client_setup(S1C1_Data_t val) { s1c1_data = val; }

    pthread_t get_c1_ping_tid() const { return c1_ping_thread_id; }

    void set_c1_ping_tid(pthread_t id) { c1_ping_thread_id = id; }

    pthread_t get_s1_ping_tid() const { return s1_ping_thread_id; }

    void set_s1_ping_tid(pthread_t id) { s1_ping_thread_id = id; }

    pthread_t get_c1_tid() const { return c1_thread_id; }

    void set_c1_tid(pthread_t id) { c1_thread_id = id; }

    pthread_t get_s1_tid() const { return s1_thread_id; }

    void set_s1_tid(pthread_t id) { s1_thread_id = id; }

    void set_callback(MediaStatus_NotifyFunc cb) { callback = cb; }

    MediaStatus_NotifyFunc get_callback() { return callback; }

    void set_timer_callback_func(C_U32 uTimeOut);

    int set_c1_heart_beat_timer(C_U32 seconds);

    C_MediaNotifyCode get_err_code() { return err_code; }

    void set_err_code(C_MediaNotifyCode code) { err_code = code; }

    int get_C1_sockId() { return s1c1_data.C1_SocketID; }

    int get_S1_sockId() { return s1c1_data.S1_SocketID; }

    void set_C1_sockId(int sockId) { s1c1_data.C1_SocketID = sockId; }

    void set_S1_sockId(int sockId) { s1c1_data.S1_SocketID = sockId; }

    MEDIA_STATUS_E get_media_status() { return media_status; }

    void set_media_status(MEDIA_STATUS_E status) { media_status = status; }

    RECV_MSG_STATUS_E get_c1_recv_msg_status(int seq);

    void set_c1_recv_msg_status(int seq, RECV_MSG_STATUS_E status);

    RECV_MSG_STATUS_E get_s1_recv_msg_status(int seq);

    void set_s1_recv_msg_status(int seq, RECV_MSG_STATUS_E status);

    RECV_MSG_STATUS_E get_c1_ping_status(int seq);

    void set_c1_ping_status(int seq, RECV_MSG_STATUS_E status);

    RECV_MSG_STATUS_E get_s1_ping_status(int seq);

    void set_s1_ping_status(int seq, RECV_MSG_STATUS_E status);

    C_U32 get_c1_ping_timeout(){ return c1_ping_time_out; }

    void set_c1_ping_timeout(C_U32 timeout){ c1_ping_time_out = timeout; }

    C_U32 get_c1_ping_sleep_time(){ return c1_ping_sleep_time; }

    void set_c1_ping_sleep_time(C_U32 time_counter){ c1_ping_sleep_time = time_counter; }

    C_U32 get_s1_ping_timeout(){ return s1_ping_time_out; }

    void set_s1_ping_timeout(C_U32 timeout){ s1_ping_time_out = timeout; }

    C_CStbMediaInfo& get_media_info(){ return mediaInfo_res; }

    bool get_s1_exit_status(){ return is_s1_thread_exit; }

    void set_s1_exit_status(bool status) { is_s1_thread_exit = status; }

    int get_s1_ping_seq(){ return s1_ping_cseq; }

    int get_c1_ping_seq(){ return c1_ping_cseq; }

	pthread_mutex_t& get_c1_ping_mutex() {return c1_ping_sleep_time_mutex;}
    
private:

    MEDIA_STATUS_E media_status;
    
    pthread_t c1_thread_id;

    pthread_t s1_thread_id;

    pthread_t c1_ping_thread_id;

    pthread_t s1_ping_thread_id;

    bool is_c1_thread_exit;

    bool is_s1_thread_exit;

    S1C1_Data_t s1c1_data;

    Setup_Request_t setup_req;

    PING_Request_t ping_req;

    TearDown_Request_t teardown_req;

    C1_Request_t c1_req;

    GetParameter_Request_t getPara_req;

    RTSP_Response_t s1c1_res;

    std::map<int, RECV_MSG_STATUS_E> c1_recv_msg_status;

    std::map<int, RECV_MSG_STATUS_E> s1_recv_msg_status;

    std::map<int, RECV_MSG_STATUS_E> c1_ping_status;

    std::map<int, RECV_MSG_STATUS_E> s1_ping_status;

    C_CStbMediaInfo mediaInfo_res;

    Announuce_Response_t announce_res;

    URL_t url_t;

    IP_ADDR ip_addr;

    MediaStatus_NotifyFunc callback;

    C_MediaNotifyCode err_code;
    
    C_U32 c1_ping_time_out; //s
    
    C_U32 c1_ping_sleep_time; //s

    C_U32 s1_ping_time_out; //s

    C_U32 callback_timer; //ms

    //int setup_cseq;

    //int teardown_cseq;

	int s1_operation_cseq;

    int s1_ping_cseq;

    //int play_cseq;

    //int getparam_cseq;

	int c1_operation_cseq;

    int c1_ping_cseq;

	pthread_mutex_t c1_ping_sleep_time_mutex;
};


#endif
