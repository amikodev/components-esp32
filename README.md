Автор: Приходько Дмитрий
[asketcnc@yandex.ru](mailto:asketcnc@yandex.ru)

# Компоненты ESP32
* [httprequest](#httprequest)
* [r8ef](#r8ef)
* [relay](#relay)
* [sdcard-storage](#sdcard-storage)
* [spiffs-storage](#spiffs-storage)
* [nvs-storage](#nvs-storage)
* [shiftload](#shiftload)
* [wifi](#wifi)

Для подключения компонента необходимо в корне проекта создать папку components и символическую ссылку на папку компонента, например:
`ln -s ../../components/cpp/httprequest`

## httprequest
Разбор и генерация HTTP-заголовков в классах **httprequest** и **httpresponce** соответственно.

Пример получения заголовка "`Upgrade`":

```cpp
HttpRequest request;
HttpRequest::HttpRequestInfo info;
bool requestParsed = request.parse(buf, buflen, &info);
if(requestParsed){
    request.printInfo(&info);       // вывод информации в консоль
    std::string strHeaderUpgrade = request.getHeader("Upgrade");
}
```

## r8ef
8-канальный приемник R8EF .

```cpp
R8EF::setPwmPins(GPIO_NUM_21, GPIO_NUM_19, GPIO_NUM_18, GPIO_NUM_5, GPIO_NUM_17, GPIO_NUM_16, GPIO_NUM_4, GPIO_NUM_2);
R8EF::startPwmTask();

R8EF::funcChannel(1, RMT1::r8ef_ch1);
R8EF::funcChannel(2, RMT1::r8ef_ch2);
R8EF::funcChannel(3, RMT1::r8ef_ch3);
R8EF::funcChannel(4, RMT1::r8ef_ch4);
R8EF::funcChannel(5, RMT1::r8ef_ch5);
R8EF::funcChannel(6, RMT1::r8ef_ch6);
R8EF::funcChannel(7, RMT1::r8ef_ch7);
R8EF::funcChannel(8, RMT1::r8ef_ch8);
```

```cpp
void RMT1::r8ef_ch1(uint8_t value){
   printf("channel %d: %d \n", 1, value);
   // j2_x
}
```

## relay
Работа с реле.

```cpp
Relay *relay = new Relay(GPIO_NUM_15, GPIO_NUM_2, GPIO_NUM_4, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23);
relay->writeByNum(0, true);
```

## sdcard-storage
Работа с SD-картой.

```cpp
SdCardStorage *card = new SdCardStorage();
if(card->initSpi(GPIO_NUM_19, GPIO_NUM_23, GPIO_NUM_18, GPIO_NUM_5)){
    // sdmmc_card_t *cardInfo = card->getCardInfo();
    std::string filePath = "/sdcard/frontend";
    filePath.append("/index.html", 11);

    FILE *f = fopen(filePath.c_str(), "rb");
    if(f == NULL){
        printf("ERROR: wrong open file \"%s\" \n", filePath.c_str());
        // ...
    } else{
        printf("Success open file \"%s\" \n", filePath.c_str());
        char fbuf[1024];
        // fgets - получение текстовых данных до первого байта со значением 0x00
        while(fgets(fbuf, 1024, f)){
            // printf("%s", fbuf);
            netconn_write(conn, fbuf, strlen(fbuf), NETCONN_NOCOPY);
        }
    }
    card->unmount();
}
```

## spiffs-storage
Работа с хранилищем во flash-памяти.

```cpp
SpiffsStorage *spiffs = new SpiffsStorage();
if(spiffs->init((char *)"/spiffs")){
    std::string filePath = "/spiffs/web";
    filePath.append("/index.html", 11);

    FILE *f = fopen(filePath.c_str(), "rb");
    if(f == NULL){
        printf("ERROR: wrong open file \"%s\" \n", filePath.c_str());
        // ...
    } else{
        printf("Success open file \"%s\" \n", filePath.c_str());

        fseek(f, 0, SEEK_END);
        long lSize = ftell(f);
        rewind(f);
        printf("File size: %ld \n", lSize);

        char fbuf[1024];
        while(true){
            size_t s = fread(fbuf, 1, 1024, f);
            if(s == 0) break;
            netconn_write(conn, fbuf, s, NETCONN_NOCOPY);
        }
    }
}
```

## nvs-storage
Работа с энергонезависимой памятью.

Запись:
```cpp
NvsStorage::init((char *) "cpp_esp32");

NvsStorage::open();
NvsStorage::setValue((char *) "wifi_mode", (char *) "STA");
NvsStorage::setValue((char *) "wifi_ssid", (char *) "QWERTY");
NvsStorage::setValue((char *) "wifi_password", (char *) "12345678");
NvsStorage::commit();
NvsStorage::close();
```

Чтение:
```cpp
NvsStorage::init((char *) "cpp_esp32");

if(NvsStorage::isInited()){
    char *wifiMode = NULL;
    char *wifiSsid = NULL;
    char *wifiPassword = NULL;

    NvsStorage::open();
    try{
        wifiMode = NvsStorage::getStr((char *) "wifi_mode");
        wifiSsid = NvsStorage::getStr((char *) "wifi_ssid");
        wifiPassword = NvsStorage::getStr((char *) "wifi_password");
    } catch(const std::runtime_error &error){}
    NvsStorage::close();
}
```

## shiftload
Работа с выводным сдвиговым регистром 74HC595N.

```cpp
ShiftLoad sl;
// работа по SPI
sl.initSpi(GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15);
sl.registerCount(1);
sl.write(0, 0xBB);
sl.writeByNum(0, 2, true);

...

ShiftLoad sl;
// работа по задаче
sl.initTask(GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_15);
sl.registerCount(1);
sl.write(0, 0x31);
```

## wifi
Работа с WIFI.

```cpp
Wifi wifi;
wifi.setup();

// при использовании SD-карты:
SdCardStorage *card = new SdCardStorage();
if(card->initSpi(GPIO_NUM_19, GPIO_NUM_23, GPIO_NUM_18, GPIO_NUM_5)){
    // sdmmc_card_t *cardInfo = card->getCardInfo();
    Wifi::setSdCard(card);
}

// при использовании spiffs
SpiffsStorage *spiffs = new SpiffsStorage();
if(spiffs->init((char *)"/spiffs")){
    Wifi::setSpiffs(spiffs);
}

ws_server_start();
xTaskCreate(&Wifi::serverTask, "server_task", 3000, NULL, 9, NULL);
xTaskCreate(&Wifi::serverHandleTask, "server_handle_task", 4000, NULL, 6, NULL);
```

# Внешние фреймворки и библиотеки
* [esp-idf](https://github.com/espressif/esp-idf)
* [esp32-websocket](https://github.com/Molorius/esp32-websocket)
* [esp32_magicsee_r1](https://github.com/LuxInTenebr1s/esp32_magicsee_r1)
