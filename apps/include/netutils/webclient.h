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
 * @defgroup HTTP_CLIENT HTTP Client
 * @ingroup HTTP
 * @{
 */

/**
 * @file webclient.h
 * @brief APIs for HTTP Client.
 */

#ifndef __APPS_INCLUDE_NETUTILS_WEBCLIENT_H
#define __APPS_INCLUDE_NETUTILS_WEBCLIENT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <tinyara/config.h>
#include <sys/types.h>

#ifdef CONFIG_NET_SECURITY_TLS
#include "tls/config.h"
#include "tls/entropy.h"
#include "tls/ctr_drbg.h"
#include "tls/certs.h"
#include "tls/x509.h"
#include "tls/ssl.h"
#include "tls/net.h"
#include "tls/error.h"
#include "tls/debug.h"
#include "tls/ssl_cache.h"
#endif

#include <apps/netutils/webserver/http_keyvalue_list.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define WGET_MODE_GET                     0
#define WGET_MODE_POST                    1
#define WGET_MODE_PUT                     2
#define WGET_MODE_DELETE                  3

#define CONTENT_LENGTH                    0
#define CHUNKED_ENCODING                  1

#define WEBCLIENT_CONF_MAX_URL_SIZE       50
#define WEBCLIENT_CONF_MAX_ENTITY_SIZE    2048
#define WEBCLIENT_CONF_MAX_PHRASE_SIZE    50
#define WEBCLIENT_CONF_MAX_MESSAGE_SIZE   2100

#define WEBCLIENT_CONF_TIMEOUT_MSEC       5000
#define WEBCLIENT_CONF_MIN_TLS_MEMORY     100000

/****************************************************************************
 * Public types
 ****************************************************************************/

typedef struct http_client_response_t *httprsp;
typedef void (*wget_callback_t)(httprsp);

#ifdef CONFIG_NET_SECURITY_TLS
/**
 * @brief HTTP client TLS structure.
 */
struct http_client_tls_t {
	int client_fd;
	mbedtls_ssl_context tls_ssl;
	mbedtls_net_context tls_client_fd;

	int tls_init;
	mbedtls_ssl_config tls_conf;
	mbedtls_entropy_context tls_entropy;
	mbedtls_ctr_drbg_context tls_ctr_drbg;
	mbedtls_x509_crt tls_clicert;
	mbedtls_pk_context tls_pkey;
	mbedtls_ssl_session tls_session;
};

/**
 * @brief SSL configure structure.
 */
struct http_client_ssl_config_t {
#ifdef CONFIG_HW_RSA_SIGN
	unsigned int ca_key_index;
	unsigned int dev_key_index;
	unsigned int ca_cert_index;
	unsigned int dev_cert_index;
#else
	char *root_ca;
	char *dev_cert;
	char *private_key;
	unsigned int root_ca_len;
	unsigned int dev_cert_len;
	unsigned int private_key_len;
#endif							/* CONFIG_HW_RSA_SIGN */
};

#endif

/**
 * @brief HTTP request structure.
 */
struct http_client_request_t {
	int method;
	char *url;
	int buflen;
	int encoding;
	char *entity;
	wget_callback_t callback;
	struct http_keyvalue_list_t *headers;
#ifdef CONFIG_NET_SECURITY_TLS
	struct http_client_ssl_config_t ssl_config;
#endif
};

/**
 * @brief HTTP response structure.
 */
struct http_client_response_t {
	int method;
	char *url;
	int status;
	char *phrase;
	struct http_keyvalue_list_t *headers;
	char *message;
	char *entity;
	unsigned int entity_len;
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C" {
#else
#define EXTERN extern
#endif

/**
 * @brief http_client_send_request() sends the HTTP request to HTTP server with
 *                                   synchronous function.
 *
 * @param[in] request a structure pointer of information of request.
 * @param[in] ssl_config a structure pointer of information of TLS config.
 * @param[in] response a structure pointer of response message.
 * @return On success, OK(0) is returned.
 *         On failure, negative value is returned.
 * @since Tizen RT v1.0
 */

int http_client_send_request(struct http_client_request_t *request, struct http_client_ssl_config_t *ssl_config, struct http_client_response_t *response);

/**
 * @brief http_client_send_request_async() sends the HTTP request to HTTP server
 *                                         with asynchronous function.
 *
 * @param[in] request a structure pointer of information of request.
 * @param[in] ssl_config a structure pointer of information of TLS config.
 * @param[in] cb a function pointer called when receive response.
 * @return On success, OK(0) is returned.
 *         On failure, negative value is returned.
 * @since Tizen RT v1.0
 */

int http_client_send_request_async(struct http_client_request_t *request, struct http_client_ssl_config_t *ssl_config, wget_callback_t cb);

/**
 * @brief http_client_response_init() initializes the response structure.
 *
 * @param[in] response a structure pointer of response message.
 * @return On success, OK(0) is returned.
 *         On failure, negative value is returned.
 * @since Tizen RT v1.0
 */

int http_client_response_init(struct http_client_response_t *response);

/**
 * @brief http_client_response_release() releases the response structure.
 *
 * @param[in] response a structure pointer of response message.
 * @return N/A.
 * @since Tizen RT v1.0
 */

void http_client_response_release(struct http_client_response_t *response);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif							/* __APPS_INCLUDE_NETUTILS_WEBCLIENT_H */
/**@} */
