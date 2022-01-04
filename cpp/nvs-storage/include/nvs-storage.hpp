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

#ifndef __NVS_STORAGE_HPP__
#define __NVS_STORAGE_HPP__

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cmath>
#include <stdexcept>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

class NvsStorage{

private:

    static char *_name;
    static nvs_handle_t nvsHandle;
    static bool inited;

public:

    /**
     * Инициализация хранилища
     * @param name Имя пространства имён
     */
    static void init(char *name);

    /**
     * Получение флага инициализации
     */
    static bool isInited();

    /**
     * Открытие хранилища
     */
    static bool open();

    /**
     * Закрытие хранилища
     */
    static void close();

    /**
     * Фиксация изменений
     */
    static void commit();

    /**
     * Получение значения ключа типа float
     * @param key ключ
     */
    static float getFloat(char *key);

    /**
     * Получение значения ключа типа string
     * @param key ключ
     */
    static char* getStr(char *key);

    /**
     * Сохранение значения ключа типа float
     * @param key ключ
     * @param val значение
     */
    static void setValue(char *key, float val);

    /**
     * Сохранение значения ключа типа string
     * @param key ключ
     * @param val значение
     */
    static void setValue(char *key, const char *val);

};

#endif      // __NVS_STORAGE_HPP__
