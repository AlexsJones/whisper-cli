/*
 * =====================================================================================
 *
 *       Filename:  app.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  20/02/15 15:46:49
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (draganglumac), dragan.glumac@gmail.com
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <jnxc_headers/jnxterm.h>
#include <jnxc_headers/jnxthread.h>
#include <jnxc_headers/jnx_tcp_socket.h>
#include <jnxc_headers/jnx_udp_socket.h>
#include <gui/gui.h>
#include "app.h"
#include "../gui/gui.h"
#include "auth_comms.h"
#include "port_control.h"

#define END_LISTEN -1
#define SESSION_INTERACTION "session_interaction"
#define RECEIVE_GUID "receive_guid"

static int get_tmp_path(char *path, char *filename) {
  int size = 0;
  size += strlen(P_tmpdir);
  int has_end_slash = (P_tmpdir[strlen(P_tmpdir)] == '/');
  if (has_end_slash) {
    size++;
  }
  size += strlen(filename);
  memset(path, 0, size);
  strcpy(path, P_tmpdir);
  if (!has_end_slash) {
    strcat(path, "/");
  }
  strcat(path, filename);
  return size;
}

static int get_session_interaction_path(char *path) {
  return get_tmp_path(path, SESSION_INTERACTION);
}

static int get_receive_guid_path(char *path) {
  return get_tmp_path(path, RECEIVE_GUID);
}

typedef struct {
  int abort;
  jnx_guid *session_guid;
} accept_reject_dto;

static jnx_int32 user_accept_reject(jnx_uint8 *payload, jnx_size bytesread,
                                    jnx_unix_socket *remote_sock, void *ctx) {
  accept_reject_dto *ar = (accept_reject_dto *) ctx;
  if (0 == strncmp((char *) payload, "a", bytesread)
      || 0 == strncmp((char *) payload, "accept", bytesread)) {
    ar->abort = 0;
  }
  else {
    ar->abort = 1;
  }
  return END_LISTEN;
}

int app_accept_or_reject_session(discovery_service *ds,
                                 jnx_guid *initiator_guid, jnx_guid *session_guid) {
  char sockpath[128], guidpath[128];
  get_session_interaction_path(sockpath);
  get_receive_guid_path(guidpath);
  unlink(sockpath);
  unlink(guidpath);
  jnx_unix_socket *s = jnx_unix_stream_socket_create(sockpath),
      *gs = jnx_unix_stream_socket_create(guidpath);
  int abort;
  peer *p = peerstore_lookup(ds->peers, initiator_guid);
  printf("You have a chat request from %s. %s",
         p->user_name, "Would you like to accept or reject the chat? [a/r]: ");
  fflush(stdout);

  accept_reject_dto ar;
  ar.session_guid = session_guid;
  jnx_unix_stream_socket_listen_with_context(
      s, 1, user_accept_reject, (void *) &ar);
  jnx_unix_stream_socket_send(gs, (void *) session_guid, sizeof(jnx_guid));
  jnx_unix_socket_destroy(&gs);
  jnx_unix_socket_destroy(&s);

  return ar.abort;
}

void unpair_session_from_gui(void *gui_context) {
  printf("[DEBUG] Unpairing session from GUI...\n");
  gui_context_t *context = (gui_context_t *)gui_context;
  app_context_t *act = (app_context_t *) context->args;
  context->is_active = 0;
  printf("Exiting GUI from accept.\n");
  session_state r = session_service_unlink_sessions(
      act->session_serv,
      E_AM_RECEIVER,
      act,
      &context->s->session_guid);

  JNXCHECK(r == SESSION_STATE_OKAY);
  JNXCHECK(session_service_session_is_linked(
      context->session_serv, &context->s->session_guid) == 0);
  printf("[DEBUG] Unlinked successfully.\n");
}

void pair_session_with_gui(session *s, void *gui_context, void *app_context) {
  s->gui_context = gui_context;
  gui_context_t *gc = (gui_context_t *) gui_context;

  // Set up quit hint callback
  gc->quit_callback = unpair_session_from_gui;
  gc->args = app_context;
}

void app_create_gui_session(session *s, app_context_t *app_context) {
  session_service *serv = app_context->session_serv;
  gui_context_t *gc = gui_create(s, serv);
  pair_session_with_gui(s, (void *) gc, (void *) app_context);
  jnx_thread_create_disposable(read_user_input_loop, (void *) gc);
  jnx_char *message;
  while (0 < session_message_read(s, (jnx_uint8 **) &message)) {
      gui_receive_message(gc, message);
  }
}

int is_equivalent(char *command, char *expected) {
  if (strcmp(command, expected) == 0) {
    return 1;
  }
  else if (strncmp(command, expected, 1) == 0) {
    return 1;
  }
  return 0;
}

int code_for_command(char *command) {
  if (is_equivalent(command, "list")) {
    return CMD_LIST;
  }
  else if (is_equivalent(command, "session")) {
    return CMD_SESSION;
  }
  else if (is_equivalent(command, "quit")) {
    return CMD_QUIT;
  }
  else if (is_equivalent(command, "accept")) {
    return CMD_ACCEPT_SESSION;
  }
  else if (is_equivalent(command, "reject")) {
    return CMD_REJECT_SESSION;
  }
  return CMD_HELP;
}

int app_code_for_command_with_param(char *command, jnx_size cmd_len, char **oparam) {
  *oparam = NULL;
  char *raw_cmd = strtok(command, " \n\r\t");
  if (!raw_cmd) {
    return CMD_HELP;
  }
  char *extra_params = strtok(NULL, " \n\r\t");
  if (is_equivalent(raw_cmd, "session")) {
    if (!extra_params) {
      printf("Requires name of peer as argument.\n");
      return CMD_HELP;
    }
    *oparam = malloc(strlen(extra_params) + 1);
    strcpy(*oparam, extra_params);
    return CMD_SESSION;
  } else {
    return code_for_command(raw_cmd);
  }
}

void app_prompt() {
  printf("Enter a command into the prompt.\n");
  printf("(For the list of commands type 'h' or 'help'.)\n");
  printf("\n");
  printf("$> ");
  fflush(stdout);
}

static void pretty_print_peer(peer *p) {
  char *guid;
  jnx_guid_to_string(&p->guid, &guid);
  printf("%-32s %-16s %s\n", guid, p->host_address, p->user_name);
  free(guid);
}

static void pretty_print_peer_in_col(peer *p, jnx_int32 colour) {
  char *guid;
  jnx_guid_to_string(&p->guid, &guid);
  jnx_term_printf_in_color(colour,
                           "%-32s %-16s %s\n", guid, p->host_address, p->user_name);
  free(guid);
}

static void show_active_peers(peerstore *ps) {
  jnx_guid **active_guids;
  int num_guids = peerstore_get_active_guids(ps, &active_guids);
  int i;
  peer *local = peerstore_get_local_peer(ps);
  for (i = 0; i < num_guids; i++) {
    jnx_guid *next_guid = active_guids[i];
    peer *p = peerstore_lookup(ps, next_guid);
    if (peers_compare(p, local) != 0) {
      pretty_print_peer(p);
    }
  }
}

void app_list_active_peers(app_context_t *context) {
  printf("\n");
  printf("Active Peers:\n");
  printf("%-32s %-16s %-16s\n", "UUID", "IP Address", "Username");
  printf("%s",
         "--------------------------------+----------------+----------------\n");
  show_active_peers(context->discovery->peers);
  printf("\n");
  printf("Local peer:\n");
  peer *local = peerstore_get_local_peer(context->discovery->peers);
  pretty_print_peer_in_col(local, JNX_COL_GREEN);
  printf("\n");
}

void app_intro() {
  printf("\n");
  printf("Welcome to Whisper Chat\n");
  printf("\n");
}

void app_show_help() {
  printf("\n");
  printf("Valid commands are:\n");
  printf("\n");
  printf("\th or help    - show help\n");
  printf("\tl or list    - show the list of active peers\n");
  printf("\ts or session - create session (PLACEHOLDER to launch GUI for now)\n");
  printf("\tq or quit    - quit the program\n");
  printf("\n");
}

void app_quit_message() {
  printf("\n");
  printf("Shutting down 'whisper'...\n");
}

extern int peer_update_interval;

static void set_up_discovery_service(app_context_t *context) {
  jnx_hashmap *config = context->config;
  char *user_name = (char *) jnx_hash_get(config, "USER_NAME");
  if (NULL == user_name) {
    user_name = getenv("USER");
    JNX_LOG(0, "[WARNING] Using the system user name '%s'.", user_name);
  }
  peerstore *ps = peerstore_init(local_peer_for_user(user_name, peer_update_interval), 0);
  char *local_ip = (char *) jnx_hash_get(config, "LOCAL_IP");
  if (local_ip != NULL) {
    free(peerstore_get_local_peer(ps)->host_address);
    peerstore_get_local_peer(ps)->host_address = local_ip;
  }

  int port = DEFAULT_BROADCAST_PORT;
  char *disc_port = (char *) jnx_hash_get(config, "DISCOVERY_PORT");
  if (disc_port != NULL) {
    port = atoi(disc_port);
  }

  char *broadcast_address = (char *) jnx_hash_get(config, "DISCOVERY_MULTICAST_IP");
  if (broadcast_address == NULL) {
    get_broadcast_ip(&broadcast_address);
  }

  char *discovery_interval = (char *) jnx_hash_get(config, "DISCOVERY_INTERVAL");
  if (discovery_interval != NULL) {
    peer_update_interval = atoi(discovery_interval);
  }

  context->discovery = discovery_service_create(port, AF_INET, broadcast_address, ps);

  char *discovery_strategy = (char *) jnx_hash_get(config, "DISCOVERY_STRATEGY");
  if (NULL == discovery_strategy) {
    JNX_LOG(0, "Starting discovery service with heartbeat srategy.");
    discovery_service_start(context->discovery, BROADCAST_UPDATE_STRATEGY);
  }
  else {
    if (0 == strcmp(discovery_strategy, "polling")) {
      JNX_LOG(0, "Starting discovery service with polling srategy.");
      discovery_service_start(context->discovery, POLLING_UPDATE_STRATEGY);
    }
  }
}

int app_is_input_guid_size(char *input) {
  if ((strlen(input) / 2) == 16) {
    return 1;
  }
  return 0;
}

peer *app_peer_from_input(app_context_t *context, char *param) {
  if (app_is_input_guid_size(param)) {
    jnx_guid g;
    jnx_guid_from_string(param, &g);
    peer *p = peerstore_lookup(context->discovery->peers, &g);
    return p;
  } else {
    peer *p = peerstore_lookup_by_username(context->discovery->peers, param);
    return p;
  }
  return NULL;
}

int link_session_protocol(session *s, linked_session_type lst, void *optarg) {
  printf("---------- link session protocol ------------- \n");
  /* both the receiving session link and sender will go through here,
   * it is necessary to differentiate 
   */
  switch (lst) {
    case E_AM_INITIATOR:
      printf("Link session protocol for initiator\n");
      app_context_t *context = optarg;
      port_control_service *ps = port_control_service_create(9001, 9291, 1);
      auth_comms_initiator_start(context->auth_comms,
                                 context->discovery, ps, s);
      printf("Auth initiator done\n");
      break;
    case E_AM_RECEIVER:
      printf("Link session for receiver\n");
      break;
  }
  return 0;
}

int unlink_session_protocol(session *s, linked_session_type stype, void *optargs) {
  app_context_t *context = optargs;
  printf("---------- unlink session protocol ----------- \n");
  auth_comms_stop(context->auth_comms,s);
  return 0;
}

void set_up_session_service(app_context_t *context) {
  context->session_serv = session_service_create(link_session_protocol,
                                                 unlink_session_protocol);
}

void set_up_auth_comms(app_context_t *context) {
  context->auth_comms = auth_comms_create();
  context->auth_comms->ar_callback = app_accept_or_reject_session;
  context->auth_comms->listener = jnx_socket_tcp_listener_create("9991",
                                                                 AF_INET, 15);

  auth_comms_listener_start(context->auth_comms, context->discovery, context->session_serv,
                            context);
}

app_context_t *app_create_context(jnx_hashmap *config) {
  app_context_t *context = calloc(1, sizeof(app_context_t));
  context->config = config;
  set_up_discovery_service(context);
  set_up_session_service(context);
  set_up_auth_comms(context);
  jnx_term_printf_in_color(JNX_COL_GREEN, "Broadcast IP: %s\n",
    context->discovery->broadcast_group_address);
  jnx_term_printf_in_color(JNX_COL_GREEN, "Local IP:     %s\n",
    peerstore_get_local_peer(context->discovery->peers)->host_address);
  return context;
}

void app_destroy_context(app_context_t **context) {
  discovery_service_cleanup(&(*context)->discovery);
  session_service_destroy(&(*context)->session_serv);
  auth_comms_destroy(&(*context)->auth_comms);
  free(*context);
  *context = NULL;
}

static jnx_int32 read_guid(jnx_uint8 *buffer, jnx_size bytesread,
                           jnx_unix_socket *remote_sock, void *context) {
  jnx_guid *guid = (jnx_guid *) context;
  JNXCHECK(bytesread == sizeof(jnx_guid));
  memcpy((void *) guid, buffer, bytesread);
  return END_LISTEN;
}

session *app_accept_chat(app_context_t *context) {
  char sockpath[128], guidpath[128];
  get_session_interaction_path(sockpath);
  get_receive_guid_path(guidpath);
  jnx_unix_socket *us = jnx_unix_stream_socket_create(sockpath),
      *gs = jnx_unix_stream_socket_create(guidpath);
  jnx_unix_stream_socket_send(us, (jnx_uint8 *) "accept",
                              strlen("accept"));
  jnx_guid session_guid;
  jnx_unix_stream_socket_listen_with_context(gs, 1, read_guid,
                                             (void *) &session_guid);
  read(us->socket, (void *) &session_guid, sizeof(jnx_guid));
#ifdef DEBUG
  char *guid_str;
  jnx_guid_to_string(&session_guid, &guid_str);
  printf("[DEBUG] Received SessionGUID = %s\n", guid_str);
  free(guid_str);
#endif
  jnx_unix_socket_destroy(&gs);
  jnx_unix_socket_destroy(&us);

  session *osession = NULL;
  session_state status = session_service_fetch_session(context->session_serv,
                                                       &session_guid, &osession);
  while (1) {
    sleep(1);
    if (!osession) {
      status = session_service_fetch_session(context->session_serv,
                                             &session_guid, &osession);
    }
    if (osession->secure_socket == -1)
      continue;
    if (secure_comms_is_socket_linked(osession->secure_socket)) {
      printf("Session is linked on receiver end.\n");
      break;
    }
  }
  return osession;
}
