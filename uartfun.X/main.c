/* 
 * File:   main.c
 * Author: tomek
 *
 * Created on 15 wrze?nia 2020, 10:23
 */
// CONFIG1L
#pragma config FEXTOSC = OFF    // External Oscillator Selection (Oscillator not enabled)
#pragma config RSTOSC = HFINTOSC_64MHZ// Reset Oscillator Selection (HFINTOSC with HFFRQ = 64 MHz and CDIV = 1:1)

// CONFIG1H
#pragma config CLKOUTEN = OFF   // Clock out Enable bit (CLKOUT function is disabled)
#pragma config PR1WAY = ON      // PRLOCKED One-Way Set Enable bit (PRLOCK bit can be cleared and set only once)
#pragma config CSWEN = ON       // Clock Switch Enable bit (Writing to NOSC and NDIV is allowed)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)

// CONFIG2L
#pragma config MCLRE = INTMCLR  // MCLR Enable bit (If LVP = 0, MCLR pin function is port defined function; If LVP =1, RE3 pin fuction is MCLR)
#pragma config PWRTS = PWRT_1   // Power-up timer selection bits (PWRT set at 1ms)
#pragma config MVECEN = ON      // Multi-vector enable bit (Multi-vector enabled, Vector table used for interrupts)
#pragma config IVT1WAY = ON     // IVTLOCK bit One-way set enable bit (IVTLOCK bit can be cleared and set only once)
#pragma config LPBOREN = OFF    // Low Power BOR Enable bit (ULPBOR disabled)
#pragma config BOREN = SBORDIS  // Brown-out Reset Enable bits (Brown-out Reset enabled , SBOREN bit is ignored)

// CONFIG2H
#pragma config BORV = VBOR_2P45 // Brown-out Reset Voltage Selection bits (Brown-out Reset Voltage (VBOR) set to 2.45V)
#pragma config ZCD = OFF        // ZCD Disable bit (ZCD disabled. ZCD can be enabled by setting the ZCDSEN bit of ZCDCON)
#pragma config PPS1WAY = ON     // PPSLOCK bit One-Way Set Enable bit (PPSLOCK bit can be cleared and set only once; PPS registers remain locked after one clear/set cycle)
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config DEBUG = OFF      // Debugger Enable bit (Background debugger disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Extended Instruction Set and Indexed Addressing Mode disabled)

// CONFIG3L
#pragma config WDTCPS = WDTCPS_31// WDT Period selection bits (Divider ratio 1:65536; software control of WDTPS)
#pragma config WDTE = OFF       // WDT operating mode (WDT Disabled; SWDTEN is ignored)

// CONFIG3H
#pragma config WDTCWS = WDTCWS_7// WDT Window Select bits (window always open (100%); software control; keyed access not required)
#pragma config WDTCCS = SC      // WDT input clock selector (Software Control)

// CONFIG4L
#pragma config BBSIZE = BBSIZE_512// Boot Block Size selection bits (Boot Block size is 512 words)
#pragma config BBEN = OFF       // Boot Block enable bit (Boot block disabled)
#pragma config SAFEN = OFF      // Storage Area Flash enable bit (SAF disabled)
#pragma config WRTAPP = OFF     // Application Block write protection bit (Application Block not write protected)

// CONFIG4H
#pragma config WRTB = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-30000Bh) not write-protected)
#pragma config WRTC = OFF       // Boot Block Write Protection bit (Boot Block (000000-0007FFh) not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM not write-protected)
#pragma config WRTSAF = OFF     // SAF Write protection bit (SAF not Write Protected)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (HV on MCLR/VPP must be used for programming)

// CONFIG5L
#pragma config CP = OFF         // PFM and Data EEPROM Code Protection bit (PFM and Data EEPROM code protection disabled)

// CONFIG5H

#include <xc.h>
#include <pic18f27k42.h>
#include <pic18.h>
//#include "uart.h"
#include <stdint.h>
#include "mcal.h"
#include "state_machine.h"
#include "answer.h"
#include "mcc_generated_files/mcc.h"

typedef uint16_t delay_timer_t;

typedef struct {
    delay_timer_t start;
    delay_timer_t delay;
} delay_t;

volatile struct state rx_state;
volatile delay_t delay;

void delay_start(delay_t* delay, delay_timer_t value) {
    delay->start = TMR0_ReadTimer();
    delay->delay = value;
}

void delay_restart(delay_t* delay) {
    delay->start = TMR0_ReadTimer();
}


uint8_t delay_is_expired(delay_t* delay)
{
    uint16_t now = TMR0_ReadTimer();
    return (now - delay->start) >= delay->delay;
}


uint8_t delay_check_and_restart_interval(delay_t* delay) {
    uint8_t expired = delay_is_expired(delay);

    if (expired)
    {
        delay->start += delay->delay;

        if (delay_is_expired(delay))
            delay_restart(delay);
    }
    return expired;    
}



void prints(const char* str)
{
    while(!delay_is_expired(&delay));
    
    while(*str)
    {
        while(U1TXIF == 0);
        
        U1TXB = *str++;
    }
}

uint8_t get_digit(char ch)
{
    uint8_t digit = ch - '0';
    if (ch >= '@')
        digit = ch - '@';
    
    if (digit > 9)
        digit = 0;
    
    return digit;
}

void send_request(volatile struct state* st) {
    char cmd[] = "{A}";
    prints(cmd);
}

void send_answer(volatile struct state* st) {
    uint16_t num = 0;
    num += get_digit(st->buf[1]) * 100;
    num += get_digit(st->buf[2]) * 10;
    num += get_digit(st->buf[3]) * 1;
    num += 1;
    
    char cmd[9] = {0};
    cmd[0] = '{'; 
    cmd[1] = st->buf[0];
    cmd[2] = ((num/100) % 10) + '0';
    cmd[3] = ((num/10) % 10) + '0';
    cmd[4] = ((num/1) % 10) + '0';
    cmd[5] = '=';
    cmd[6] = '=';
    cmd[7] = '}';
    prints(cmd);
}

void Led_blinking(int how_many_times) {
    for(int i = 0; i < how_many_times;i++) {
        LATBbits.LATB5 = ~LATBbits.LATB5;
        __delay_ms(100);
        LATBbits.LATB5 = ~LATBbits.LATB5;
        __delay_ms(100);
    }
}

char readb(void)
{
    while(U1RXIF == 0);
    return U1RXB;
}

void scanfs() {
    char buffer[4];
    for(size_t i = 0;i < 4 ;i++) {
        buffer[i] = 0;
    }
    char ch;
    int i = 0;
    if(readb() == '%') {
       while( (ch = readb()) != ' ') {
           buffer[i] = ch;
           i++;
        }
       for(size_t i = 0;i < 4; i++) {
           while(U1TXIF == 0);
           U1TXB = buffer[i];
        }
        if(buffer[0] == 's') {
            LATBbits.LATB5 = ~LATBbits.LATB5;
        }
        else if (buffer[0] == 'a') {
            LATBbits.LATB5 = ~LATBbits.LATB5;
        }
        else if (buffer[0] == 'b') {
            Led_blinking(buffer[1] - '0');   
        }
      for(size_t i = 0;i < 6;i++) {
        buffer[i] = 0;
      }
    }
//     while(U1IF == 0) {
//        LATBbits.LATB5 = ~LATBbits.LATB5;
//        __delay_ms(100);
//        LATBbits.LATB5 = ~LATBbits.LATB5;
//        __delay_ms(100);
//    } 
}


MCAL_ISR(U1RX)
{
    
    char znak = U1RXB;
    state_rx_byte(&rx_state, znak);
    
    delay_start(&delay, 10);
    
    
//    if (znak == '1')
//        LATBbits.LATB5 = 1;
//    else
//        LATBbits.LATB5 = 0;
}

void handler_command_a(char cmd, char arg) {
    LATBbits.LATB5 = ~LATBbits.LATB5;
    prints("a");
}


void handler_command_s(char cmd, char arg) { 
    LATBbits.LATB5 = 1;
    prints("s");
}

void handler_command_b(char cmd, char arg) {
    Led_blinking(arg - '0');
    prints("b");
}

void handler_command_d(char cmd, char arg) {
    LATBbits.LATB5 = 0;
    prints("d");
}

const struct cmd_handler handlers[5] = {
    {
        .cmd = 'a',
        .handler = handler_command_a,
    },
    {
        .cmd = 'b',
        .handler = handler_command_b,
    },
    {
        .cmd = 's',
        .handler = handler_command_s,
    },
    {
        .cmd = 'd',
        .handler = handler_command_d,
    },
    {0}
};

void printnum(uint16_t num)
{
    char frm[8] = {0};
    frm[0] = '{';
    frm[1] = 'A';
    frm[2] = ((num/100) % 10) + '0';
    frm[3] = ((num/10) % 10) + '0';
    frm[4] = ((num/1) % 10) + '0';
    frm[5] = '=';
    frm[6] = '=';
    frm[7] = '}';
    prints(frm);
    
}

void main(void) {
    
    SYSTEM_Initialize();
    
    TRISBbits.TRISB5 = 0;
    ANSELBbits.ANSELB5 = 0;
    LATBbits.LATB5 = 1;
    
    U1CON0bits.MODE = 0; // Async 8-bit mode
    U1BRGL = (1666) & 0xFF;
    U1BRGH = (1666 >> 8) & 0xFF;
    
    MCAL_GPIO_AS_OUTPUT(PIN_UART_TX);
    MCAL_GPIO_AS_DIGITAL_INPUT(PIN_UART_RX);
    MCAL_GPIO_AS_OUTPUT(PIN_UART_TE);

    MCAL_GPIO_ALT_OUT(PIN_UART_TX, UART1_TX); 
    MCAL_GPIO_ALT_OUT(PIN_UART_TE, UART1_TXDE); 
    MCAL_GPIO_ALT_IN(PIN_UART_RX, UART1_RX);
    MCAL_PPSI_UART1_CTS = 0xFF; // Use some unimplemented pin
    
    U1CON2bits.FLO = 0b10; // rts/cts and TXDE
    U1CON0bits.TXEN = 1;
    U1CON0bits.RXEN = 1;
    U1CON1bits.ON = 1;
    
    PIE3bits.U1RXIE = 1;
    TMR0_StartTimer();
    
    INTCON0bits.GIEH = 1;

    delay_t timeout;
  
    //const char * ala = "Ala ma kota\n";
    uint16_t cnt = 0;
    while(1) 
    {
        __delay_ms(2000);
        send_request(&rx_state);
//            state_reset(&rx_state);
        
        delay_start(&timeout, 500);
        while (! state_is_frame_ready(&rx_state) && ! delay_is_expired(&timeout))
            ;
        
        if (state_is_frame_ready(&rx_state)) { 
            send_answer(&rx_state);
            state_reset(&rx_state);
        }
        
    }
}

