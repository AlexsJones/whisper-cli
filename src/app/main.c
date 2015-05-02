/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Main launch point for whisper
 *
 *        Version:  1.0
 *        Created:  19/01/15 09:36:16
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  AlexsJones (jonesax@hush.com), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include "whisper_errors.h"
#include <jnxc_headers/jnxcheck.h>
#include "app.h"

jnx_hashmap *load_config(int argc, char **argv) {
  if (argc > 1) {
    int i = 1;
    while (i < argc) {
      if (0 == strncmp("--config=", argv[i], 9)) {
        char *path = argv[i] + 9;
        jnx_hashmap *config = jnx_file_read_kvp(path, 80, "=");
        if (config != NULL) {
          return config;
        }
      }
      i++;
    }
  }
  JNX_LOG(0, "[ERROR] You must supply a valid configuration file on the command \
      line. Pass it using --config=PATH_TO_CONFIG_FILE command line option.");
  exit(1);
}

int run_app(app_context_t *context) {
  char command[CMDLEN];
  char *broadcast, *local;

  app_intro();
  while (1) {
    app_prompt();

    char *cmd_string = NULL;
    jnx_size read_bytes;
    jnx_size s = getline(&cmd_string, &read_bytes, stdin);
    char *param = NULL;
    jnx_vector *active_peers = NULL;
    session *osession;
    switch (app_code_for_command_with_param(cmd_string, read_bytes, &param)) {
      case CMD_ACCEPT_SESSION:
        osession = app_accept_chat(context);
        app_create_gui_session(osession, context->session_serv);
        printf("Exiting GUI from accept.\n");
        session_state r = session_service_unlink_sessions(
            context->session_serv,
            E_AM_INITIATOR,
            NULL,
            &osession->session_guid);

        JNXCHECK(r == SESSION_STATE_OKAY);
        JNXCHECK(session_service_session_is_linked(
            context->session_serv,
            &osession->session_guid) == 0);
        session_service_destroy_session(
            context->session_serv,
            &osession->session_guid);
        break;
      case CMD_REJECT_SESSION:
        break;
      case CMD_SESSION:
        if (!param) {
          printf("Session requires a username to connect to.\n");
          break;
        }

        peer *remote_peer = app_peer_from_input(context, param);

        if (remote_peer) {

          peer *local_peer = peerstore_get_local_peer(context->discovery->peers);
          if (strcmp(remote_peer->host_address, local_peer->host_address) == 0) {
            printf("You cannot t a session with yourself.\n");
            break;
          }
          printf("Found peer.\n");
          /*
           * Version 1.0
           */
          session *s;
          /* create session */
          session_service_create_session(context->session_serv, &s);
          /* link our peers to our session information */
          session_service_link_sessions(context->session_serv,
                                        E_AM_INITIATOR, context,
                                        &s->session_guid,
                                        local_peer, remote_peer);

          printf("Session link hit\n");
          while (s->secure_socket == -1)
            sleep(1);
          while(!secure_comms_is_socket_linked(s->secure_socket))
            sleep(1);
          app_create_gui_session(s, context->session_serv);

          session_state r = session_service_unlink_sessions(context->session_serv,
                                                            E_AM_INITIATOR,
                                                            NULL,
                                                            &(*s).session_guid);

          JNXCHECK(r == SESSION_STATE_OKAY);
          JNXCHECK(session_service_session_is_linked(context->session_serv,
                                                     &(*s).session_guid) == 0);
          session_service_destroy_session(context->session_serv,
                                          &(*s).session_guid);

        } else {
          printf("Session could not be started.\nDid you spell your target%s",
                 " username correctly?\n");
        }
        if (param) {
          free(param);
        }
        break;
      case CMD_LIST:
        app_list_active_peers(context);
        break;
      case CMD_HELP:
        app_show_help();
        break;
      case CMD_QUIT:
        app_quit_message();
        discovery_notify_peers_of_shutdown(context->discovery);
        return 0;
    }
    free(cmd_string);
  }
  return 0;
}

int main(int argc, char **argv) {
  jnx_hashmap *config = load_config(argc, argv);
  app_context_t *app_context = app_create_context(config);
  run_app(app_context);
  printf("Bye.\n");
  return 0;
}
