#ifndef __HTTPSERVER_NETCONN_H__
#define __HTTPSERVER_NETCONN_H__
#include "lwip/api.h"
#include <stdbool.h>

/////////// part of the config!
#ifndef MAX_GET_ENDPOINTS
#define MAX_GET_ENDPOINTS 6
#endif
#ifndef MAX_POST_ENDPOINTS
#define MAX_POST_ENDPOINTS 0
#endif
#ifndef MAX_PUT_ENDPOINTS
#define MAX_PUT_ENDPOINTS 0
#endif
#ifndef MAX_PATCH_ENDPOINTS
#define MAX_PATCH_ENDPOINTS 0
#endif
#ifndef MAX_DELETE_ENDPOINTS
#define MAX_DELETE_ENDPOINTS 0
#endif
//////////

typedef err_t (*endpoint_handler)(struct netconn *connection_context);
typedef enum {
#if MAX_GET_ENDPOINTS > 0
  GET,
#endif
#if MAX_POST_ENDPOINTS > 0
  POST,
#endif
#if MAX_PUT_ENDPOINTS > 0
  PUT,
#endif
#if MAX_PATCH_ENDPOINTS > 0
  PATCH,
#endif
#if MAX_DELETE_ENDPOINTS > 0
  DELETE,
#endif
} HTTP_Method;

void http_server_netconn_init(void);
void DynWebPage(struct netconn *conn);
bool register_endpoint(HTTP_Method method, const char *endpoint_match_string,
                       endpoint_handler handler);

//////// move to handler module

err_t alive_handler(struct netconn *connection_context);
err_t page_not_found_handler(struct netconn *connection_context);

#endif /* __HTTPSERVER_NETCONN_H__ */
