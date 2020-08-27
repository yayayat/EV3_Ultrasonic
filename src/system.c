#include "system.h"

volatile uint16_t delC = 0;

void delay_ms(uint16_t t) {
    TIM4_PSCR = 0x06;
    TIM4_ARR = 249;
    delC = t;
    TIM4_IER |= TIM_IER_UIE;
    TIM4_CR1 |= TIM_CR1_CEN;
    while (delC) nop();
    TIM4_CR1 &= ~TIM_CR1_CEN;
}

void delay_us(uint16_t t) {
    TIM4_PSCR = 0x06;  
    TIM4_ARR = 1;      
    delC = t >> 3;
    TIM4_IER |= TIM_IER_UIE;
    TIM4_CR1 |= TIM_CR1_CEN;
    while (delC) nop();
    TIM4_CR1 &= ~TIM_CR1_CEN;
}

inline void pinInit(){
    // PD5  - UART_TX   - UART_PUSH_PULL
    // PD6  - UART_RX   - UART_PULL_UP
    setRB(PD_DDR, 5);
    setRB(PD_CR1, 5);
    setRB(PD_CR1, 6);

    setRB(PC_CR1, 3);
    setRB(PC_DDR, 3);

}

inline void uartInit(uint32_t baud_rate, uint32_t f_master) {
    xdev_out(uart_char);
    f_master /= baud_rate;
    UART1_BRR2 = (uint8_t)(((f_master >> 8) & 0xF0) | (f_master & 0x0F));
    UART1_BRR1 = (uint8_t)(f_master >> 4);
    UART1_CR2 = UART_CR2_TEN | UART_CR2_REN;
}

inline void  uart_print(char *str) {
    while (*str != 0) {
        while (!(UART1_SR & UART_SR_TXE)) nop();
        UART1_DR = *str++;
    }
}

void uart_char(unsigned char c){
    while (!(UART1_SR & UART_SR_TXE)) nop();
        UART1_DR = c;
}   

void systemInit(){
    CLK_CKDIVR = 0x00;
    pinInit();
    uartInit(2400, 16000000);
    rim();
}