#include "system.h"
#include "protocol.h"

#define TRIG_SET resRB(PC_ODR, 3)
#define TRIG_RES setRB(PC_ODR, 3)

uint8_t curMode;
volatile uint8_t send = 0;

void sendData(uint8_t mode, uint8_t len, uint8_t* data) {
    while (!(UART1_SR & UART_SR_TXE)) nop();
    uint8_t st = 0xC0 | (len << 3) | mode;
    UART1_DR = st;
    uint8_t crc = 0xFF ^ st;
    for (uint8_t i = 0; i < (1 << len); i++) {
        while (!(UART1_SR & UART_SR_TXE)) nop();
        UART1_DR = data[i];
        crc ^= data[i];
    }
    while (!(UART1_SR & UART_SR_TXE)) nop();
    UART1_DR = crc;
}

void main() {
    xdev_out(uart_char);
    systemInit();

    for (uint16_t i = 0; i < sizeof(protocol); i++)
        uart_char(protocol[i]);
    
    IWDG_KR = IWDG_KR_KEY_ENABLE;
    IWDG_KR = IWDG_KR_KEY_ACCESS;
    IWDG_PR = 0x5;
    IWDG_RLR = 0xFF;
    IWDG_REFRESH;

    while (1)
        if (UART1_SR & UART_SR_RXNE)
            if (UART1_DR == 0x04) break;

    UART1_SR &= ~UART_SR_RXNE;
    uartInit(57600, 16000000);
    UART1_CR2 |= UART_CR2_RIEN;

    TIM1_PSCRL = 7;

    while (1) {
        while (!send) nop();
        send=0;
        TRIG_RES;
        delay_us(24);
        TRIG_SET;
        delay_us(24);
        TRIG_RES;
        waitR(PC_IDR, 4);

        TIM1_CR1 |= TIM_CR1_CEN;
        waitS(PC_IDR, 4);
        TIM1_CR1 &= ~TIM_CR1_CEN;

        uint32_t temp=((uint32_t)TIM1_CNTRH << 8) | (uint32_t)TIM1_CNTRL;

        //xprintf("%lu\n", temp);
        
        uint8_t buf[2];
        if(curMode==0){
            temp*=43;
            temp/=500;
            buf[0]=temp;
            buf[1]=temp>>8;
            sendData(curMode, 1, buf);
        }
        else if(curMode==1){
            temp*=27;
            temp/=800;
            buf[0]=temp;
            buf[1]=temp>>8;
            sendData(curMode,1,buf);
        }
        else if(curMode==2){
            *buf=0xFF;
            sendData(2, 0, buf);
        }
        else{
            buf[0]=0xFF;
            buf[1]=0xFF;
            sendData(curMode,1,buf);
        }
        TIM1_CNTRH=0;
        TIM1_CNTRL=0;
    }
}

void uart_rx(void) __interrupt(18) {
    static uint8_t stat = 0;
    static uint8_t crc = 0;
    static uint8_t m = 0;
    if ((UART1_SR & UART_SR_RXNE)) {
        IWDG_REFRESH;
        uint8_t d = UART1_DR;
        UART1_SR &= ~UART_SR_RXNE;
        switch (stat) {
            case 0:
                if (d == 0x02) send = 1;
                if (d == 0x43) crc = 0xBC, stat = 1;
                break;
            case 1:
                m = d;
                crc ^= d;
                stat = 2;
                break;
            case 2:
                if (crc == d) curMode = m;
                stat = 0;
                break;
        }
    }
}

extern volatile uint16_t delC;

void tim4(void) __interrupt(23) {
    if (delC) delC--;
    TIM4_SR &= ~TIM_SR1_UIF;
}
