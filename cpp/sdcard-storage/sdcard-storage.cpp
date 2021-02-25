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

#include "sdcard-storage.hpp"

#define TAG "SdCard"

/**
 * Инициализация SPI
 * @param miso пин MISO
 * @param mosi пин MOSI
 * @param sclk пин SCLK
 * @param cs пин CS
 * @return bool успешность определения доступа к карте памяти
 */
bool SdCardStorage::initSpi(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs){

    ESP_LOGI(TAG, "initSpi");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = miso;
    slot_config.gpio_mosi = mosi;
    slot_config.gpio_sck  = sclk;
    slot_config.gpio_cs   = cs;
    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc_mount is an all-in-one convenience function.
    // Please check its source code and implement error recovery when developing
    // production applications.
    // sdmmc_card_t* card;
    // esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mPoint, &host, &slot_config, &mount_config, &card);

    if(ret != ESP_OK){
        if(ret == ESP_FAIL){
            ESP_LOGW(TAG, "SdCard ERROR: Failed to mount filesystem.");
        } else{
            ESP_LOGW(TAG, "SdCard ERROR: Failed to initialize the card (%s).\nMake sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return false;
    }

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    return true;

}

/**
 * Установка точки монтирования
 * @param mountPoint точка мантирования
 */
void SdCardStorage::setMountPoint(char* mountPoint){
    mPoint = mountPoint;
}

/**
 * Размонтирование файловой системы
 */
void SdCardStorage::unmount(){
    esp_vfs_fat_sdmmc_unmount();
    ESP_LOGI(TAG, "card unmounted");
}

/**
 * Получение информации о карте памяти
 * @return sdmmc_card_t* SD/MMC card information structure
 */
sdmmc_card_t* SdCardStorage::getCardInfo(){
    return card;
}

