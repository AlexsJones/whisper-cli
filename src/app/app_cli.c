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
#include "gui/gui.h"
#include "app_cli.h"
#include "auth_comms.h"
#include "port_control.h"
#include <unistd.h>
#include "app_gui_bindings.h"


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
  int retval;
  char *raw_cmd = strtok(command, " \n\r\t");
  if (!raw_cmd) {
    return CMD_HELP;
  }
  char *extra_params = strtok(NULL, " \n\r\t");
  if (is_equivalent(raw_cmd, "session")) {
    if (!extra_params) {
      printf("Requires name of peer as argument.\n");
      retval = CMD_HELP;
    }
    else {
      *oparam = malloc(strlen(extra_params) + 1);
      strcpy(*oparam, extra_params);
      retval = CMD_SESSION;
    }
  } else {
    retval = code_for_command(raw_cmd);
  }
  free(command);
  return retval;
}

void app_prompt() {
  printf("Enter a command into the prompt.\n");
  printf("(For the list of commands type 'h' or 'help'.)\n");
  printf("\n");
  printf("$> ");
  fflush(stdout);
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

char *app_get_session_message() {
  char *session_message = NULL;
  size_t message_size = 0;

  printf("Set the chat topic or hit Enter for none: ");
  fflush(stdout);
  getline(&session_message, &message_size, stdin);

  if (strcmp("\n", session_message) == 0) {
    return NULL;
  }
  else {
    session_message[strlen(session_message) - 1] = '\0';
    return session_message;
  }
}