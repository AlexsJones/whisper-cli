/*
 * =====================================================================================
 *
 *       Filename:  gui.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  02/19/2015 06:49:01 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdlib.h>
#include "gui.h"
#include <string.h>
#include <pthread.h>
#define COL_LOGO   1
#define COL_LOCAL  2
#define COL_REMOTE 3
#define COL_ALERT  4

void init_colours() {
  if (has_colors() == FALSE) {
    endwin();
    printf("Your terminal does not support colours.\n");
    exit(1);
  }
  start_color();
  init_pair(COL_LOGO, COLOR_WHITE, COLOR_BLUE);
  init_pair(COL_LOCAL, COLOR_WHITE, COLOR_BLACK);
  init_pair(COL_REMOTE, COLOR_GREEN, COLOR_BLACK);
  init_pair(COL_ALERT, COLOR_YELLOW, COLOR_BLACK);
}
void show_prompt(ui_t *ui) {
  wmove(ui->prompt, 1, 1);
  wclear(ui->prompt);
  mvwprintw(ui->prompt, 1, 1, "$> ");
  wrefresh(ui->prompt);
}
void display_logo() {
  attron(COLOR_PAIR(COL_LOGO) | A_BOLD);
  move(0,0);
  printw("%s", " Whisper Chat ");
  attroff(COLOR_PAIR(COL_LOGO) | A_BOLD);
  refresh();
}
gui_context_t *gui_create(session *s) {
  gui_context_t *c = malloc(sizeof(gui_context_t));
  ui_t *ui = malloc(sizeof(ui_t));
  initscr();
  init_colours();
  display_logo();
  ui->screen = newwin(LINES - 6, COLS - 1, 1, 1);
  scrollok(ui->screen, TRUE);
  box(ui->screen, 0, 0);
//  wborder(ui->screen, '|', '|', '-', '-', '+', '+', '+', '+');
  ui->next_line = 1;
  wrefresh(ui->screen);
  ui->prompt = newwin(4, COLS - 1, LINES - 5, 1);
  show_prompt(ui);

  c->ui = ui;
  c->s = s;
  c->msg = NULL;
  c->is_active = 1;
  return c;
}
void gui_destroy(gui_context_t *c) {
  delwin(c->ui->screen);
  delwin(c->ui->prompt);
  endwin();
  c->is_active = 0;
}
char *get_message(gui_context_t *c){
  char *msg = malloc(1024);
  wmove(c->ui->prompt, 1, 4);
  wgetstr(c->ui->prompt, msg);
  show_prompt(c->ui);
  return msg;
}
void update_next_line(ui_t *ui) {
  int lines, cols;
  getmaxyx(ui->screen, lines, cols);
  if (ui->next_line >= --lines) {
    scroll(ui->screen);
    ui->next_line--;
  }
  else {
    getyx(ui->screen, lines, cols);
    ui->next_line = lines;
  }
}
void display_message(ui_t *ui, char *msg, int col_flag) {
  int row, col;
  getyx(ui->prompt, row, col);
  wattron(ui->screen, COLOR_PAIR(col_flag));
  mvwprintw(ui->screen, ui->next_line, 1, "%s\n", msg);
  wattroff(ui->screen, COLOR_PAIR(col_flag));
  update_next_line(ui);
  box(ui->screen, 0, 0);
  wrefresh(ui->screen);
  wmove(ui->prompt, row, col);
  wrefresh(ui->prompt);
}
void display_local_message(gui_context_t *c, char *msg) {
  display_message(c->ui, msg, COL_LOCAL);
  free(msg);
}
void display_remote_message(gui_context_t *c, char *msg) {
  display_message(c->ui, msg, COL_REMOTE);
  free(msg);
}
void display_alert_message(gui_context_t *c, char *msg) {
  display_message(c->ui, msg, COL_ALERT);
}
static void gui_unpair_session(gui_context_t *c) {
  c->s->session_callback = NULL;
}
void *read_loop(void *data) {
  gui_context_t *context = (gui_context_t *) data;
  while(TRUE) {
    char *msg = get_message(context);
    if (strcmp(msg, ":q") == 0) {
      break;
    }
    else {
      session_state res = session_message_write(context->s, msg);
      if (SESSION_STATE_OKAY == res) {
        display_local_message(context, msg);
      }
    }
  }
  session_disconnect(context->s);
  gui_unpair_session(context);
  gui_destroy(context);
  return NULL;
}
void gui_receive_message(void *gc, jnx_guid *session_guid, jnx_char *message) {
  gui_context_t *c = (gui_context_t *) gc;
  if (!c->is_active) {
    return;
  }
  if (c->s->is_connected) {
    display_remote_message(c, message);
  }
  else {
    display_alert_message(c, message);
  }
}
