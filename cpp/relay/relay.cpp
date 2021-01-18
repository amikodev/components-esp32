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

#include "relay.hpp"

/**
 * Установка выводов pins для управления реле
 */
Relay::Relay(gpio_num_t pin1, gpio_num_t pin2, gpio_num_t pin3, gpio_num_t pin4, gpio_num_t pin5, gpio_num_t pin6, gpio_num_t pin7, gpio_num_t pin8){

    relayType = RT_8;

    relayPins[0] = pin1;
    relayPins[1] = pin2;
    relayPins[2] = pin3;
    relayPins[3] = pin4;
    relayPins[4] = pin5;
    relayPins[5] = pin6;
    relayPins[6] = pin7;
    relayPins[7] = pin8;

    uint64_t bitMask = 0;
    for(uint8_t i=0; i<8; i++){
        if(relayPins[i] > GPIO_NUM_NC){
            bitMask |= (1Ull << relayPins[i]);
        }
    }

    gpio_config_t io_conf;
    // io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    // io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
    io_conf.pin_bit_mask = bitMask;
    gpio_config(&io_conf);

    for(uint8_t i=0; i<8; i++){
        if(relayPins[i] > GPIO_NUM_NC){
            gpio_set_level(relayPins[i], 1);
        }
    }

}

/**
 * Запись данных
 */
void Relay::setData(uint8_t data){
    relayData = data;

    for(uint8_t i=0; i<8; i++){
        if(relayPins[i] > GPIO_NUM_NC){
            if( ((relayData >> i) & 0x01) == 0 ){
                gpio_set_level(relayPins[i], 1);
            } else{
                gpio_set_level(relayPins[i], 0);
            }
        }
    }

}

/**
 * Получение данных
 */
uint8_t Relay::getData(){
    return relayData;
}

/**
 * Управление нагрузкой
 * @param num номер вывода
 * @param state статус (HIGH, LOW)
 */
void Relay::writeByNum(uint8_t num, bool state){


    uint8_t data = relayData;
    if(state){
        data |= 1<<num;
    } else{
        data &= ~(1<<num);
    }

    // printf("ShiftLoad::writeByNum ind: %d; load data: 0x%02x \n", ind, data);
    setData(data);

}


