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

#include "httpresponce.hpp"
// #include "httprequest.hpp"


const char* HttpResponce::HTML_HEADER = "HTTP/1.1 200 OK\nContent-type: text/html\n\n";
const char* HttpResponce::ERROR_HEADER = "HTTP/1.1 404 Not Found\nContent-type: text/html\n\n";
const char* HttpResponce::JS_HEADER = "HTTP/1.1 200 OK\nContent-type: text/javascript\n\n";
const char* HttpResponce::CSS_HEADER = "HTTP/1.1 200 OK\nContent-type: text/css\n\n";

/**
 * Получение заголовка для вывода файла
 * @param ext расширение файла
 */
char* HttpResponce::getHeaderByFileExtension(HttpRequest::HttpRequestFileExtension ext){
    char *header = NULL;
    switch(ext){
        case HttpRequest::EXT_PNG:
        case HttpRequest::EXT_JPG:
        case HttpRequest::EXT_BMP:
        case HttpRequest::EXT_ICO:
            break;
        case HttpRequest::EXT_HTML:
            header = (char *)HTML_HEADER;
            break;
        case HttpRequest::EXT_JS:
            header = (char *)JS_HEADER;
            break;
        case HttpRequest::EXT_CSS:
            header = (char *)CSS_HEADER;
            break;
        case HttpRequest::EXT_CPP:

        default:
            break;
    }
    return header;
}

