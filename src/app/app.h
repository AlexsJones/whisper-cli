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

#include <jnxc_headers/jnxhash.h>
#include <jnxc_headers/jnxvector.h>
#include <jnxc_headers/jnxunixsocket.h>
#include "../net/discovery.h"
#include "../net/auth_comms.h"
#include "../session/session_service.h"
#define CMDLEN 64

#define CMD_SESSION 1
#define CMD_LIST    2
#define CMD_HELP    3
#define CMD_QUIT    4
#define CMD_ACCEPT_SESSION 5
#define CMD_REJECT_SESSION 6

typedef struct {
  jnx_hashmap *config;
  discovery_service *discovery;
  session_service *session_serv;
  auth_comms_service *auth_comms;
} app_context_t;

app_context_t *app_create_context(jnx_hashmap *config);
void app_destroy_context(app_context_t **app_context);

void app_intro();
void app_prompt();
void app_show_help();
void app_quit_message();
int app_code_for_command_with_param(char *command,\
    jnx_size cmd_len, char **oparam);
void app_create_gui_session(session *s);
void app_list_active_peers(app_context_t *context);
peer *app_peer_from_input(app_context_t *context,char *param);
void app_initiate_handshake(app_context_t *context,session *s);
int app_accept_or_reject_session(discovery_service *ds,
    jnx_guid *ig, jnx_guid *sg);
session *app_accept_chat(app_context_t *context);
#endif // __APP_H__
