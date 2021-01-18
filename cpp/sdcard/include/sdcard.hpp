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

/**
 * 
 * 
 * Примеры:
 * 1. открытие файла:
 *      FILE* f = fopen("/sdcard/hello.txt", "w");
 */

// Включить длинные имена файлов
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html#config-fatfs-long-filenames
// Component config > FAT Filesystem support
// Long filename buffer in heap (FATFS_LFN_HEAP)

#ifndef __SDCARD_HPP__
#define __SDCARD_HPP__

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "driver/sdmmc_host.h"
#include "driver/sdmmc_types.h"


class SdCard{

private:

    sdmmc_card_t *card;     // информация о карте памяти
    char *mPoint = (char *)"/sdcard";   // точка монтирования ( FILE *f = fopen("/sdcard/hello.txt", "w"); )

public:

    /**
     * Инициализация SPI
     * @param miso пин MISO
     * @param mosi пин MOSI
     * @param sclk пин SCLK
     * @param cs пин CS
     * @return bool успешность определения доступа к карте памяти
     */
    bool initSpi(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs);

    /**
     * Установка точки монтирования
     * @param mountPoint точка мантирования
     */
    void setMountPoint(char* mountPoint);

    /**
     * Размонтирование файловой системы
     */
    void unmount();

    /**
     * Получение информации о карте памяти
     * @return sdmmc_card_t* SD/MMC card information structure
     */
    sdmmc_card_t* getCardInfo();

private:

};

#endif          // __SDCARD_HPP__
