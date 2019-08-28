#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define HIGH                    1
#define LOW                     0
#define TRUE                    1
#define FALSE                   0

#define MASTER                  1
#define SLAVE                   0

#define ISR_TIMER0_A0_OFFSET    1045 // 1ms overflow
#define BUFFER_LENGTH           51

#define SHORT_DELAY             100    // 100ms
#define MEDIUM_DELAY            500    // 500ms
#define LONG_DELAY              1000   // 1s
#define NO_BLINKY               0      // Continuous mode

#define SHORT_TIMEOUT           1000   // 1s
#define MEDIUM_TIMEOUT          5000   // 5s
#define LONG_TIMEOUT            10000  // 10s

#define SHORT_BUTTON_PRESS      3000   // 3s
#define MEDIUM_BUTTON_PRESS     5000   // 5s
#define LONG_BUTTON_PRESS       10000  // 10s

//Capacitive Soil Moisture
#define N_SAMPLES               50
#define SENSOR_FULL_DRY         0
#define SENSOR_FULL_WET         1
#define SENSOR_CALIBRATION      2


//Button
#define DEBOUNCE_BUTTON_DELAY   5

//FRAM
#define FRAM_kL_CALIBRATION_ADDR   0x1800      // 7B -> 4b * 7 = 18b -> 0x181C
#define FRAM_kL_CALIBRATION_LEN    7

#define FRAM_kA_CALIBRATION_ADDR   0x1824      // 7B -> 4b * 7 = 18b -> 0x1838
#define FRAM_kA_CALIBRATION_LEN    7




#define FRAM_ERROR 0
#define FRAM_SUCCESS 1


//DEFINIO DOS ESTADOS DA MQUINA DE ESTADOS FINITA
typedef enum {
    STARTING = 0,
    GET_SOIL_MOISTURE,
    BROKER_CONNECT,
    CAPACITIVE_SENSOR_CALIBRATION,
} StatesFSM1;

StatesFSM1 stateFSM1;



/* GLOBAL VARIABLES */

// UART Peripheral
volatile unsigned char Buffer_UART[BUFFER_LENGTH];
unsigned int Index_UART;

// TIMER Peripheral
unsigned long timebase;

// ADC Peripheral
volatile unsigned char Buffer_ADC[BUFFER_LENGTH];
unsigned int Index_ADC;

//Soil Moisture
unsigned int Sensor_Value;
unsigned int Sensor_Dry_Value;
unsigned int Sensor_Wet_Value;
unsigned int Soil_Moisture;


//Capacitive Sensor
float kA_Calibration;
float kL_Calibration;
_Bool Calibration_Mode_Running;
_Bool Successful_LED_Notification;


// LED
unsigned int LED_Current_Period;
unsigned int LED_Setpoint_Period;

// Auxiliaries
volatile unsigned char array[BUFFER_LENGTH];

// Button
_Bool Current_Button_State;
_Bool Last_Button_State;
_Bool Reading_Button;
unsigned long lastTime;
unsigned short Button_Pressed_Time;
_Bool Short_Press_Detected;
_Bool Medium_Press_Detected;
_Bool Long_Press_Detected;



//DEFINIÇÃO DOS PINOS UTILIZADOS PELO uC
#define UART_TX_PIN                     BIT1 // P1.1
#define UART_RX_PIN                     BIT0 // P1.0

#define HIGH_FREQ_SQUAREWAVE_PIN        BIT4 // P1.4
#define SENSOR_CAPACITIVE_PIN              2 // P1.2

#define LED_STATUS_PIN                  BIT3 // P1.3
#define BUTTON_PIN                      BIT7 // P1.7


void init_TimerA0(void);
void Write_UART(unsigned char* sentence);
void init_UART(unsigned long _baudrate);
void init_GPIO(void);
void init_Oscillator(void);
void init_Watchdog(void);
void init_ADC(void);
void init_Global_Variables(void);
char* itoa(int value, char* result, int base);


unsigned char ADC_Read(void);
void ADC_Start(void);
void ADC_Stop(void);

void delay(unsigned int n);
void LED_Blink();
void LED_Blink_Successful(char _nBlink);
_Bool Write_Wait_Response(unsigned char* _command, char* _expected_answer, unsigned int _timeout);
void setup_BLE(_Bool HIERARCHY, char* newName);
unsigned int Get_Soil_Moisture();
unsigned int Get_Sensor_Value();
unsigned long millis();
void FRAM_Write(unsigned long *Write_Address, char* Write_Data, unsigned short Number_Bytes);
void FRAM_Read(unsigned long *Read_Address,  unsigned short Number_Bytes);
void ftoa(volatile float f, char * buf);
void Load_Calibration_Coefficients();
void Clear_Calibration_Coefficients();
void Save_Calibration_Coefficients(float kA, float kL);
void LED_State(char _state);
void Two_Points_Sensor_Calibration(volatile  int _Dry_Value, volatile  int _Wet_Value);
void Load_Default_Calibration_Coefficients();


int main(void){
    init_Watchdog();
    init_Oscillator();
    init_GPIO();
    init_ADC();
    init_TimerA0();
    init_UART(9600);
    init_Global_Variables();

    __bis_SR_register(GIE);


    Load_Default_Calibration_Coefficients();



    while(1)
    {
        runFSM();
    }
}

void Load_Default_Calibration_Coefficients(){
    Two_Points_Sensor_Calibration(182, 149);
    kA_Calibration = 0;
    kL_Calibration = 0;
}

void runFSM() {
    switch(stateFSM1) {
       case STARTING: {
           LED_Setpoint_Period = NO_BLINKY;
           Load_Calibration_Coefficients();
           stateFSM1 = GET_SOIL_MOISTURE;
           break;
       }

       case GET_SOIL_MOISTURE: {

           Soil_Moisture = Get_Soil_Moisture();

           sprintf((char*)array, "%d%s", Sensor_Value, "\r\n");
           Write_UART(array);


           sprintf((char*)array, "%d%s", Soil_Moisture, "\r\n");
           Write_UART(array);

           Write_UART("\r\r\n");

           delay(500);

           break;
       }

       case BROKER_CONNECT: {
           sprintf((char*)array, "%d%s", Soil_Moisture, "\n");
           Write_UART(array);
           delay(500);
           break;
       }

       case CAPACITIVE_SENSOR_CALIBRATION: {
           char _step = 0;
           do
           {
               switch(_step)
               {
                   case SENSOR_FULL_DRY:
                   {
                       do
                       {
                           Sensor_Dry_Value = Get_Sensor_Value();
                       }while((P1IN & BUTTON_PIN));

                       LED_Blink_Successful(10);
                       _step++;

                       break;
                   }

                   case SENSOR_FULL_WET:
                   {
                       do
                       {
                           Sensor_Wet_Value = Get_Sensor_Value();
                       }while((P1IN & BUTTON_PIN));

                       LED_Blink_Successful(10);
                       _step++;

                       break;
                   }
                   case SENSOR_CALIBRATION:
                   {
                       kA_Calibration = (100.0 / (Sensor_Wet_Value - Sensor_Dry_Value));
                       kL_Calibration = (kA_Calibration * Sensor_Dry_Value);

                       Save_Calibration_Coefficients(kA_Calibration, kL_Calibration);
                       LED_Blink_Successful(10);
                       LED_State(LOW);
                       Calibration_Mode_Running = FALSE;
                       break;
                   }
               }
           }while(Calibration_Mode_Running);

           stateFSM1 = STARTING;
           break;
       }

    }
}

void Two_Points_Sensor_Calibration(volatile  int _Dry_Value, volatile  int _Wet_Value) {

    Clear_Calibration_Coefficients();

    kA_Calibration = (100.0 / (float)(_Wet_Value - _Dry_Value));
    kL_Calibration = (kA_Calibration * _Dry_Value);

    Save_Calibration_Coefficients(kA_Calibration, kL_Calibration);
}

void Load_Calibration_Coefficients() {

    FRAM_Read(FRAM_kA_CALIBRATION_ADDR, FRAM_kA_CALIBRATION_LEN);
    kA_Calibration = atof(array);

    FRAM_Read(FRAM_kL_CALIBRATION_ADDR, FRAM_kL_CALIBRATION_LEN);
    kL_Calibration = atof(array);

    __no_operation();
}

void Clear_Calibration_Coefficients(){

    memset(array, '\0', sizeof(array));
    ftoa(0.00000, array);
    FRAM_Write(FRAM_kA_CALIBRATION_ADDR, array, FRAM_kA_CALIBRATION_LEN);

    memset(array, '\0', sizeof(array));
    ftoa(0.00000, array);
    FRAM_Write(FRAM_kL_CALIBRATION_ADDR, array, FRAM_kL_CALIBRATION_LEN);

    __no_operation();
}

void Save_Calibration_Coefficients(float kA, float kL){
    memset(array, '\0', sizeof(array));
    ftoa(kA, array);
    FRAM_Write(FRAM_kA_CALIBRATION_ADDR, array, FRAM_kA_CALIBRATION_LEN);
    __no_operation();

    memset(array, '\0', sizeof(array));
    ftoa(kL, array);
    FRAM_Write(FRAM_kL_CALIBRATION_ADDR, array, FRAM_kL_CALIBRATION_LEN);
    __no_operation();
}

void ftoa(volatile float f, char * buf) {

    volatile int intPart = 0;
    volatile int decPart = 0;

    intPart = f;
    f -= intPart;
    f *= 10000;
    decPart = f;

    if(decPart < 0)
        decPart  *= -1;

    if(decPart == 0)
        sprintf(buf, "%d.0000", intPart);
    else
        sprintf(buf, "%d.%d", intPart, decPart);

    __no_operation();
}

void FRAM_Read(unsigned long *Read_Address,  unsigned short Number_Bytes) {

    unsigned short i;
    memset(array, '\0', sizeof(array));

    for(i = 0; i < Number_Bytes; i++)
    {
      array[i] = *Read_Address++;
    }
}

void FRAM_Write(unsigned long *Write_Address, char* Write_Data, unsigned short Number_Bytes) {

    unsigned short i;


    __no_operation();

    SYSCFG0 &= ~DFWP; // Write Protection Disable

    for(i = 0; i < Number_Bytes; i++)
    {
      *Write_Address++ = Write_Data[i];
    }

    SYSCFG0 |= DFWP; // Write Protection Enable
}

unsigned int Get_Soil_Moisture(){
    char i;
    int _soil_moisture;

    for(i = 0; i < N_SAMPLES; i++)
        Sensor_Value += ADC_Read();


    Sensor_Value /= N_SAMPLES;

    _soil_moisture = (Sensor_Value * kA_Calibration) - kL_Calibration;
    __no_operation();

    if(_soil_moisture < 0)
        _soil_moisture = 0;
    if(_soil_moisture > 100)
        _soil_moisture = 100;

    return _soil_moisture;
}

unsigned int Get_Sensor_Value(){
    char i;

    Sensor_Value = 0;

    for(i = 0; i < N_SAMPLES; i++)
        Sensor_Value += ADC_Read();

    return Sensor_Value /= N_SAMPLES;
}

void init_Global_Variables(void){
    Buffer_UART[BUFFER_LENGTH-1] = '\0';
    Buffer_ADC[BUFFER_LENGTH-1] = '\0';

    Last_Button_State = 1;

    LED_Setpoint_Period = NO_BLINKY;

    Short_Press_Detected = FALSE;
    Medium_Press_Detected = FALSE;
    Long_Press_Detected = FALSE;

    Calibration_Mode_Running = FALSE;
    Successful_LED_Notification = FALSE;

    stateFSM1 = STARTING;
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
    P1DIR &= ~BUTTON_PIN;

    /*  PULL-UP OR PULL-DOWN  */
    P1REN |= BUTTON_PIN; // Pull-up enable to push-button
    P1OUT |= BUTTON_PIN; // Pull-up enable to push-button

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

void Write_UART(unsigned char* sentence) {
    unsigned int size = strlen((char*) sentence);
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

    ADCCTL0 |= ADCENC;          // Sampling and conversion start
    ADCCTL0 |= ADCSC;
    while(ADCCTL1 & ADCBUSY);
}

unsigned char ADC_Read(void){

    ADC_Start();
    if(Index_ADC > 0)
        return Buffer_ADC[Index_ADC-1];
    else
        return Buffer_ADC[BUFFER_LENGTH-1];
}

void init_ADC(void){
    ADCCTL0  &= ~ADCENC;            // Disable ADC conversion (needed for the next steps)
    ADCCTL0  |= ADCSHT_12 | ADCON;  // ADC sample-and-hold time = 1024 ADCCLK cycles | ADC on
    ADCCTL1  |= ADCSHP;            // ADC sample-and-hold pulse-mode select = SAMPCON signal is sourced from the sampling timer
    ADCCTL2  |= ADCRES_1;             // 10-bit conversion results
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

    unsigned char _slave_name[20];
    char _step = 0;

    //TODO: in PCB version 1.2 or later, changes this lines for BLE_STATUS checking
    Write_UART("AT+DROP\r"); delay(100);
    Write_UART("AT+DROP\r"); delay(100);
    Write_UART("AT+DROP\r"); delay(100);
    Write_UART("AT+DROP\r"); delay(100);

    sprintf((char*)_slave_name, "%s%s\r", "AT+NAME", (char*)_new_name);
    if(_HIERARCHY == SLAVE){
        _step = 0;
        do{
            switch(_step){
                case 0:
                    if(Write_Wait_Response("AT+RENEW\r", "OK", SHORT_TIMEOUT)) //Test UART connection
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
                    if(Write_Wait_Response("AT+RENEW\r", "OK", SHORT_TIMEOUT)) //Test UART connection
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

_Bool Write_Wait_Response(unsigned char* _command, char* _expected_answer, unsigned int _timeout) {

    _Bool _answer = FALSE;
    unsigned long _previous_time;

    memset((void*)Buffer_UART, '\0', sizeof(Buffer_UART));
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

void LED_Blink(){
    if(!Successful_LED_Notification)
    {
        LED_Current_Period++;

        if(LED_Current_Period == LED_Setpoint_Period){
            P1OUT ^= LED_STATUS_PIN;
            LED_Current_Period = 0;
        }
    }
}

void LED_State(char _state){
    if(_state == LOW)
        P1OUT &= ~LED_STATUS_PIN;
    else if(_state == TRUE)
        P1OUT |= LED_STATUS_PIN;
}

void LED_Blink_Successful(char _nBlink) {
    Successful_LED_Notification = TRUE;

    char i;

    for(i = 0; i < _nBlink; i++)
    {
        P1OUT ^= LED_STATUS_PIN;
        delay(250);
    }

    Successful_LED_Notification = FALSE;

}

unsigned long millis() {
    return timebase;
}

void Read_PROG_Button() {

    Reading_Button = (P1IN & BUTTON_PIN);

    if (Reading_Button != Last_Button_State)
        lastTime = millis();

    if ( ((millis() - lastTime) > DEBOUNCE_BUTTON_DELAY) || Button_Pressed_Time > 0)
    {

        if ( (Reading_Button != Current_Button_State) || Button_Pressed_Time > 0)
        {
            Current_Button_State = Reading_Button;

            if(Current_Button_State == HIGH && Button_Pressed_Time < SHORT_BUTTON_PRESS)
                Button_Pressed_Time = 0;

            if (Current_Button_State == LOW || Button_Pressed_Time > 0)
            {
                Button_Pressed_Time++;

                if( (Button_Pressed_Time >= SHORT_BUTTON_PRESS  &&  Button_Pressed_Time <= MEDIUM_BUTTON_PRESS) || Short_Press_Detected){ //&& Current_Button_State == HIGH){

                    __no_operation();
                    if(!Short_Press_Detected)
                        LED_Current_Period = 0;

                    Short_Press_Detected = TRUE;
                    Medium_Press_Detected = FALSE;
                    Long_Press_Detected = FALSE;

                    LED_Setpoint_Period = MEDIUM_DELAY;

                    if(Current_Button_State == HIGH){
                        Button_Pressed_Time = 0;
                        Short_Press_Detected = FALSE;

                        Calibration_Mode_Running = TRUE;
                        stateFSM1 = CAPACITIVE_SENSOR_CALIBRATION;
                    }
                }

                if( (Button_Pressed_Time >= MEDIUM_BUTTON_PRESS  && Button_Pressed_Time <= LONG_BUTTON_PRESS) || Medium_Press_Detected){ //&& Current_Button_State == HIGH){

                    __no_operation();
                    if(!Medium_Press_Detected)
                        LED_Current_Period = 0;

                    Short_Press_Detected = FALSE;
                    Medium_Press_Detected = TRUE;
                    Long_Press_Detected = FALSE;

                    LED_Setpoint_Period = MEDIUM_DELAY;

                    if(Current_Button_State == HIGH){
                        Button_Pressed_Time = 0;
                        Medium_Press_Detected = FALSE;
                    }
                }

                if( (Button_Pressed_Time >= LONG_BUTTON_PRESS) || Long_Press_Detected){

                    __no_operation();
                    if(!Long_Press_Detected)
                        LED_Current_Period = 0;

                    Short_Press_Detected = FALSE;
                    Medium_Press_Detected = FALSE;
                    Long_Press_Detected = TRUE;

                    LED_Setpoint_Period = SHORT_DELAY;

                    if(Current_Button_State == HIGH){
                        Button_Pressed_Time = 0;
                        Clear_Calibration_Coefficients();
                        setup_BLE(SLAVE, "PlantsCare");
                        Long_Press_Detected = FALSE;
                    }
                }
            }
        }
    }
    else
        Current_Button_State = HIGH;

    Last_Button_State = Reading_Button;
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

    Read_PROG_Button();
    LED_Blink();
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
