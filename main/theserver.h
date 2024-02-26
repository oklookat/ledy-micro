#ifndef LEDY_SERVER
#define LEDY_SERVER

#include <esp_http_server.h>
#include "theled.h"

#define LEDY_COMMAND_SET_COLORS 0

#ifdef LEDY_TESTING
// https://stackoverflow.com/a/19995527
#pragma pack(push, 1)
typedef struct LedyCommand {
    uint8_t whatCommand;
    uint16_t dataLen;
    uint8_t *data;
} LedyCommand;
#pragma pack(pop)
#endif

httpd_handle_t ledyStartServer(void);
esp_err_t ledyStopServer(httpd_handle_t server);

#endif