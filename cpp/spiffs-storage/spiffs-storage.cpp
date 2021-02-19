/*
amikodev/components-esp32 - library components on esp-idf
Copyright © 2020-2021 Prihodko Dmitriy - asketcnc@yandex.ru
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

#include "spiffs-storage.hpp"

#define TAG "SpiffsStorage"

/**
 * Инициализация
 * @param base базовый путь
 */
bool SpiffsStorage::init(char *base){
    basePath = base;

    esp_vfs_spiffs_conf_t conf = {
      .base_path = basePath,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if(ret != ESP_OK){
        if(ret == ESP_FAIL){
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if(ret == ESP_ERR_NOT_FOUND){
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else{
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return false;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return true;
}

/**
 * Получение базового пути
 */
const char* SpiffsStorage::getBasePath(){
    return basePath;
}



