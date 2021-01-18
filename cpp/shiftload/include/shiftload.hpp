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

#ifndef __SHIFTLOAD_H__
#define __SHIFTLOAD_H__

#include <stdio.h>
#include <string.h>


#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"


/**
 * ShiftLoad
 * Для работы со сдвиговым регистром 74HC595N
 * @author Приходько Д. Г.
 */
class ShiftLoad{

private:

    enum WorkType{
        WT_NONE = 0,
        WT_SPI,
        WT_TIMER,
        WT_TASK
    };

    /**
     * 
     * @deprecated
     */
    struct TimerData{
        uint8_t _cnt = 0;       // текущий бит
        ShiftLoad *shiftLoad;   // сдвиговый регистр
        // uint8_t ind;            // индекс сдвигового регистра (0-3)
        // uint8_t *loadData;      // данные
        // uint8_t _count;         // Количество сдвиговых регистров
    };

    struct TaskData{
        // ShiftLoad *shiftLoad;   // сдвиговый регистр
        gpio_num_t dataPin = GPIO_NUM_NC;
        gpio_num_t latchPin = GPIO_NUM_NC;
        gpio_num_t clockPin = GPIO_NUM_NC;
        uint8_t count = 0;
        uint8_t loadData[4] = {0x00};
        bool isInverse = false;
    };

    /**
     * SPI
     */
    spi_device_handle_t spi;
    
    /**
     * Количество сдвиговых регистров
     */
    uint8_t _count = 0;

    /**
     * Load data
     */
    uint8_t loadData[4] = {0x00};

    /**
     * Инверсия выводов
     */
    bool isInverse = false;

    /**
     * Тип работы
     */
    WorkType workType = WT_NONE;

    /**
     * Данные таймера
     */
    TimerData *timerData = NULL;

    /**
     * Данные задачи
     */
    TaskData *taskData = NULL;

    /**
     * Вывод данных
     */
    gpio_num_t _dataPin = GPIO_NUM_NC;

    /**
     * Вывод защёлки
     */
    gpio_num_t _latchPin = GPIO_NUM_NC;

    /**
     * Вывод таймера
     */
    gpio_num_t _clockPin = GPIO_NUM_NC;

    /**
     * Очередь вывода
     */
    static xQueueHandle taskEvtQueue;

    /**
     * Управление нагрузкой определённого сдвигового регистра
     * по SPI
     * @param ind индекс сдвигового регистра (0-3)
     * @param data данные
     */
    void writeViaSpi(uint8_t ind, uint8_t data);

    /**
     * Управление нагрузкой определённого сдвигового регистра
     * по таймеру
     * @param ind индекс сдвигового регистра (0-3)
     * @param data данные
     * @deprecated
     */
    void writeViaTimer(uint8_t ind, uint8_t data);

    /**
     * Управление нагрузкой определённого сдвигового регистра
     * по задаче
     * @param ind индекс сдвигового регистра (0-3)
     * @param data данные
     */
    void writeViaTask(uint8_t ind, uint8_t data);

    /**
     * Остановка таймера
     * @deprecated
     */
    static void stopTimer();

public:

    static const uint8_t PIN_NOT_USED = 255;

    /**
     * Таймер
     */
    // static esp_timer_handle_t timer = NULL;
    static esp_timer_handle_t timer;

    /**
     * Инициализация SPI
     * @param miso пин MISO
     * @param mosi пин MOSI
     * @param sclk пин SCLK
     * @param cs пин CS
     */
    void initSpi(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs);

    /**
     * Инициализация таймера
     * @param dataPin вывод данных
     * @param latchPin вывод защёлки
     * @param clockPin вывод таймера
     * @deprecated
     */
    void initTimer(gpio_num_t dataPin, gpio_num_t latchPin, gpio_num_t clockPin);

    /**
     * Инициализация задачи.
     * Запись в сдвиговый регистр будет производиться при её запуске.
     * @param dataPin вывод данных
     * @param latchPin вывод защёлки
     * @param clockPin вывод таймера
     */
    void initTask(gpio_num_t dataPin, gpio_num_t latchPin, gpio_num_t clockPin);

    /**
     * Регистрация количества сдвиговых регистров
     * @param count количество
     */
    void registerCount(uint8_t count);

    /**
     * Управление нагрузкой определённого сдвигового регистра
     * @param ind индекс сдвигового регистра (0-3)
     * @param data данные
     */
    void write(uint8_t ind, uint8_t data);

    /**
     * Управление нагрузкой
     * @param ind индекс сдвигового регистра (0-3)
     * @param num номер вывода (0-7)
     * @param state статус (HIGH, LOW)
     */
    void writeByNum(uint8_t ind, uint8_t num, bool state);

    /**
     * Получение данных определённого сдвигового регистра
     * @param ind индекс сдвигового регистра (0-3)
     */
    uint8_t getData(uint8_t ind);

    /**
     * Установка инверсии выводов. 
     * Применяется для реле.
     * @param inverse инверсия выводов
     */
    void setInverse(bool inverse);

    /**
     * Получить установлена ли инверсия выводов.
     */
    bool getInverse();

    /**
     * Получить рабочие данные
     */
    uint8_t* getLoadData();

    /**
     * Получить количество зарегистрированных сдвиговых регистров
     */
    uint8_t getRegisteredCount();

    /**
     * Получение вывод данных
     */
    gpio_num_t getDataPin();

    /**
     * Вывод защёлки
     */
    gpio_num_t getLatchPin();

    /**
     * Вывод таймера
     */
    gpio_num_t getClockPin();


    /**
     * Отправка данных по таймеру
     * @param arg параметры таймера
     * @deprecated
     */
    static void timerCallback(void* arg);

    /**
     * Отправка данных по задаче
     * @param arg параметры задачи
     */
    static void writeTask(void *arg);

};

#endif      // __SHIFTLOAD_H__

