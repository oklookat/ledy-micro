#ifndef LEDY_SERVER
#define LEDY_SERVER

#include <esp_http_server.h>
#include "theled.h"

#define LEDY_COMMAND_SET_COLORS 0

httpd_handle_t ledyStartServer(void);
esp_err_t ledyStopServer(httpd_handle_t server);

#endif