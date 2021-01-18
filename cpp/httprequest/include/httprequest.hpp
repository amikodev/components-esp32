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

#ifndef __HTTP_REQUEST_HPP__
#define __HTTP_REQUEST_HPP__

#include <stdio.h>
#include <string.h>
#include <map>
#include <utility>
#include <iostream>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include <unistd.h>
#include <algorithm>
#include <regex>


/**
 * Разбор HTTP-запросов
 */
class HttpRequest{

private:

    char *defaultPath = (char *)"/index.html";
    std::map <std::string, std::string> headers;      // заголовки
    

public:

    enum HttpRequestMethod{
        METHOD_NONE = 0,
        METHOD_GET,
        METHOD_POST,
        METHOD_OPTIONS
    };
    enum HttpRequestFileExtension{
        EXT_UNKNOWN = 0,
        EXT_EMPTY,
        EXT_PNG,
        EXT_JPG,
        EXT_BMP,
        EXT_ICO,
        EXT_HTML,
        EXT_JS,
        EXT_CSS,

        EXT_CPP
    };
    struct HttpRequestInfo{
        bool parsed = false;
        HttpRequestMethod method = METHOD_NONE;
        HttpRequestFileExtension fileExt = EXT_UNKNOWN;
        char *path = NULL;
        uint16_t cleanPathLength = 0;
    };

    /**
     * Парсинг http-запроса
     * @param buf входящий запрос
     * @param buflen длина запроса
     * @param info распарсенная информация
     * @return успешность парсинга
     */
    bool parse(char* buf, uint16_t buflen, HttpRequestInfo *info);

    /**
     * Показать информацию о запросе
     * @param info распарсенная информация
     */
    void printInfo(HttpRequestInfo *info);

    /**
     * Установка пути к файлу по умолчанию
     * @param path путь к файлу
     */
    void setDefaultPath(char *path);

    /**
     * Получение значения заголовка
     * @param name имя заголовка
     */
    std::string getHeader(std::string name);

    /**
     * Утилита
     * Обрезание строки
     */
    static std::string trim(const std::string &str);

private:

    // HttpRequestInfo info;

};

#endif      // __HTTP_REQUEST_HPP__
