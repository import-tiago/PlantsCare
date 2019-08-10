#include <msp430.h>

#define ISR_TIME                8064   // 1ms timer overflow

typedef enum {
    STARTING = 0,
    WITHOUT_WIFI_NETWORK_ASSIGNED,
    NEW_DEVICE_SETUP,

    SELECT_DEVICE_NUMBER_TO_CONNECT,
    DEVICE_CONNECTED,
    SAVE_RECEIVED_DATA_EEPROM,
    DEVICE_DATA_REQUEST,
    DEVICE_DATA_RECEIVED,

    WIFI_NETWORK_CONNECTED,
    CLOUD_CONNECTED,
    REST_PACKAGE_SENDING

} StatesFSM1;

StatesFSM1 stateFSM1;

void main(void) {
    init_Watchdog();
    init_Oscillator();
    init_GPIO();
    init_ADC();
    init_UART(BAUDRATE_9600);
    init_Timer0();
    init_GPIO_Interrupt();

    __bis_SR_register(GIE);

	
	while(1) {

	}
}

void runFSM(void) {
    switch(stateFSM1)
    {
       case STARTING:
       {
       }
    }
}

void init_Watchdog(void) {
    WDTCTL = WDTPW | WDTHOLD;  // stop watchdog timer
}

void init_ADC(void) {
      ADCCTL0  &= ~ADCENC;               // Disable ADC conversion (needed for the next steps)
      ADCCTL0  |=  ADCSHT_12 | ADCON;    // ADC sample-and-hold time = 1024 ADCCLK cycles | ADC on
      ADCCTL1  |=  ADCSHP;               // ADC sample-and-hold pulse-mode select = SAMPCON signal is sourced from the sampling timer
      ADCCTL2  |=  ADCRES;               // ADC resolution = 10 bit
      ADCCTL0  |=  ADCENC;               // Enable ADC conversion.
      ADCIE    &= ~ADCIE0;               // ADC interrupt disabled
}

unsigned short analogRead(char channel) {
    ADCCTL0  &= ~ADCENC;            // Disable ADC conversion (needed to the next steps)
    ADCMCTL0  =  channel;           // Input channel select
    ADCMCTL0 &=  ~ADCSREF_1;
    ADCMCTL0 |=  ADCSREF_0;         // VREF = VCC and VSS
    ADCCTL0  |=  ADCENC | ADCSC;    // Enable ADC conversion and start conversion

    while(ADCCTL1 & ADCBUSY);       // Wait the ends of sample or conversion operation
    ADCValue = ADCMEM0;                 // Gets the conversion results

    return ADCValue;
}

unsigned short analogRead_Internal_Temperature() {
    ADCCTL0  &= ~ADCENC;            // Disable ADC conversion (needed to the next steps)
    ADCMCTL0  =  ADCINCH_12;        // Input channel select
    ADCMCTL0 |=  ADCSREF_1;         // VREF = internal 1.5V and VSS

    // Configure reference
    PMMCTL0_H = PMMPW_H;                                          // Unlock the PMM registers
    PMMCTL2 |= INTREFEN | TSENSOREN;                              // Enable internal reference and temperature sensor
    __delay_cycles(400);                                          // Delay for reference settling

    ADCCTL0  |=  ADCENC | ADCSC;    // Enable ADC conversion and start conversion
    while(ADCCTL1 & ADCBUSY);       // Wait the ends of sample or conversion operation

    ADCValue = ADCMEM0;
    temp = (float)ADCValue;
    // Temperature in Celsius
    IntDegC = (temp-CALADC_15V_30C)*(85-30)/(CALADC_15V_85C-CALADC_15V_30C)+30;

    return IntDegC;                 // Gets the conversion results
}

void delay(unsigned int n) {
  for (; n > 0; n--)
  {
    __delay_cycles(5000);
  }
  __no_operation();
}

void init_Oscillator(void) {


    // Configure XT1 oscillator
        P4SEL0 |= BIT1 | BIT2;                                     // P4.2~P4.1: crystal pins

           do
        {
            CSCTL7 &= ~(XT1OFFG | DCOFFG);                         // Clear XT1 and DCO fault flag
            SFRIFG1 &= ~OFIFG;
        }while (SFRIFG1 & OFIFG);                                  // Test oscillator fault flag


        CSCTL6 = (CSCTL6 & ~(XT1DRIVE_3)) | XT1DRIVE_2;            // Higher drive strength and current consumption for XT1 oscillator

        __bis_SR_register(SCG0);                   // disable FLL
        CSCTL3 |= SELREF__XT1CLK;                  // Set external crystal as FLL reference source
        CSCTL0 = 0;                                // clear DCO and MOD registers
        CSCTL1 &= ~(DCORSEL_7);                    // Clear DCO frequency select bits first
        CSCTL1 |= DCORSEL_3;                       // Set DCO = 8MHz
        CSCTL2 = FLLD_0 + 243;                     // DCODIV = 8MHz
        __delay_cycles(3);
        __bic_SR_register(SCG0);                   // enable FLL
        while(CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)); // Poll until FLL is locked

        CSCTL4 = SELMS__DCOCLKDIV | SELA__XT1CLK; // set external crystall as ACLK source, ACLK = 32768Hz
                                                  // set DCODIV as MCLK and SMCLK source



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
/*
    P8DIR  = 0xFF;  //All pins as OUTPUT
    P8SEL0 = 0x00;  //All pins as GPIO
    P8OUT  = 0x00;  //All pins as LOW
*/




/*  SECOND STEP: INPUT PINS         */

    P1DIR &= ~VBAT_PIN;
    P1DIR &= ~UART_RX;
    P3DIR &= ~BUTTOM_PROG_PIN;




/*  THIRD STEP: SECUNDARY FUNCTIONS  */

    P1SEL0 |= VBAT_PIN;                             // ADC pins
    P1SEL0 |= UART_TX | UART_RX;                    // UART pins
    P4SEL0 |= BIT1 | BIT2;                          // Crystal pins
    P5SEL0 |= BIT2 | BIT3;                          // I2C pins


    PM5CTL0 &= ~LOCKLPM5; // GPIO High-Impedance OFF
}

void Write_UART(char* sentence) {
    unsigned int size = strlen(sentence);
    unsigned int i = 0;

    while(size > 0)
    {
        while(!(UCA0IFG & UCTXIFG));
        UCA0TXBUF = sentence[i++];
      //  while(!(UCA0IFG & UCTXIFG));
        size--;
    }
}

void init_Timer0() {

    TA0CCR0   = ISR_TIME;                         // Add Offset to TACCR0
    TA0CTL   |= TASSEL__SMCLK | MC__CONTINOUS;    // SMCLK, continuous mode
    TA0CCTL0 |= CCIE;                             // TACCR0 interrupt enabled

}

unsigned long millis() {
    return timebase;
}

_Bool Write_Wait_Response(char* ATcommand, char* expected_answer, unsigned int timeout) {
    _Bool answer = FALSE;
    unsigned long previous;

    memset(Rx_UART, '\0', 100); // Clean rx UART buffer
    __delay_cycles(70);

    Index_UART = 0;
    Write_UART(ATcommand); // Send the AT command

    previous = millis();
    // this loop waits for the answer
    do
    {
        // check if the desired answer is in the ex buffer of the module

        if (strstr((const char*)Rx_UART, (const char*)expected_answer) != NULL)
        {
            answer = TRUE;
            break;
        }
        // Waits for the answer with time out
    } while((answer == FALSE) && ((millis() - previous) < timeout));
    __no_operation();

    return answer;
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void) {

    TA0CCR0 += ISR_TIME;
}
