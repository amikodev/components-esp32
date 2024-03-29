/*
amikodev/components-esp32 - library components on esp-idf
Copyright © 2020-2022 Prihodko Dmitriy - asketcnc@yandex.ru
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

#ifndef __WIFI_H__
#define __WIFI_H__

#include <stdio.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_netif.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "string.h"

#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
#include "websocket_server.h"
#endif

#include "httprequest.hpp"
#include "httpresponce.hpp"

#if CONFIG_AMIKODEV_WIFI_SD_CARD
#include "sdcard-storage.hpp"
#endif

#if CONFIG_AMIKODEV_WIFI_SPIFFS
#include "spiffs-storage.hpp"
#endif

#if CONFIG_AMIKODEV_WIFI_NVS_FLASH
#include "nvs-storage.hpp"
#endif


/**
 * WiFi
 * @author Приходько Д. Г.
 */
class Wifi{

public:

    struct WsData{
        uint64_t size;
        void *ptr;
    };

    enum STORAGE_TYPE{
        STORAGE_NONE = 0,
        STORAGE_EMBED,
        STORAGE_SDCARD,
        STORAGE_SPIFFS
    };

private:
    static QueueHandle_t clientQueue;
    const static int clientQueueSize = 10;

    static system_event_id_t staState;
    static system_event_id_t apState;

    static char *hostname;

    static bool connected;

#if CONFIG_AMIKODEV_WIFI_SD_CARD
    static SdCardStorage *sdCard;
#endif

#if CONFIG_AMIKODEV_WIFI_SPIFFS
    static SpiffsStorage *spiffs;
#endif

#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
    static xQueueHandle wsSendCustomEvtQueue;
#endif

    static const uint8_t WS_OBJ_NAME_WIFI;

    static const uint8_t WS_WIFI_SETTINGS;
    static const uint8_t WS_WIFI_SCAN;

    static const uint8_t WS_CMD_READ;
    static const uint8_t WS_CMD_WRITE;

    static const uint8_t WS_WIFI_MODE_STA;
    static const uint8_t WS_WIFI_MODE_AP;


public:

    static void (*recieveBinaryFunc)(uint8_t *data, uint32_t length);

    static void (*apStaConnectFunc)();
    static void (*apStaDisconnectFunc)();

    static bool (*httpServeReqFunc)(struct netconn *conn, char *buf, uint32_t length);

    static void (*scanFunc)();      // for next release

    /**
     * 
     */
    void setup();

    /**
     * Подключение к точке доступа из параметров компиляции (menuconfig)
     */
    void connectSystemSta();

    /**
     * Создание точки доступа по параметрам компиляции (menuconfig)
     */
    void connectSystemAp();

    /**
     * Подключение к точке доступа или создание новой из полльзовательских параметров
     */
    void connectUserSettingsApSta();


    static esp_err_t systemStaEventHandler(void* ctx, system_event_t* event);
    static esp_err_t systemApEventHandler(void* ctx, system_event_t* event);
    static esp_err_t userSettingsApStaEventHandler(void* ctx, system_event_t* event);


#if CONFIG_AMIKODEV_WIFI_INCLUDE_EMBED_WEB_FILES
    static void getResourceHandler(struct netconn *conn, const char *header, const uint8_t start[], const uint8_t end[]);
    static void handler_index_html(struct netconn *conn, const char *header);
    static void handler_style_css(struct netconn *conn, const char *header);
    static void handler_main_js(struct netconn *conn, const char *header);
    static void handler_error_html(struct netconn *conn, const char *header);
#endif

    static void httpServe(struct netconn *conn);
    
    static void serverTask(void* pvParameters);
    
    static void serverHandleTask(void* pvParameters);
    
    void recieveBinary(void (*func)(uint8_t *data, uint32_t length));

    void apStaConnect(void (*func)());
    void apStaDisconnect(void (*func)());

#if CONFIG_AMIKODEV_WIFI_USE_WEBSOCKET
    static void (*wsDisconnectFunc)();
    static void websocketCallback(uint8_t num, WEBSOCKET_TYPE_t type, char* msg, uint64_t len);
    void wsDisconnect(void (*func)());
    static void countTask(void* pvParameters);

    /**
     * Обработка входящего сообщения
     * @return bool флаг системного сообщения
     */
    static bool websocketRecieveBinary(uint8_t *data, uint32_t length);

    /**
     * 
     */
    static void wsSendCustomTask(void *arg);

#endif

    void httpServeReq(bool (*func)(struct netconn *conn, char *buf, uint32_t length));

    /**
     * Установка hostname
     * @param name hostname
     */
    void setHostname(char *name);

    /**
     * Получение hostname
     */
    char* getHostname();

    /**
     * Сканирование сетей
     * ( for next release )
     */
    static void scanNetworks(void (*func)());
    static void scanNetworksList(system_event_t* event);
    static void sendScanListToWs();

#if CONFIG_AMIKODEV_WIFI_SD_CARD
    /**
     * Установка карты памяти SdCard
     * @param card карта памяти
     */
    static void setSdCard(SdCardStorage *card);
#endif

#if CONFIG_AMIKODEV_WIFI_SPIFFS
    /**
     * Установка хранилища spiffs
     * @param storage хранилище
     */
    static void setSpiffs(SpiffsStorage *storage);
#endif


};




#endif      // __WIFI_H__

