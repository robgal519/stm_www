/**
 ******************************************************************************
 * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    18-November-2015
 * @brief   Basic http server implementation using LwIP netconn API
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "httpserver-netconn.h"
#include "cmsis_os.h"
#include "err.h"
#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/opt.h"
#include "temp.h"
#include <string.h>

void log_message(char *ch) {
  while (*ch)
    ITM_SendChar(*ch++);
}

#define HTTP_OK "HTTP/1.1 200 OK\n"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found\n"

#define CONTENT_TYPE_JSON "Content-Type: application/json; charset=utf-8\n"
#define CONTENT_TYPE_TEXT "Content-Type: text/html; charset=utf-8\n"

#define END_OF_HEADER "\r\n"

#define ENPOINT_NOT_FOUND -1

/////////////
// in module storage for handlers

#if MAX_GET_ENDPOINTS > 0
static endpoint_handler GET_HANDLERS[MAX_GET_ENDPOINTS];
static const char *GET_ENDPOINTS[MAX_GET_ENDPOINTS];
static uint32_t next_free_get_endpoint = 0;
#endif

#if MAX_POST_ENDPOINTS > 0
static endpoint_handler POST_HANDLERS[MAX_POST_ENDPOINTS];
static const char *POST_ENDPOINTS[MAX_POST_ENDPOINTS];
static uint32_t next_free_post_endpoint = 0;
#endif

#if MAX_PUT_ENDPOINTS > 0
static endpoint_handler PUT_HANDLERS[MAX_PUT_ENDPOINTS];
static const char *PUT_ENDPOINTS[MAX_PUT_ENDPOINTS];
static uint32_t next_free_put_endpoint = 0;
#endif

#if MAX_PATCH_ENDPOINTS > 0
static endpoint_handler PATCH_HANDLERS[MAX_PATCH_ENDPOINTS];
static const char *PATCH_ENDPOINTS[MAX_PATCH_ENDPOINTS];
static uint32_t next_free_patch_endpoint = 0;
#endif

#if MAX_DELETE_ENDPOINTS > 0
static endpoint_handler DELETE_HANDLERS[MAX_DELETE_ENDPOINTS];
static const char *DELETE_ENDPOINTS[MAX_DELETE_ENDPOINTS];
static uint32_t next_free_delete_endpoint = 0;
#endif

err_t page_not_found_handler(struct netconn *connection_context) {
  netconn_write(connection_context, HTTP_NOT_FOUND END_OF_HEADER,
                strlen(HTTP_NOT_FOUND) + strlen(END_OF_HEADER), NETCONN_NOCOPY);
  static const char not_found_page[] =
      "<html><body><p> Page Not found</p></body></html>";
  netconn_write(connection_context, not_found_page, sizeof(not_found_page),
                NETCONN_NOCOPY);
  return ERR_OK;
}

err_t alive_handler(struct netconn *connection_context) {
  netconn_write(connection_context, HTTP_OK CONTENT_TYPE_JSON END_OF_HEADER,
                strlen(HTTP_OK) + strlen(CONTENT_TYPE_JSON) +
                    strlen(END_OF_HEADER),
                NETCONN_NOCOPY);
  static const char alive[] = "{\"alive\":true}";
  netconn_write(connection_context, alive, sizeof(alive), NETCONN_NOCOPY);
  return ERR_OK;
}

static int32_t match_endpoint(const char **endpoint_repository,
                              uint32_t repository_size,
                              const char *request_endpoint) {
  const char *end_of_endpoint = strpbrk(
      request_endpoint,
      " "); // after endpoint there is " " - find it and get endpoint len
  if (end_of_endpoint == NULL) {
    return ENPOINT_NOT_FOUND;
  }
  uint32_t request_endpoint_len =
      ((uint32_t)end_of_endpoint - (uint32_t)request_endpoint);

  for (uint32_t endpointID = 0; endpointID < repository_size; endpointID++) {
    uint32_t expected_endpoint_len = strlen(endpoint_repository[endpointID]);

    // get max len for cmp
    uint32_t final_len = request_endpoint_len > expected_endpoint_len
                             ? request_endpoint_len
                             : expected_endpoint_len;

    if (expected_endpoint_len != 0 &&
        strncmp(endpoint_repository[endpointID], request_endpoint, final_len) ==
            0) {
      return endpointID;
    }
  }
  return ENPOINT_NOT_FOUND;
}

static err_t handle_request(struct netconn *connection_context, char *request) {
#if MAX_GET_ENDPOINTS > 0
  const char *get = "GET ";
  if (strncmp(get, request, strlen(get)) == 0) {
    int32_t endpointID =
        match_endpoint(GET_ENDPOINTS, MAX_GET_ENDPOINTS, request + strlen(get));

    if (endpointID == ENPOINT_NOT_FOUND) {
      return page_not_found_handler(connection_context);
    } else {
      return GET_HANDLERS[endpointID](connection_context);
    }
  }
#endif
#if MAX_POST_ENDPOINTS > 0
  const char *post = "POST ";
  if (strncmp(post, request, strlen(post))) {

    return ERR_OK;
  }
#endif
#if MAX_PUT_ENDPOINTS > 0
  const char *put = "PUT ";
  if (strncmp(put, request, strlen(put))) {

    return ERR_OK;
  }
#endif
#if MAX_PATCH_ENDPOINTS > 0
  const char *patch = "PATCH ";
  if (strncmp(patch, request, strlen(patch))) {

    return ERR_OK;
  }
#endif
#if MAX_DELETE_ENDPOINTS > 0
  const char *delete = "DELET ";
  if (strncmp(delete, request, strlen(delete))) {

    return ERR_OK;
  }
#endif

  page_not_found_handler(connection_context);
  return ERR_OK;
}
////////////

bool register_endpoint(HTTP_Method method, const char *endpoint_match_string,
                       endpoint_handler handler) {
  switch (method) {
#if MAX_GET_ENDPOINTS > 0
  case GET: {
    if (next_free_get_endpoint == MAX_GET_ENDPOINTS) {
      log_message("ERROR: can not register next handler\n");
      return false;
    }
    GET_ENDPOINTS[next_free_get_endpoint] = endpoint_match_string;
    GET_HANDLERS[next_free_get_endpoint] = handler;
    next_free_get_endpoint++;
    return true;
  }
#endif
#if MAX_POST_ENDPOINTS > 0
  case POST: {
    if (next_free_post_endpoint == MAX_POST_ENDPOINTS) {
      return false;
    }
    POST_ENDPOINTS[next_free_post_endpoint] = endpoint_match_string;
    POST_HANDLERS[next_free_post_endpoint] = handler;
    next_free_post_endpoint++;
    return true;
  }
#endif
#if MAX_PUT_ENDPOINTS > 0
  case PUT: {
    if (next_free_put_endpoint == MAX_PUT_ENDPOINTS) {
      return false;
    }
    PUT_ENDPOINTS[next_free_put_endpoint] = endpoint_match_string;
    PUT_HANDLERS[next_free_put_endpoint] = handler;
    next_free_put_endpoint++;
    return true;
  }
#endif
#if MAX_PATCH_ENDPOINTS > 0
  case PATCH: {
    if (next_free_patch_endpoint == MAX_PATCH_ENDPOINTS) {
      return false;
    }
    PATCH_ENDPOINTS[next_free_patch_endpoint] = endpoint_match_string;
    PATCH_HANDLERS[next_free_patch_endpoint] = handler;
    next_free_patch_endpoint++;
    return true;
  }
#endif
#if MAX_DELETE_ENDPOINTS > 0
  case DELETE: {
    if (next_free_delete_endpoint == MAX_DELETE_ENDPOINTS) {
      return false;
    }
    DELETE_ENDPOINTS[next_free_delete_endpoint] = endpoint_match_string;
    DELETE_HANDLERS[next_free_delete_endpoint] = handler;
    next_free_delete_endpoint++;
    return true;
  }
#endif
  }

  return true;
}

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define WEBSERVER_THREAD_PRIO (tskIDLE_PRIORITY + 4)

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
u32_t nPageHits = 0;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
 * @brief serve tcp connection
 * @param conn: pointer on connection structure
 * @retval None
 */
void http_server_serve(struct netconn *conn) {
  log_message("connection received!\n");
  struct netbuf *inbuf;
  err_t recv_err;
  char *buf;
  u16_t buflen;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);

  if (recv_err == ERR_OK) {
    if (netconn_err(conn) == ERR_OK) {
      netbuf_data(inbuf, (void **)&buf, &buflen);

      log_message(buf);
      log_message("\n");

      handle_request(conn, buf);

/* Is this an HTTP GET command? (only check the first 5 chars, since
there are other formats for GET, and we're keeping it very simple )*/
#if 0
      if ((buflen >=5) && (strncmp(buf, "GET /", 5) == 0))
      {
    	  if (strncmp((char const *)buf,"GET /index.html",15)==0 || strncmp((char const *)buf,"GET / HTTP",10)==0) {
          log_message("GET index.html request\n");

           netconn_write(conn, HTTP_OK CONTENT_TYPE_TEXT "\r\n", strlen(HTTP_OK)+strlen(CONTENT_TYPE_TEXT)+2,NETCONN_NOCOPY);

    		  netconn_write(conn, (const unsigned char*)index_html, index_html_len, NETCONN_NOCOPY);
    	  }
    	  if (strncmp((char const *)buf,"GET /led1", 9) == 0) {
          netconn_write(conn, 
          HTTP_OK 
          CONTENT_TYPE_JSON 
          "\n{\"led1\":\"OK\"}", 
          14+strlen(HTTP_OK) + strlen(CONTENT_TYPE_JSON), 
          NETCONN_NOCOPY);

          netconn_write(conn, 
          "\n{\"led2\":\"OK\"}", 
          14, 
          NETCONN_NOCOPY);
    		  //HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
    	  }
    	  if (strncmp((char const *)buf,"GET /led2", 9) == 0) {
    		  //HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    	  }
    	  if (strncmp((char const *)buf,"GET /led3", 9) == 0) {
    		  //HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
    	  }
    	  if (strncmp((char const *)buf,"GET /btn1", 9) == 0) {
    		  // if(HAL_GPIO_ReadPin(User_Blue_Button_GPIO_Port, User_Blue_Button_Pin) == GPIO_PIN_SET)
    			//   netconn_write(conn, (const unsigned char*)"ON", 2, NETCONN_NOCOPY);
    		  // else
    			  netconn_write(conn, (const unsigned char*)"OFF", 3, NETCONN_NOCOPY);
    	  }
    	  if (strncmp((char const *)buf,"GET /adc", 8) == 0) {
    		  sprintf(buf, "HTTP/1.1 200 OK\nContent-Type: text/html;\n\n %d.%2d °C", (int)getMCUTemperature(),(int)(getMCUTemperature()*100)%100);
          log_message("MCU temperature:\n");
          log_message(buf);
    		  netconn_write(conn, (const unsigned char*)buf, strlen(buf), NETCONN_NOCOPY);
    	  }
      }
#endif
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

/**
 * @brief  http server thread
 * @retval None
 */
static void http_server_netconn_thread() {
  log_message("http_thread_created\n");
  struct netconn *conn, *newconn;
  err_t err, accept_err;

  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);

  if (conn != NULL) {
    /* Bind to port 80 (HTTP) with default IP address */
    err = netconn_bind(conn, IP4_ADDR_ANY, 80);

    if (err == ERR_OK) {
      log_message("http listen on port 80\n");
      /* Put the connection into LISTEN state */
      netconn_listen(conn);

      while (1) {
        /* accept any icoming connection */
        accept_err = netconn_accept(conn, &newconn);
        if (accept_err == ERR_OK) {
          /* serve connection */
          http_server_serve(newconn);

          /* delete connection */
          netconn_delete(newconn);
        }
      }
    }
  }
}

/**
 * @brief  Initialize the HTTP server (start its thread)
 * @param  none
 * @retval None
 */
void http_server_netconn_init() {
  sys_thread_new("HTTP", http_server_netconn_thread, NULL,
                 DEFAULT_THREAD_STACKSIZE, WEBSERVER_THREAD_PRIO);
}
