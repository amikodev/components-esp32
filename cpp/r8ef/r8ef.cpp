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

#include "r8ef.hpp"

gpio_num_t R8EF::pwmChannelMap[8] = {GPIO_NUM_NC};
int64_t R8EF::pwmst[8] = {0};
int64_t R8EF::pwmsl[8] = {0};
int64_t R8EF::pwms[8] = {0};
int64_t R8EF::pwmsf[8] = {0};

R8EF::FuncChannelCallback R8EF::funcChannelCallbacks[8] = {NULL};

xQueueHandle R8EF::evtQueue = NULL;



/**
 * Установка пинов на каналы в режиме PWM
 */
void R8EF::setPwmPins(gpio_num_t chPin1, gpio_num_t chPin2, gpio_num_t chPin3, gpio_num_t chPin4, gpio_num_t chPin5, gpio_num_t chPin6, gpio_num_t chPin7, gpio_num_t chPin8){
    R8EF::pwmChannelMap[0] = chPin1;
    R8EF::pwmChannelMap[1] = chPin2;
    R8EF::pwmChannelMap[2] = chPin3;
    R8EF::pwmChannelMap[3] = chPin4;
    R8EF::pwmChannelMap[4] = chPin5;
    R8EF::pwmChannelMap[5] = chPin6;
    R8EF::pwmChannelMap[6] = chPin7;
    R8EF::pwmChannelMap[7] = chPin8;

    // int64_t pwmst[8] = {0};
    // int64_t pwms[8] = {0};
    // pwmst = (int64_t[8]){0, 0, 0, 0, 0, 0, 0, 0};
    R8EF::pwmst[8] = {0};
    R8EF::pwmsl[8] = {0};
    R8EF::pwms[8] = {0};


    // создание очереди
    evtQueue = xQueueCreate(20, sizeof(EvtChannelPWMData));


    // установка прерываний на PWM

    uint64_t bitMask = 0;
    for(uint8_t i=0; i<8; i++){
        if(R8EF::pwmChannelMap[i] > GPIO_NUM_NC){
            bitMask |= (1Ull << R8EF::pwmChannelMap[i]);
        }
    }

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = bitMask;
    gpio_config(&io_conf);



    gpio_install_isr_service((int) 0);

    for(uint32_t i=0; i<8; i++){
        if(R8EF::pwmChannelMap[i] > GPIO_NUM_NC){
            // PArg parg = {._this=this, .arg=i};
            gpio_isr_handler_add(R8EF::pwmChannelMap[i], R8EF::pwmRiseIsrHandler, (void *) i);
        }
    }


}


/**
 * Запуск задачи PWM
 */
void R8EF::startPwmTask(){
    // printf("address pwms (1): %p \n", pwms);

    printf("pwm (1): ");
    for(int i=0; i<8; i++){
        printf("[%d] %lld; ", i, R8EF::pwms[i]);
    }
    printf("\n");



    xTaskCreate(R8EF::pwmTask, "R8EF::pwmTask", 2048, NULL, 10, NULL);


}

/**
 * Задача PWM
 */
void R8EF::pwmTask(void* arg){
    EvtChannelPWMData d;
    for(;;){
        if(xQueueReceive(evtQueue, &d, portMAX_DELAY)){

            uint32_t pNum = d.pNum;
            int64_t val = d.val;

            if(val < R8EF::pwmsl[pNum]-10 || val > R8EF::pwmsl[pNum]+10){
                R8EF::pwms[pNum] = val;
            }

            if(pNum == 4) val = R8EF::filterPWMValue5();
            else if(pNum == 6) val = R8EF::filterPWMValue7();

            // фильтрация значения канала
            if(val < 1000) val = 1000; else if(val > 2000) val = 2000;
            uint8_t valF = (uint8_t)((val-1000)*255/1000);
            pwmsf[pNum] = valF;
            
            // вызов функции при изменении значения
            if(val != R8EF::pwmsl[pNum] && funcChannelCallbacks[pNum] != NULL){
                (funcChannelCallbacks[pNum])(valF);
            }

            R8EF::pwmsl[pNum] = R8EF::pwms[pNum];


            // printf("pwm (3): ");
            // for(int i=0; i<8; i++){
            //     printf("[%d] %lld; ", i, R8EF::pwms[i]);
            // }
            // printf("\n");

            // vTaskDelay(pdMS_TO_TICKS(500));
        }
    }

}

/**
 * Прерывание на подъём фронта PWM
 */
void IRAM_ATTR R8EF::pwmRiseIsrHandler(void *arg){
    uint32_t pNum = (uint32_t) arg;
    int64_t time = esp_timer_get_time();
    R8EF::pwmst[pNum] = time;

    gpio_set_intr_type(R8EF::pwmChannelMap[pNum], GPIO_INTR_NEGEDGE);
    gpio_isr_handler_add(R8EF::pwmChannelMap[pNum], R8EF::pwmFallIsrHandler, (void *) pNum);
}

/**
 * Прерывание на спуск фронта PWM
 */
void IRAM_ATTR R8EF::pwmFallIsrHandler(void *arg){
    uint32_t pNum = (uint32_t) arg;
    int64_t time = esp_timer_get_time();

    int64_t val = time - R8EF::pwmst[pNum];

    // uint32_t btnNum = (uint32_t) arg;
    EvtChannelPWMData d = {.pNum=pNum, .val=val};
    xQueueSendFromISR(evtQueue, &d, NULL);



    // if(val < R8EF::pwmsl[pNum]-3 || val > R8EF::pwmsl[pNum]+3){
    //     R8EF::pwms[pNum] = val;
    // }

    // if(pNum == 4) val = filterPWMValue5();

    // if(val != pwmsl[pNum] && funcChannelCallbacks[pNum] != NULL){
    //     // вызов функции при изменении значения
    //     // (*recieveBinaryFunc)((uint8_t *)msg, (uint32_t)len);
    //     if(val < 1000) val = 1000; else if(val > 2000) val = 2000;
    //     uint8_t valF = (uint8_t)((val-1000)*255/1000);
    //     // (*funcChannelCallbacks[pNum])(valF);
    //     // (*(funcChannelCallbacks+pNum))(valF);
    //     (funcChannelCallbacks[pNum])(valF);
    // }

    // R8EF::pwmsl[pNum] = R8EF::pwms[pNum];

    // R8EF::pwms[pNum] = time - R8EF::pwmst[pNum];
    gpio_set_intr_type(R8EF::pwmChannelMap[pNum], GPIO_INTR_POSEDGE);
    gpio_isr_handler_add(R8EF::pwmChannelMap[pNum], R8EF::pwmRiseIsrHandler, (void *) pNum);
}

/**
 * Фильтрация значения канала 5
 */
int64_t R8EF::filterPWMValue5(){
    uint8_t dt = 20;
    int64_t value = R8EF::pwms[4];
    if(value >= 1000-dt && value <= 1000+dt) value = 1000;
    else if(value >= 1500-dt && value <= 1500+dt) value = 1500;
    else if(value >= 2000-dt && value <= 2000+dt) value = 2000;
    R8EF::pwms[4] = value;
    return value;
}

/**
 * Фильтрация значения канала 7
 */
int64_t R8EF::filterPWMValue7(){
    uint8_t dt = 20;
    int64_t value = R8EF::pwms[6];
    if(value >= 1000-dt && value <= 1000+dt) value = 1000;
    else if(value >= 1500-dt && value <= 1500+dt) value = 1500;
    else if(value >= 2000-dt && value <= 2000+dt) value = 2000;
    R8EF::pwms[6] = value;
    return value;
}


// void R8EF::funcChannel(uint8_t chNum, void (*func)(uint8_t value)){
//     // funcChannelCallbacks[chNum-1] = func;
// }
void R8EF::funcChannel(uint8_t chNum, FuncChannelCallback func){
    funcChannelCallbacks[chNum-1] = func;
}



