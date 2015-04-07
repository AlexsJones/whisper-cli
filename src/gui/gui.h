/*
 * =====================================================================================
 *
 *       Filename:  gui.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/19/2015 06:48:40 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __GUI_H__
#define __GUI_H__

#include <pthread.h>
#include <ncurses.h>
#include "../session/session.h"

typedef struct {
  WINDOW *prompt;
  WINDOW *screen;	
  int next_line;
} ui_t;

typedef struct {
  ui_t *ui;
  session *s;
  char *msg;
  int is_active;
} gui_context_t;

gui_context_t *gui_create(session *s);
void gui_destroy(gui_context_t *c);
char *get_message(gui_context_t *c);
void display_local_message(gui_context_t *c, char *msg);
void display_remote_message(gui_context_t *c, char *msg);
void *read_loop(void *data);
void gui_receive_message(void *gc, jnx_guid *session_guid, jnx_char *message);

#endif
