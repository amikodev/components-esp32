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

#include "shiftload.hpp"

#define TAG "ShiftLoad"

esp_timer_handle_t ShiftLoad::timer = NULL;

xQueueHandle ShiftLoad::taskEvtQueue = NULL;


/**
 * Инициализация SPI
 * @param miso пин MISO
 * @param mosi пин MOSI
 * @param sclk пин SCLK
 * @param cs пин CS
 */
void ShiftLoad::initSpi(gpio_num_t miso, gpio_num_t mosi, gpio_num_t sclk, gpio_num_t cs){
    workType = WT_SPI;

    esp_err_t ret;

    spi_bus_config_t busCfg;
    busCfg.miso_io_num = miso;
    busCfg.mosi_io_num = mosi;
    busCfg.sclk_io_num = sclk;
    busCfg.quadwp_io_num = -1;
    busCfg.quadhd_io_num = -1;
    busCfg.max_transfer_sz = 32;
    
    spi_device_interface_config_t devCfg;
    devCfg.command_bits = 0;
    devCfg.address_bits = 0;
    devCfg.clock_speed_hz = 100000; // 10*1000*1000;               //Clock out at 10 MHz
    devCfg.mode = 0;
    devCfg.queue_size = 1;
    devCfg.spics_io_num = cs;
	devCfg.pre_cb = NULL;
	devCfg.post_cb = NULL;
    devCfg.flags = SPI_DEVICE_NO_DUMMY;
    
    ret = spi_bus_initialize(VSPI_HOST, &busCfg, 0);
    assert(ret==ESP_OK);

    ret = spi_bus_add_device(VSPI_HOST, &devCfg, &spi);
    assert(ret==ESP_OK);
}

/**
 * Инициализация таймера
 * @param dataPin вывод данных
 * @param latchPin вывод защёлки
 * @param clockPin вывод таймера
 * @deprecated
 */
void ShiftLoad::initTimer(gpio_num_t dataPin, gpio_num_t latchPin, gpio_num_t clockPin){
    workType = WT_TIMER;

    timerData = new TimerData();

    _dataPin = dataPin;
    _latchPin = latchPin;
    _clockPin = clockPin;

    gpio_set_direction(_dataPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(_latchPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(_clockPin, GPIO_MODE_OUTPUT);
}

/**
 * Инициализация задачи.
 * Запись в сдвиговый регистр будет производиться при её запуске.
 * @param dataPin вывод данных
 * @param latchPin вывод защёлки
 * @param clockPin вывод таймера
 */
void ShiftLoad::initTask(gpio_num_t dataPin, gpio_num_t latchPin, gpio_num_t clockPin){
    workType = WT_TASK;

    taskData = new TaskData();

    _dataPin = dataPin;
    _latchPin = latchPin;
    _clockPin = clockPin;

    uint64_t bitMask = 0;
    bitMask |= (1Ull << dataPin);
    bitMask |= (1Ull << latchPin);
    bitMask |= (1Ull << clockPin);

    gpio_config_t io_conf;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = bitMask;
    gpio_config(&io_conf);

    ShiftLoad::taskEvtQueue = xQueueCreate(10, sizeof(TaskData));
    xTaskCreate(ShiftLoad::writeTask, "writeTask", 2048, NULL, 9, NULL);
}

/**
 * Регистрация количества сдвиговых регистров
 * @param count количество
 */
void ShiftLoad::registerCount(uint8_t count){
    _count = count;
}

/**
 * Управление нагрузкой определённого сдвигового регистра
 * по SPI
 * @param ind индекс сдвигового регистра (0-3)
 * @param data данные
 */
void ShiftLoad::writeViaSpi(uint8_t ind, uint8_t data){
    esp_err_t ret;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));

    t.length = 8*_count;

    loadData[ind] = data;
    for(uint8_t i=0; i<_count; i++){
        uint8_t ld = loadData[i];
        if(isInverse) ld = ~ld;     // инверсия выводов
        t.tx_data[_count-i-1] = ld;
    }
    t.user = (void*)1;
    t.flags = SPI_TRANS_USE_TXDATA;

    ret = spi_device_polling_transmit(spi, &t);
    assert(ret==ESP_OK);
}

/**
 * Управление нагрузкой определённого сдвигового регистра
 * по таймеру
 * @param ind индекс сдвигового регистра (0-3)
 * @param data данные
 * @deprecated
 */
void ShiftLoad::writeViaTimer(uint8_t ind, uint8_t data){

    loadData[ind] = data;

    TimerData *td = timerData;
    td->_cnt = 0;
    td->shiftLoad = this;
    // td->ind = ind;
    // td->loadData = loadData;
    // td->_count = _count;

    // timerData._cnt = 0;
    // timerData.ind = ind;
    // timerData.loadData = loadData;
    // timerData._count = _count;

    // printf("ShiftLoad td pointer: %p \n", td);

    esp_timer_create_args_t timerArgs = {
        .callback = &ShiftLoad::timerCallback,
        .arg = (void *)td,
        // .arg = (void *)&timerData,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "SL timer"
    };

    // esp_timer_handle_t timerActionRun;
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &ShiftLoad::timer));

    ESP_LOGI(TAG, "latch %d", 1);
    gpio_set_level(_latchPin, 1);

    ESP_LOGI(TAG, "start timer");
    ESP_ERROR_CHECK(esp_timer_start_periodic(ShiftLoad::timer, 1));
}

/**
 * Управление нагрузкой определённого сдвигового регистра
 * по задаче
 * @param ind индекс сдвигового регистра (0-3)
 * @param data данные
 */
void ShiftLoad::writeViaTask(uint8_t ind, uint8_t data){
    loadData[ind] = data;

    TaskData *td = taskData;
    td->dataPin = _dataPin;
    td->latchPin = _latchPin;
    td->clockPin = _clockPin;
    td->count = _count;
    memcpy(td->loadData, loadData, sizeof(uint8_t)*4);
    td->isInverse = isInverse;

    // отправляем данные в очередь
    // дальнейшая обработка в методе writeTask
    xQueueGenericSend(ShiftLoad::taskEvtQueue, (void *) td, (TickType_t) 0, queueSEND_TO_BACK);
}

/**
 * Отправка данных по задаче
 * @param arg параметры задачи
 */
void ShiftLoad::writeTask(void *arg){
    TaskData *td = new TaskData();
    for(;;){
        if(xQueueReceive(taskEvtQueue, td, portMAX_DELAY)){
            // устанавливаем защёлку
            gpio_set_level(td->latchPin, 0x00);

            for(uint8_t i=0; i<td->count; i++){
                uint8_t ld = td->loadData[i];
                if(td->isInverse) ld = ~ld;     // инверсия выводов

                for(uint8_t j=0; j<8; j++){
                    uint8_t level = (ld >> j) & 0x01;
                    gpio_set_level(td->dataPin, level);

                    // дёргание вывода таймера
                    for(uint8_t k=0; k<2; k++){
                        gpio_set_level(td->clockPin, k%2==0);

                        // задержка времени
                        uint32_t us = 5;    // мкс
                        uint32_t m = esp_timer_get_time();
                        uint32_t e = m + us;
                        if(m > e){      // overflow
                            while(esp_timer_get_time() > e){}
                        }
                        while(esp_timer_get_time() < e){}
                    }
                }
            }

            // сбрасываем защёлку
            gpio_set_level(td->latchPin, 0xFF);
            // задержка времени
            uint32_t us = 5;    // мкс
            uint32_t m = esp_timer_get_time();
            uint32_t e = m + us;
            if(m > e){      // overflow
                while(esp_timer_get_time() > e){}
            }
            while(esp_timer_get_time() < e){}

        }
    }
}

/**
 * Управление нагрузкой определённого сдвигового регистра
 * @param ind индекс сдвигового регистра (0-3)
 * @param data данные
 */
void ShiftLoad::write(uint8_t ind, uint8_t data){
    if(_count < 1){
        ESP_LOGW(TAG, "ERROR: count of registers not registered");
        return;
    } else if(_count <= ind){
        ESP_LOGW(TAG, "ERROR: count of registers below that current index");
        return;
    }

    ESP_LOGI(TAG, "write ind: %d; load data: 0x%02x", ind, data);

    if(workType == WT_SPI){
        writeViaSpi(ind, data);
    } else if(workType == WT_TIMER){
        writeViaTimer(ind, data);
    } else if(workType == WT_TASK){
        writeViaTask(ind, data);
    }
}

/**
 * Управление нагрузкой определённого сдвигового регистра по номеру выводу
 * @param ind индекс сдвигового регистра (0-3)
 * @param num номер вывода (0-7)
 * @param state статус (HIGH, LOW)
 */
void ShiftLoad::writeByNum(uint8_t ind, uint8_t num, bool state){
    if(num == ShiftLoad::PIN_NOT_USED){
        ESP_LOGI(TAG, "writeByNum: pin not used");
        return;
    }

    uint8_t data = loadData[ind];
    if(state){
        data |= 1<<num;
    } else{
        data &= ~(1<<num);
    }

    ESP_LOGI(TAG, "writeByNum ind: %d; load data: 0x%02x", ind, data);
    write(ind, data);
}

/**
 * Получение данных определённого сдвигового регистра
 * @param ind индекс сдвигового регистра (0-3)
 */
uint8_t ShiftLoad::getData(uint8_t ind){
    return loadData[ind];
}

/**
 * Установка инверсии выводов. 
 * Применяется для реле.
 * @param inverse инверсия выводов
 */
void ShiftLoad::setInverse(bool inverse){
    ESP_LOGI(TAG, "setInverse : %d", inverse);
    isInverse = inverse;
}
/**
 * Получить установлена ли инверсия выводов.
 */
bool ShiftLoad::getInverse(){
    return isInverse;
}

/**
 * Остановка таймера
 * @deprecated
 */
void ShiftLoad::stopTimer(){
    if(timer != NULL){
        ESP_ERROR_CHECK(esp_timer_stop(timer));
        ESP_ERROR_CHECK(esp_timer_delete(timer));
        timer = NULL;
    }
    ESP_LOGI(TAG, "stop timer");
}

/**
 * Получить рабочие данные
 */
uint8_t* ShiftLoad::getLoadData(){
    return loadData;
}

/**
 * Получить количество зарегистрированных сдвиговых регистров
 */
uint8_t ShiftLoad::getRegisteredCount(){
    return _count;
}

/**
 * Получение вывод данных
 */
gpio_num_t ShiftLoad::getDataPin(){
    return _dataPin;
}

/**
 * Вывод защёлки
 */
gpio_num_t ShiftLoad::getLatchPin(){
    return _latchPin;
}

/**
 * Вывод таймера
 */
gpio_num_t ShiftLoad::getClockPin(){
    return _clockPin;
}

/**
 * Отправка данных по таймеру
 * @param arg параметры таймера
 * @deprecated
 */
void ShiftLoad::timerCallback(void* arg){
    TimerData *td = (TimerData *)arg;

    ESP_LOGI(TAG, "clock %d", 1);
    gpio_set_level(td->shiftLoad->getClockPin(), 1);

    uint8_t *loadData = td->shiftLoad->getLoadData();
    uint8_t count = td->shiftLoad->getRegisteredCount();

    // for(uint8_t i=0; i<_count; i++){
    //     uint8_t ld = loadData[i];
    //     if(isInverse) ld = ~ld;     // инверсия выводов
    //     t.tx_data[_count-i-1] = ld;
    // }

    uint8_t bit = ( loadData[td->_cnt>>3] >> (td->_cnt%8) ) & 0x01;

    ESP_LOGI(TAG, "data %d", bit);
    gpio_set_level(td->shiftLoad->getDataPin(), bit);


    ESP_LOGI(TAG, "clock %d", 0);
    gpio_set_level(td->shiftLoad->getClockPin(), 0);

    if(++td->_cnt >= count*8){
        stopTimer();
        ESP_LOGI(TAG, "latch %d", 0);
        gpio_set_level(td->shiftLoad->getLatchPin(), 0);
    }
}

