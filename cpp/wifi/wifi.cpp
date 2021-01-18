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


QueueHandle_t Wifi::clientQueue;
system_event_id_t Wifi::staState = SYSTEM_EVENT_STA_STOP;
system_event_id_t Wifi::apState = SYSTEM_EVENT_AP_STOP;

const char* Wifi::HTML_HEADER = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const char* Wifi::ERROR_HEADER = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
const char* Wifi::JS_HEADER = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
const char* Wifi::CSS_HEADER = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";

char* Wifi::hostname = NULL;

#if CONFIG_AMIKODEV_WIFI_SD_CARD
SdCard* Wifi::sdCard = NULL;
#endif

// WifiBinFunc_t Wifi::recieveBinaryFunc;
void (*Wifi::recieveBinaryFunc)(uint8_t *data, uint32_t length);
void (*Wifi::apStaDisconnectFunc)();
void (*Wifi::wsDisconnectFunc)();
bool (*Wifi::httpServeReqFunc)(struct netconn *conn, char *buf, uint32_t length);

void Wifi::setup(){

    #if CONFIG_AMIKODEV_WIFI_AP_ENABLED
    setupAP();
    #endif

    #if CONFIG_AMIKODEV_WIFI_STA_ENABLED
    setupSTA();
    #endif

}

void Wifi::setupSTA(){

}

void Wifi::setupAP(){

    const char* TAG = "wifi_setup";

    esp_err_t ret;

    ESP_LOGI(TAG,"starting tcpip adapter");
    tcpip_adapter_init();
    nvs_flash_init();
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
    //tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP,"esp32");
    tcpip_adapter_ip_info_t info;
    memset(&info, 0, sizeof(info));
    IP4_ADDR(&info.ip, 192, 168, 4, 1);
    IP4_ADDR(&info.gw, 192, 168, 4, 1);
    // IP4_ADDR(&info.ip, 192, 168, 1, 17);
    // IP4_ADDR(&info.gw, 192, 168, 1, 17);
    IP4_ADDR(&info.netmask, 255, 255, 255, 0);
    ESP_LOGI(TAG,"setting gateway IP");
    ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
    //ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP,"esp32"));
    //ESP_LOGI(TAG,"set hostname to \"%s\"",hostname);
    ESP_LOGI(TAG,"starting DHCPS adapter");
    ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
    //ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_AP,hostname));
    ESP_LOGI(TAG,"starting event loop");
    ESP_ERROR_CHECK(esp_event_loop_init(eventHandler, NULL));

    ESP_LOGI(TAG,"initializing WiFi");
    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    #if CONFIG_AMIKODEV_WIFI_AP_HIDDEN
    uint8_t hidden = 1;
    #else
    uint8_t hidden = 0;
    #endif


    #if CONFIG_AMIKODEV_WIFI_AP_ENABLED
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


    // esp_wifi_set_config()

}


// handles WiFi events
esp_err_t Wifi::eventHandler(void* ctx, system_event_t* event){
    const char* TAG = "event_handler";
    switch(event->event_id) {

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
            ESP_LOGI(TAG, "STA Connected");
            staState = event->event_id;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "STA Disconnected");
            staState = event->event_id;
            break;




        case SYSTEM_EVENT_AP_START:
            //ESP_ERROR_CHECK(tcpip_adapter_set_hostname(TCPIP_ADAPTER_IF_STA, "esp32"));
            ESP_LOGI(TAG, "Access Point Started");
            apState = event->event_id;
            break;
        case SYSTEM_EVENT_AP_STOP:
            ESP_LOGI(TAG, "Access Point Stopped");
            apState = event->event_id;
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "STA Connected, MAC=%02x:%02x:%02x:%02x:%02x:%02x AID=%i",
                event->event_info.sta_connected.mac[0], event->event_info.sta_connected.mac[1],
                event->event_info.sta_connected.mac[2], event->event_info.sta_connected.mac[3],
                event->event_info.sta_connected.mac[4], event->event_info.sta_connected.mac[5],
                event->event_info.sta_connected.aid
            );
            apState = event->event_id;
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "STA Disconnected, MAC=%02x:%02x:%02x:%02x:%02x:%02x AID=%i",
                event->event_info.sta_disconnected.mac[0], event->event_info.sta_disconnected.mac[1],
                event->event_info.sta_disconnected.mac[2], event->event_info.sta_disconnected.mac[3],
                event->event_info.sta_disconnected.mac[4], event->event_info.sta_disconnected.mac[5],
                event->event_info.sta_disconnected.aid
            );
            apState = event->event_id;

            if(apStaDisconnectFunc != NULL){
                ESP_LOGI(TAG, "Wifi::apStaDisconnectFunc is defined");
                (*apStaDisconnectFunc)();
            }
            break;
        case SYSTEM_EVENT_AP_PROBEREQRECVED:
            ESP_LOGI(TAG,"AP Probe Received");
            apState = event->event_id;
            break;
        // case SYSTEM_EVENT_STA_GOT_IP6:
        case SYSTEM_EVENT_AP_STA_GOT_IP6:
            ESP_LOGI(TAG, "Got IP6=%01x:%01x:%01x:%01x",
                event->event_info.got_ip6.ip6_info.ip.addr[0], event->event_info.got_ip6.ip6_info.ip.addr[1],
                event->event_info.got_ip6.ip6_info.ip.addr[2], event->event_info.got_ip6.ip6_info.ip.addr[3]
            );
            staState = event->event_id;
            break;
        default:
            ESP_LOGI(TAG, "Unregistered event=%i", event->event_id);
            break;
    }
    ESP_LOGI(TAG, "Event 0x%X; staState: 0x%X; apState: 0x%X", event->event_id, staState, apState);
    return ESP_OK;
}


// handles websocket events
void Wifi::websocketCallback(uint8_t num, WEBSOCKET_TYPE_t type, char* msg, uint64_t len){
    const static char* TAG = "websocket_callback";
    // int value;

    switch(type) {
        case WEBSOCKET_CONNECT:
            ESP_LOGI(TAG, "client %i connected!", num);
            break;
        case WEBSOCKET_DISCONNECT_EXTERNAL:
            ESP_LOGI(TAG, "client %i sent a disconnect message", num);
            if(wsDisconnectFunc != NULL){
                ESP_LOGI(TAG, "Wifi::wsDisconnectFunc is defined");
                (*wsDisconnectFunc)();
            }
            break;
        case WEBSOCKET_DISCONNECT_INTERNAL:
            ESP_LOGI(TAG, "client %i was disconnected", num);
            if(wsDisconnectFunc != NULL){
                ESP_LOGI(TAG, "Wifi::wsDisconnectFunc is defined");
                (*wsDisconnectFunc)();
            }
            break;
        case WEBSOCKET_DISCONNECT_ERROR:
            ESP_LOGI(TAG, "client %i was disconnected due to an error", num);
            break;
        case WEBSOCKET_TEXT:
            if(len){ // if the message length was greater than zero
                switch(msg[0]){
                    // case 'L':
                    //     if(sscanf(msg,"L%i",&value)) {
                    //         ESP_LOGI(TAG,"LED value: %i",value);
                    //         //   led_duty(value);
                    //         ws_server_send_text_all_from_callback(msg,len); // broadcast it!
                    //     }
                    //     break;
                    // case 'M':
                    //     ESP_LOGI(TAG, "got message length %i: %s", (int)len-1, &(msg[1]));
                    //     break;
                    default:
                        ESP_LOGI(TAG, "got an unknown message with length %i", (int)len);
                        break;
                }
            }
            break;
        case WEBSOCKET_BIN:
            ESP_LOGI(TAG, "client %i sent binary message of size %i:\n%s", num, (uint32_t)len, msg);
            if(recieveBinaryFunc != NULL){
                ESP_LOGI(TAG, "wifi_recieve_binary_func is defined");
                (*recieveBinaryFunc)((uint8_t *)msg, (uint32_t)len);
            } else{
                ESP_LOGI(TAG, "wifi_recieve_binary_func is NULL");
            }
            break;
        case WEBSOCKET_PING:
            ESP_LOGI(TAG, "client %i pinged us with message of size %i:\n%s", num, (uint32_t)len, msg);
            break;
        case WEBSOCKET_PONG:
            ESP_LOGI(TAG, "client %i responded to the ping", num);
            break;
    }
}


void Wifi::getResourceHandler(struct netconn *conn, const char *header, const uint8_t start[], const uint8_t end[]){
    const uint32_t len = end - start -1;
    // printf("getResourceHandler; %d, %d, %d \n", len, sizeof(header), strlen(header));
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


// serves any clients
void Wifi::httpServe(struct netconn *conn){
    const static char* TAG = "http_server";

    struct netbuf* inbuf;
    static char* buf;
    static uint16_t buflen;
    static err_t err;

    netconn_set_recvtimeout(conn, 1000); // allow a connection timeout of 1 second
    ESP_LOGI(TAG, "reading from client...");
    err = netconn_recv(conn, &inbuf);
    ESP_LOGI(TAG, "read from client");
    if(err == ERR_OK) {
        netbuf_data(inbuf, (void**)&buf, &buflen);
        if(buf){

#if CONFIG_AMIKODEV_WIFI_SD_CARD
            if(sdCard != NULL){
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
                    // printf("wifi : strHeaderUpgrade : %s, %d, %d \n", strHeaderUpgrade.c_str(), info.method == HttpRequest::METHOD_GET, compareWs);
                    if(info.method == HttpRequest::METHOD_GET && (compareWs == 0 || compareWs == 1)){
                        // is WebSocket
                        ESP_LOGI(TAG, " Requesting websocket on /");
                        printf("Request is WebSocket \n");
                        ws_server_add_client(conn, buf, buflen, "/", websocketCallback);
                        netbuf_delete(inbuf);
                    } else if(info.method == HttpRequest::METHOD_GET){
                        // method GET

                        std::string filePath = "/sdcard/frontend";
                        filePath.append(info.path, info.cleanPathLength);

                        // printf("filePath: %s \n", filePath.c_str());

                        FILE *f = fopen(filePath.c_str(), "rb");
                        if(f == NULL){
                            printf("ERROR: wrong open file \"%s\" \n", filePath.c_str());
                            netconn_close(conn);
                            netconn_delete(conn);
                            fclose(f);
                        } else{
                            printf("Success open file \"%s\" \n", filePath.c_str());
                            // printf("Read file: \n");
                            char *header = HttpResponce::getHeaderByFileExtension(info.fileExt);
                            if(header != NULL){
                                netconn_write(conn, header, strlen(header), NETCONN_NOCOPY);
                            }
                            char fbuf[1024];
                            while(fgets(fbuf, 1024, f)){
                                // printf("%s", fbuf);
                                netconn_write(conn, fbuf, strlen(fbuf), NETCONN_NOCOPY);
                            }
                            // printf("\n");
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
                    printf("Request not parsed \n");

                }


            } else{
#endif                
                // карта памяти не подключена,
                // выполняется поведение по умолчанию

                // default page
                if(strstr(buf, "GET / ")
                        && !strstr(buf, "Upgrade: websocket")) {
                    ESP_LOGI(TAG, "Sending /");
                    handler_index_html(conn, HTML_HEADER);
                    netbuf_delete(inbuf);
                }

                // default page websocket
                else if(strstr(buf, "GET / ")
                        && strstr(buf, "Upgrade: websocket")) {
                    ESP_LOGI(TAG," Requesting websocket on /");
                    ws_server_add_client(conn, buf, buflen, "/", websocketCallback);
                    netbuf_delete(inbuf);
                }

                else if(strstr(buf, "GET /style.css ")) {
                    ESP_LOGI(TAG, "Sending /style.css");
                    handler_style_css(conn, CSS_HEADER);
                    netbuf_delete(inbuf);
                }

                else if(strstr(buf, "GET /main.js ")) {
                    ESP_LOGI(TAG, "Sending /main.js");
                    handler_main_js(conn, JS_HEADER);
                    netbuf_delete(inbuf);
                }

                else if(strstr(buf,"GET /")) {
                    bool retReq = false;
                    if(httpServeReqFunc != NULL){
                        retReq = httpServeReqFunc(conn, buf, (uint32_t)buflen);
                    }
                    if(retReq){
                        netbuf_delete(inbuf);
                    } else{
                        ESP_LOGI(TAG,"Unknown request, sending error page: %s",buf);
                        handler_error_html(conn, ERROR_HEADER);
                        netbuf_delete(inbuf);
                    }
                }

                else {
                    ESP_LOGI(TAG,"Unknown request");
                    netconn_close(conn);
                    netconn_delete(conn);
                    netbuf_delete(inbuf);
                }
                
#if CONFIG_AMIKODEV_WIFI_SD_CARD
            }
#endif

        }
        else {
            ESP_LOGI(TAG,"Unknown request (empty?...)");
            netconn_close(conn);
            netconn_delete(conn);
            netbuf_delete(inbuf);
        }
    }
    else { // if err==ERR_OK
        ESP_LOGI(TAG,"error on read, closing connection");
        netconn_close(conn);
        netconn_delete(conn);
        netbuf_delete(inbuf);
    }
}

// void Wifi::httpServe2(struct netconn *conn){

// }


// handles clients when they first connect. passes to a queue
void Wifi::serverTask(void* pvParameters){
    const static char* TAG = "server_task";
    struct netconn *conn, *newconn;
    static err_t err;
    clientQueue = xQueueCreate(clientQueueSize, sizeof(struct netconn*));

    conn = netconn_new(NETCONN_TCP);
    netconn_bind(conn, NULL, 80);
    netconn_listen(conn);
    ESP_LOGI(TAG, "server listening");
    do{
        err = netconn_accept(conn, &newconn);
        ESP_LOGI(TAG, "new client");
        if(err == ERR_OK){
            xQueueSendToBack(clientQueue, &newconn, portMAX_DELAY);
            // httpServe(newconn);  // перезагружает устройство :((
        }
    } while(err == ERR_OK);
    netconn_close(conn);
    netconn_delete(conn);
    ESP_LOGE(TAG, "task ending, rebooting board");
    esp_restart();
}

// receives clients from queue, handles them
void Wifi::serverHandleTask(void* pvParameters){
    const static char* TAG = "server_handle_task";
    struct netconn* conn;
    ESP_LOGI(TAG, "task starting");
    for(;;){
        xQueueReceive(clientQueue, &conn, portMAX_DELAY);
        if(!conn) continue;
        httpServe(conn);
    }
    vTaskDelete(NULL);
}

void Wifi::countTask(void* pvParameters){
    const static char* TAG = "count_task";
    char out[20];
    int len;
    int clients;
    const static char* word = "%i";
    uint8_t n = 0;
    const int DELAY = 1000 / portTICK_PERIOD_MS; // 1 second

    ESP_LOGI(TAG, "starting task");
    for(;;){
        len = sprintf(out, word, n);
        clients = ws_server_send_text_all(out, len);
        if(clients > 0){
            //ESP_LOGI(TAG,"sent: \"%s\" to %i clients",out,clients);
        }
        n++;
        vTaskDelay(DELAY);
        // lightTurn();
    }
}


void Wifi::recieveBinary(void (*func)(uint8_t *data, uint32_t length)){
    Wifi::recieveBinaryFunc = func;
}

void Wifi::apStaDisconnect(void (*func)()){
    Wifi::apStaDisconnectFunc = func;
}

void Wifi::wsDisconnect(void (*func)()){
    Wifi::wsDisconnectFunc = func;
}

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
void Wifi::setSdCard(SdCard *card){
    Wifi::sdCard = card;
}
#endif

