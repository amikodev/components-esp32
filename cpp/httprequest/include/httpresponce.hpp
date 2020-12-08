/*
amikodev/components-esp32 - library components on esp-idf
Copyright © 2020 Prihodko Dmitriy - prihdmitriy@yandex.ru
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

#ifndef __HTTP_RESPONCE_HPP__
#define __HTTP_RESPONCE_HPP__

#include "httprequest.hpp"

/**
 * Формирование HTTP-ответа
 */
class HttpResponce{

private:

public:

    const static char *HTML_HEADER;
    const static char *ERROR_HEADER;
    const static char *JS_HEADER;
    const static char *CSS_HEADER;

    /**
     * Получение заголовка для вывода файла
     * @param ext расширение файла
     */
    static char* getHeaderByFileExtension(HttpRequest::HttpRequestFileExtension ext);

private:

};

#endif      // __HTTP_RESPONCE_HPP__
