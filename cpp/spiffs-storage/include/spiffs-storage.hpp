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

#ifndef __SPIFFS_HPP__
#define __SPIFFS_HPP__

#include <iostream>
#include <cstring>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_spiffs.h"

/**
 * SPIFFS
 */
class SpiffsStorage{
public:

private:

    char *basePath;     // базовый путь

public:

    /**
     * Инициализация
     * @param base базовый путь
     */
    bool init(char *base);

    /**
     * Получение базового пути
     */
    const char* getBasePath();

};

#endif      // __SPIFFS_HPP__
