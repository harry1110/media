/***************************************************************************
*
*    @copyright (c) 2010-2019, BeiJing Baustem Technology Co., LTD. All Right Reserved.
*
***************************************************************************/
/**
* @file     A7Client.h
* @version  1.0
* @brief
* @author   hejun
* @date     2017.05.04
* @note
**/


#ifndef _A7CLIENT_H_
#define _A7CLIENT_H_

#include "DataDef.h"
#include "ClientBase.h"

class A7Client : public ClientBase
{
public:
    A7Client();

    virtual ~A7Client();

    bool parse_url(char* url);

    IP_ADDR get_ip_addr();

    //std::string get_asset_id();

    std::string get_purchaseToken();

	void set_purchaseToken(std::string token);

    C_S32 get_playType();

	int check_dynamic_url(const char* url);

	int set_dynamic_data();

    int build_A7_SelectionStart_msg();

    int build_A7_ChannelSelectionStart_msg();

    C_S32 create_A7_client();

    int send_A7_selection_request();

    int recv_A7_selection_response();

    int handle_A7_selection_response();

    void set_callback_timer(C_U32 timer) { callback_timer = timer; }

private:

    void init_data();

    void set_ip_addr();
    
    std::string load_text_file(const std::string& path);

    A7_Data_t a7_data;

    IP_ADDR ip_addr;

    C_U32 callback_timer; //ms

    C_S32 play_type;
};

#endif

