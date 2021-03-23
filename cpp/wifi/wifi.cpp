/*
amikodev/components-esp32 - library components on esp-idf
Copyright © 2020 Prihodko Dmitriy - asketcnc@yandex.ru
*/

/*
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "wifi.hpp"

#define TAG "Wifi"
#define TAG_EVENT "Wifi-Event"
#define TAG_WS "Wifi-Websocket"
#define TAG_HTTP "Wifi-Http"
#define TAG_SERVER "Wifi-Server"
#define TAG_COUNT "Wifi-Count"


QueueHandle_t Wifi::clientQueue;
system_event_id_t Wifi::staState = SYSTEM_EVENT_STA_STOP;
system_event_id_t Wifi::apState = SYSTEM_EVENT_AP_STOP;

char* Wifi::hostname = NULL;

#if CONFIG_AMIKODEV_WIFI_SD_CARD
SdCardStorage* Wifi::sdCard = NULL;
#endif

#if CONFIG_AMIKODEV_WIFI_SPIFFS
SpiffsStorage* Wifi::spiffs = NULL;
#endif


// WifiBinFunc_t Wifi::recieveBinaryFunc;
void (*Wifi::recieveBinaryFunc)(uint8_t *data, uint32_t length);
void (*Wifi::apStaConnectFunc)();
void (*Wifi::apStaDisconnectFunc)();

#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
void (*Wifi::wsDisconnectFunc)();
#endif

bool (*Wifi::httpServeReqFunc)(struct netconn *conn, char *buf, uint32_t length);

void Wifi::setup(){
    ESP_LOGI(TAG, "Setup");

    esp_err_t ret;

    tcpip_adapter_init();
    nvs_flash_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    IP4_ADDR(&info.ip, 192, 168, 4, 1);
    IP4_ADDR(&info.gw, 192, 168, 4, 1);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
    ESP_ERROR_CHECK(esp_event_loop_init(eventHandler, NULL));

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    wifi_mode_t mode = WIFI_MODE_NULL;
    #if CONFIG_AMIKODEV_WIFI_AP_ENABLED && CONFIG_AMIKODEV_WIFI_STA_ENABLED
        mode = WIFI_MODE_APSTA;
    #elif CONFIG_AMIKODEV_WIFI_AP_ENABLED
        mode = WIFI_MODE_AP;
    #elif CONFIG_AMIKODEV_WIFI_STA_ENABLED
        mode = WIFI_MODE_STA;
    #endif

    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));


    #if CONFIG_AMIKODEV_WIFI_AP_ENABLED
    #if CONFIG_AMIKODEV_WIFI_AP_HIDDEN
    uint8_t hidden = 1;
    #else
    uint8_t hidden = 0;
    #endif
    wifi_config_t wifiConfigAP = {
        .ap = {
            {.ssid = CONFIG_AMIKODEV_WIFI_AP_SSID},
            {.password = CONFIG_AMIKODEV_WIFI_AP_PSSWD},
            .channel = 0,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .ssid_hidden = hidden,
            .max_connection = 4,
            .beacon_interval = 100
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifiConfigAP));
    ESP_LOGI(TAG, "WiFi AP set config");
    #endif


    #if CONFIG_AMIKODEV_WIFI_STA_ENABLED
    wifi_config_t wifiConfigSTA = {
        .sta = {
            {.ssid = CONFIG_AMIKODEV_WIFI_STA_SSID},
            {.password = CONFIG_AMIKODEV_WIFI_STA_PSSWD},

        },
    };
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifiConfigSTA);
    if(ret != ESP_OK){
        ESP_LOGI(TAG, "WiFi STA set config ERROR: [%d] %s", ret, esp_err_to_name(ret));
    }
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfigSTA));
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "WiFi STA set config");
    #endif


    #if CONFIG_AMIKODEV_WIFI_AP_ENABLED || CONFIG_AMIKODEV_WIFI_STA_ENABLED
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "WiFi set up");
    #else
    ESP_LOGI(TAG, "WiFi turned off");
    #endif

}


// handles WiFi events
esp_err_t Wifi::eventHandler(void* ctx, system_event_t* event){
    switch(event->event_id){

        // STA
        case SYSTEM_EVENT_STA_START:
            staState = event->event_id;
            if(hostname != NULL)
                ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, hostname));
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_STOP:
            staState = event->event_id;
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG_EVENT, "STA Connected");
            staState = event->event_id;
            if(apStaConnectFunc != NULL){
                ESP_LOGI(TAG_EVENT, "apStaConnectFunc is defined");
                (*apStaConnectFunc)();
            }
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG_EVENT, "STA Disconnected");
            staState = event->event_id;
            if(apStaDisconnectFunc != NULL){
                ESP_LOGI(TAG_EVENT, "apStaDisconnectFunc is defined");
                (*apStaDisconnectFunc)();
            }
            break;

        // AP
        case SYSTEM_EVENT_AP_START:
            ESP_LOGI(TAG_EVENT, "Access Point Started");
            apState = event->event_id;
            break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGI(TAG_EVENT, "Access Point Stopped");
            apState = event->event_id;
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGI(TAG_EVENT, "AP Probe Received");
            apState = event->event_id;
            break;

        // AP_STA
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG_EVENT, "STA Connected, MAC=%02x:%02x:%02x:%02x:%02x:%02x AID=%i",
                event->event_info.sta_connected.mac[0], event->event_info.sta_connected.mac[1],
                event->event_info.sta_connected.mac[2], event->event_info.sta_connected.mac[3],
                event->event_info.sta_connected.mac[4], event->event_info.sta_connected.mac[5],
                event->event_info.sta_connected.aid
            );
            apState = event->event_id;
            if(apStaConnectFunc != NULL){
                ESP_LOGI(TAG_EVENT, "apStaConnectFunc is defined");
                (*apStaConnectFunc)();
            }
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG_EVENT, "STA Disconnected, MAC=%02x:%02x:%02x:%02x:%02x:%02x AID=%i",
                event->event_info.sta_disconnected.mac[0], event->event_info.sta_disconnected.mac[1],
                event->event_info.sta_disconnected.mac[2], event->event_info.sta_disconnected.mac[3],
                event->event_info.sta_disconnected.mac[4], event->event_info.sta_disconnected.mac[5],
                event->event_info.sta_disconnected.aid
            );
            apState = event->event_id;
            if(apStaDisconnectFunc != NULL){
                ESP_LOGI(TAG_EVENT, "apStaDisconnectFunc is defined");
                (*apStaDisconnectFunc)();
            }
            break;
        // case SYSTEM_EVENT_STA_GOT_IP6:
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            ESP_LOGI(TAG_EVENT, "Got IP6=%01x:%01x:%01x:%01x",
                event->event_info.got_ip6.ip6_info.ip.addr[0], event->event_info.got_ip6.ip6_info.ip.addr[1],
                event->event_info.got_ip6.ip6_info.ip.addr[2], event->event_info.got_ip6.ip6_info.ip.addr[3]
            );
            staState = event->event_id;
            break;
        default:
            ESP_LOGI(TAG_EVENT, "Unregistered event=%i", event->event_id);
            break;
    }
    ESP_LOGI(TAG_EVENT, "Event 0x%X; staState: 0x%X; apState: 0x%X", event->event_id, staState, apState);
    return ESP_OK;
}


#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
// handles websocket events
void Wifi::websocketCallback(uint8_t num, WEBSOCKET_TYPE_t type, char* msg, uint64_t len){
    switch(type) {
        case WEBSOCKET_CONNECT:
            ESP_LOGI(TAG_WS, "client %i connected!", num);
            break;
        case WEBSOCKET_DISCONNECT_EXTERNAL:
            ESP_LOGI(TAG_WS, "client %i sent a disconnect message", num);
            if(wsDisconnectFunc != NULL){
                ESP_LOGI(TAG_WS, "wsDisconnectFunc is defined");
                (*wsDisconnectFunc)();
            }
            break;
        case WEBSOCKET_DISCONNECT_INTERNAL:
            ESP_LOGI(TAG_WS, "client %i was disconnected", num);
            if(wsDisconnectFunc != NULL){
                ESP_LOGI(TAG_WS, "wsDisconnectFunc is defined");
                (*wsDisconnectFunc)();
            }
            break;
        case WEBSOCKET_DISCONNECT_ERROR:
            ESP_LOGI(TAG_WS, "client %i was disconnected due to an error", num);
            break;
        case WEBSOCKET_TEXT:
            if(len){ // if the message length was greater than zero
                switch(msg[0]){
                    default:
                        ESP_LOGI(TAG_WS, "got an unknown message with length %i", (int)len);
                        break;
                }
            }
            break;
        case WEBSOCKET_BIN:
            ESP_LOGI(TAG_WS, "client %i sent binary message of size %i", num, (uint32_t)len);
            if(recieveBinaryFunc != NULL){
                ESP_LOGI(TAG_WS, "wifi_recieve_binary_func is defined");
                (*recieveBinaryFunc)((uint8_t *)msg, (uint32_t)len);
            } else{
                ESP_LOGI(TAG_WS, "wifi_recieve_binary_func is NULL");
            }
            break;
        case WEBSOCKET_PING:
            ESP_LOGI(TAG_WS, "client %i pinged us with message of size %i", num, (uint32_t)len);
            break;
        case WEBSOCKET_PONG:
            ESP_LOGI(TAG_WS, "client %i responded to the ping", num);
            break;
    }
}
#endif

#if CONFIG_AMIKODEV_WIFI_INCLUDE_EMBED_WEB_FILES
void Wifi::getResourceHandler(struct netconn *conn, const char *header, const uint8_t start[], const uint8_t end[]){
    const uint32_t len = end - start -1;
    netconn_write(conn, header, strlen(header), NETCONN_NOCOPY);
    netconn_write(conn, start, len, NETCONN_NOCOPY);
    netconn_close(conn);
    netconn_delete(conn);
}
void Wifi::handler_index_html(struct netconn *conn, const char *header){
    extern const uint8_t start[] asm("_binary_index_html_start");
    extern const uint8_t end[]   asm("_binary_index_html_end");
    getResourceHandler(conn, header, start, end);
}
void Wifi::handler_style_css(struct netconn *conn, const char *header){
    extern const uint8_t start[] asm("_binary_style_css_start");
    extern const uint8_t end[]   asm("_binary_style_css_end");
    getResourceHandler(conn, header, start, end);
}
void Wifi::handler_main_js(struct netconn *conn, const char *header){
    extern const uint8_t start[] asm("_binary_main_js_start");
    extern const uint8_t end[]   asm("_binary_main_js_end");
    getResourceHandler(conn, header, start, end);
}
void Wifi::handler_error_html(struct netconn *conn, const char *header){
    extern const uint8_t start[] asm("_binary_error_html_start");
    extern const uint8_t end[]   asm("_binary_error_html_end");
    getResourceHandler(conn, header, start, end);
}
#endif


// serves any clients
void Wifi::httpServe(struct netconn *conn){
    struct netbuf* inbuf;
    static char* buf;
    static uint16_t buflen;
    static err_t err;

    STORAGE_TYPE storageType = STORAGE_EMBED;
    std::string basePath = "";


#if CONFIG_AMIKODEV_WIFI_SD_CARD
    if(sdCard != NULL){
        storageType = STORAGE_SDCARD;
        basePath = "/sdcard/frontend";
    }
#elif CONFIG_AMIKODEV_WIFI_SPIFFS
    if(spiffs != NULL){
        storageType = STORAGE_SPIFFS;
        basePath = spiffs->getBasePath();
        basePath.append("/web");
    }
#endif


    netconn_set_recvtimeout(conn, 1000); // allow a connection timeout of 1 second
    ESP_LOGI(TAG_HTTP, "reading from client...");
    err = netconn_recv(conn, &inbuf);
    ESP_LOGI(TAG_HTTP, "read from client");
    if(err == ERR_OK) {
        netbuf_data(inbuf, (void**)&buf, &buflen);
        if(buf){
            if(storageType == STORAGE_SDCARD || storageType == STORAGE_SPIFFS){
                // парсим http-запрос и
                // получаем файл из файловой системы и отдаём клиенту

                HttpRequest request;
                HttpRequest::HttpRequestInfo info;

                bool requestParsed = request.parse(buf, buflen, &info);
                // bool requestParsed = request.parse(str2, strlen(str2), &info);
                if(requestParsed){
                    request.printInfo(&info);

                    std::string strHeaderUpgrade = request.getHeader("Upgrade");
                    int compareWs = strHeaderUpgrade.compare("websocket");
                    if(info.method == HttpRequest::METHOD_GET && (compareWs == 0 || compareWs == 1)){
                        // is WebSocket
                        #if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
                            ESP_LOGI(TAG_HTTP, "Requesting websocket on /");
                            ESP_LOGI(TAG_HTTP, "Request is WebSocket");
                            ws_server_add_client(conn, buf, buflen, "/", websocketCallback);
                        #endif
                        netbuf_delete(inbuf);
                    } else if(info.method == HttpRequest::METHOD_GET){
                        // method GET

                        std::string filePath = basePath;
                        filePath.append(info.path, info.cleanPathLength);

                        FILE *f = fopen(filePath.c_str(), "rb");
                        if(f == NULL){
                            ESP_LOGW(TAG_HTTP, "Wrong open file \"%s\"", filePath.c_str());
                            netconn_close(conn);
                            netconn_delete(conn);
                            fclose(f);
                        } else{
                            ESP_LOGI(TAG_HTTP, "Success open file \"%s\"", filePath.c_str());
                            char *header = HttpResponce::getHeaderByFileExtension(info.fileExt);
                            if(header != NULL){
                                netconn_write(conn, header, strlen(header), NETCONN_NOCOPY);
                            }

                            fseek(f, 0, SEEK_END);
                            long lSize = ftell(f);
                            rewind(f);

                            ESP_LOGI(TAG_HTTP, "File size: %ld", lSize);

                            char fbuf[1024];

                            while(true){
                                size_t s = fread(fbuf, 1, 1024, f);
                                if(s == 0) break;
                                // ESP_LOG_BUFFER_HEXDUMP(TAG, fbuf, s, ESP_LOG_INFO);
                                netconn_write(conn, fbuf, s, NETCONN_NOCOPY);
                            }
                            netconn_close(conn);
                            netconn_delete(conn);
                            fclose(f);
                        }
                    } else if(info.method == HttpRequest::METHOD_POST){
                        // method POST

                    } else if(info.method == HttpRequest::METHOD_OPTIONS){
                        // method OPTIONS

                    }

                    free(info.path);
                } else{
                    ESP_LOGW(TAG_HTTP, "Request not parsed");
                }


            } else{
                // карта памяти не подключена,
                // выполняется поведение по умолчанию

                // websocket
                if(strstr(buf, "GET / ") && strstr(buf, "Upgrade: websocket")) {
                    #if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
                        ESP_LOGI(TAG_HTTP, "Requesting websocket on /");
                        ws_server_add_client(conn, buf, buflen, "/", websocketCallback);
                    #endif
                }

#if CONFIG_AMIKODEV_WIFI_INCLUDE_EMBED_WEB_FILES
                // index.html
                else if(strstr(buf, "GET / ") && !strstr(buf, "Upgrade: websocket")) {
                    ESP_LOGI(TAG_HTTP, "Sending /");
                    handler_index_html(conn, HttpResponce::HTML_HEADER);
                    netbuf_delete(inbuf);
                }
                // style.css
                else if(strstr(buf, "GET /style.css ")) {
                    ESP_LOGI(TAG_HTTP, "Sending /style.css");
                    handler_style_css(conn, HttpResponce::CSS_HEADER);
                    netbuf_delete(inbuf);
                }
                // main.js
                else if(strstr(buf, "GET /main.js ")) {
                    ESP_LOGI(TAG_HTTP, "Sending /main.js");
                    handler_main_js(conn, HttpResponce::JS_HEADER);
                    netbuf_delete(inbuf);
                }
#endif

                else if(strstr(buf, "GET /")) {
                    bool retReq = false;
                    if(httpServeReqFunc != NULL){
                        retReq = httpServeReqFunc(conn, buf, (uint32_t)buflen);
                    }
                    if(retReq){
                        netbuf_delete(inbuf);
                    } else{
                        ESP_LOGI(TAG_HTTP, "Unknown request, sending error page: %s", buf);
#if CONFIG_AMIKODEV_WIFI_INCLUDE_EMBED_WEB_FILES
                        handler_error_html(conn, HttpResponce::ERROR_HEADER);
#endif
                        netbuf_delete(inbuf);
                    }
                }

                else {
                    ESP_LOGI(TAG_HTTP, "Unknown request");
                    netconn_close(conn);
                    netconn_delete(conn);
                    netbuf_delete(inbuf);
                }
            }

        }
        else {
            ESP_LOGI(TAG_HTTP, "Unknown request (empty?...)");
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }
    }
    else { // if err==ERR_OK
        ESP_LOGI(TAG_HTTP, "error on read, closing connection");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
    }
}

// handles clients when they first connect. passes to a queue
void Wifi::serverTask(void* pvParameters){
    struct netconn *conn, *newconn;
    static err_t err;
    clientQueue = xQueueCreate(clientQueueSize, sizeof(struct netconn*));

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, 80);
    netconn_listen(conn);
    ESP_LOGI(TAG_SERVER, "server listening");
    do{
        err = netconn_accept(conn, &newconn);
        ESP_LOGI(TAG_SERVER, "new client");
        if(err == ERR_OK){
            xQueueSendToBack(clientQueue, &newconn, portMAX_DELAY);
            // httpServe(newconn);  // перезагружает устройство :((
        }
    } while(err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
    ESP_LOGE(TAG_SERVER, "task ending, rebooting board");
    esp_restart();
}

// receives clients from queue, handles them
void Wifi::serverHandleTask(void* pvParameters){
    struct netconn* conn;
    ESP_LOGI(TAG_SERVER, "task starting");
    for(;;){
        xQueueReceive(clientQueue, &conn, portMAX_DELAY);
        if(!conn) continue;
        httpServe(conn);
    }
    vTaskDelete(NULL);
}

#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
void Wifi::countTask(void* pvParameters){
    char out[20];
    int len;
    int clients;
    const static char* word = "%i";
    uint8_t n = 0;
    const int DELAY = 1000 / portTICK_PERIOD_MS; // 1 second

    ESP_LOGI(TAG_COUNT, "starting task");
    for(;;){
        len = sprintf(out, word, n);
        clients = ws_server_send_text_all(out, len);
        if(clients > 0){
            //ESP_LOGI(TAG,"sent: \"%s\" to %i clients",out,clients);
        }
        n++;
        vTaskDelay(DELAY);
    }
}
#endif

void Wifi::recieveBinary(void (*func)(uint8_t *data, uint32_t length)){
    Wifi::recieveBinaryFunc = func;
}

void Wifi::apStaConnect(void (*func)()){
    Wifi::apStaConnectFunc = func;
}

void Wifi::apStaDisconnect(void (*func)()){
    Wifi::apStaDisconnectFunc = func;
}

#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
void Wifi::wsDisconnect(void (*func)()){
    Wifi::wsDisconnectFunc = func;
}
#endif

void Wifi::httpServeReq(bool (*func)(struct netconn *conn, char *buf, uint32_t length)){
    Wifi::httpServeReqFunc = func;
}

/**
 * Установка hostname
 * @param name hostname
 */
void Wifi::setHostname(char *name){
    Wifi::hostname = name;
}

#if CONFIG_AMIKODEV_WIFI_SD_CARD
/**
 * Установка карты памяти SdCard
 * @param card карта памяти
 */
void Wifi::setSdCard(SdCardStorage *card){
    Wifi::sdCard = card;
}
#endif

#if CONFIG_AMIKODEV_WIFI_SPIFFS
/**
 * Установка хранилища spiffs
 * @param storage хранилище
 */
void Wifi::setSpiffs(SpiffsStorage *storage){
    Wifi::spiffs = storage;
}
#endif



