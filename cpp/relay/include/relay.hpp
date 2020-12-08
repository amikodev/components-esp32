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

#ifndef __RELAY_HPP__
#define __RELAY_HPP__

#include <stdio.h>
#include <string.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"


/**
 * Реле
 * @author Приходько Д. Г.
 */
class Relay{

public:

    enum RelayType{
        RT_NONE = 0,
        RT_2 = 2,
        RT_4 = 4,
        RT_6 = 6,
        RT_8 = 8
    };

    /**
     * Установка выводов pins для управления реле.
     * Конструктор.
     */
    Relay(gpio_num_t pin1, gpio_num_t pin2, gpio_num_t pin3, gpio_num_t pin4, gpio_num_t pin5, gpio_num_t pin6, gpio_num_t pin7, gpio_num_t pin8);

    /**
     * Запись данных
     */
    void setData(uint8_t data);

    /**
     * Получение данных
     */
    uint8_t getData();

    /**
     * Управление нагрузкой
     * @param num номер вывода
     * @param state статус (HIGH, LOW)
     */
    void writeByNum(uint8_t num, bool state);

    


private:

    RelayType relayType = RT_NONE; 
    gpio_num_t relayPins[8];
    uint8_t relayData = 0x00;


};



#endif      // __RELAY_HPP__