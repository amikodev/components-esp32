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

#include "nvs-storage.hpp"

#define TAG "NvsStorage"

char* NvsStorage::_name = (char *) "Amikodev";
nvs_handle_t NvsStorage::nvsHandle = NULL;
bool NvsStorage::inited = false;


/**
 * Инициализация хранилища
 * @param name Имя пространства имён
 */
void NvsStorage::init(char *name){
    _name = name;
    ESP_LOGI(TAG, "Init with name \"%s\"", _name);

    esp_err_t ret;
    ret = nvs_flash_init();
    if(ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
    inited = true;
}

/**
 * Получение флага инициализации
 */
bool NvsStorage::isInited(){
    return inited;
}

/**
 * Открытие хранилища
 */
bool NvsStorage::open(){
    esp_err_t ret;
    ret = nvs_open(_name, NVS_READWRITE, &nvsHandle);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(ret));
    }
    return ret == ESP_OK;
}

/**
 * Закрытие хранилища
 */
void NvsStorage::close(){
    if(nvsHandle != NULL){
        nvs_close(nvsHandle);
        nvsHandle = NULL;
    }
}

/**
 * Фиксация изменений
 */
void NvsStorage::commit(){
    esp_err_t ret;
    if(nvsHandle != NULL){
        ret = nvs_commit(nvsHandle);
    }
}

/**
 * Получение значения ключа типа float
 * @param key ключ
 */
float NvsStorage::getFloat(char *key){
    esp_err_t ret;

    uint32_t uiVal;
    float fVal;

    ret = nvs_get_u32(nvsHandle, key, &uiVal);
    if(ret == ESP_OK){
        memcpy(&fVal, &uiVal, 4);
        ESP_LOGI(TAG, "Read %s: %f", key, fVal);
    } else{
        throw std::runtime_error("Error reading NVS");
    }

    return fVal;
}

/**
 * Получение значения ключа типа string
 * @param key ключ
 */
char* NvsStorage::getStr(char *key){
    esp_err_t ret;

    size_t required_size;
    ret = nvs_get_str(nvsHandle, key, NULL, &required_size);
    if(ret != ESP_OK) throw std::runtime_error("Error reading NVS");
    char *value = (char *)malloc(required_size);
    ret = nvs_get_str(nvsHandle, key, value, &required_size);
    if(ret != ESP_OK) throw std::runtime_error("Error reading NVS");

    ESP_LOGI(TAG, "Read %s: %s", key, value);

    return value;
}


/**
 * Сохранение значения ключа типа float
 * @param key ключ
 * @param val значение
 */
void NvsStorage::setValue(char *key, float val){
    esp_err_t ret;

    uint32_t uiVal;
    memcpy(&uiVal, &val, 4);
    ret = nvs_set_u32(nvsHandle, key, uiVal);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Wrong save %s", key);        
    }
}

/**
 * Сохранение значения ключа типа string
 * @param key ключ
 * @param val значение
 */
void NvsStorage::setValue(char *key, const char *val){
    esp_err_t ret;

    ret = nvs_set_str(nvsHandle, key, val);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Wrong save %s", key);        
    }
}


