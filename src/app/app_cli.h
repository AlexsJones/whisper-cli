/*
 * =====================================================================================
 *
 *       Filename:  app.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20/02/15 15:47:57
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (draganglumac), dragan.glumac@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __APP_H__
#define __APP_H__

#include <jnxc_headers/jnxvector.h>
#include <jnxc_headers/jnxunixsocket.h>
#include "app_context.h"
#include "app_auth_callbacks.h"
#include "app_gui_bindings.h"

#define CMD_SESSION 1
#define CMD_LIST    2
#define CMD_HELP    3
#define CMD_QUIT    4
#define CMD_ACCEPT_SESSION 5
#define CMD_REJECT_SESSION 6

void app_intro();
void app_prompt();
void app_show_help();
void app_quit_message();
char *app_get_session_message();
int app_code_for_command_with_param(char *command, jnx_size cmd_len,
    char **oparam);


#endif // __APP_H__
