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

#ifndef __R8EF_H__
#define __R8EF_H__

#include <stdio.h>
#include <string.h>


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"


/**
 * R8EF
 */
class R8EF{

public:

    /**
     * Установка пинов на каналы в режиме PWM
     */
    static void setPwmPins(gpio_num_t chPin1, gpio_num_t chPin2, gpio_num_t chPin3, gpio_num_t chPin4, gpio_num_t chPin5, gpio_num_t chPin6, gpio_num_t chPin7, gpio_num_t chPin8);

    /**
     * Запуск задачи PWM
     */
    static void startPwmTask();

    /**
     * Задача PWM
     */
    static void pwmTask(void* arg);

    /**
     * Прерывание на подъём фронта PWM
     */
    static void IRAM_ATTR pwmRiseIsrHandler(void *arg);

    /**
     * Прерывание на спуск фронта PWM
     */
    static void IRAM_ATTR pwmFallIsrHandler(void *arg);
    // void IRAM_ATTR ...

    /**
     * Фильтрация значения канала 5
     */
    static int64_t filterPWMValue5();

    /**
     * Фильтрация значения канала 7
     */
    static int64_t filterPWMValue7();

    /**
     * Имя канала с привязкой к индексу
     */
    enum ChannelTypeNum{
        J1_X = 3,
        J1_Y = 2,
        J2_X = 0,
        J2_Y = 1,
        SWA = 6,
        VRA = 7,
        SWB = 4,
        VRB = 5
    };


    static xQueueHandle evtQueue;
    struct EvtChannelPWMData {
        uint32_t pNum;
        int64_t val;
    };

    typedef void (*FuncChannelCallback)(uint8_t value);
    static FuncChannelCallback funcChannelCallbacks[8];

    static void funcChannel(uint8_t chNum, FuncChannelCallback func);


    static gpio_num_t pwmChannelMap[8];     // привязка канала к пину
    static int64_t pwmst[8];                // время начала сигнала
    static int64_t pwmsl[8];                // предыдущая длительность сигнала
    static int64_t pwms[8];                 // длительность сигнала
    static int64_t pwmsf[8];                // фильтрованное значение канала


private:



};



#endif      // __R8EF_H__