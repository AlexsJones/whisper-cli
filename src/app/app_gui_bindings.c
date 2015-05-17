/*
 * =====================================================================================
 *
 *       Filename:  app_gui_bindings.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  17/05/2015 08:33:39
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac (DG), 
 *   Organization:  
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include "app_gui_bindings.h"
#include "gui/gui.h"

void unpair_session_from_gui(void *gui_context) {
  gui_context_t *context = (gui_context_t *) gui_context;
  app_context_t *act = (app_context_t *) context->args;

  context->is_active = 0;
  session_state r = session_service_unlink_sessions(
      act->session_serv,
      E_AM_RECEIVER,
      act,
      &context->s->session_guid);

  JNXCHECK(r == SESSION_STATE_OKAY);
  JNXCHECK(session_service_session_is_linked(
      context->session_serv, &context->s->session_guid) == 0);
}

void pair_session_with_gui(session *s, void *gui_context, void *app_context) {
  s->gui_context = gui_context;
  gui_context_t *gc = (gui_context_t *) gui_context;

  // Set up quit hint callback
  gc->quit_callback = unpair_session_from_gui;
  gc->args = app_context;
}

void app_start_gui_for_session(session *s, app_context_t *app_context) {
  session_service *serv = app_context->session_serv;
  gui_context_t *gc = gui_create(s, serv);
  pair_session_with_gui(s, (void *) gc, (void *) app_context);
  jnx_thread *user_input_thread =
      jnx_thread_create(read_user_input_loop, (void *) gc);
  jnx_char *message;
  while (0 < session_message_read(s, (jnx_uint8 **) &message)) {
    gui_receive_message(gc, message);
  }
  if (QUIT_NONE == gc->quit_hint) {
    gc->quit_hint = QUIT_ON_NEXT_USER_INPUT;
  }
  // wait for user input thread to complete
  pthread_join(user_input_thread->system_thread, NULL);
}