#include <msp430.h>
#include <string.h>
#include <stdio.h>


#define HIGH                    1
#define LOW                     0
#define TRUE                    1
#define FALSE                   0

#define MASTER                  1
#define SLAVE                   0

#define ISR_TIMER0_A0_OFFSET 500
#define BUFFER_LENGTH 51

#define SHORT_DELAY             100    // 100ms
#define MEDIUM_DELAY            500    // 500ms
#define LONG_DELAY              1000   // 1s
#define NO_BLINKY               0      // Continuous mode

#define SHORT_TIMEOUT           1000   // 1s
#define MEDIUM_TIMEOUT          5000   // 5s
#define LONG_TIMEOUT            10000  // 10s


#define SHORT_BUTTON_PRESS      1500   // 2s
#define MEDIUM_BUTTON_PRESS     3500   // 5s
#define LONG_BUTTON_PRESS       8000  // 10s



volatile unsigned char Buffer_UART[BUFFER_LENGTH];
volatile unsigned char Buffer_ADC[BUFFER_LENGTH];

unsigned char array[BUFFER_LENGTH];

unsigned int Index_UART;
unsigned int Index_ADC;

unsigned int LED_Period;
unsigned long timebase;
unsigned int Sensor_Value;


//DEFINIÇÃO DOS PINOS UTILIZADOS PELO uC
#define UART_TX_PIN                     BIT1 // P1.1
#define UART_RX_PIN                     BIT0 // P1.0
#define LED_STATUS_PIN                  BIT3 // P1.3
#define HIGH_FREQ_SQUAREWAVE_PIN        BIT4 // P1.4
#define SENSOR_CAPACITIVE_PIN              2 // P1.2

void init_TimerA0(void);
void Write_UART(char* sentence);
void init_UART(unsigned long _baudrate);
void init_GPIO(void);
void init_Oscillator(void);
void init_Watchdog(void);
void init_ADC(void);
void init_Global_Variables(void);
char* itoa(int value, char* result, int base);

void ADC_Stop(void);
void ADC_Start(void);
void delay(unsigned int n);

void LED_Blink(unsigned int _period);
_Bool Write_Wait_Response(char* _command, char* _expected_answer, unsigned int _timeout);
void setup_BLE(_Bool HIERARCHY, char* newName) ;

unsigned long millis();

int main(void){
    init_Watchdog();
    init_Oscillator();
    init_GPIO();
    init_ADC();
    init_TimerA0();
    init_UART(9600);
    init_Global_Variables();

    __bis_SR_register(GIE);

    Write_Wait_Response("AT\r", "OK", SHORT_TIMEOUT);

    setup_BLE(SLAVE, "PlantsCare");

    while(1)
    {
        Write_UART("AT\r");
        delay(1000);
    }
    __no_operation();

   // ADC_Start();

    while(1)
    {
        /*
        ADC_Start();
        itoa(Buffer_ADC[Index_ADC-1], array, 10);
        Write_UART(array);
        Write_UART("\r\n");
        */
        Index_ADC = 0;
        do
        {
            ADC_Start();

        }while(Index_ADC != 0);
        __no_operation();

        char i;
        for(i = 0; i < BUFFER_LENGTH-1; i++)
        {
            Sensor_Value += Buffer_ADC[i];
        }
        Sensor_Value /= (BUFFER_LENGTH-1);
        __no_operation();
        itoa(Sensor_Value, array, 10);
        Write_UART(array);
        Write_UART("\r\n");

        delay(500);

    }
}

void init_Global_Variables(void){
    Buffer_UART[BUFFER_LENGTH-1] = '\0';
    Buffer_ADC[BUFFER_LENGTH-1] = '\0';
}

void init_Oscillator(void){
    P4SEL0 |= BIT1 | BIT2;                  // set XT1 pin as second function

    do
    {
        CSCTL7 &= ~(XT1OFFG | DCOFFG);           // Clear XT1 and DCO fault flag
        SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);                    // Test oscillator fault flag

    __bis_SR_register(SCG0);                     // disable FLL

    CSCTL3 |= SELREF__XT1CLK;                    // Set XT1CLK as FLL reference source
    CSCTL0 = 0;                                  // clear DCO and MOD registers

    __bic_SR_register(SCG0);                     // enable FLL

    while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1));   // Poll until FLL is locked

    CSCTL7 &= ~DCOFFG;                           // Clear DCO fault flag
    CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK;  // set ACLK = XT1CLK = 32768Hz
                                               // DCOCLK = MCLK and SMCLK source
}

void init_Watchdog(void){
    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
}

void init_GPIO(void) {

    /*  FIRST STEP: All pins as GPIO, OUTPUT at LOW level (LPM Optimization)       */
    P1DIR  = 0xFF;  //All pins as OUTPUT
    P1SEL0 = 0x00;  //All pins as GPIO
    P1OUT  = 0x00;  //All pins as LOW

    P2DIR  = 0xFF;  //All pins as OUTPUT
    P2SEL0 = 0x00;  //All pins as GPIO
    P2OUT  = 0x00;  //All pins as LOW

    P3DIR  = 0xFF;  //All pins as OUTPUT
    P3SEL0 = 0x00;  //All pins as GPIO
    P3OUT  = 0x00;  //All pins as LOW

    P4DIR  = 0xFF;  //All pins as OUTPUT
    P4SEL0 = 0x00;  //All pins as GPIO
    P4OUT  = 0x00;  //All pins as LOW

    P5DIR  = 0xFF;  //All pins as OUTPUT
    P5SEL0 = 0x00;  //All pins as GPIO
    P5OUT  = 0x00;  //All pins as LOW

    P6DIR  = 0xFF;  //All pins as OUTPUT
    P6SEL0 = 0x00;  //All pins as GPIO
    P6OUT  = 0x00;  //All pins as LOW

    P7DIR  = 0xFF;  //All pins as OUTPUT
    P7SEL0 = 0x00;  //All pins as GPIO
    P7OUT  = 0x00;  //All pins as LOW

    P8DIR  = 0xFF;  //All pins as OUTPUT
    P8SEL0 = 0x00;  //All pins as GPIO
    P8OUT  = 0x00;  //All pins as LOW


    /*  SECOND STEP: INPUT PINS         */
    P1DIR &= ~UART_RX_PIN;
    P1DIR &= ~SENSOR_CAPACITIVE_PIN;


    /*  THIRD STEP: SECUNDARY FUNCTIONS  */
    P1SEL0 |= UART_TX_PIN | UART_RX_PIN;    // UART pins
    P1SEL0 |= SENSOR_CAPACITIVE_PIN;        // ADC pin
    P1SEL0 |= HIGH_FREQ_SQUAREWAVE_PIN;     // MCLK as OUTPUT
    P4SEL0 |= BIT1 | BIT2;                  // Crystal pins

    PM5CTL0 &= ~LOCKLPM5; // GPIO High-Impedance OFF
}

void init_TimerA0(void) {
    TA0CCR0   = (int)ISR_TIMER0_A0_OFFSET;                         // Add Offset to TACCR0
    TA0CTL   |= TASSEL__SMCLK | MC__CONTINOUS;    // SMCLK, continuous mode
    TA0CCTL0 |= CCIE;                             // TACCR0 interrupt enabled
}

void delay(unsigned int n) {
  for (; n > 0; n--){
    __delay_cycles(625);
  }
}

void Write_UART(char* sentence) {
    unsigned int size = strlen(sentence);
    unsigned int i = 0;

    while(size > 0) {
        while(!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = sentence[i++];
        size--;
    }
}

void init_UART(unsigned long _baudrate) {
    UCA0CTLW0 |= UCSWRST;
    UCA0CTLW0 |= UCSSEL__SMCLK;

    switch(_baudrate)
    {
        case 9600:
            UCA0BR0 = 6;
            UCA0BR1 = 0;
            UCA0MCTLW |= 0x20 + UCOS16 + UCBRF_8;
            break;

        case 115200:
            UCA0BR0 = 0x45;
            UCA0BR1 = 0x00;
            UCA0MCTLW = 0x4A;
            break;
    }

    UCA0CTLW0 &= ~UCSWRST;
    UCA0IE |= UCRXIE;
}

void ADC_Stop(void){
    ADCCTL0  &= ~ADCON;
    ADCIE    &= ~ADCIE0;
}

void ADC_Start(void){
    ADCCTL0 |= ADCENC | ADCSC;                           // Sampling and conversion start
    while(ADCCTL1 & ADCBUSY);
}

void init_ADC(void){
    ADCCTL0  &= ~ADCENC;            // Disable ADC conversion (needed for the next steps)
    ADCCTL0  |= ADCSHT_12 | ADCON;  // ADC sample-and-hold time = 1024 ADCCLK cycles | ADC on
    ADCCTL1  |=  ADCSHP;            // ADC sample-and-hold pulse-mode select = SAMPCON signal is sourced from the sampling timer
    ADCCTL2  |= ADCRES;             // 10-bit conversion results
    ADCMCTL0 |= ADCINCH_2;          // A2 ADC input select; Vref=AVCC
    ADCIE    |= ADCIE0;             // Enable ADC conv complete interrupt
}

char* itoa(int value, char* result, int base) {
    // check that the base if valid
    if (base < 2 || base > 36)
    {
        *result = '\0';
        return result;
    }
    char* ptr = result, *ptr1 = result, tmp_char;
    int tmp_value;
    do
    {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    }while (value);

    // Apply negative sign
    if (tmp_value < 0)
        *ptr++ = '-';
    *ptr-- = '\0';
    while(ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr--= *ptr1;
        *ptr1++ = tmp_char;
    }
    return result;
}

void setup_BLE(_Bool _HIERARCHY, char* _new_name) {

    __bis_SR_register(GIE);

    char _slave_name[20];
    char _step = 0;

    //TODO: in PCB version 1.2 or later, changes this lines for BLE_STATUS checking
    Write_UART("AT+DROP\r"); delay(100);
    Write_UART("AT+DROP\r"); delay(100);
    Write_UART("AT+DROP\r"); delay(100);
    Write_UART("AT+DROP\r"); delay(100);

    sprintf(_slave_name, "%s%s\r", "AT+NAME", _new_name);
    if(_HIERARCHY == SLAVE){
        _step = 0;
        do{
            switch(_step){
                case 0:
                    if(Write_Wait_Response("AT\r", "OK", SHORT_TIMEOUT)) //Test UART connection
                        _step++;
                    break;

                case 1:
                    if(Write_Wait_Response("AT+MODE0\r", "OK", SHORT_TIMEOUT)) //Operation mode set as TRANSMITION only
                        _step++;
                    break;

                case 2:
                    if(Write_Wait_Response("AT+ROLE0\r", "OK", SHORT_TIMEOUT)) //Hierarchical function set as SLAVE
                        _step++;
                    break;

                case 3:
                    if(Write_Wait_Response("AT+RESET\r", "\r", MEDIUM_TIMEOUT)) //Reset BLE module to save all new configurations
                        _step++;
                    break;

                case 4:
                    if(Write_Wait_Response("AT+ADDR?\r", "\r", SHORT_TIMEOUT)) //Request MAC Address
                        _step++;
                    break;

                case 5:
                    if(Write_Wait_Response(_slave_name, "\r", SHORT_TIMEOUT)) // Set a new module name
                        _step++;
                    break;

                case 6:
                    if(Write_Wait_Response("AT+NAME?\r", "\r", MEDIUM_TIMEOUT)) // Checks the new name
                        _step++;
                    break;
            }
        }while(_step <= 6);
    }
    else if(_HIERARCHY == MASTER){
        _step = 0;
        do{
            switch(_step){
                case 0:
                    if(Write_Wait_Response("AT\r", "OK", SHORT_TIMEOUT)) //Test UART connection
                        _step++;
                    break;

                case 1:
                    if(Write_Wait_Response("AT+NOTI1\r", "OK", SHORT_TIMEOUT)) //Notify when a connections has beens established
                        _step++;
                    break;

                case 2:
                    if(Write_Wait_Response("AT+MODE0\r", "OK", SHORT_TIMEOUT)) //Operation mode set as TRANSMITION only
                        _step++;
                    break;

                case 3:
                    if(Write_Wait_Response("AT+IMME0\r", "OK", SHORT_TIMEOUT)) //Starts working immediately after power-up
                        _step++;
                    break;

                case 4:
                    if(Write_Wait_Response("AT+ROLE1\r", "OK", SHORT_TIMEOUT)) //Hierarchical function set as MASTER
                        _step++;
                    break;

                case 5:
                    if(Write_Wait_Response("AT+RESET\r", "\r", MEDIUM_TIMEOUT)) //Reset BLE module to save all new configurations
                        _step++;
                    break;

                case 6:
                    if(Write_Wait_Response("AT+ADDR?\r", "\r", SHORT_TIMEOUT)) //Request MAC Address
                        _step++;
                    break;
            }
        }while(_step <= 6);
    }
}

_Bool Write_Wait_Response(char* _command, char* _expected_answer, unsigned int _timeout) {

    _Bool _answer = FALSE;
    unsigned long _previous_time;

    memset(Buffer_UART, '\0', sizeof(Buffer_UART));
    Index_UART = 0;

    _previous_time = millis();

    do
    {
        Write_UART(_command);
        delay(_timeout/4);
        if (strstr((const char*)Buffer_UART, (const char*)_expected_answer) != NULL)
            _answer = TRUE;

    } while((_answer == FALSE) && ((millis() - _previous_time) < _timeout));

    __no_operation();

    return _answer;
}

void LED_Blink(unsigned int _period){
    LED_Period++;
    if(LED_Period == _period){
        P1OUT ^= LED_STATUS_PIN;
        LED_Period = 0;
    }
}

unsigned long millis() {
    return timebase;
}

#pragma vector = USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
  switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
  {
    case USCI_NONE: break;
    case USCI_UART_UCRXIFG:
    {
        while(!(UCA0IFG&UCTXIFG));

        Buffer_UART[Index_UART++] = UCA0RXBUF;

        if (Index_UART == BUFFER_LENGTH-1)
            Index_UART = 0;
      break;
    }
    case USCI_UART_UCTXIFG: break;
    case USCI_UART_UCSTTIFG: break;
    case USCI_UART_UCTXCPTIFG: break;
    default: break;
  }
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR (void){

    timebase++;

    LED_Blink(LONG_DELAY);
    TA0CCR0 += ISR_TIMER0_A0_OFFSET; // Add Offset to TACCR0
}

#pragma vector=ADC_VECTOR
__interrupt void ADC_ISR(void){
    switch(__even_in_range(ADCIV,ADCIV_ADCIFG))
    {
        case ADCIV_ADCIFG:
            Buffer_ADC[Index_ADC++] = ADCMEM0;

            if (Index_ADC == BUFFER_LENGTH-1)
                Index_ADC = 0;

            break;

        default:
            break;
    }
}
