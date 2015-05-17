/*
 * =====================================================================================
 *
 *       Filename:  app_auth_callbacks.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/12/2015 07:47:16 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Dragan Glumac 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef __APP_AUTH_CALLBACKS_H__
#define __APP_AUTH_CALLBACKS_H__

#include <jnxc_headers/jnxguid.h>
#include "app_context.h"

int app_accept_or_reject_session(discovery_service *ds,
    jnx_guid *ig, jnx_guid *sg);
session *app_accept_chat(app_context_t *context);
session *app_reject_chat(app_context_t *context);

#endif // __APP_AUTH_CALLBACKS_H__
