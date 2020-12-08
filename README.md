Автор: Приходько Дмитрий

# Компоненты ESP32

Для подключения компонента необходимо в корне проекта создать папку components и символическую ссылку на папку компонента, например:
ln -s ../../components/cpp/httprequest

## httprequest
Разбор и генерация HTTP-заголовков в классах **httprequest** и **httpresponce** соответственно.

Пример получения заголовка "`Upgrade`":

`HttpRequest request;`
`HttpRequest::HttpRequestInfo info;`
`bool requestParsed = request.parse(buf, buflen, &info);`
`if(requestParsed){`
`    request.printInfo(&info);`
`    std::string strHeaderUpgrade = request.getHeader("Upgrade");`
`}`

