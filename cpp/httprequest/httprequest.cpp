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

#include "httprequest.hpp"

/**
 * Парсинг http-запроса
 * @param buf входящий запрос
 * @param buflen длина запроса
 * @param info распарсенная информация
 * @return успешность парсинга
 */
bool HttpRequest::parse(char* buf, uint16_t buflen, HttpRequest::HttpRequestInfo *info){
    bool ret = false;

    // разбор строки "GET /index.html HTTP/1.1"
    char *ch1 = strchr(buf, ' ');
    char *ch2 = NULL; //strrchr(str, ' ');
    if(ch1 != NULL){
        ch2 = strchr(ch1+1, ' ');
    }
    if(ch1 != NULL && ch2 != NULL){
        HttpRequestMethod method = METHOD_NONE;
        if(memcmp(buf, "GET", 3) == 0){
            method = METHOD_GET;
        } else if(memcmp(buf, "POST", 4) == 0){
            method = METHOD_POST;
        } else if(memcmp(buf, "OPTIONS", 7) == 0){
            method = METHOD_OPTIONS;
        } else{
            // printf("Req method is unknown \n");
        }

        if(method != METHOD_NONE){

            info->method = method;

            size_t pathLen = ch2-ch1-1;

            char *strPath;
            if(pathLen == 1 && defaultPath != NULL){   // путь не указан, path=/
                pathLen = strlen(defaultPath);
                strPath = (char *)malloc(pathLen+1);
                memcpy(strPath, defaultPath, pathLen);
            } else{
                strPath = (char *)malloc(pathLen+1);
                memcpy(strPath, ch1+1, pathLen);
            }

            // *(strPath+ch2-ch1) = 0;
            strPath[pathLen] = '\0';
            info->path = strPath;
            // printf("Request path: --%s-- \n", strPath);

            // получение чистого пути к файлу
            char *chSP = strchr(strPath, '?');
            if(chSP == NULL){
                info->cleanPathLength = strlen(strPath);
            } else{
                // убираем из пути параметры ?a=1&b=2&c=q.w
                info->cleanPathLength = chSP-strPath;
            }

            // получение расширения файла
            HttpRequestFileExtension ext = EXT_UNKNOWN;
            char *chExt = strchr(strPath, '.');
            uint8_t extLen = 0;     // длина расширения
            if(chExt != NULL){
                if(chSP != NULL){
                    if(chExt < chSP){
                        // ...
                        extLen = chSP-chExt-1;
                    } else{
                        ext = EXT_EMPTY;
                    }
                } else{
                    // ...
                    // extLen = ch2-ch1-*chExt;
                    // extLen = (ch2-ch1)-(chExt-ch1);
                    // extLen = ch2-chExt-1;
                    extLen = pathLen - (chExt-strPath) - 1;
                    // extLen -= chExt;
                }
            }

            if(extLen > 0){
                if(memcmp(chExt+1, "png", extLen) == 0) ext = EXT_PNG;
                else if(memcmp(chExt+1, "jpg", extLen) == 0) ext = EXT_JPG;
                else if(memcmp(chExt+1, "bmp", extLen) == 0) ext = EXT_BMP;
                else if(memcmp(chExt+1, "ico", extLen) == 0) ext = EXT_ICO;
                else if(memcmp(chExt+1, "html", extLen) == 0) ext = EXT_HTML;
                else if(memcmp(chExt+1, "js", extLen) == 0) ext = EXT_JS;
                else if(memcmp(chExt+1, "css", extLen) == 0) ext = EXT_CSS;
                else if(memcmp(chExt+1, "cpp", extLen) == 0) ext = EXT_CPP;
            }

            // printf("Request file extension: %d, length: %d \n", ext, extLen);
            info->fileExt = ext;

            if(ext != EXT_UNKNOWN){
                if(ext == EXT_CPP){
                    // расширение .cpp
                    // выполняем скрипт
                    
                } else{
                    // получаем файл из файловой системы и отдаём клиенту
                    // if(cardSpiInited){
                        // FILE *f = fopen("/sdcard");


    // f = fopen("/sdcard/foo.txt", "r");
    // if (f == NULL) {
    //     ESP_LOGE(TAG, "Failed to open file for reading");
    //     return;
    // }
    // char line[64];
    // fgets(line, sizeof(line), f);
    // fclose(f);


                    // }

                }
            }

            // free(strPath);


            // разбор заголовков
            // char *ch1 = strchr(buf, '\n');
            // std::map <std::string*, std::string*> headers;
            char *chH = buf;
            char *chH2 = NULL;
            chH2 = strchr(chH, '\n');
            if(chH2 != NULL){
                chH = chH2+1;
            }

            headers.clear();
            // uint8_t count = 0;
            while(true){
                chH2 = strchr(chH, '\n');
                if(chH2 != NULL){
                    if(chH2-chH > 1){
                        char *chSH = strchr(chH, ':');
                        std::string headerName(chH, chSH-chH);
                        std::string headerValue(chSH+2, chH2-chSH-2);
                        headers.insert(std::pair<std::string, std::string>(headerName, headerValue));
                    } else{
                        // пустая строка, заголовки закончились
                        break;
                    }
                } else{
                    break;
                }
                chH = chH2+1;
            }

            ret = true;
            info->parsed = true;
        }

    } else{
        // printf("Request not parsed \n");
    }

    return ret;
}

/**
 * Показать информацию о запросе
 * @param info распарсенная информация
 */
void HttpRequest::printInfo(HttpRequest::HttpRequestInfo *info){
    if(!info->parsed){
        printf("HttpRequest : request not parsed \n");
        return;
    }

    printf("HttpRequest : request info: \n");
    printf(
        "\tmethod: %d;\n\tfile extension: %d;\n\tpath: %s;\n\tclean path: %.*s;\n", 
        info->method, info->fileExt, info->path, info->cleanPathLength, info->path
    );

    printf("HttpRequest : Headers: \n");
    // uint8_t i=0;
    // printf("headers map size: %d\n", headers.size());
    for(std::map<std::string, std::string>::iterator it=headers.begin(); it != headers.end(); ++it){
        // printf("\t%s=%s;\n", it->first->c_str(), it->second->c_str());
        std::string headerName = (std::string)it->first;
        std::string headerValue = (std::string)it->second;
        printf("\t%s = %s\n", headerName.c_str(), headerValue.c_str());
        // printf("%d\n", i++);
        // std::cout << &it->first << " = " << &it->second << '\n';

    }

}

/**
 * Установка пути к файлу по умолчанию
 * @param path путь к файлу
 */
void HttpRequest::setDefaultPath(char *path){
    defaultPath = path;
}

/**
 * Получение значения заголовка
 * @param name имя заголовка
 */
std::string HttpRequest::getHeader(std::string name){
    std::string ret = "(NULL)";
    std::map<std::string, std::string>::iterator it = headers.find(name);
    if(it != headers.end()){
        std::string headerValue = (std::string)it->second;
        ret = headerValue;
    }
    return ret;
}


std::string HttpRequest::trim(const std::string &str){
    static std::regex regular("^\\s+(.*?)\\s+$");
    static std::smatch result;
 
    if (std::regex_match(str, result, regular))
        return result[1];
    return str;
}


