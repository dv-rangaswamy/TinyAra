/****************************************************************************
 *
 * Copyright 2016 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

/**
 * @defgroup LWM2M LWM2M
 * @ingroup DM
 * @{
 */

/**
 * @file dm_lwm2m.h
 * @brief device management APIs for DM
 */

#ifndef _DMAPI_H
#define _DMAPI_H

#include "liblwm2m.h"
#include "er-coap-13/er-coap-13.h"

#define IPADDRLEN_MAX  32
#define PORTLEN        6

/**
 * @brief Enumeration of dm client state
 * @details
 * Enumeration Details:\n
 * DM_LWM2M_CLIENT_STARTED\n
 * DM_LWM2M_CLIENT_STOPPED\n
 */
typedef enum {
	DM_LWM2M_CLIENT_STARTED, DM_LWM2M_CLIENT_STOPPED,
} dm_lwm2m_client_state_e;

/**
 * @brief Struct definition of DM context for a LWM2M session
 */

struct dm_lwm2m_context_s {
	struct server_info_s {
		char ipAddress[IPADDRLEN_MAX];
		char port[PORTLEN];
		bool isBootstrap;
	} server_info;
	struct client_info_s {
		int lifetime;
	} client_info;
};

/**
 * @brief Start a DM client
 *
 * @param[in] dm_context pointer to DM context
 * @return On success, 0 is returned.
 *         On failure, a negative value is returned.
 *         If client is already started, return DM_ERROR_ALREADY_STARTED.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_start_client(struct dm_lwm2m_context_s *dm_context);

/**
 * @brief Close a DM client
 *
 * @param  None
 * @return On success, 0 is returned.
 *         On failure, a negative value is returned.
 *         If client is already stopped, return DM_ERROR_ALREADY_STOPPED.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_stop_client(void);

/**
 * @brief Get server IP address
 *
 * @param[out] IP address pointer to memory to store server IP address
 * @return     On success, 0 is returned.
 *             On failure, a negative value is returned.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_get_server_address(char *server_ipAddr);

/**
 * @brief Get server port number
 *
 * @param[out] port pointer to memory to store server port
 * @return     On success, 0 is returned.
 *             On failure, a negative value is returned.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_get_server_port(char *server_port);

/**
 * @brief Get lifetime for a client
 *
 * @param[out] lifetime pointer to memory to store client lifetime
 * @return     On success, 0 is returned.
 *             On failure, a negative value is returned.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_get_client_lifetime(int *lifetime);

/**
 * @brief Get state of client
 *
 * @param[out] lifetime pointer to memory to store client state
 * @return     On success, 0 is returned.
 *             On failure, a negative value is returned.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_get_client_state(dm_lwm2m_client_state_e *state);

/**
 * @brief Get client resource value
 *
 * @param[in] buffer pointer to resource URI
 *
 * @return     On success, 0 is returned.
 *             On failure, a negative value is returned.
 * @since Tizen RT v1.0
 */
int dm_lwm2m_display_client_resource(char *buffer);

#endif

/** @} */ // end of LWM2M group
