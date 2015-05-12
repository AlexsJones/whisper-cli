/*
 * =====================================================================================
 *
 *       Filename:  app_context.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/12/2015 08:16:01 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __APP_CONTEXT_H__
#define __APP_CONTEXT_H__

#include <jnxc_headers/jnxhash.h>
#include "discovery.h"
#include "auth_comms.h"
#include "secure_comms.h"

typedef struct {
  jnx_hashmap *config;
  discovery_service *discovery;
  session_service *session_serv;
  auth_comms_service *auth_comms;
} app_context_t;

#endif // __APP_CONTEXT_H__
