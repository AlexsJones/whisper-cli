/*
 * =====================================================================================
 *
 *       Filename:  app_auth_callbacks.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/12/2015 07:55:09 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "app_auth_callbacks.h"
#include <string.h>
#include <stdio.h>
#include <jnxc_headers/jnxthread.h>
#include <jnxc_headers/jnxunixsocket.h>
#include <jnxc_headers/jnxtypes.h>
#include <unistd.h>

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
    jnx_unix_socket *remote_sock, void *ctx)
{  
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

int app_accept_or_reject_session(discovery_service *ds, jnx_guid *initiator_guid,
    jnx_guid *session_guid)
{
  char sockpath[128], guidpath[128];
  get_session_interaction_path(sockpath);
  get_receive_guid_path(guidpath);
  unlink(sockpath);
  unlink(guidpath);
  jnx_unix_socket *s = jnx_unix_stream_socket_create(sockpath);
  int abort;
  peer *p = peerstore_lookup(ds->peers, initiator_guid);
  printf("You have a chat request from %s. %s",
         p->user_name, "Would you like to accept or reject the chat? [a/r]: ");
  fflush(stdout);

  accept_reject_dto ar;
  ar.session_guid = session_guid;
  ar.abort = -1;
	jnx_unix_stream_socket_listen_with_context(
      s, 1, user_accept_reject, (void *) &ar);
  if (ar.abort == 0) { 
    jnx_unix_socket *gs = jnx_unix_stream_socket_create(guidpath);
    jnx_unix_stream_socket_send(gs, (void *) session_guid, sizeof(jnx_guid));
    jnx_unix_socket_destroy(&gs);
  }
  jnx_unix_socket_destroy(&s);

  return ar.abort;
}

static jnx_int32 read_guid(jnx_uint8 *buffer, jnx_size bytesread,
    jnx_unix_socket *remote_sock, void *context)
{
  jnx_guid *guid = (jnx_guid *) context;
  JNXCHECK(bytesread == sizeof(jnx_guid));
  memcpy((void *) guid, buffer, bytesread);
  return END_LISTEN;
}

session *app_reject_chat(app_context_t *context) {
  sleep(1);
  char sockpath[128];
  get_session_interaction_path(sockpath);
  jnx_unix_socket *us = jnx_unix_stream_socket_create(sockpath);
  jnx_unix_stream_socket_send(us, (jnx_uint8 *) "reject",
                              strlen("reject"));
  jnx_unix_socket_destroy(&us);
  return NULL;
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
  session_state status = session_service_fetch_session(
      context->session_serv, &session_guid, &osession);
  while (1) {
    sleep(1);
    if (!osession) {
      status = session_service_fetch_session(
          context->session_serv, &session_guid, &osession);
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
