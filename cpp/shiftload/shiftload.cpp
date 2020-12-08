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

#include "shiftload.hpp"

// const uint8_t ShiftLoad::PIN_NOT_USED = 255;
esp_timer_handle_t ShiftLoad::timer = NULL;


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
 */
void ShiftLoad::initTimer(gpio_num_t dataPin, gpio_num_t latchPin, gpio_num_t clockPin){
    workType = WT_TIMER;

    timerData = new TimerData();

    _dataPin = dataPin;
    _latchPin = latchPin;
    _clockPin = clockPin;

    // uint64_t bitMask = 0;
    // bitMask |= (1Ull << dataPin);
    // bitMask |= (1Ull << latchPin);
    // bitMask |= (1Ull << clockPin);

    // gpio_config_t io_conf;
    // io_conf.mode = GPIO_MODE_OUTPUT;
    // io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    // io_conf.pin_bit_mask = bitMask;
    // gpio_config(&io_conf);

    gpio_set_direction(_dataPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(_latchPin, GPIO_MODE_OUTPUT);
    gpio_set_direction(_clockPin, GPIO_MODE_OUTPUT);

}

/**
 * Регистрация количества сдвиговых регистров
 * @param count количество
 */
void ShiftLoad::registerCount(uint8_t count){
    _count = count;
}



/**
 * Управление нагрузкой
 * @param data данные
 */
// void ShiftLoad::write(uint8_t data){

//     printf("ShiftLoad::write load data: 0x%02x \n", data);
//     // fflush(stdout);

//     esp_err_t ret;

//     spi_transaction_t t;
//     memset(&t, 0, sizeof(t));

//     t.length = 8;
//     // t.tx_buffer = data;
//     t.tx_data[0] = data;
//     t.user = (void*)1;
//     t.flags = SPI_TRANS_USE_TXDATA;

//     ret = spi_device_polling_transmit(spi, &t);
//     assert(ret==ESP_OK);

//     loadData[0] = data;

// }

// void ShiftLoad::write(uint8_t data0, uint8_t data1){
//     printf("ShiftLoad::write load data0: 0x%02x, data1: 0x%02x \n", data0, data1);
//     // fflush(stdout);

//     esp_err_t ret;

//     spi_transaction_t t;
//     memset(&t, 0, sizeof(t));

//     t.length = 8*2;
//     // t.tx_buffer = data;
//     t.tx_data[0] = data1;
//     t.tx_data[1] = data0;
//     t.user = (void*)1;
//     t.flags = SPI_TRANS_USE_TXDATA;

//     ret = spi_device_polling_transmit(spi, &t);
//     assert(ret==ESP_OK);

//     loadData[0] = data0;
//     loadData[1] = data1;


// }


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
        .name = "SL timer"
    };

    // esp_timer_handle_t timerActionRun;
    ESP_ERROR_CHECK(esp_timer_create(&timerArgs, &ShiftLoad::timer));

    printf("ShiftLoad latch %d \n", 1);
    gpio_set_level(_latchPin, 1);

    printf("ShiftLoad start timer \n");
    ESP_ERROR_CHECK(esp_timer_start_periodic(ShiftLoad::timer, 1));


}

/**
 * Управление нагрузкой определённого сдвигового регистра
 * @param ind индекс сдвигового регистра (0-3)
 * @param data данные
 */
void ShiftLoad::write(uint8_t ind, uint8_t data){
    if(_count < 1){
        printf("ShiftLoad ERROR: count of registers not registered \n");
        return;
    } else if(_count <= ind){
        printf("ShiftLoad ERROR: count of registers below that current index \n");
        return;
    }

    printf("ShiftLoad::write ind: %d; load data: 0x%02x \n", ind, data);
    // fflush(stdout);

    if(workType == WT_SPI){
        writeViaSpi(ind, data);
    } else if(workType == WT_TIMER){
        writeViaTimer(ind, data);
    }

    // loadData[0] = data0;
    // loadData[1] = data1;
}

/**
 * Управление нагрузкой определённого сдвигового регистра по номеру выводу
 * @param ind индекс сдвигового регистра (0-3)
 * @param num номер вывода (0-7)
 * @param state статус (HIGH, LOW)
 */
void ShiftLoad::writeByNum(uint8_t ind, uint8_t num, bool state){

    if(num == ShiftLoad::PIN_NOT_USED){
        printf("ShiftLoad::writeByNum: pin not used \n");
        return;
    }

    uint8_t data = loadData[ind];
    if(state){
        data |= 1<<num;
    } else{
        data &= ~(1<<num);
    }

    printf("ShiftLoad::writeByNum ind: %d; load data: 0x%02x \n", ind, data);
    // fflush(stdout);

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
    printf("ShiftLoad::setInverse : %d \n", inverse);
    isInverse = inverse;
}

/**
 * Остановка таймера
 */
void ShiftLoad::stopTimer(){
    if(timer != NULL){
        ESP_ERROR_CHECK(esp_timer_stop(timer));
        ESP_ERROR_CHECK(esp_timer_delete(timer));
        timer = NULL;
    }
    printf("ShiftLoad stop timer \n");
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
 */
void ShiftLoad::timerCallback(void* arg){
    TimerData *td = (TimerData *)arg;

    printf("ShiftLoad clock %d \n", 1);
    gpio_set_level(td->shiftLoad->getClockPin(), 1);

    uint8_t *loadData = td->shiftLoad->getLoadData();
    uint8_t count = td->shiftLoad->getRegisteredCount();

    // for(uint8_t i=0; i<_count; i++){
    //     uint8_t ld = loadData[i];
    //     if(isInverse) ld = ~ld;     // инверсия выводов
    //     t.tx_data[_count-i-1] = ld;
    // }

    // printf("ShiftLoad td pointer: %p \n", td);
    // printf("ShiftLoad _cnt: %d, _count: %d \n", td->_cnt, td->_count);

    // uint8_t ldInd = td->_cnt>>3;
    uint8_t bit = ( loadData[td->_cnt>>3] >> (td->_cnt%8) ) & 0x01;
    // printf("ShiftLoad bit[%d]=%d \n", td->_cnt, bit);

    printf("ShiftLoad data %d \n", bit);
    gpio_set_level(td->shiftLoad->getDataPin(), bit);


    printf("ShiftLoad clock %d \n", 0);
    gpio_set_level(td->shiftLoad->getClockPin(), 0);

    if(++td->_cnt >= count*8){
        stopTimer();
        printf("ShiftLoad latch %d \n", 0);
        gpio_set_level(td->shiftLoad->getLatchPin(), 0);
    }


}

