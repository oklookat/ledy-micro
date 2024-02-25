#include "theserver.h"

static const char *TAG = "ledy-server";

static bool processCommand(uint8_t *payload, size_t length)
{
    if (payload == NULL || length == 0) {
        return true;
    }

    switch (payload[0])
    {
    case LEDY_COMMAND_SET_COLORS:
    {
        return ledySetLeds(payload, length);
    }
    default:
    {
        return true;
    }
    }
}

static esp_err_t wsHandler(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "New client connected");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    // Set max_len = 0 to get the frame len.
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len)
    {
        buf = calloc(1, ws_pkt.len);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        // Set max_len = ws_pkt.len to get the frame payload.
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
    }

    bool clearMem = processCommand(ws_pkt.payload, ws_pkt.len);
    // ESP_LOGI(TAG, "Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());

    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    static const char *data = "ok";
    ws_pkt.payload = (uint8_t *)data;
    ws_pkt.len = strlen(data);

    ret = httpd_ws_send_frame(req, &ws_pkt);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
    }

    if (clearMem == true)
    {
        free(buf);
    }

    return ret;
}

static const httpd_uri_t wsConfig = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = wsHandler,
    .user_ctx = NULL,
    .is_websocket = true};

httpd_handle_t ledyStartServer(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &wsConfig);
        return server;
    }

    ESP_LOGE(TAG, "Error starting server!");
    return NULL;
}

esp_err_t ledyStopServer(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}
