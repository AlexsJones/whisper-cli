/*
 * =====================================================================================
 *
 *       Filename:  app_context.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  17/05/2015 08:46:18
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (DG), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include <jnxc_headers/jnxterm.h>
#include "app_context.h"
#include "app_auth_callbacks.h"

extern int peer_update_interval;

static void set_up_discovery_service(app_context_t *context) {
  jnx_hashmap *config = context->config;
  char *user_name = (char *) jnx_hash_get(config, "USER_NAME");
  if (NULL == user_name) {
    user_name = malloc(strlen(getenv("USER")) + 1);
    strcpy(user_name, getenv("USER"));
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

int link_session_protocol(session *s, linked_session_type lst, void *optarg) {
  printf("---------- link session protocol ------------- \n");
  /* both the receiving session link and sender will go through here,
   * it is necessary to differentiate
   */
  int r = 0;
  switch (lst) {
    case E_AM_INITIATOR:
      printf("Link session protocol for initiator\n");
      app_context_t *context = optarg;
      port_control_service *ps = port_control_service_create(9001, 9291, 1);
      r = auth_comms_initiator_start(
          context->auth_comms, context->discovery, ps, s,
          s->initiator_message);
      printf("Auth initiator done\n");
      break;
    case E_AM_RECEIVER:
      printf("Link session for receiver\n");
      break;
  }
  return r;
}

int unlink_session_protocol(session *s, linked_session_type stype, void *optargs) {
  app_context_t *context = optargs;
  printf("---------- unlink session protocol ----------- \n");
  auth_comms_stop(context->auth_comms, s);
  return 0;
}

static void set_up_session_service(app_context_t *context) {
  context->session_serv = session_service_create(
      link_session_protocol, unlink_session_protocol);
}

static void set_up_auth_comms(app_context_t *context) {
  context->auth_comms = auth_comms_create();
  context->auth_comms->ar_callback = app_accept_or_reject_session;
  context->auth_comms->listener = jnx_socket_tcp_listener_create(
      "9991", AF_INET, 15);

  auth_comms_listener_start(context->auth_comms, context->discovery,
    context->session_serv, context);
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

/**********************************/
/* discovery_service interactions */
/**********************************/

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

static int app_is_input_guid_size(char *input) {
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